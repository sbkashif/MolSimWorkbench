# Automating the loop with an agent

:::{admonition} Draft
:class: warning
This page is being built. The run described here is the first one, made on
2026-11-07, with settings chosen for a first try.
:::

[Use case: training an MLIP](01_training_an_mlip.md) ended with training as a
loop: fit the parameters, check the errors on frames it was not trained on,
run the model in MD,
decide what to change, and go round again. On that page a person did the
deciding. This page hands the deciding to a language model, and shows the
first run.

## The loop, and the step to automate

{numref}`agent-pipeline-figure` draws the loop. Every box except one runs a
program: training, the checks, the data. The box that does not is the choice
of what to do next. Is the error low enough? Is the MD stable? If not, is
more data the answer, or a wider model, or longer training? That choice needs
someone to read the numbers and judge, and it is the step an agent takes over.

```{figure} ../figures/agent_pipeline.svg
:name: agent-pipeline-figure

The training loop. The teal boxes are programs. The purple box is the choice
that a person made on the last page and a language model makes on this one.
```

## What an agent is

An **agent** is a language model inside a loop. It is built from four parts,
and none of them is new on its own:

- A **large language model (LLM)** takes text in and gives text out. It cannot
  run anything, and it remembers nothing from one call to the next.
- A **tool** is an ordinary function, with a name and a description the LLM can
  read. Training a model is a tool.
- A **tool call** is a short, structured piece of text the LLM writes, naming a
  tool and its inputs. It is only text until something acts on it.
- The **harness** is the ordinary program around the LLM. It sends the text,
  runs the tools, keeps the history and enforces the limits.

{numref}`agent-parts-figure` shows how they talk. The LLM and the tools never
touch each other: every message passes through the harness, so nothing runs
unless the harness runs it.

```{figure} ../figures/agent_parts.svg
:name: agent-parts-figure

The parts of the agent built for this page. The LLM chooses, the harness
enforces and records, and the tools do the work.
```

### How a model with no memory keeps track

If the LLM remembers nothing, how does it know what it did two steps ago? The
harness sends it the whole history on every pass: the goal, every tool call so
far, and every result. The LLM rereads all of it and writes the next tool call,
so it looks as if it remembers ({numref}`agent-history-figure`).

```{figure} ../figures/agent_history.svg
:name: agent-history-figure

What the harness sends on each pass. The list grows by one tool call and one
result each time.
```

## The tools and the limits

The agent gets four tools, one for each program box in
{numref}`agent-pipeline-figure`, and a fifth to end the run. Each returns
measured numbers, never estimates.

| Tool | What it does | What it returns |
| --- | --- | --- |
| `train` | Trains a MACE model on the current training frames | validation force error, time taken |
| `evaluate` | Runs the model on 500 selection frames, never trained on | force error, energy error, the energy shift |
| `run_md` | Runs 1 ps of MD in OpenMM from 500 K | energy range, closest pair, whether the bonds held |
| `add_data` | Uses more frames from the training pool next time | the new number of frames |
| `stop` | Ends the run | the reason, written by the LLM |

The LLM picks the settings for `train`: how many epochs, and how wide the model
is (the number of channels, which was 32 on the last page). The harness checks
every call before it runs it, and refuses any that breaks a limit:

| Limit | Value | Why |
| --- | --- | --- |
| Trainings per run | 5 | each one takes minutes |
| Training frames | 200 to start, 1000 at most | the rMD17 readme warns against more than 1000 |
| Epochs | 10 to 150 | keeps each training to minutes on a laptop |
| Channels | 16, 32 or 64 | three model sizes |
| Tool calls per run | 20 | stops a run that goes in circles |

A refused call is not an error. The harness sends back the reason, and the LLM
has to choose again.

## The goal

The LLM gets one message before it starts, written by us. It sets the target
and the rules:

- force error below 20 meV/Å on the selection frames;
- in 1 ps of MD from 500 K, total energy within 0.01 eV, no two atoms closer
  than 0.75 Å, and the same bonds at the end as at the start;
- start from 200 frames, train at most 5 times, and check every model in MD
  before saying it meets the goal.

After the LLM stops, the harness scores the last model once more, on 500
final test frames the LLM never sees. The LLM decides using the selection
frames, so their error is no longer an untouched test; the final test frames
are.

These thresholds were chosen for this first run, not tuned. The test run on the
last page reached 48.2 meV/Å with 450 frames of the older MD17 data, so
20 meV/Å asks for a clearly better model, but not one out of reach.

