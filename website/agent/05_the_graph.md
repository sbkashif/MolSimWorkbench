# The loop as a graph

:::{admonition} Draft
:class: warning
This page is being built. The code shown is read straight from the `agent/`
folder of the repository. The run on this page was made on 2026-11-11 with
`agent/graph.py`; the two runs on the earlier pages were made with
`agent/harness.py`, which is unchanged.
:::

[How our agent was built](04_the_code.md) ended with the harness's loop: a
`while` loop that sends the history, checks the call, runs the tool and logs
it. That loop has two weak points that matter once a run takes more than a few
minutes. The Qwen3 run took 36 minutes, and the training is most of that. If
the laptop sleeps or the program crashes in the middle, the whole run is lost
and must start again. And once it starts, nobody can step in: every training
the LLM chooses runs.

This page writes the same loop a second way, as a graph, using two common
libraries, LangChain and LangGraph. The goal, the tools, the limits and the log
stay the same. Two things are new: a run that stops part-way can carry on from
where it stopped, and the run can pause before each training until a person
says yes.

## What changes, and what does not

In the terms of [The case for an agent](02_the_agent.md), this is still
level 3. The LLM still chooses each step, and the script still runs and checks
it. The libraries build the script; they do not change who decides.

- **LangChain** gives every LLM the same interface. The script hands it the
  tool list once (`bind_tools`), and each reply comes back in the same form,
  with the tool calls as a list of names and inputs, whichever LLM sent it.
  `harness.py` talks to Ollama and to Anthropic's API directly, with code for
  each.
- **LangGraph** runs a loop written as a graph: a set of steps, called
  *nodes*, and arrows between them. After every node it can save everything the
  run needs to carry on, called the *state*, to a file.

LangChain also ships ready-made agents that run the whole loop for you. We do
not use them: they would hide the steps this module is about. Every node and
arrow below is our own code.

## The graph

The loop of `harness.py`, drawn as a graph ({numref}`agent-graph-figure`):

```{mermaid}
:name: agent-graph-figure
:caption: The loop in `agent/graph.py`. Every arrow is fixed in code, except the ones out of `llm`, which the LLM's reply picks. `approve` is used only with `--approve`.

flowchart TD
    llm -- no tool call --> nudge
    nudge -- ask again --> llm
    llm -- a tool call --> check
    llm -- stop --> final
    check -- breaks a limit --> respond
    check -- allowed --> run
    check -- "train, with --approve" --> approve
    approve -- yes --> run
    approve -- "no, with a reason" --> respond
    run --> respond
    respond --> llm
    respond -- 20 tool calls --> final
    nudge -- "after 3 reminders" --> final
```

Each node is one part of the old loop:

| Node | What it does | In `harness.py` |
| --- | --- | --- |
| `llm` | sends the history, logs the reply | the call to `ollama.chat` |
| `nudge` | reminds the LLM to call a tool | the same reminder, at most three |
| `check` | the limit checks | `check`, imported unchanged |
| `approve` | waits for a person's yes | new |
| `run` | runs the tool | the same tools |
| `respond` | logs the step, hands the result back | the same log line |
| `final` | the final test and the summary | the same, after the loop |

