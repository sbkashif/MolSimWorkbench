"""Tests for agent/graph.py, with a scripted LLM and fake tools: no Ollama, no MACE.

    python -m agent.test_graph

Each test plays a run through one path of the graph and checks log.jsonl.
"""
import json
import sqlite3
import tempfile
from pathlib import Path

from langchain_core.messages import AIMessage
from langgraph.checkpoint.sqlite import SqliteSaver

from . import config, graph, tools


class ScriptedLLM:
    """Replies with the next tool call from a list; None means a reply with no call."""

    def __init__(self, script):
        self.script, self.n = list(script), 0

    def invoke(self, messages):
        # The history must never carry back earlier reasoning: harness.py sends
        # only each reply's text and tool calls.
        for m in messages:
            assert "reasoning_content" not in m.additional_kwargs, "reasoning sent back"
        step = self.script[self.n]
        self.n += 1
        calls = [] if step is None else [{"name": step[0], "args": step[1], "id": f"c{self.n}"}]
        return AIMessage(content="", tool_calls=calls,
                         additional_kwargs={"reasoning_content": f"thinking {self.n}"},
                         response_metadata={"prompt_eval_count": 10, "eval_count": 5})


def fake_tools(crash_first_train=False):
    """Replace the real tools with instant ones. Returns a list that counts trainings."""
    started = []

    def train(rs, epochs, channels):
        started.append(epochs)
        if crash_first_train and len(started) == 1:
            raise RuntimeError("killed during training")
        rs.trainings += 1
        rs.model = rs.run_dir / f"model_{rs.trainings}.model"
        rs.model_info = {"frames": rs.frames, "epochs": epochs, "channels": channels}
        return {"model": rs.model.stem, "epochs": epochs}

    tools.train = train
    tools.evaluate = lambda rs: {"force_mae_meV_per_A": 15.0}
    tools.run_md = lambda rs: {"bonds_same_as_start": True}
    tools.final_test = lambda rs: {"force_mae_meV_per_A": 16.0}
    return started


def new_run(llm, run_dir, approve=False):
    saver = SqliteSaver(sqlite3.connect(run_dir / graph.CHECKPOINTS, check_same_thread=False))
    g = graph.build(llm, run_dir).compile(checkpointer=saver)
    cfg = {"configurable": {"thread_id": "run"}, "recursion_limit": 500}
    return g, cfg, graph.first_state("scripted", approve)


def read_log(run_dir):
    return [json.loads(line) for line in open(run_dir / "log.jsonl")]


def test_full_run(run_dir):
    """A refusal, a training, an evaluation, then stop."""
    fake_tools()
    llm = ScriptedLLM([("train", {"epochs": 200, "channels": 64}),
                       ("train", {"epochs": 50, "channels": 32}),
                       ("evaluate", {}),
                       ("stop", {"reason": "target met"})])
    g, cfg, s = new_run(llm, run_dir)
    graph.drive(g, cfg, s)
    log = read_log(run_dir)
    kinds = [e["kind"] for e in log]
    assert kinds == ["llm", "tool", "llm", "tool", "llm", "tool", "llm", "end",
                     "final_test", "summary"], kinds
    assert log[1]["result"] == {"refused_by_harness": "epochs must be an integer from 10 to 150"}
    assert log[0]["thinking"] == "thinking 1" and log[0]["usage"]["input_tokens"] == 10
    assert log[-1]["tool_calls"] == 4 and log[-1]["trainings"] == 1
    assert log[-1]["last_model"] == "model_1.model"


def test_nudges(run_dir):
    """Four replies with no tool call end the run, as in harness.py."""
    fake_tools()
    g, cfg, s = new_run(ScriptedLLM([None] * 4), run_dir)
    graph.drive(g, cfg, s)
    log = read_log(run_dir)
    assert [e["kind"] for e in log] == ["llm"] * 4 + ["end", "summary"]
    assert log[4]["reason"] == "the LLM stopped calling tools"