## What it runs on

- **Data:** rMD17 ethanol, the corrected version of the MD17 data on the last
  page. Its official test frames are cut in two: the first 500 are the
  selection frames, the next 500 the final test frames.
- **MLIP:** MACE, trained on the laptop's CPU.
- **MD:** OpenMM, not this site's engine. That engine works in reduced units
  with every mass fixed at 1.0, and it cannot call a trained model. Adding that
  would be a separate project and would not test the training loop, so OpenMM
  sits beside it as a second engine. The MLIP fills the same slot there that LJ
  fills here.
- **LLM:** Qwen3 8B, running on the laptop through Ollama. The LLM is a
  setting: any model that can write tool calls can take its place.

## The first run

The first run took 12.6 minutes on a laptop, and the agent stopped on its own.
{numref}`run-timeline-figure` lists every step: on the left what the LLM chose,
on the right what the tool measured. The harness refused nothing.

```{figure} ../figures/agent_run_timeline.svg
:name: run-timeline-figure

The first run, from its log. Purple is the LLM's choice, teal is a measured
result. The minutes include 3 minutes of the LLM thinking, spread over its six
replies.
```

The first model, from 200 frames, had a validation force error of 35.5 meV/Å,
far from 20. The LLM did not spend time on the selection frames or MD for it. It
added 200 frames, then trained a wider model for longer. The second model
reached 19.6 meV/Å on the selection frames, passed the MD check, and the LLM
stopped with the goal met. The first run's harness did not yet score a final
test; run afterwards on the 500 final test frames, the same model gives
19.9 meV/Å. Each step was the one the goal asked for, in order.

### What to read with care

The run worked, but it was one run with an 8-billion-parameter model, and some
of it should not be taken further than it goes:

- **Three changes at once.** Between the two trainings the LLM changed the
  frames (200 to 400), the width (32 to 64 channels) and the epochs (100 to
  150). That saved a training, but the run cannot say which change helped.
- **Only just under the target.** 19.6 meV/Å against 20 is a margin of 2%.
  On the validation frames the same model scored 20.03, just over, and on the
  final test frames 19.9. Another seed could land on the other side.
- **Small misreadings.** The stop reason gives the energy as "±0.0055 eV", but
  0.0055 eV was the whole range, not a spread on each side. On the way to its
  last choice the LLM's reasoning also mixed up the 18.8 s the MD took to run
  with the 1 ps it simulated, before reaching the right answer from the step
  count. Neither changed a decision here, but both are the kind of slip a
  stronger LLM, or a check in code, should catch.
- **The thresholds were ours.** 20 meV/Å and the MD limits were set for this
  first run, not from what a later simulation needs.

The energy error was 6.9 meV/atom, against 37.9 on the last page, and 0.6 once
the constant shift is taken away. The data, the model size and the training
all changed between the two, so this run does not say which of them made the
difference.

## What this section does not show

- It uses a small molecule, not a catalyst.
- It simulates collecting new data by drawing on labels that already exist. It
  runs no new quantum chemistry.
- One search found no public Fe-N-C dataset. Periodic catalyst systems would
  likely need new DFT labels and different tools.
- The page shows a recorded run, because MACE training and OpenMM cannot run
  in a browser, and there is no budget for hosting them. Readers can clone the code and run it
  themselves.

## Why I built this

This section began as a demonstration case for a job application, for a
position that combines agentic AI with computational chemistry.

My background is molecular simulation. I am newer to agentic AI and to MLIPs,
and my papers do not show work in either. This section is a public record of
what I have built so far. It is a start, not expertise.

## Check yourself

:::{dropdown} What does the language model do, and what does it not do?
It chooses the next step and its settings. It does not produce any of the
numbers: those come from its tools, and the harness enforces the limits.
:::

:::{dropdown} The LLM remembers nothing between calls. How does it know what it has already tried?
The harness sends the whole history on every pass, and the LLM rereads it.
:::

:::{dropdown} Why does this section use OpenMM rather than the engine built on this site?
The engine has no way to call a trained model, and it uses reduced units with
masses fixed at 1.0. OpenMM already runs MLIPs, and the section is about the
training loop, not about extending the engine.
:::

## Next

This page treated the tools and the harness as boxes.
[How the agent is built](03_the_code.md) opens them and goes through the code,
one file at a time.
