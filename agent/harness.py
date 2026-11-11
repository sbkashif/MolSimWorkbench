"""The harness: the program around the LLM.

It gives the LLM the goal and the tool list, runs the tool the LLM asks for if the
request is within the limits, hands back the measured result, and writes every step
to a log. The LLM chooses; the harness enforces and records.

    python -m agent.harness --model qwen3:8b --run-dir agent/runs/first
    python -m agent.harness --model claude-sonnet-5-5 --run-dir agent/runs/claude_sonnet

Model names starting with "claude-" go to the Anthropic API (needs ANTHROPIC_API_KEY);
everything else goes to Ollama. --llm overrides the guess.
"""
import argparse
import json
import time
from datetime import datetime
from pathlib import Path

from . import config, tools

# Claude settings, recorded in the "start" entry. Sonnet 5.5 rejects a temperature
# setting and cannot turn thinking fully off, so the run uses adaptive thinking with
# readable summaries (the raw reasoning is never returned) at the default effort.
CLAUDE_THINKING = {"type": "adaptive", "display": "summarized"}
CLAUDE_EFFORT = "high"
CLAUDE_MAX_TOKENS = 16000


def anthropic_tools():
    """tools.describe() in Anthropic's schema: name, description, input_schema."""
    return [{"name": t["function"]["name"], "description": t["function"]["description"],
             "input_schema": t["function"]["parameters"]} for t in tools.describe()]

GOAL = f"""You are training a machine learning interatomic potential (a MACE model) for
the ethanol molecule, using DFT data from the rMD17 dataset.

The goal: a model with a force error below {config.TARGET_FORCE_MAE} meV/Å on the
selection frames (the frames evaluate uses, never trained on), that also runs
stably in MD: over 1 ps starting at
{config.MD_TEMPERATURE:.0f} K, the total energy should stay within
{config.MD_DRIFT_LIMIT} eV, no two atoms should come closer than
{config.MIN_PAIR_LIMIT} Å, and the bonds should stay the same.

You start with {config.START_FRAMES} training frames and can add more, up to
{config.MAX_FRAMES}. You may train at most {config.MAX_TRAININGS} times. Training takes
minutes, so do not waste trainings. Use the tools one at a time, read each result,
and decide the next step. Every model must be evaluated and checked in MD before you
can say it meets the goal. Call stop when the goal is met, or when it cannot be met
within the limits, and give the reason. After you stop, the latest model is scored
once on a separate set of test frames that you never see."""


