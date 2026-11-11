# What our agent did

:::{admonition} Draft
:class: warning
This page is being built. It reports two runs, made on 2026-11-09, with
settings chosen for a first try; what the results mean for the use case will
follow.
:::

[The case for an agent](02_the_agent.md) built up to an agent in three levels.
Here is one at work, on the use case of
[the first page](01_training_an_mlip.md): an LLM choosing each step of MLIP
training, inside a script that runs and checks it. This page is about two of
its runs, one with each of two LLMs, and the models they produced.

## What it was asked to do

The agent was given the tools, limits and goal set out in
[Guardrails](02_the_agent.md#guardrails): reach a force error below 20 meV/Å
on the selection frames with a model that stays stable in MD, starting from 200
frames and training at most five times.

## What it did

The first of the two runs used Qwen3 8B, running on the laptop. It took 35.6
minutes, and
the agent stopped on its own. {numref}`run-timeline-figure` lists every
step: on the left what the LLM chose, on the right what the tool measured.

```{figure} ../figures/agent_run_timeline.svg
:name: run-timeline-figure

The Qwen3 run, from its log. On the left, what the LLM chose; on the right,
what the tool measured. The minutes include 9.7 minutes of the LLM thinking,
spread over its 14 replies.
```

The first model, from 200 frames, scored 57.9 meV/Å on the selection frames,
far from 20. The LLM then went in small steps, scoring each model before the
next. It added 200 frames and trained for 100 epochs (28.2 meV/Å), then trained
the same frames for 150 epochs (23.8). It added 400 more frames, in two steps
of 200, and asked for a wider model trained for 200 epochs. The script refused
that call, because epochs must be from 10 to 150, and the LLM asked again with
150. That fourth model reached 14.1 meV/Å, passed the MD check, and the LLM
stopped with the goal met, one training short of the limit.

The LLM chose its steps by looking at the selection frames, so their error is
no longer an untouched test. When the LLM stopped, the script scored the last
model once on the **final test frames**, rMD17's official 1000 test frames,
which the LLM never saw: 14.2 meV/Å.

## What to read with care

The run worked, but it was one run with an 8-billion-parameter model, and some
of it should not be taken further than it goes:

- **Two changes at once.** Between its third and fourth trainings the LLM
  doubled the frames (400 to 800) and the width (32 to 64 channels). The run
  cannot say which of the two helped. Only one step changed a single setting:
  from the second training to the third, only the epochs went up (100 to 150),
  and the error fell from 28.2 to 23.8 meV/Å.
- **Reasoning and action can differ.** Before the refused call, the LLM's
  reasoning twice settled on "channels=64 and epochs=150", but the call it
  sent asked for 200 epochs. On the way there it also recalled the third
  training as one that added frames, which it did not. The script caught the
  first slip because it is a limit; the second changed nothing, and nothing in
  code would have caught it.
- **The thresholds were ours.** 20 meV/Å and the MD limits were set for this
  first try, not from what a later simulation needs.

On the same 500 selection frames, the first page's model, trained by hand, has
a force error of 40.9 meV/Å and an energy error of 19.8 meV/atom (1.3 once the
constant shift is taken away). The agent's last model has 14.1 meV/Å and
0.4 meV/atom (0.4). It was trained on more frames (720 against 450, after 10%
are set aside to validate), is twice as wide, and was trained almost four times
as long (150 epochs against 40). The run does not say which of the three made
the difference.

## A second LLM

To see how much the run depends on the LLM, the same script, tools, limits and
goal were given to a larger model, Claude Sonnet 5.5, reached over Anthropic's
API. Nothing else changed.

| | Qwen3 8B | Claude Sonnet 5.5 |
| --- | --- | --- |
| Tool calls | 14, one refused | 7 |
| Trainings | 4 | 2 |
| Last model | 800 frames, 64 channels, 150 epochs | 800 frames, 64 channels, 100 epochs |
| Force error, selection frames | 14.1 meV/Å | 17.2 meV/Å |
| Force error, final test frames | 14.2 meV/Å | 17.1 meV/Å |
| MD check | passed | passed |
| Time | 35.6 min, 9.7 of them the LLM | 13.8 min, 8 s of them the LLM |
| Cost | none (runs locally) | about $0.03 (10,929 tokens in, 477 out) |

Claude took a shorter path. It trained once (32 channels, 60 epochs), scored
that model on the selection frames (53.5 meV/Å), added 600 frames in one step,
and trained a wider model for 100 epochs. That second model reached
17.2 meV/Å, passed the MD check, and Claude stopped.

Both last models used the same 800 frames and the same width. The difference
is the epochs, 150 against 100, and Qwen3's model is the more accurate of the
two. Qwen3 got there with two more trainings and well over twice the time.
One run each cannot say which LLM chooses better: run again, either could take
another path.

Once, Claude asked for `evaluate` and `run_md` in the same reply. The script
ran only the first and said why, and Claude called `run_md` next.

## Run it yourself

Both runs can be repeated from a clone of the
[repository](https://github.com/sbkashif/MolSimWorkbench). They were made on
macOS 15, on a laptop with an Apple M4 processor and 16 GB of memory, using
only the CPU. That is the only system tested so far; Linux, and Windows
through WSL, are being tested.

1. Create the environment, which holds MACE, OpenMM, OpenMM-ML, Ollama and
   Anthropic's Python client:

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

3. Choose the LLM and start the run from the repository root:

   ::::{tab-set}

   :::{tab-item} Qwen3 8B, on your machine
   Start the Ollama server in a second terminal, and download the model once
   (about 5 GB):

   ```bash
   ollama serve
   ```

   ```bash
   ollama pull qwen3:8b
   ```

   Then run the agent:

   ```bash
   python -m agent.harness --model qwen3:8b --run-dir agent/runs/qwen3
   ```

   This costs nothing, and nothing leaves your machine.
   :::

   :::{tab-item} Claude Sonnet 5.5, over the API
   This needs your own API key from the
   [Claude Console](https://platform.claude.com/), and the run is billed to
   your account: about $0.03 for the run on this page. Set the key in the
   terminal you run from, and keep it out of files you commit:

   ```bash
   export ANTHROPIC_API_KEY=your-key-here
   ```

   Then run the agent:

   ```bash
   python -m agent.harness --model claude-sonnet-5-5 --run-dir agent/runs/claude
   ```

   The training, evaluation and MD still run on your machine; only the LLM's
   choices come from the API.
   :::

   ::::

Each line of the log is printed as it is written. When the run ends, the run
folder holds `log.jsonl`, the trained models and MACE's own training logs. To
list what the agent did, as in {numref}`run-timeline-figure`:

```bash
python -c "
import json, sys
for e in map(json.loads, open(sys.argv[1])):
    if e['kind'] in ('tool', 'end', 'final_test'):
        print(e['t_min'], e.get('name', e['kind']), e.get('args', ''), e.get('result', e.get('reason')))
" agent/runs/qwen3/log.jsonl
```

Do not expect the same steps or numbers as this page. Qwen3 runs at
temperature 0 with a fixed seed, but its replies can still change with the
machine and the Ollama version. Claude takes no temperature setting, so its
replies to the same history can differ from run to run. Training on a
different processor may also move the force error a little.

## Check yourself

:::{dropdown} Qwen3's reasoning settled on 150 epochs, but its call asked for 200. What caught it, and what would not have?
`check` caught it, because the number of epochs is a limit checked before a
tool runs. A slip that stays inside the limits, or one in the LLM's own text,
passes through; only a check in code on what it writes, or a person reading the
log, would catch it.
:::

:::{dropdown} Between its third and fourth trainings, Qwen3 changed two settings at once. What does that cost?
It saved a training, but the run cannot say which change helped: more frames
or a wider model.
:::

:::{dropdown} Qwen3's model scored 14.1 meV/Å and Claude's 17.2. Does that show Qwen3 chooses better?
No. Each LLM ran once, and the two last models differ mainly in epochs (150
against 100). Run again, either LLM could take another path. It would take
repeated runs to compare them.
:::

## Next

The runs treated the agent as one whole: an LLM that chooses, inside a script
that runs and checks each step. [How our agent was built](04_the_code.md)
takes it apart: first the parts every agent shares and their standard names,
then our code, one file at a time.
