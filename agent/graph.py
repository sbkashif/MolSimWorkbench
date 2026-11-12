"""The harness as a LangGraph graph: the same loop, saved after every step.

harness.py runs the loop as a `while` loop. This file runs the same loop as a
graph of nodes, with LangChain for the model and LangGraph for the loop. The LLM,
its goal, the tool list, the limits and the log format are all harness.py's. Two
things are new:

- Resume. LangGraph saves the run after every node, to a SQLite file in the run
  folder. A run that stops part-way (a crash, a closed laptop) carries on from
  the last finished node with --resume. A training that was cut off is run
  again, and counts once.
- Approval (--approve, off by default). Before each training the run pauses,
  shows the call the LLM chose and its reasoning, and waits for a yes. A no,
  with a reason, goes back to the LLM like one of the harness's refusals.

    python -m agent.graph --model qwen3:8b --run-dir agent/runs/graph_qwen3
    python -m agent.graph --run-dir agent/runs/graph_qwen3 --resume

The graph (every arrow is the harness's code, except the one after llm, which
the LLM's tool call picks):

    llm ──no call──► nudge ──► llm
     │
    tool call ──► check ──refused──► respond ──► llm
                    │                   ▲
                    ├──train, --approve─► approve ──no──┘
                    │                      │yes
                    └──────allowed──────► run ──► respond
    stop, or a limit ──► final ──► end

Only Ollama models for now.
"""
import argparse
import json
import time
from datetime import datetime
from pathlib import Path
from typing import Annotated, TypedDict

from langchain_core.messages import AIMessage, HumanMessage, ToolMessage
from langgraph.graph import START, StateGraph
from langgraph.graph.message import add_messages
from langgraph.types import Command, interrupt

from . import config, tools
from .harness import GOAL, check

NUDGE = "Call one of the tools, or call stop with a reason."
CHECKPOINTS = "graph_checkpoints.sqlite"


class State(TypedDict):
    """Everything the run needs to carry on after a stop. LangGraph saves it after
    every node."""
    messages: Annotated[list, add_messages]   # the history the LLM is sent
    llm: str                  # the model name, kept so a resume uses the same one
    approve: bool             # pause for a yes before each training
    started: float            # wall-clock start, so t_min continues after a resume
    # tools.RunState, as plain values
    frames: int
    trainings: int
    model: str | None
    model_info: dict | None
    # the loop's counters, as in harness.py
    calls: int
    nudges: int
    thinking: str | None      # the LLM's latest reasoning: logged, never sent back
    call: dict | None         # the tool call being handled: name, args, id
    result: dict | None       # its result, or the refusal


def run_state(s, run_dir):
    """A tools.RunState with the run's values, for the tools to read and update."""
    rs = tools.RunState(run_dir)
    rs.frames, rs.trainings, rs.model_info = s["frames"], s["trainings"], s["model_info"]
    rs.model = Path(s["model"]) if s["model"] else None
    return rs


def saved(rs):
    """A RunState's values, to store back in the graph state."""
    return {"frames": rs.frames, "trainings": rs.trainings, "model_info": rs.model_info,
            "model": str(rs.model) if rs.model else None}


def clear_checkpoints(run_dir, name):
    """Delete MACE checkpoints left by a training of this name that was cut off.

    At the end of a training MACE loads the checkpoint with the highest epoch
    number, and it deletes only the files it wrote itself. A file left by a
    cut-off attempt at a later epoch would then become the final model."""
    for f in (run_dir / "checkpoints").glob(f"{name}_run-*_epoch-*.pt"):
        print(f"removing {f.name}, left by a training that was cut off", flush=True)
        f.unlink()


def make_logger(run_dir):
    """log(state, kind, **fields): one line in log.jsonl, the same format as harness.py."""
    def log(s, kind, **fields):
        entry = {"t_min": round((time.time() - s["started"]) / 60, 2), "kind": kind, **fields}
        with open(run_dir / "log.jsonl", "a") as f:
            f.write(json.dumps(entry) + "\n")
        print(json.dumps(entry)[:400], flush=True)
    return log