def check(name, args, state):
    """Refuse a tool call that breaks a limit. Returns a reason, or None if allowed."""
    # train: trainings left, epochs in range, a known width.
    if name == "train":
        e, c = args.get("epochs"), args.get("channels")
        if state.trainings >= config.MAX_TRAININGS:
            return f"no trainings left (limit {config.MAX_TRAININGS})"
        if not isinstance(e, int) or not config.EPOCH_RANGE[0] <= e <= config.EPOCH_RANGE[1]:
            return f"epochs must be an integer from {config.EPOCH_RANGE[0]} to {config.EPOCH_RANGE[1]}"
        if c not in config.CHANNEL_CHOICES:
            return f"channels must be one of {list(config.CHANNEL_CHOICES)}"
    # add_data: a positive number, and frames left in the pool.
    if name == "add_data":
        n = args.get("frames")
        if not isinstance(n, int) or n <= 0:
            return "frames must be a positive integer"
        if state.frames >= config.MAX_FRAMES:
            return f"the pool is used up: already {state.frames} frames"
    # evaluate and run_md need a model to work on.
    if name in ("evaluate", "run_md") and state.model is None:
        return "no model trained yet"
    return None


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--model", default="qwen3:8b",
                   help="Ollama model name, or a Claude model id such as claude-sonnet-5-5")
    p.add_argument("--llm", choices=["auto", "ollama", "anthropic"], default="auto",
                   help="backend; auto means anthropic for names starting with claude-")
    p.add_argument("--run-dir", default=f"agent/runs/{datetime.now():%Y%m%d_%H%M}")
    a = p.parse_args()
    backend = a.llm if a.llm != "auto" else (
        "anthropic" if a.model.startswith("claude-") else "ollama")
    if backend == "anthropic":
        import anthropic
        client = anthropic.Anthropic(max_retries=5)
        claude_tools = anthropic_tools()
    else:
        import ollama
    run_dir = Path(a.run_dir).resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    log_file = open(run_dir / "log.jsonl", "a")
    t0 = time.time()

    def log(kind, **fields):
        entry = {"t_min": round((time.time() - t0) / 60, 2), "kind": kind, **fields}
        log_file.write(json.dumps(entry) + "\n")
        log_file.flush()
        print(json.dumps(entry)[:400], flush=True)

    state = tools.RunState(run_dir)
    run = {"train": lambda **k: tools.train(state, **k),
           "evaluate": lambda **k: tools.evaluate(state),
           "run_md": lambda **k: tools.run_md(state),
           "add_data": lambda **k: tools.add_data(state, **k)}
    messages = [{"role": "user", "content": GOAL}]
    if backend == "anthropic":
        log("start", llm=a.model, backend="anthropic", goal=GOAL,
            llm_settings={"thinking": CLAUDE_THINKING, "effort": CLAUDE_EFFORT,
                          "max_tokens": CLAUDE_MAX_TOKENS, "temperature": "not settable"},
            limits={"max_trainings": config.MAX_TRAININGS,
                    "max_frames": config.MAX_FRAMES,
                    "max_tool_calls": config.MAX_TOOL_CALLS})
    else:
        log("start", llm=a.model, goal=GOAL, limits={
            "max_trainings": config.MAX_TRAININGS, "max_frames": config.MAX_FRAMES,
            "max_tool_calls": config.MAX_TOOL_CALLS})

    def ask_claude():
        """One Claude turn. Returns the tool calls as (name, args, id)."""
        t = time.time()
        reply = client.messages.create(
            model=a.model, max_tokens=CLAUDE_MAX_TOKENS, messages=messages,
            tools=claude_tools, thinking=CLAUDE_THINKING,
            output_config={"effort": CLAUDE_EFFORT})
        blocks = reply.content
        uses = [b for b in blocks if b.type == "tool_use"]
        u = reply.usage
        log("llm", seconds=round(time.time() - t, 1),
            thinking="\n\n".join(b.thinking for b in blocks
                                 if b.type == "thinking" and b.thinking) or None,
            text="".join(b.text for b in blocks if b.type == "text"),
            tool_calls=[{"name": b.name, "args": dict(b.input)} for b in uses],
            stop_reason=reply.stop_reason, served_by=reply.model,
            usage={"input_tokens": u.input_tokens, "output_tokens": u.output_tokens,
                   "cache_read_input_tokens": u.cache_read_input_tokens,
                   "cache_creation_input_tokens": u.cache_creation_input_tokens})
        # Blocks go back unchanged (thinking blocks included), as the API requires.
        messages.append({"role": "assistant", "content": blocks})
        return [(b.name, dict(b.input), b.id) for b in uses]

    calls, nudges = 0, 0
    while calls < config.MAX_TOOL_CALLS:
        if backend == "anthropic":
            claude_calls = ask_claude()
            if not claude_calls:
                nudges += 1
                if nudges > 3:
                    log("end", reason="the LLM stopped calling tools")
                    break
                messages.append({"role": "user", "content":
                                 "Call one of the tools, or call stop with a reason."})
                continue
            name, args, use_id = claude_calls[0]   # one at a time, as the goal asks
            skipped = claude_calls[1:]
        else:
            t = time.time()
            reply = ollama.chat(model=a.model, messages=messages, tools=tools.describe(),
                                think=True, options={"temperature": 0, "seed": 1,
                                                     "num_ctx": 16384})
            msg = reply.message
            log("llm", seconds=round(time.time() - t, 1), thinking=msg.thinking,
                text=msg.content, tool_calls=[{"name": c.function.name,
                                               "args": dict(c.function.arguments)}
                                              for c in (msg.tool_calls or [])],
                usage={"input_tokens": reply.prompt_eval_count,
                       "output_tokens": reply.eval_count})
            messages.append({"role": "assistant", "content": msg.content or "",
                             "tool_calls": msg.tool_calls or []})
            if not msg.tool_calls:
                nudges += 1
                if nudges > 3:
                    log("end", reason="the LLM stopped calling tools")
                    break
                messages.append({"role": "user", "content":
                                 "Call one of the tools, or call stop with a reason."})
                continue
            call = msg.tool_calls[0]           # one at a time, as the goal asks
            name, args = call.function.name, dict(call.function.arguments)
        calls += 1
        if name == "stop":
            log("end", reason=args.get("reason", ""))
            break
        refusal = check(name, args, state) if name in run else f"unknown tool {name}"
        if refusal:
            result = {"refused_by_harness": refusal}
        else:
            result = run[name](**args)
        log("tool", name=name, args=args, result=result)
        if backend == "anthropic":
            # Every tool_use needs a tool_result; extra calls are answered, not run.
            results = [{"type": "tool_result", "tool_use_id": use_id,
                        "content": tools.as_text(result)}]
            for s_name, s_args, s_id in skipped:
                log("tool", name=s_name, args=s_args,
                    result={"refused_by_harness":
                            "not run: one tool at a time, only the first call ran"})
                results.append({"type": "tool_result", "tool_use_id": s_id,
                                "is_error": True, "content":
                                "Not run: use the tools one at a time. Only the first "
                                "tool call in your reply was run."})
            messages.append({"role": "user", "content": results})
        else:
            messages.append({"role": "tool", "content": tools.as_text(result),
                             "tool_name": name})
    else:
        log("end", reason=f"tool call limit reached ({config.MAX_TOOL_CALLS})")
    # Scored once, on frames the agent never saw, after it has made every decision
    if state.model is not None:
        log("final_test", result=tools.final_test(state))
    log("summary", minutes=round((time.time() - t0) / 60, 1), tool_calls=calls,
        trainings=state.trainings, final_frames=state.frames,
        last_model=state.model.name if state.model else None)


if __name__ == "__main__":
    main()
