# How the agent is built

:::{admonition} Draft
:class: warning
This page is being built. The code shown is read straight from the `agent/`
folder of the repository. It is the code that made the first run on
2026-11-07, except for two later changes, both described below: a second way
to reach an LLM, and a final test that the LLM never sees.
:::

[Automating the loop with an agent](02_the_agent.md) named the parts of the
agent (the LLM, the tools, the tool calls and the harness) and showed what the
first run did. It treated each part as a box. This page opens the boxes and
goes through the code, one file at a time, in the order the data flows: the
settings, the training frames, the four tools, and the harness that joins them
to the LLM.

The whole agent is four short Python files, about 500 lines in all:

| File | What it holds |
| --- | --- |
| `agent/config.py` | the goal's thresholds and every limit the harness enforces |
| `agent/data.py` | turns rMD17 ethanol into the files MACE reads: training, selection and final test frames |
| `agent/tools.py` | `train`, `evaluate`, `run_md`, `add_data`, `final_test`, and the list of tools the LLM sees |
| `agent/harness.py` | the loop: send the history, check the call, run the tool, log it, and score the final test at the end |

None of it is machine learning code. MACE, OpenMM and the LLM are all
existing programs. The agent is the glue that lets an LLM drive them, and the
rules that keep it inside the limits.

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

One of the limits comes from the data rather than from the budget. The pool of
training frames stops at 1000 because the rMD17 readme says not to train on
more: the frames come from one simulation, so neighbouring frames are nearly
the same, and a larger set mostly repeats itself.

rMD17's official test set has 1000 frames, and the model never trains on any of them. They are cut in
two. The first 500 are the **selection frames**: `evaluate` uses them, and the
LLM decides on their errors. Because the LLM chooses by them, they are no
longer an untouched test. The next 500 are the **final test frames**: the
harness scores the last model on them once, after the LLM stops, and the LLM
never sees them.

## The training frames: `data.py`

rMD17 stores each molecule as one NumPy file of positions, energies and forces,
plus files listing which frames belong to the official training and test sets.
MACE reads a different format, extended XYZ, so `data.py` converts between
the two:

```{literalinclude} ../../agent/data.py
:language: python
:pyobject: write_frames
```

A second function writes the final test frames. Only the harness calls it:

```{literalinclude} ../../agent/data.py
:language: python
:pyobject: write_final_test
```

The one decision in this file is how adding data works. The 1000 training
frames are shuffled once, in a fixed order (the random seed is 0), and a run
with `n` frames always takes the first `n` of that order. So `add_data` can only
ever add frames: the 400 frames of the second training in the first run were
the 200 frames of the first training plus 200 new ones. And the same `n` always
means the same frames, so two runs that end with 400 frames trained on the
same data.

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
process, and waits for it to finish:

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
| `--seed` | 1 | the same settings give the same model |

When training ends, the tool reads the last validation force error that MACE
printed in its log and returns it, with the time taken. The validation frames
come out of the training frames, so this number is only a first look. The
selection frames are kept for `evaluate`.

:::{admonition} A simplification to know about
:class: note
`{channels}x0e` builds a MACE model with only invariant features. Full MACE
models also carry features that turn with the molecule (for example `64x0e +
64x1o`), which are usually more accurate and slower to train. The first run
used the cheap kind to keep each training to minutes on a CPU.
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
dynamics, unchanged. The last page met this shift on the test run. In the
first agent run the energy error was 6.9 meV/atom, and 0.6 once the shift was
taken away, so nearly all of it was the constant shift.

### `run_md`

`run_md` puts the model into OpenMM as its energy function and runs 1 ps of
MD from the first selection frame:

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
not controlled: the first run started at 500 K and ended at 366 K, as kinetic
energy flowed into the potential energy of the bonds.

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
reads before anything else. The previous page lists what it asks for. Two
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
choose again. In the first run nothing was refused.