def build(llm, run_dir):
    """The graph. llm is a chat model with the tools already bound (bind_tools)."""
    log = make_logger(run_dir)
    run = {"train": lambda rs, **k: tools.train(rs, **k),
           "evaluate": lambda rs, **k: tools.evaluate(rs),
           "run_md": lambda rs, **k: tools.run_md(rs),
           "add_data": lambda rs, **k: tools.add_data(rs, **k)}

    def llm_node(s):
        """Send the history; log the reply; go on by what the LLM asked for."""
        t = time.time()
        m = llm.invoke(s["messages"])
        meta = m.response_metadata
        thinking = m.additional_kwargs.get("reasoning_content") or None
        log(s, "llm", seconds=round(time.time() - t, 1), thinking=thinking,
            text=m.content, tool_calls=[{"name": c["name"], "args": c["args"]}
                                        for c in m.tool_calls],
            usage={"input_tokens": meta.get("prompt_eval_count"),
                   "output_tokens": meta.get("eval_count")})
        # The history keeps the reply without its reasoning, as harness.py does.
        # ChatOllama would otherwise send every earlier reasoning back to the LLM.
        reply = AIMessage(content=m.content, tool_calls=m.tool_calls, id=m.id)
        if not m.tool_calls:
            return Command(goto="nudge", update={"messages": [reply], "thinking": thinking})
        c = m.tool_calls[0]                    # one at a time, as the goal asks
        call = {"name": c["name"], "args": dict(c["args"]), "id": c["id"]}
        update = {"messages": [reply], "thinking": thinking, "call": call,
                  "calls": s["calls"] + 1}
        if call["name"] == "stop":
            log(s, "end", reason=call["args"].get("reason", ""))
            return Command(goto="final", update=update)
        return Command(goto="check", update=update)

    def nudge(s):
        """A reply with no tool call: ask again, at most three times."""
        if s["nudges"] + 1 > 3:
            log(s, "end", reason="the LLM stopped calling tools")
            return Command(goto="final", update={"nudges": s["nudges"] + 1})
        return Command(goto="llm", update={"nudges": s["nudges"] + 1,
                                           "messages": [HumanMessage(NUDGE)]})

    def check_node(s):
        """The harness's own limit checks, unchanged."""
        name, args = s["call"]["name"], s["call"]["args"]
        refusal = (check(name, args, run_state(s, run_dir)) if name in run
                   else f"unknown tool {name}")
        if refusal:
            return Command(goto="respond", update={"result": {"refused_by_harness": refusal}})
        if name == "train" and s["approve"]:
            return Command(goto="approve")
        return Command(goto="run")

    def approve(s):
        """Pause for a yes before a training. Nothing happens before interrupt(),
        because LangGraph runs this node again from the top when the run resumes."""
        answer = interrupt({"tool": s["call"]["name"], "args": s["call"]["args"],
                            "reasoning": s["thinking"]})
        log(s, "approval", name=s["call"]["name"], args=s["call"]["args"], **answer)
        if answer["approved"]:
            return Command(goto="run")
        return Command(goto="respond",
                       update={"result": {"refused_by_human": answer.get("reason", "")}})

    def run_node(s):
        """Run the tool. If the run stops in here, nothing is saved, so the tool
        runs again on resume and a training counts once."""
        rs = run_state(s, run_dir)
        if s["call"]["name"] == "train":
            clear_checkpoints(run_dir, f"model_{rs.trainings + 1}")
        result = run[s["call"]["name"]](rs, **s["call"]["args"])
        return Command(goto="respond", update={"result": result, **saved(rs)})

    def respond(s):
        """Log the step and hand the result back to the LLM."""
        c = s["call"]
        log(s, "tool", name=c["name"], args=c["args"], result=s["result"])
        update = {"messages": [ToolMessage(content=tools.as_text(s["result"]),
                                           tool_call_id=c["id"], name=c["name"])]}
        if s["calls"] >= config.MAX_TOOL_CALLS:
            log(s, "end", reason=f"tool call limit reached ({config.MAX_TOOL_CALLS})")
            return Command(goto="final", update=update)
        return Command(goto="llm", update=update)

    def final(s):
        """Scored once, on frames the agent never saw, after every decision."""
        rs = run_state(s, run_dir)
        if rs.model is not None:
            log(s, "final_test", result=tools.final_test(rs))
        log(s, "summary", minutes=round((time.time() - s["started"]) / 60, 1),
            tool_calls=s["calls"], trainings=rs.trainings, final_frames=rs.frames,
            last_model=rs.model.name if rs.model else None)
        return {}

    g = StateGraph(State)
    for name, fn in [("llm", llm_node), ("nudge", nudge), ("check", check_node),
                     ("approve", approve), ("run", run_node), ("respond", respond),
                     ("final", final)]:
        g.add_node(name, fn)
    g.add_edge(START, "llm")
    return g


