# How our agent was built

:::{admonition} Draft
:class: warning
This page is being built. The code shown is read straight from the `agent/`
folder of the repository. It is the code that made both runs on
2026-11-09.
:::

[What our agent did](03_first_run.md) followed the agent as one whole: an LLM
that chooses, inside a script that runs and checks each step. That pattern is
not special to training MLIPs. Every agent has it, and its pieces have standard
names. This page first gives those names, then opens each piece and goes
through our code, one file at a time.

## Understanding an agent

### The four parts

In the terms used for agents, the loop from level 3 of
[The case for an agent](02_the_agent.md) has four
parts ({numref}`agent-parts-figure`):

- The **LLM**: Qwen3 8B on the laptop in one run, Claude Sonnet 5.5 through
  Anthropic's API in the other. The script talks to it directly, not
  through a chatbot app.
- The **tools** are the programs in `tools`: Python functions, each with a
  description and its inputs.
- A **tool call** is the LLM's answer, `choice` in the loop: a tool's name and
  its inputs.
- The **harness** is the script that joins them: it sends the history, checks
  each tool call against the limits, runs it, and records the result. Here it
  is one Python file, `agent/harness.py`.

```{figure} ../figures/agent_parts.svg
:name: agent-parts-figure

The parts of the agent in the Qwen3 run. The LLM chooses, the harness
enforces and records, and the tools do the work.
```

### Agents in MD and MLIP work

