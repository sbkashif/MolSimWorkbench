# The case for an agent

:::{admonition} Draft
:class: warning
This page is being built. The run described here was made on 2026-11-09,
with settings chosen for a first try.
:::

On [the last page](01_training_an_mlip.md), one pass round the training loop
was done by hand. It stopped at step 5, the decision: is the model good enough,
and if not, what should change? Doing that by hand means reading the numbers,
choosing a change and starting the next training, again and again. This work
can be automated using an **agent**: a program that chooses each next step
itself, and then carries it out. We give it one clear target, broader
guidance on the limits within which it must reach that target, and tools it
can use to carry out and check its decisions. It does the rest: it reads each
result, chooses the next change and runs it, with no person in between.

## Handing over the small decisions, step by step

To see how an agent does this, we build up to it in three levels, each
automating more than the last. We start with the first: asking a chatbot what
to do.

### 1. Asking a chatbot

The last page's pass ended with a model whose force
error on the validation frames, the 50 frames held out to watch training, was
39 meV/Å. The target is now 20. You paste that result into a chatbot such as
ChatGPT, with a question:

> I trained a MACE model for ethanol on 450 frames, for 40 epochs, with 32
> channels (numbers kept per atom). The validation force error is 39 meV/Å,
> and I want it below 20 meV/Å. What can I do to make it better?

It answers in plain words, for example: "Add more training frames, and train a
wider model for longer." You run that training yourself, paste the new result,
and ask again. The chatbot is the brain; you are the hands.

The other two levels hand more and more of this to the same kind of brain, so
it is worth seeing what it does with your question.