Most of the arrows are the harness's rules: a call that breaks a limit goes to
`respond` with the reason, never to `run`. Only the arrows out of `llm` depend
on the LLM. That one choice is what makes it an agent, as in
[The three levels side by side](02_the_agent.md#the-three-levels-side-by-side).

## The state

The state is everything a stopped run needs to carry on. LangGraph saves it
after every node, to a SQLite file in the run's folder:

```{literalinclude} ../../agent/graph.py
:language: python
:pyobject: State
```

`messages` is the history the LLM is sent. The four fields after `started`
are what the tools keep in `RunState` in `harness.py`: how many frames the next
training uses, how many trainings have run, and the latest model.

## The nodes

The `llm` node sends the history, writes the same `llm` line to the log as the
harness, and picks the next node from the reply:

```{literalinclude} ../../agent/graph.py
:language: python
:start-at: "    def llm_node"
:end-before: "    def nudge"
```

`Command(goto=..., update=...)` is how a node says where the run goes next and
what changes in the state. `llm.invoke` is LangChain: the same call for any
LLM.

The line that builds `reply` is there because of a difference we found by
comparing logs. LangChain's Ollama interface sends each earlier reply's
reasoning back to the LLM with the history; `harness.py` sends only the reply's
text and tool calls. In the first graph run, the LLM read about 450 more tokens
at its second step than in the harness's run, all of it its own earlier
reasoning. The reasoning still goes to the log, but not back into the history.

The `check` node calls the harness's own `check`, so the limits are the same
code:

```{literalinclude} ../../agent/graph.py
:language: python
:start-at: "    def check_node"
:end-before: "    def approve"
```

## Carrying on after a stop

Because the state is saved after every node, a run that stops can carry on:

```bash
python -m agent.graph --run-dir agent/runs/graph_qwen3 --resume
```

LangGraph reads the last saved state and runs the next node. If the run
stopped during a training, the `run` node had not finished, so nothing from it
was saved: the training runs again, from the start, and counts once. The LLM is
not asked again; its choice was saved when the `llm` node finished.

We tested this on a real run with Qwen3 8B. The run was killed on purpose a
minute into its second training, after four tool calls. On `--resume`, the
first line written to the log was

```json
{"t_min": 195.28, "kind": "resume", "next": ["run"], "trainings": 1, "tool_calls": 4}
```

so the run carried on at the `run` node, with one training done. The second
training started again from its first epoch, with the settings the LLM had
already chosen (700 frames, 64 channels, 100 epochs), and the log has one
`tool` line for it. `t_min` counts minutes on the clock since the run started,
so it includes the three hours the run sat stopped; the steps themselves took
about 15 minutes.

The test also turned up a trap that the graph now handles. At the end of a
training, MACE loads the checkpoint file with the highest epoch number, and it
deletes only the files it wrote itself. A cut-off attempt that got further
than the new one would leave a file that MACE then loads as the final model,
without any error. Before each training, the `run` node deletes any
checkpoints left under that model's name.

## Pausing for a yes

With `--approve`, the run pauses before each training. It shows the call the
LLM chose and its reasoning, and waits:

```{literalinclude} ../../agent/graph.py
:language: python
:start-at: "    def approve"
:end-before: "    def run_node"
```

`interrupt` stops the run and saves it; the program asks the person, and the
answer comes back as the result of `interrupt`. A yes goes on to `run`. A no,
with a reason, goes back to the LLM as the tool's result, in the same way as a
limit the harness refuses: `{"refused_by_human": "..."}`. Both answers are
written to the log as an `approval` line.

The pause gives a person back the deciding for the step that costs most,
without giving up the agent for the others. It is off by default, so a run can
still go from start to end alone, as the two runs on the earlier pages did.

## The same run, in both versions?

The graph version sends Qwen3 the same goal, tools and settings as
`harness.py`, with temperature 0 and a fixed seed. So the first question is
whether it makes the same choices as the Qwen3 run on the earlier pages.

At the first step it does, exactly: the same tool call, `train` with 50 epochs
and 32 channels, the same 661 tokens read and 480 written, and the same
reasoning, word for word. At each of the first four steps the LLM read the
same number of tokens as in the harness's run (661, 753, 879 and 933), so the
history it was sent had the same shape.

At the second step the two runs part. The LLM chose the same tool, `evaluate`,
but its reasoning was twice as long, and at the third step it added 500 frames
instead of 200. From then on it was a different run:

| | Harness, 2026-11-09 | Graph, 2026-11-11 |
| --- | --- | --- |
| Tool calls | 14 | 7 |
| Trainings | 4 | 2 |
| Training frames at the end | 800 | 700 |
| Selection force error, last model | 14.1 meV/Å | 19.3 meV/Å |
| Final test force error | 14.2 meV/Å | 19.2 meV/Å |
| MD check | passed | passed |

The cause was not LangChain. The first training's result includes how long it
took, and it took 0.8 minutes this time against 0.9 in the harness's run. When
we sent the LLM the harness's history with only that number changed, it gave
this run's second reply: the same call, and the same 1330 tokens written. It
did so both through LangChain and through the Ollama client that `harness.py`
uses. With 0.9, it gave the harness's reply, word for word.

Temperature 0 does not make Qwen3 fully repeatable, though. In one of three
tries of that same request through the Ollama client, the LLM did not stop
reasoning: it wrote 163,840 tokens, the most it was allowed, and called no
tool. The other two tries gave the reply above.

So a small difference in a measured time was enough to change the run, even at
temperature 0. That says more about the runs on the earlier pages than about
the graph: running the harness again would not repeat them either, because the
time a training takes is never quite the same. Both runs met the goal, and as
before, one run each cannot say which path is better.

## Run it yourself

The setup is the same as in
[What our agent did](03_first_run.md#run-it-yourself): the `mlip-agent` conda
environment, the rMD17 data and Ollama with Qwen3 8B. Then, from the
repository root:

```bash
python -m agent.graph --model qwen3:8b --run-dir agent/runs/graph_mine
```

Stop it at any point with Ctrl-C, and carry on with

```bash
python -m agent.graph --run-dir agent/runs/graph_mine --resume
```

Add `--approve` to a new run to be asked before each training. The graph's
paths can be tested without Ollama or MACE, with a scripted LLM and tools that
answer at once:

```bash
python -m agent.test_graph
```

`graph.py` works with Ollama models only for now.

## Check yourself

:::{dropdown} Is the graph version still an agent, or a workflow?
An agent. Most arrows are fixed in code, but the arrow out of `llm` depends on
the tool the LLM calls. In a workflow every arrow is fixed.
:::

:::{dropdown} A run stops halfway through its third training. What happens on `--resume`?
The third training runs again from the start, and counts as one training. The
LLM is not asked again, because its choice was saved when the `llm` node
finished.
:::

:::{dropdown} With `--approve`, the person says no to a training. What does the LLM see?
The reason, as the result of its tool call: `{"refused_by_human": "..."}`,
in the same way as a limit the harness refuses. It then chooses again.
:::

:::{dropdown} Why does the `llm` node not keep the reasoning in the history?
To send the LLM the same history as `harness.py`. LangChain's Ollama interface
would send every earlier reasoning back, so the LLM would read more than in
the harness's runs, and could choose differently.
:::

## Next

The graph keeps the agent the same and makes its loop sturdier: a run can stop
and carry on, and a person can be asked before each training. Writing it also
showed that the LLM's choices depend on something nobody chose, the time a
training happens to take.

That changes the next steps from [How our agent was built](04_the_code.md#next).
Repeating the runs is still the way to learn how often the agent meets the
goal, and the graph makes long batches of runs safer to leave alone. Before
repeating them, the training time could be left out of what the LLM reads, so
that the same choices give the same history. Moving the order of work into
`check` would apply to both versions, since the graph uses the same `check`.

This is the last page of the module for now. To read it again from the task
itself, go back to [the first page](01_training_an_mlip.md).