Agents like this are already used around MD and MLIPs, with more or less
freedom. Some, like ours, choose among fixed tools: MDCrow
runs MD workflows with over 40 expert-designed tools
([Campbell et al. 2025](https://arxiv.org/abs/2502.09565)), and Lang2MLIP picks
each step of MLIP development from the data, the model and its errors
([Li et al. 2026](https://arxiv.org/abs/2605.14527)). Others go further and
edit the training code itself, keeping a change only if it passes a fixed set
of checks ([Osaro et al. 2026](https://arxiv.org/abs/2605.30889)). The agent
here is at the simple end: it picks from five tools and writes no code.

## Our code

Our agent is four Python files in the `agent/` folder, about 500 lines in all.
Each has one job:

| File | What it holds |
| --- | --- |
| `config.py` | the numbers in the goal, and the limits the harness enforces |
| `data.py` | the step that turns rMD17 ethanol into files MACE can read: the training, selection and final test frames |
| `tools.py` | the four tools (`train`, `evaluate`, `run_md`, `add_data`); `final_test`, which only the harness calls; and `describe`, the tool list the LLM reads |
| `harness.py` | the goal message, `check`, and the loop that joins the LLM to the tools |

None of these files trains a model, runs MD or chooses a step. MACE trains,
OpenMM runs the MD, and the LLM chooses. The four files connect them, and hold
the limits the LLM cannot change.

The sections below take the files in the order of the table. The settings
come first because every other file reads them; then the frames the tools
train on; then the tools; and last the harness that calls them.

## The settings: `config.py`

Every number the run depends on sits in one file, so that changing the goal or
a limit never means hunting through the code:

```{literalinclude} ../../agent/config.py
:language: python
```

The values fall into two groups. The first group is the **goal**: what counts
as a good model. These numbers go into the message the LLM reads before it
starts. The second group is the **limits**: what the agent is allowed to
spend. The LLM is told these too, but it does not have to respect them, because
the harness checks every call against them before running it. The goal is a
request. The limits are enforced. The rest says where the data and programs
are, and how the test frames are split.

None of these numbers was tuned. They were picked by hand for a first try, and
the comments say which ones rest on a reason.

The frames the models are judged on come in two sets. The **selection
frames** are 500 frames drawn at random from the 98,000 that split 01 uses for
neither training nor testing. `evaluate` uses them, and the LLM decides on
their errors, so they are no longer an untouched test. The **final test
frames** are the split's official test set, all 1000. The harness scores the
last model on them once, after the LLM stops, and the LLM never sees them. The
rMD17 readme limits how many frames a model trains on, not how many it is
tested on, so the two sets need not share the official 1000.

## The training frames: `data.py`

rMD17 stores each molecule as one NumPy file of positions, energies and forces,
plus files listing which frames belong to the official training and test sets.
MACE reads a different format, extended XYZ, so `data.py` converts between
the two:

```{literalinclude} ../../agent/data.py
:language: python
:pyobject: write_frames
```

The selection frames come from `_selection`, which takes every frame the split
leaves out and draws 500 of them, the same 500 in every run:

```{literalinclude} ../../agent/data.py
:language: python
:pyobject: _selection
```

A last function writes the final test frames. Only the harness calls it:

```{literalinclude} ../../agent/data.py
:language: python
:pyobject: write_final_test
```

The one decision in this file is how adding data works. The 1000 training
frames are shuffled once, in a fixed order (the random seed is 0), and a run
with `n` frames always takes the first `n` of that order. So `add_data` can only
ever add frames: the 400 frames of the Qwen3 run's second training were the
200 frames of its first training plus 200 new ones. And the same `n` always
means the same frames, so the two runs started from the same 200 frames, and
their last models were trained on the same 800.

The conversion also changes units. rMD17 gives energies in kcal/mol and forces
in kcal/mol/Å. The MACE models here are trained in eV and eV/Å, so every value
is multiplied by 0.0433641. OpenMM itself works in kJ/mol; OpenMM-ML converts
the model's output, and `run_md` converts back to eV when it reports.

## The tools: `tools.py`

Each tool is an ordinary Python function that does real work and returns a
small dictionary of measured numbers. The tools share one object,
`RunState`, which remembers what the run has done so far:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: RunState
```

The LLM never sees this object. It sees only what the tools return.

### `train`

`train` runs MACE's own training program, `mace_run_train`, as a separate
process, and waits for it to finish. The program comes with MACE's Python
package, `mace-torch`, which is built on PyTorch. It runs the training cycle of
{numref}`fit-loop-figure` and saves the trained model to a file, so our code
contains no training loop of its own:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: train
```

Most of the command is fixed. The LLM chooses only two settings: `epochs`, the
number of passes through the training frames, and `channels`, how many numbers
describe each atom inside the model (the width). Everything else stays the
same from one training to the next:

| Setting | Value | What it means |
| --- | --- | --- |
| `--valid_fraction` | 0.1 | 10% of the training frames are held back to report a validation error |
| `--r_max` | 4.0 Å | an atom only sees neighbours within 4 Å |
| `--E0s` | `average` | the per-element baseline energies are fitted from the average frame energy |
| `--hidden_irreps` | `{channels}x0e` | the width; `0e` keeps only rotation-invariant features, the cheapest kind |
| `--device`, `--default_dtype` | `cpu`, `float32` | the laptop has no CUDA GPU |
| `--seed` | 1 | the same settings give the same model on the same machine |

::::{dropdown} Detour: the command on the first page's figures
:class-container: detour-dropdown

Almost every number in the first page's picture of MACE is set by one setting
of this program. Some are in our command; the rest are MACE's defaults, which
the command leaves alone. MACE writes all of them to its log at the start of a
training.

| Setting | Value | Where it is on the first page |
| --- | --- | --- |
| `--hidden_irreps` (the LLM's `channels`) | `32x0e` | the 32 numbers per atom in every stage of {numref}`mace-steps-figure`, and the 32 + 32 = 64 squares of $\mathbf{h}_i$ in {numref}`mlip-atom-figure` |
| `--r_max` | 4.0 Å | the 4 Å cutoff in Stage 1 |
| `num_radial_basis` (default) | 8 | a distance becomes 8 numbers |
| `max_ell` (default) | 3 | the direction numbers, in groups of 1, 3, 5 and 7 |
| `radial_MLP` (default) | [64, 64, 64] | the chain 8 → 64 → 64 → 64 → 128 in Stage 2 |
| `correlation` (default) | 3 | Stage 3 multiplies up to three at a time |
| `num_interactions` (default) | 2 | Round 1 and Round 2 |
| `--E0s` | `average` | the baselines, such as −616.0 eV for hydrogen |
| `--valid_fraction`, `--batch_size`, `--max_num_epochs` (the LLM's `epochs`) | 0.1, 10, 10 to 150 | the validation frames, the batch and the epoch in {numref}`fit-loop-figure` |

So when the LLM asks for 64 channels, every 32 in {numref}`mace-steps-figure`
becomes 64. The weights grow from 67,472 to 172,816, about 2.6 times as many,
because most of them sit in 32 × 32 mixes that become 64 × 64. The chain in
Stage 2 keeps its middle 64s, which come from `radial_MLP`, but its output
grows from 128 to 256 numbers: 64 × 4 instead of 32 × 4.
::::

When training ends, the tool reads the last validation force error that MACE
printed in its log and returns it, with the time taken. The validation frames
come out of the training frames, so this number is only a first look. The
selection frames are kept for `evaluate`.

:::{admonition} A simplification to know about
:class: note
`{channels}x0e` builds a MACE model with only invariant features. Full MACE
models also carry features that turn with the molecule (for example `64x0e +
64x1o`), which are usually more accurate and slower to train. Both runs used
the cheap kind to keep each training to minutes on a CPU.
:::

### `evaluate`

`evaluate` loads the latest model and compares its predictions with the DFT
values on the 500 selection frames. The comparison itself is in `errors`, which
the final test reuses:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: errors
```

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: evaluate
```

It returns four errors. The **force error** is the mean absolute difference over
every force component on every atom, in meV/Å; this is the number the goal
targets. The **energy error** is the mean absolute difference in total energy,
divided by the 9 atoms of ethanol. The other two separate out a constant
offset: if the model is wrong by the same amount on every frame, that shift
moves all energies up or down together and leaves the forces, and so the
dynamics, unchanged. [The first page](01_training_an_mlip.md) met this shift
on its test frames. The agent's first models had it too: Qwen3's first model
had an energy error of 68.3 meV/atom, and 2.1 once the shift was taken away.
Its last model, trained longer on more frames, has 0.4 either way.

### `run_md`

`run_md` puts the model into OpenMM as its energy function and runs 1 ps of
MD from the first selection frame. OpenMM, not this site's engine: that engine
works in reduced units with every mass fixed at 1.0, and it cannot call a
trained model. Adding that would be a separate project and would not test the
training loop, so OpenMM sits beside it as a second engine. The MLIP fills the
same slot there that LJ fills here.


```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: run_md
```

The model goes in through OpenMM-ML: `MLPotential("mace", modelPath=...)`
builds an OpenMM system whose only force is the MACE model. From there it is
an ordinary OpenMM run: OpenMM's `VerletIntegrator`, which is the leapfrog
form of Verlet, with no thermostat, 0.5 fs steps,
velocities drawn at 500 K, single precision on the CPU. With no thermostat the
total energy should stay constant, so how far it wanders tests whether the
model's forces are consistent with its energies.

Every 50 steps (25 fs) the tool records the total energy and the closest
distance between any two atoms. Two atoms that came close between those
readings and moved apart again would be missed. At the end it checks the bonds: two atoms count as
bonded if they are closer than 1.2 times the sum of their covalent radii, and
the list of bonded pairs at the end must match the list at the start. A
molecule that broke apart, or a hydrogen that hopped to another atom, would
fail this check.

The first energy reading is taken after 50 steps, not at the start. A test
made before the agent existed found a one-off jump in the total energy between
the reading at the start and the next one. The jump did not grow with time
and did not change with the time step. Leapfrog keeps the velocities half a
step apart from the positions, which can shift the first kinetic energy
reading, but that shift would shrink with a smaller time step and this one did
not, so the cause is not yet known. Reading from step 50 on leaves the jump out and measures
only how the energy changes during the run.

The temperature it reports at the end comes from the kinetic energy. It is
not controlled: both runs started at 500 K and ended near 600 K. With only 9
atoms, kinetic and potential energy trade back and forth in large swings, so a
single reading at the end can sit well above or below the start, while the
total stays within its limit.

### `add_data`

The smallest tool. It moves the pointer into the shuffled pool, so that the
next training uses more frames, and stops at the end of the pool:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: add_data
```

It does not train anything. Adding data and training are separate tools, so
the LLM can change the data and the model settings in one training, or one at
a time.

### What the LLM sees: `describe`

The LLM cannot read the Python above. What it reads is a description of each
tool: a name, a sentence on what it does, and the inputs it takes, written as
JSON Schema. This is the entry for `train`:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: describe
:lines: 1-12
```

The descriptions are the LLM's whole knowledge of the tools. If one said
"`train` is fast", the LLM would have no way to know better until it tried.
So they are kept short and true, and they repeat the limits that matter.

## The harness: `harness.py`

The harness is the program around the LLM. It holds three things: the goal, a
function that checks each call, and the loop.

### The goal

The goal is one message, built from the values in `config.py`, that the LLM
reads before anything else.
[What it must reach: the goal](02_the_agent.md#what-it-must-reach-the-goal)
lists what it asks for. Two
sentences in it set the order of work: every model must be evaluated and
checked in MD before the LLM says it meets the goal, and the LLM must call
`stop` with a reason when it is done or cannot succeed.

### The check

Before a tool runs, `check` compares the call with the limits:

```{literalinclude} ../../agent/harness.py
:language: python
:pyobject: check
```

If it finds a problem, the tool does not run. The harness sends the reason
back as the tool's result, as if the tool had answered, and the LLM has to
choose again. This happened once, in the Qwen3 run: it asked for 200 epochs,
above the limit of 150, and asked again with 150.

`check` enforces the limits, but not the order of work. If the LLM called
`stop` straight after training, without `evaluate` or `run_md`, the harness
would end the run. Both runs did the checks because the goal asked for them,
not because the code required them. Moving that rule from the goal
into `check` is a small change, and one of the next steps.

### The loop

The rest of `harness.py` is one loop. On each pass it:

1. sends the LLM the goal, the tool descriptions and the whole history so far;
2. reads the reply, which may contain a tool call;
3. if there is no tool call, reminds the LLM to call one (it gives up after
   three reminders);
4. if the call is `stop`, ends the run with the LLM's reason;
5. otherwise runs `check`, then the tool, and adds the call and its result to
   the history;
6. writes every step to the log.

The loop also ends after 20 tool calls, so a run cannot go in circles.

When the loop ends, for whatever reason, and a model has been trained, the
harness runs one more check that the LLM did not ask for: `final_test`, the same errors as `evaluate`, on the
final test frames. It writes the result to the log as a `final_test` line.
The LLM has already made every decision, so nothing it chose was tuned to
these frames:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: final_test
```

In the Qwen3 run, the last model scored 14.2 meV/Å on the final test frames,
against 14.1 on the selection frames; in the Claude run, 17.1 against 17.2.

The harness runs only the first tool call in a reply, since the goal asks for
one at a time and each result should shape the next choice. With Claude, any
other calls in the reply get an error back saying they were not run; this
happened once in the Claude run.

The harness also asks the LLM for its reasoning as well as its answer. For
Qwen3 (`think=True`) it sets temperature 0 and a fixed seed, so that the same
history gives the same reply as often as the model allows. Claude does not
accept a temperature setting, so its replies to the same history can differ
from run to run.

The LLM is a setting, chosen by `--model` when the run starts. A name such as
`qwen3:8b` goes to Ollama on the same machine. A name starting with `claude-`
goes to Anthropic's API instead, which needs the reader's own API key. The
Claude run, described in [What our agent did](03_first_run.md), used
`claude-sonnet-5-5` this way; it cost a few cents.

## What the log records

Every pass writes one line of JSON to `log.jsonl` in the run's folder. Every
figure and number about the two runs on the previous page comes from these
files.
These two lines are from the Qwen3 run, word for word. First the LLM's choice
(its reasoning, which runs to several paragraphs, is cut here):

```json
{"t_min": 0.74, "kind": "llm", "seconds": 44.4, "thinking": "Okay, let's see. ...", "text": "", "tool_calls": [{"name": "train", "args": {"epochs": 50, "channels": 32}}], "usage": {"input_tokens": 661, "output_tokens": 480}}
```

then what the tool measured:

```json
{"t_min": 1.74, "kind": "tool", "name": "train", "args": {"epochs": 50, "channels": 32}, "result": {"model": "model_1", "training_frames": 200, "epochs": 50, "channels": 32, "minutes": 0.9, "validation_force_mae_meV_per_A": 53.55}}
```

`t_min` is the minutes since the run started, `seconds` is how long the LLM
took to reply, and `usage` counts the tokens it read and wrote. The `thinking` field keeps the LLM's reasoning, which is how the
previous page could point out where it misread a number. For Claude it can
hold only a summary of the reasoning, since Anthropic's API returns that, not
the full text; in the run on the previous page Claude's replies were short
and the field stayed empty.
The log also keeps
the goal, the limits and a summary line at the end, so a run can be read back
without the code.

## Run it yourself

The steps to repeat either run, with Qwen3 on your own machine or Claude over
Anthropic's API, are at the end of
[What our agent did](03_first_run.md#run-it-yourself).

## Check yourself

:::{dropdown} What does the LLM do, and what does it not do?
It chooses the next step and its settings. It does not produce any of the
numbers: those come from its tools, and the harness enforces the limits.
:::

:::{dropdown} The LLM remembers nothing between calls. How does it know what it has already tried?
The harness sends the whole history on every pass, and the LLM rereads it.
:::

:::{dropdown} Why does the agent use OpenMM rather than the engine built on this site?
The engine has no way to call a trained model, and it uses reduced units with
masses fixed at 1.0. OpenMM already runs MLIPs, and the agent is about the
training loop, not about extending the engine.
:::

:::{dropdown} In the Qwen3 run, the LLM asked for a training with 200 epochs. What happened?
`check` refused it, because epochs must be from 10 to 150. The tool did not
run. The LLM got the reason back as the tool's result and asked again with
150.
:::

:::{dropdown} After `add_data` with 200 frames, which frames does the next training use?
The first 400 frames of the fixed shuffled pool: the same 200 as before, plus
the next 200. Adding data never swaps frames out.
:::

:::{dropdown} Why does `run_md` start reading the energy after 50 steps?
To leave out a one-off jump in the total energy between the first reading
and the next, whose cause is not yet known. The check is meant to measure how
the energy changes during the run.
:::

:::{dropdown} Which rule does the goal ask for that the harness does not enforce?
That every model is evaluated and checked in MD before the LLM stops. `check`
tests the limits on each call, but it would let `stop` through at any point.
:::

## Next

The same code ran twice, once with Qwen3 8B and once with Claude Sonnet 5.5,
and both runs met the goal, by different paths and with different models.
What the two runs cannot say is how often that would happen, or which LLM
chooses better: each LLM ran once. The next
steps are to repeat the runs, and to move the order of work from the goal into
`check`, so that a model cannot be declared done without its checks.

Before either, the loop itself can be made sturdier. A run takes more than half
an hour, and if it stops part-way it is lost.
[The loop as a graph](05_the_graph.md) writes the same loop with LangGraph, so
that a stopped run carries on where it stopped, and a person can be asked
before each training.