def test_tool_call_limit(run_dir):
    """The run ends after MAX_TOOL_CALLS calls."""
    fake_tools()
    g, cfg, s = new_run(ScriptedLLM([("add_data", {"frames": 1})] * config.MAX_TOOL_CALLS),
                        run_dir)
    graph.drive(g, cfg, s)
    log = read_log(run_dir)
    assert log[-2]["reason"] == f"tool call limit reached ({config.MAX_TOOL_CALLS})"
    assert log[-1]["tool_calls"] == config.MAX_TOOL_CALLS


def test_resume_after_crash(run_dir):
    """A run killed during a training carries on: the training runs again, counts once,
    and the LLM is not asked again for the step it already chose."""
    started = fake_tools(crash_first_train=True)
    llm = ScriptedLLM([("train", {"epochs": 50, "channels": 32}),
                       ("stop", {"reason": "done"})])
    g, cfg, s = new_run(llm, run_dir)
    try:
        graph.drive(g, cfg, s)
        raise AssertionError("the fake training should have crashed")
    except RuntimeError:
        pass
    assert g.get_state(cfg).next == ("run",)
    # A new graph on the same checkpoint file, as `--resume` makes.
    g2, cfg, _ = new_run(llm, run_dir)
    graph.drive(g2, cfg, None)
    log = read_log(run_dir)
    assert started == [50, 50]                      # the training ran again
    assert llm.n == 2                               # one LLM call per step, none repeated
    assert [e["kind"] for e in log] == ["llm", "tool", "llm", "end", "final_test", "summary"]
    assert log[-1]["trainings"] == 1                # and counted once


def test_stale_checkpoints_removed(run_dir):
    """A cut-off training's MACE checkpoints are deleted before it runs again, so
    MACE cannot load one of them as the final model. Other models' files stay."""
    fake_tools()
    ck = run_dir / "checkpoints"
    ck.mkdir()
    (ck / "model_1_run-1_epoch-80.pt").write_text("stale")
    (ck / "model_0_run-1_epoch-5.pt").write_text("keep")
    llm = ScriptedLLM([("train", {"epochs": 50, "channels": 32}),
                       ("stop", {"reason": "done"})])
    g, cfg, s = new_run(llm, run_dir)
    graph.drive(g, cfg, s)
    assert sorted(f.name for f in ck.iterdir()) == ["model_0_run-1_epoch-5.pt"]


def test_approval(run_dir):
    """With approve on, a refused training goes back to the LLM; an approved one runs."""
    started = fake_tools()
    llm = ScriptedLLM([("train", {"epochs": 150, "channels": 64}),
                       ("train", {"epochs": 50, "channels": 32}),
                       ("stop", {"reason": "done"})])
    g, cfg, s = new_run(llm, run_dir, approve=True)
    answers = iter([{"approved": False, "reason": "too long for now"}, {"approved": True}])
    shown = []
    graph.drive(g, cfg, s, answer=lambda req: (shown.append(req), next(answers))[1])
    log = read_log(run_dir)
    assert [r["args"]["epochs"] for r in shown] == [150, 50]
    assert shown[0]["reasoning"] == "thinking 1"
    assert started == [50]                          # only the approved training ran
    tool_entries = [e for e in log if e["kind"] == "tool"]
    assert tool_entries[0]["result"] == {"refused_by_human": "too long for now"}
    assert [e["approved"] for e in log if e["kind"] == "approval"] == [False, True]


def main():
    real = (tools.train, tools.evaluate, tools.run_md, tools.final_test)
    tests = [test_full_run, test_nudges, test_tool_call_limit,
             test_resume_after_crash, test_stale_checkpoints_removed, test_approval]
    try:
        for t in tests:
            with tempfile.TemporaryDirectory() as d:
                t(Path(d))
            print("ok", t.__name__)
    finally:
        tools.train, tools.evaluate, tools.run_md, tools.final_test = real
    print(f"{len(tests)} tests passed")


if __name__ == "__main__":
    main()