`check` enforces the limits, but not the order of work. If the LLM called
`stop` straight after training, without `evaluate` or `run_md`, the harness
would end the run. The first run did the checks because the goal asked for
them, not because the code required them. Moving that rule from the goal
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

When the loop ends, for whatever reason, the harness runs one more check that
the LLM did not ask for: `final_test`, the same errors as `evaluate`, on the
final test frames. It writes the result to the log as a `final_test` line.
The LLM has already made every decision, so nothing it chose was tuned to
these frames:

```{literalinclude} ../../agent/tools.py
:language: python
:pyobject: final_test
```

The first run was made before this step existed. Run afterwards by hand, it
gives 19.9 meV/Å for the first run's final model, against 19.6 on the
selection frames.

The harness takes only the first tool call in a reply, since the goal asks for
one at a time and each result should shape the next choice. It asks the LLM
for its reasoning as well as its answer (`think=True` for Qwen3), at
temperature 0 with a fixed seed, so that the same history gives the same reply
as often as the model allows.

The LLM is a setting, chosen by `--model` when the run starts. A name such as
`qwen3:8b` goes to Ollama on the same machine. A name starting with `claude-`
goes to Anthropic's API instead, which needs the reader's own API key. That
second path has been written and tested offline, but not yet used for a run.

## What the log records

Every pass writes one line of JSON to `log.jsonl` in the run's folder. Every
figure and number about the run on the previous page comes from this file.
These two lines are from the first run, word for word. First the LLM's choice
(its reasoning, which runs to several paragraphs, is cut here):

```json
{"t_min": 0.89, "kind": "llm", "seconds": 53.2, "thinking": "Okay, let's see. ...", "text": "", "tool_calls": [{"name": "train", "args": {"channels": 32, "epochs": 100}}]}
```

then what the tool measured:

```json
{"t_min": 2.59, "kind": "tool", "name": "train", "args": {"channels": 32, "epochs": 100}, "result": {"model": "model_1", "training_frames": 200, "epochs": 100, "channels": 32, "minutes": 1.6, "validation_force_mae_meV_per_A": 35.53}}
```

`t_min` is the minutes since the run started and `seconds` is how long the LLM
took to reply. The `thinking` field keeps the LLM's reasoning, which is how the
previous page could point out where it misread a number. The log also keeps
the goal, the limits and a summary line at the end, so a run can be read back
without the code.

## Run it yourself

The first run was made on macOS 15, on a laptop with an Apple M4 processor and
16 GB of memory, using only the CPU; it took 12.6 minutes. That is the only
system it has been tested on. Running it on Linux and on Windows through WSL
is being tested. From a clone of the repository:

1. Create the environment, which holds MACE, OpenMM, OpenMM-ML and Ollama:

   ```bash
   mamba env create -f agent/environment.yml
   conda activate mlip-agent
   ```

2. Download rMD17 from [Figshare](https://doi.org/10.6084/m9.figshare.12672038)
   and unpack it into `agent/data/rmd17/`, so that this folder holds
   `npz_data/` and `splits/`. To keep the data somewhere else, point
   `RMD17_DIR` at it instead:

   ```bash
   export RMD17_DIR=~/data/rmd17
   ```

3. Start the Ollama server in a second terminal, and download the model once:

   ```bash
   ollama serve
   ```

   ```bash
   ollama pull qwen3:8b
   ```

4. Run the agent from the repository root:

   ```bash
   python -m agent.harness --model qwen3:8b --run-dir agent/runs/mine
   ```

Each line of the log is printed as it is written. When the run ends,
`agent/runs/mine/` holds the log, the trained models and MACE's own training
logs. The run folders are not part of the repository.

## Check yourself

:::{dropdown} The LLM asks for a training with 300 epochs. What happens?
`check` refuses it, because epochs must be from 10 to 150. The tool does not
run. The LLM gets the reason back as the tool's result and has to choose
again.
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

The first run used one LLM, Qwen3 8B, on a laptop. Whether a stronger LLM
makes better choices with the same tools and limits is the next question,
and the next page will answer it with a second recorded run. It is not
written yet.