def first_state(llm_name, approve):
    return {"messages": [HumanMessage(GOAL)], "llm": llm_name, "approve": approve,
            "started": time.time(), "frames": config.START_FRAMES, "trainings": 0,
            "model": None, "model_info": None, "calls": 0, "nudges": 0,
            "thinking": None, "call": None, "result": None}


def ask_human(request):
    """Show the training the LLM chose, and read a yes or a reason for no."""
    print("\nThe LLM wants to run:", request["tool"], json.dumps(request["args"]))
    if request.get("reasoning"):
        print("Its reasoning (last 1500 characters):\n" + request["reasoning"][-1500:])
    reply = input("Approve? Type y, or a reason to refuse: ").strip()
    if reply.lower() in ("y", "yes"):
        return {"approved": True}
    return {"approved": False, "reason": reply or "refused, no reason given"}


def drive(graph, cfg, inp, answer=ask_human):
    """Run until the graph ends, stopping to ask at each approval pause."""
    while True:
        graph.invoke(inp, cfg, durability="sync")   # save before the next node starts
        snap = graph.get_state(cfg)
        if not snap.interrupts:
            return snap
        inp = Command(resume=answer(snap.interrupts[0].value))


def ollama_model(name):
    from langchain_ollama import ChatOllama
    # The settings harness.py passes to ollama.chat.
    return ChatOllama(model=name, reasoning=True, temperature=0, seed=1,
                      num_ctx=16384).bind_tools(tools.describe())


def main():
    from langgraph.checkpoint.sqlite import SqliteSaver
    p = argparse.ArgumentParser()
    p.add_argument("--model", default="qwen3:8b", help="Ollama model name")
    p.add_argument("--run-dir", default=f"agent/runs/graph_{datetime.now():%Y%m%d_%H%M}")
    p.add_argument("--approve", action="store_true", help="pause for a yes before each training")
    p.add_argument("--resume", action="store_true", help="carry on a run that stopped")
    a = p.parse_args()
    run_dir = Path(a.run_dir).resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    cfg = {"configurable": {"thread_id": "run"}, "recursion_limit": 500}
    with SqliteSaver.from_conn_string(str(run_dir / CHECKPOINTS)) as saver:
        llm_name = a.model
        # A run folder holds one run. --resume carries it on; otherwise start fresh.
        probe = build(None, run_dir).compile(checkpointer=saver)
        snap = probe.get_state(cfg)
        if snap.values and not a.resume:
            raise SystemExit(f"{run_dir} already holds a run: add --resume, or pick a new --run-dir")
        if a.resume:
            if not snap.values:
                raise SystemExit(f"nothing to resume in {run_dir}")
            if not snap.next:
                raise SystemExit("that run already finished")
            llm_name = snap.values["llm"]     # the model the run started with
        graph = build(ollama_model(llm_name), run_dir).compile(checkpointer=saver)
        log = make_logger(run_dir)
        if a.resume:
            log(snap.values, "resume", next=list(snap.next),
                trainings=snap.values["trainings"], tool_calls=snap.values["calls"])
            drive(graph, cfg, None)
        else:
            s = first_state(llm_name, a.approve)
            log(s, "start", llm=llm_name, harness="langgraph", approve=a.approve, goal=GOAL,
                limits={"max_trainings": config.MAX_TRAININGS,
                        "max_frames": config.MAX_FRAMES,
                        "max_tool_calls": config.MAX_TOOL_CALLS})
            drive(graph, cfg, s)


if __name__ == "__main__":
    main()