The chatbot is an app. The part that writes the answers is a **large language
model (LLM)**: in ChatGPT, one of OpenAI's GPT models; on this page, Qwen3,
which runs on a laptop. It is a neural network like the small one on the last
page. Text goes in, and text comes out ({numref}`agent-llm-figure`). It writes its reply
one small piece, a **token**, at a time. A common English word is usually one
token, so here you can read "token" as "word"; rarer words and units such as
meV/Å are split into smaller pieces. At each step it reads the question and
the reply so far, scores every token that could come next, and adds the one
that scores highest (chatbots often pick at random among the top few; for
the local model the agent's code asks for the top one).

The scores come from fitting, done once, before any question: as page 1's
network was fitted to DFT energies, the LLM was first fitted to a very large amount
of text, to score highest the word that really came next. Answering fits
nothing: your text, the question and the reply so far, goes through the fitted
network once for each new token. The prediction rests only on what came next
in the text it was fitted to; nothing is run to check it. In text like yours,
a high validation error is likely followed by advice like "add more training
data", so that reply scores highest here. That advice is common for high
errors in general, but the LLM has not looked at your data or run your model,
so it cannot know whether more data is what this model needs.

```{figure} ../figures/agent_llm_io.svg
:name: agent-llm-figure

An LLM in one picture: text goes in, text comes back. The labels below the
boxes are its three limits. How the reply is written, and where the limits
come from, is in the Detour below.
```

::::{dropdown} Detour: inside the LLM
:class-container: detour-dropdown

*What answers.* An LLM is a neural network, the same kind of
function as the [small network](01_training_an_mlip.md#neural-network-basics)
on the last page: numbers go in, weighted sums and $\phi$ act on them step
after step, and numbers come out. Only what the numbers stand for is different.
There, three numbers $x_1$, $x_2$, $x_3$ went in and one came out. In an LLM,
your text goes in, and out comes a score for every piece of text that could
come next ({numref}`agent-llm-nn-figure`).

```{figure} ../figures/agent_llm_nn.svg
:name: agent-llm-nn-figure

An LLM drawn like the small network of the last page. The question "My
validation error is still high. What should I change?" goes in as tokens (some
are left out to fit). Each token is looked up by its place in Qwen3's list,
the real number shown above its arrow, and replaced by its row of 4,096
numbers. These pass
through 36 steps with 8.2 billion weights in all. Out comes one score for each
of the 151,936 tokens Qwen3 8B knows, and the top one ("Add") is the next piece
of the answer. The sizes are Qwen3 8B's
([model configuration](https://huggingface.co/Qwen/Qwen3-8B/blob/main/config.json));
the numbers in the rows and the scores are made up to show the idea.
```

*From text to numbers.* The text is first cut into tokens from a fixed list:
whole words, parts of words, or signs such as "?". Each
token is replaced by its place in the list; in Qwen3's list, " validation"
(with the space in front) is number 10,519 and " error" is number 1,465. A
table with one row of 4,096 numbers per place then gives each token its row:
" validation" becomes row 10,519 ({numref}`agent-llm-lookup-figure`). In
Qwen3 8B the table has 151,936 rows. Its
numbers are weights too, fitted in training with all the others, and tokens
used in similar ways tend to end up with similar rows.

```{figure} ../figures/agent_llm_lookup.svg
:name: agent-llm-lookup-figure

From one token to numbers. The token " validation" (␣ marks the space in front)
is found in Qwen3's list at place 10,519; row 10,519 of the table, 4,096
numbers long, is what goes into the network. The list entries and their places
are Qwen3's own, from its published tokenizer; the numbers in the rows are made
up.
```

*How it was fitted.* The same way as on the last page: compare the output with
the right answer, change the weights a little, and repeat. For the MLIP, the
right answer was the DFT energy and forces of each frame. For an LLM, it is the
token that really comes next in a text. Say the training text holds a forum
post: "My validation error is still high. What should I change? Add more
training data." The LLM reads everything up to "change?" and scores every
token. The right answer is "Add", so the weights are changed a little to raise
the score of "Add" there; then the same is done for "more" after "Add", and so
on.

For Qwen3, the LLM used here, its makers have published the outline of this
([Qwen team 2025](https://qwenlm.github.io/blog/qwen3/);
[Yang et al. 2025](https://arxiv.org/abs/2505.09388)). It was first fitted on
about 36 trillion tokens of text in 119 languages, from the web and from
PDF-like documents, plus maths and code written by earlier Qwen models. It was
then trained on worked, step-by-step solutions to problems in maths, coding and
science, and finally rewarded on more than 20 kinds of task, among them
following instructions, keeping to a set format, and acting as an agent. The
size used here, Qwen3 8B, has 8.2 billion weights, against 16 in the small
network and 67,472 in the last page's MLIP.

*What it learned.* From all that text, an LLM has learned how answers tend to
follow questions. The exact texts Qwen3 saw are not public, but they likely
include papers, tutorials and forum posts on machine learning, where a high
validation error is often followed by advice such as "add more training data"
or "use a larger model". So when you ask what to change, that kind of advice is
the reply that fits best, and the one it gives.

*How it writes the reply.* One token at a time
({numref}`agent-llm-steps-figure`). At each step it reads the question and the
reply so far, scores every token that could come next, adds the top one, and
starts again. The reply is just the text that grows this way.

```{figure} ../figures/agent_llm_steps.svg
:name: agent-llm-steps-figure

The reply built one token at a time. Each step reads the question (grey) and
the reply so far, and adds the token that most often comes next in text like
it (purple); the "why" lines say what makes each one fit.
```

*Three limits.* All three follow from what is above:

- **It only writes text.** It cannot run a training or an MD check; someone or
  something else has to.
- **It can be wrong.** Its reply is what fits text like yours, not a result it
  has checked. It can also misread its own history: in the run on the next
  page, Qwen3 counted an evaluation as one of its trainings, and later its
  reasoning settled on 150 epochs while the step it wrote asked for 200. And a
  wrong answer sounds as sure as a right one.
- **It sees only what it is sent.** It keeps nothing from one question to the
  next. A chatbot seems to remember because the app sends the whole
  conversation again with every question.

::::

The LLM stays the same in levels 2 and 3, and so do its limits. What changes is
who does your part.

### 2. A script

In level 1, your part had three steps: paste the result into the chatbot, read
its answer, and run the training it suggests. A Python script can do all three
for you. Sending the result is easy, and so is starting a training. The hard
part is reading the answer: it comes back in plain words, so the script can
only search it for phrases you expected, with a rule for each.

Here is such a script. It starts from the last page's model and goes round the
loop five times. `ask_chatbot` sends a question to the chatbot and returns its
reply; `add_data` and `train` stand for the programs you ran by hand. For now,
take them as ready to run, just as if you typed the commands yourself; how they
are built is in [How our agent was built](04_the_code.md).

```python
frames, channels = 450, 32         # the last page's model
error = 39                         # its validation force error, in meV/Å
for step in range(5):
    answer = ask_chatbot(          # the same question as in level 1
        f"I trained a MACE model for ethanol on {frames} frames, "
        f"for 40 epochs, with {channels} channels (numbers kept "
        f"per atom). The validation force error is {error} meV/Å, "
        "and I want it below 20 meV/Å. "
        "What can I do to make it better?")
    if "more frames" in answer:    # your rules, written in advance
        add_data(200)              # 200 more rMD17 frames next time
        frames += 200
    if "wider" in answer:
        channels = 64              # a wider model: 64 numbers per atom
    error = train(channels=channels, epochs=40)  # always trains next
```

The script saves you the typing, but the decisions are still yours, written in
advance as rules. That has two weak points. First, any answer your rules did
not expect is lost. "Train for longer" or "add 300 frames" matches no rule, so
nothing changes; "don't add more frames" matches the first rule, so the wrong
thing happens. Second, the order of the steps is fixed: whatever the answer,
the script trains next, and it never runs an MD check unless you wrote one in.
So you still watch what comes out, and add a rule by hand whenever a new kind
of answer appears.

### 3. An agent

Level 2 already takes the tedious part off your hands, and it
can be enough. Anthropic's guide to building agents advises starting from the
simplest solution that works, which "might mean not building agentic systems
at all"
([Schluntz and Zhang 2024](https://www.anthropic.com/research/building-effective-agents)).
If the possible answers are few and well known, you can read
them in batches, adjust your rules, and keep every decision your own: the LLM
suggests and the script does the work, but you decide. Every action then comes
from a rule you wrote.

Many real jobs, though, are made of many small decisions. Building an MLIP is
one: it goes round data collection, model choice, training and validation, and
testing and refinement, again and again
([Miksch et al. 2021](https://arxiv.org/abs/2101.10468)). Each round asks: add
frames or widen the model? How many frames? Is it time for an MD check, or to
stop? The cases multiply (error high but falling, error stuck, MD failing,
frames near the limit), and a rule for each, with you stepping in whenever none
fits, slows the work down.

How do we take these decisions off your hands? Not with more rules: writing
them is the very work we want to drop. Instead, the LLM makes them. In
levels 1 and 2 it already said what to do next, but in words, such as "train
for longer", that you or a rule had to turn into an action. Now it answers with
the action itself, one per pass, in a form the script can run:

```text
levels 1 and 2, the reply:   "Add more training frames, and train a wider
                              model for longer."

level 3, the reply:          train(epochs = 150, channels = 64)
```

The first is advice for a person to read. The second is one step, with its
numbers filled in, that the script runs as it is; it is the last training
the LLM chose in the run on the next page (see [the results](03_first_run.md)). To get answers like this, the level 2 loop changes as
follows:

```{raw} html
<div class="code-diff-wrap">
<table class="code-diff">
<thead><tr><th>Level 2: your rules decide</th><th>Level 3: the LLM decides</th></tr></thead>
<tbody>
<tr class="cd-ask"><td><code>channels = 32
# the level 1 question:
question = "..."</code></td><td><code># the menu of programs:
tools = [train, evaluate, run_md,
         add_data, stop]
# the goal: target, limits, guidelines
history = [goal]</code></td></tr>
<tr class="cd-same"><td><code>for step in range(5):</code></td><td><code>for step in range(20):</code></td></tr>
<tr class="cd-ask"><td><code>    answer = ask_chatbot(question)
    if "more frames" in answer:
        add_data(200)
    if "wider" in answer:
        channels = 64</code></td><td><code>    choice = ask_chatbot(history, tools)</code></td></tr>
<tr class="cd-run"><td><code>    error = train(channels, epochs=40)</code></td><td><code>    result = run(choice)</code></td></tr>
<tr class="cd-new"><td></td><td><code>    history += [choice, result]</code></td></tr>
</tbody>
</table>
</div>
<p class="code-diff-key"><span class="cd-chip cd-ask">blue</span> the question and your rules become one call that returns a step
<span class="cd-chip cd-run">orange</span> the fixed training becomes whatever step was chosen
<span class="cd-chip cd-new">green</span> new: the record of every step so far</p>
```

Each row on the left is replaced by the row beside it. In blue, the question
and your `if` rules become one call, `ask_chatbot(history, tools)`: the LLM is
sent the goal and everything so far (`history`) and a menu of programs
(`tools`), and it answers with one step. In orange, the fixed training becomes
`run(choice)`, which runs whatever step that was. In green is the one new
piece: `history` keeps every choice and result for the next pass. On the menu,
`evaluate` and `run_md` are the error and MD checks you ran by hand, and `stop`
ends the run.

The LLM's part is `choice`: it picks one program from the menu and fills in
that program's inputs. The question is still there, but it is asked only once,
at the end of the goal: "Use the tools one at a time, read each result, and
decide the next step." Here is everything the LLM read and wrote on the third
pass of the run. Text is cut where marked with "…", and the menu, which
is sent as JSON with an instruction that Qwen3's chat template adds, is written
out plainly:

```text
WHAT THE LLM READS

the goal      You are training a machine learning interatomic potential
              (a MACE model) for the ethanol molecule … The goal: a model
              with a force error below 20.0 meV/Å … that also runs stably
              in MD … You start with 200 training frames and can add more,
              up to 1000. You may train at most 5 times. Training takes
              minutes, so do not waste trainings. Use the tools one at a
              time, read each result, and decide the next step. …

the menu      train(epochs, channels)  Train a new MACE model on the
                                       current training frames. …
              add_data(frames)         Use more frames from the training
                                       pool in the next training. …
              evaluate(), run_md(), stop(reason)  …
              Answer with a program's name and its inputs between
              <tool_call> and </tool_call>.

so far        it chose:  train(channels = 32, epochs = 50)
              result:    {"model": "model_1", "training_frames": 200,
                          "epochs": 50, "channels": 32, "minutes": 0.9,
                          "validation_force_mae_meV_per_A": 53.55}
              it chose:  evaluate()
              result:    {"model": "model_1", "frames": 500,
                          "force_mae_meV_per_A": 57.9, …}

WHAT IT WRITES

reasoning     … the assistant called the train function with channels
              32 and epochs 50. The response showed that the validation
              force MAE was 53.55 meV/Å, which is above the target of
              20.0. Then, they evaluated the model, and the result was
              57.9 meV/Å for force MAE, still way above the target. …
              Since the initial 200 frames might not be sufficient,
              adding more could help the model generalize better … So
              the next tool call should be add_data with frames=200 to
              increase the training data.

the step      <tool_call>
              {"name": "add_data", "arguments": {"frames": 200}}
              </tool_call>
```

The LLM wrote all of this token by token, like any reply: first the reasoning,
then the step. The reasoning is text too, so it shows what fit best, not a
worked-out calculation. Ollama, the program that runs Qwen3 on the laptop,
takes the name and inputs out from between the markers and hands them to the
script as data, so no rule has to search a sentence. Answering this way is
called **tool calling**. Qwen3's last training stage rewarded "format
following" and "agent capabilities", and its makers report that it does tool
calling well.

The script then runs the step, adds it and its result to `history`, and asks
again, until the LLM picks `stop`. The LLM remembers nothing between passes, so
the whole history is sent every time (for the local model, without its
reasoning: only the steps and results). The goal stays the same while the history grows, and the answers
change with it: after a high error the LLM may add frames; after a low one,
run an MD check. The script also checks each step against its limits before
running it, such as at most five trainings; that and the other details are in
[Guardrails](#guardrails) below.

Put together, the agent runs the training loop of the last page by itself
({numref}`agent-pipeline-figure`). Every box is a program the script runs,
except one: the decision, which is now the LLM's.

```{figure} ../figures/agent_pipeline.svg
:name: agent-pipeline-figure

The training loop. Every box but one is done by code. "LLM decides" is the
choice that the last page left to a person, and that the agent's LLM makes on
this one.
```

No code is rewritten during the run. In the run on the next page, the 14
answers that filled `choice` began with `train(channels=32, epochs=50)`,
`evaluate()` and `add_data(frames=200)`, and ended with `evaluate()`,
`run_md()` and `stop(...)`; in between, the LLM trained three more times and
added frames twice more. None of that sequence is in the script; the LLM
produced it, one answer at a time, from the results. This is what makes it an
agent.

The price is that you no longer know the decisions in advance. You see each one
only after it is made, and some can be wrong; [the results](03_first_run.md) show where it
slipped. When level 2 does the job, it is the safer choice.

### The three levels side by side

| Level | Who decides | Who runs | What the LLM answers with |
| --- | --- | --- | --- |
| 1. Chatbot | the chatbot | you | advice, for you to act on |
| 2. Script | you, through your rules | the script | advice, for your rules to match |
| 3. Agent | the LLM | the script | a step the script can run |

The guide quoted under level 3 has names for the last two. Level 2 is a
*workflow*: LLMs and tools run "through predefined code paths", so the order
of the steps is written in code. Level 3 is an *agent*: the LLM directs which
tools are used, and when
([Schluntz and Zhang 2024](https://www.anthropic.com/research/building-effective-agents)).

An agent takes the deciding from level 1 and the running from level 2. Before
handing it the deciding, we decide what it may do.

## Guardrails

In level 2 every decision was yours, written in advance as a rule. In level 3
you give that up: the LLM chooses each step, and you see it only after it is
made. That trade is only worth it if the choices left to the LLM are safe to
leave to it. The agent we build has three guardrails, all set before the run:
what it can do (its tools), how far it can go (the limits), and what it must
reach (the goal). A fourth comes after the run: a score on frames the LLM never
sees.

### What it can do: the tools

The agent gets four tools, one for each program box in
{numref}`agent-pipeline-figure`, and a fifth, `stop`, to end the run. These are
the only things it can do, and the four that do work return numbers measured
in the run, never estimates.

| Tool | What it does | What it returns |
| --- | --- | --- |
| `train` | Trains a MACE model on the current training frames | validation force error, time taken |
| `evaluate` | Runs the model on 500 selection frames, never trained on | force error, energy error, the energy shift |
| `run_md` | Runs 1 ps of MD in OpenMM from 500 K | energy range, closest pair, whether the bonds held |
| `add_data` | Uses more frames from the training pool next time | the new number of frames |
| `stop` | Ends the run | the reason, written by the LLM |

The LLM picks the settings for `train`: how many epochs, and how wide the model
is (the number of channels, which was 32 on the last page).

### How far it can go: the limits

The script checks every call before it runs it, and refuses any that breaks a
limit:

| Limit | Value | Why |
| --- | --- | --- |
| Trainings per run | 5 | each one takes minutes |
| Training frames | 200 to start, 1000 at most | the rMD17 readme warns against more than 1000 |
| Epochs | 10 to 150 | keeps each training to minutes on a laptop |
| Channels | 16, 32 or 64 | three model sizes |
| Tool calls per run | 20 | stops a run that goes in circles |

A refused call is not an error. The script sends back the reason, and the LLM
has to choose again.

### What it must reach: the goal

The LLM gets one message before it starts, written by us. It sets a target,
limits, and the rules for reaching them. The frames it is judged on come from
rMD17 ethanol, the same data as on the last page. The **selection frames**,
the ones `evaluate` uses, are 500 frames in neither its official training set
nor its official test set.

- **Force error below 20 meV/Å on the selection frames.** The average DFT force
  component in these frames is about 870 meV/Å, so the last page's 41.0 meV/Å
  is an error of about 5%. 20 meV/Å is about half of that: a clearly better
  model, but one a laptop can train in minutes.
- **Stable MD.** 1 ps from 500 K, with no thermostat, and one limit for each of
  the three failures the last page listed: no two atoms closer than 0.75 Å,
  well under the shortest bond in ethanol (O–H, 0.97 Å); the same bonds at the
  end as at the start; and a total energy that varies by no more than 0.01 eV,
  about 2% of the kinetic energy at 500 K.
- **Rules:** start from 200 frames, train at most 5 times, and check every
  model in MD before saying it meets the goal.

The last page's model passes this MD check (energy range 0.0035 eV, closest
pair 0.96 Å, bonds unchanged), but not the force target: 40.9 meV/Å on the
selection frames. So the agent's task is to bring the force error down without
breaking MD.

## Check yourself

:::{dropdown} Level 2 already runs the trainings for you. What does it still leave to you?
The decisions. They are written in advance as rules, so any answer the rules
did not expect is lost, and the order of the steps is fixed in code.
:::

:::{dropdown} What changes about the LLM's answer between level 2 and level 3?
Its form. In level 2 it answers in words, which a rule has to match. In level
3 it answers with one step from a menu, with its inputs filled in, which the
script can run as it is.
:::

:::{dropdown} Is a script that asks an LLM what to do, then always trains next, an agent?
No. Its order of steps is written in code, so it is a workflow. In an agent,
the LLM chooses which tool comes next.
:::

:::{dropdown} When is level 2 the better choice?
When the possible answers are few and well known. Then every action comes from
a rule you wrote, and nothing is decided that you did not see in advance.
:::

## Next

This page built up to an agent one level at a time, and set the guardrails it
runs inside. [What our agent did](03_first_run.md) shows it at work on the
use case of the first page: the steps it chose with two different LLMs, what
they measured, and what to read with care.
