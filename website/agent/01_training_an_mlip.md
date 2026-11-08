# Use case: training an MLIP

:::{admonition} Draft
:class: warning
This section is being built. Every number on this page comes from one small
test run, made by hand before the agent existed.
:::

This section builds an agent for a molecular simulation task. It stands on its
own: this page sets out the task, and the next two build the agent that does it.

Previously, we presented a step-by-step walkthrough of a
[simple molecular dynamics loop](../molsim/architecture.md). That loop used the
Lennard-Jones (LJ) equation to calculate the energy. There are many other
energy functions that can take its place ({ref}`the dashed boxes in one time
step <md-loop-figure>`) to represent the physics better. One such replacement
is the **machine learning interatomic potential (MLIP)**. An MLIP would be needed
where LJ, or classical force fields in general, cannot capture the physics of
interest. An MLIP has to be trained before it can be used, 
and automating that training with an agent is our use case here.

The page goes in three steps: what an MLIP is, how it computes an energy, and
how it is trained.

## What an MLIP is

LJ gives the energy of each pair of atoms from a formula with two numbers, ε
and σ. That suits argon, but LJ has no term for a chemical bond.
Classical force fields add fixed bond terms, but those bonds cannot break or
form. Density functional theory (DFT) can describe one, because it computes the
energy from the electrons, but it is far too slow to call at every step of a
long run. An MLIP sits between the two: it learns from DFT results, then
predicts energies and forces without doing the electronic calculation again.

LJ and an MLIP both start from the positions of the atoms and end with an
energy $U$. {numref}`lj-mlip-figure` shows what happens in between.


```{figure} ../figures/agent_lj_vs_mlip.svg
:name: lj-mlip-figure

The energy calculation in LJ and in an MLIP. In LJ the formula is written down
by hand. In an MLIP it is a neural network with 67,472 adjustable numbers. The
next section opens that box.
```

LJ works one pair at a time. It puts the distance between two atoms into a
fixed formula, and the only parameters are $\varepsilon$ and $\sigma$.

An MLIP works one atom at a time. It assumes that the energy of atom $i$
depends only on the atoms close to it, its **neighbours**: those within a
cutoff distance. Call the positions of those neighbours relative to atom $i$,
and their atomic numbers, $\mathcal{N}_i$. A function with many adjustable numbers, written together as
$\theta$, reads $\mathcal{N}_i$ and gives the atom's share of the energy,
$E_i$. The shares add up to $U$.

In both cases the forces come from the same line, the one the
[simple molecular dynamics loop](../molsim/architecture.md) started from:

$$
\mathbf{F}_i = -\frac{\partial U}{\partial \mathbf{r}_i}
$$

So an MLIP answers the same two questions as LJ: the energy, and the force on
every atom.

## How an MLIP computes the energy

The last section showed an MLIP from the outside: positions and atomic numbers
go into a function with 67,472 parameters, and an energy comes out. This section
opens that function.

Unlike LJ, the function is not written down from a physical idea of how atoms
attract and repel. It is a **neural network**: a general-purpose function whose
shape is set entirely by the values of its parameters. For the same atoms, one
set of values gives one energy, and another set gives a different one. Here we
take the values as given, and first see what a neural network is.

### Neural network basics

A neural network is a function: a list of numbers goes in, and another list
comes out. Its parameters decide what comes out. Let us look at the
function first, which does one calculation, concurrently for all the numbers
coming in. Call one such calculation a **step**. A step
takes a list of $n$ numbers, $x_1, \dots, x_n$, and makes a new list of $m$
numbers, $y_1, \dots, y_m$. The two lengths need not match. Each new number
$y_k$ is a weighted sum of all $n$ numbers in the old list, passed through a
simple function $\phi$:

$$
y_k = \phi\Big(\sum_{j=1}^{n} W_{kj}\, x_j\Big)
$$

The numbers $W_{kj}$ are the **weights**. Each new number $y_k$ has its own row
of $n$ weights, $W_{k1}, \dots, W_{kn}$, so the sum is a dot product of that
row with the list, and the step has $m \times n$ weights in all. The new list
becomes the input of the next step, so the steps form a chain.
{numref}`nn-basics-figure` draws a small chain of two steps and counts
their weights.

```{figure} ../figures/agent_nn_basics.svg
:name: nn-basics-figure
:width: 520px

A small neural network. Each circle is one number in a list, and each line is
one weight: 12 + 4 = 16 in all.
```

The function $\phi$ matters. Think of fitting a line through points on a 2D plot.
Here the list in is one number, $x$, and the list out is one number, $y$.

Without $\phi$, follow the equation above with one number in. Each sum then
has a single term, so the first step makes $y_k = W_k x$: every new number is
$x$ times its own weight. The next step adds these up, each with a weight $V_k$
of its own:

$$
y = V_1 W_1 x + V_2 W_2 x + \dots = (V_1 W_1 + V_2 W_2 + \dots)\, x
$$

The bracket is just one number. However many numbers each list holds, and
however many steps the chain has, the result is one number times $x$: a
straight line, and the weights only set its tilt. (Real networks also add a
constant to each sum; that moves the line up or down, and is left out of the
equation above.) Fitting picks the tilt and shift that bring the line as close
as it can to all the points at once. On points that curve, the best it can do
is cut through the middle, above some points and below others (panel a of
{numref}`line-vs-bends-figure`).

$\phi$ changes that. The one an MLIP uses is close to flat for negative numbers
and rises almost along a straight line for positive ones, with a rounded corner
in between. Put the weighted sum through it, and the line gets one bend, at a
place the weights choose (panel b). A step with more numbers in its new list has
more $\phi$, and their bends add up, so the line can twist down and up again to
follow the points. Panel c uses seven, one step from $x$ to seven numbers and
another back to $y$, and each ▼ marks where one $\phi$ bends.

A simpler $\phi$, one that sets negative numbers to zero, makes sharp corners
instead of rounded ones. An MLIP uses the smooth one, because the forces are
slopes of the energy and a corner would make them jump.

```{figure} ../figures/agent_line_vs_bends.svg
:name: line-vs-bends-figure

The same made-up points, fitted three ways. (a) Without $\phi$: the best
straight line (solid); other weights and constants only tilt or shift it
(dashed). (b) With one $\phi$: one bend (▼). (c) With seven: seven bends, enough
to follow the points.
```

The weights set the shape of the fit: a different set of weights gives a
different curve. In an MLIP, the weights make up almost all of the parameters
$\theta$ from the last section. Unlike $\varepsilon$ and $\sigma$, no single weight means
anything by itself.

### Ethanol, the molecule to fit

```{figure} ../figures/agent_ethanol_frame.svg
:name: ethanol-frame-figure

One real ethanol frame from the data used on this page: the 27 numbers that
describe it, and its energy from DFT.
```

The 2D plot had one number in and one number out. The molecule this page
trains a model for has many more numbers in. It is ethanol, with 9 atoms: two
carbons, one oxygen and six hydrogens. Each atom has 3 coordinates, $x$, $y$
and $z$, so one arrangement of ethanol is 9 × 3 = 27 numbers
({numref}`ethanol-frame-figure`). Call one such arrangement a **frame**. Its
energy, from DFT, is the one number out. So the energy of ethanol is a surface over 27 numbers, and each frame with its DFT
energy is one point on it. The DFT forces give the slope there: each force
points downhill.

Not all 27 numbers matter. Moving the whole molecule along $x$, $y$ or $z$
changes the numbers but not the energy. The same is true of turning it about
any of three axes. Setting those 3 + 3 aside leaves 21 numbers that the energy
really depends on. LJ would build this surface from one fixed curve shape, the
pair well, scaled for each pair type and applied to every pair. An MLIP fits it to the points, the way
the chain in {numref}`line-vs-bends-figure` fits a curve, so it is not limited
to sums of one curve.

### From a neural network to an MLIP energy function

We now have both pieces. A neural network can bend to fit points, and ethanol
gives the points to fit: frames and their DFT energies. The obvious way to
combine them is to feed a frame's 27 numbers into a chain and fit it so that
the number out is the energy. An MLIP does not do that, because three things go
wrong:

- **Where the molecule sits.** Moving the whole molecule changes all 27
  numbers, but not the energy.
- **Size and order.** A chain built for 27 numbers works only for ethanol, and
  only with its atoms listed in one order. Swap two hydrogens in the list and
  the input changes, though the molecule has not.
- **Turning.** Turning the molecule changes the 27 numbers, but not the energy.
  A chain would have to learn that for every possible turn.

Part of the answer is the one in {numref}`lj-mlip-figure`: an MLIP works one
atom at a time. Each atom's neighbours, $\mathcal{N}_i$, give that atom's share
$E_i$, and the shares add up to $U$, so any number of atoms works. But
$\mathcal{N}_i$ is still not a fixed list of numbers, and the other two
problems remain. So an MLIP wraps chains in steps that fix all three. The
MLIP used here is called **MACE**. {numref}`mace-steps-figure` shows its steps for one atom, with
the real number of weights in each step of the model trained for this page.

```{figure} ../figures/agent_mace_steps.svg
:name: mace-steps-figure

The steps of the trained MACE model for atom $i$. Steps 2 and 3 together make
one round. The steps marked φ(Wx) contain a chain like the one in
{numref}`nn-basics-figure`.
```

**1. Describe.** Each neighbour is described relative to atom $i$, so where the
molecule sits no longer matters. Its distance becomes 8 numbers, each a smooth
wave, damped so it fades to zero at the cutoff of 4 Å, so a neighbour drifting out of
range changes the energy smoothly. Its direction becomes 16 numbers, in four
groups of 1, 3, 5 and 7, that turn with the molecule in a fixed, known way. Both
come from fixed formulas, so neither needs weights. The weights in this step
belong to the atomic number: it becomes a list of 32 numbers, one list each for
H, C and O, so 3 × 32 = 96 weights, learned so the model can tell carbon from
hydrogen.

**2. Gather.** Here the chain from the basics appears. A chain of four steps,
8 → 64 → 64 → 64 → 128 numbers, takes a neighbour's 8 distance numbers and
returns 128 numbers. It has 8×64 + 64×64 + 64×64 + 64×128 = 16,896 weights.
The 128 numbers it returns are 32 × 4: one for each of the neighbour's 32
numbers and each of the four groups of direction numbers. They set how much
each of those counts at this distance. The weighted results
are then added up over all neighbours. A sum does not care how many terms it
has or in what order they come, which solves the second problem.

The rest of this step's weights are plain weighted sums that mix the 32
numbers, as in the basics but without $\phi$. One 32 × 32 mix comes before
gathering (1,024 weights), and one after it for each of the four groups
(4 × 1,024 = 4,096). A last mix, with a separate 32 × 32 for each group and
each atomic number, lets the atom's own atomic number count
(4 × 3 × 1,024 = 12,288). With the chain, that makes 34,304.

**3. Combine.** A sum over neighbours only adds up what each neighbour gives on
its own. The energy also depends on how neighbours sit relative to each other,
for example the H–C–H angle. So this step multiplies the gathered numbers, up
to three at a time. A product of two sums holds terms with two neighbours at
once, and through their direction numbers, the angle between them. These
products are a second source of bending, besides $\phi$. Because the direction
numbers turn in a known way, the products can be arranged so that the 32
numbers kept for the atom do not change at all when the molecule turns. That
solves the third problem. There are 28 such products, each with its own weight
for each of the 32 numbers and each atomic number (28 × 32 × 3 = 2,688), and a
final 32 × 32 mix (1,024), so 3,712 weights.

**Round 2.** Steps 2 and 3 run again on the 32 numbers from round 1. Each
neighbour has already gathered from its own neighbours, so information now
reaches farther than one cutoff. Round 2 has a chain and a combine step of its
own, with the same counts as round 1. Only its last atomic-number mix differs. It acts on the 32 numbers the atom
kept from round 1, not on all four groups, so it is smaller
(3 × 1,024 = 3,072), and its result is added to the round's output, so what
round 1 found carries through.

**4. Read out.** Each round leaves 32 numbers per atom. Together these 64
numbers, written $\mathbf{h}_i$, describe the surroundings of atom $i$. Round
1's numbers go through one weighted sum, round 2's through a small chain with
$\phi$, and the two results add up to the atom's share, $E_i$, apart from a
fixed baseline described in the next section.

### From one ethanol frame to an energy

```{figure} ../figures/agent_mlip_atom.svg
:name: mlip-atom-figure

One ethanol frame through the trained model, following its first carbon. All
values are real. The grid is the carbon's 64 numbers $\mathbf{h}_i$, one square
each.
```

Here is the whole path for the frame in {numref}`ethanol-frame-figure`.
{numref}`mlip-atom-figure` follows its first carbon, atom 1, from left to
right.

**Neighbours in.** The cutoff is 4 Å, and no atom of ethanol is more than
2.35 Å from the first carbon, so $\mathcal{N}_i$ holds all eight other atoms.
In this frame no two atoms are more than 3.63 Å apart, so the same is true of
every atom.

**64 numbers.** Steps 1 to 3 and round 2 turn $\mathcal{N}_i$ into
$\mathbf{h}_i$, which starts 0.235, 0.087, −0.185. No single one of these has a
physical meaning.

**One share, then the total.** The readout turns $\mathbf{h}_i$ into this
carbon's share, $E_i = -206.112$ eV. The same path for each of the nine atoms
gives nine shares, and they add up to $U = -4215.102$ eV, the model's energy
for this frame.

The shares look large because the readout does not produce them whole. Each
atomic number gets a fixed **baseline**, and the readout supplies a small
**correction** on top, scaled by a fixed factor of 1.13 (set from the typical force size in the
training data):

| Atom | Baseline (eV) | Correction (eV) | Share $E_i$ (eV) |
| --- | ---: | ---: | ---: |
| C | −205.601 | −0.511 | −206.112 |
| C | −205.601 | −0.276 | −205.877 |
| O | −102.800 | −0.253 | −103.053 |
| H | −616.802 | +0.097 | −616.705 |
| H | −616.802 | +0.191 | −616.611 |
| H | −616.802 | +0.095 | −616.707 |
| H | −616.802 | +0.151 | −616.651 |
| H | −616.802 | +0.159 | −616.643 |
| H | −616.802 | +0.058 | −616.744 |
| **Total** | **−4214.816** | **−0.287** | **−4215.102** |

Each entry is rounded, so a column can differ from its total in the last digit
or two. A hydrogen at −616.8 eV looks wrong;
[Where the baselines come from](#where-the-baselines-come-from) explains why.

**The forces.** The force on the first carbon is minus the slope of $U$ with
respect to that atom's position. The software works this slope out exactly, by
going back through every step of the model, which is why every step has to be
smooth. The model gives (−2.266, −1.763, −2.638) eV/Å.

So an MLIP gets an energy like this: each atom's neighbours go into the network,
one number comes out per atom, and the numbers add up. Whether −4215.102 eV is
right can only be told by comparing it with DFT for the same frame. That
comparison is what training is built on.

## How an MLIP is trained

The last section took the parameters $\theta$ as given. Training finds their
values: they start random and are adjusted, over many small steps, until the
model reproduces the DFT energies and forces of known frames. For LJ, two numbers can be taken from a table. Nobody can
choose 67,472 numbers by hand, so they are fitted to examples: frames with
their energies and forces from DFT.

The examples here are ethanol frames from MD17, a public dataset. Each frame is
a snapshot from a simulation of one ethanol molecule, with its energy and
forces computed by DFT. The frames are split three ways before training starts:

- **Training frames** are the ones the weights are fitted to.
- **Validation frames** are checked during training, to watch the error on
  frames the fit does not use.
- **Test frames** are checked once, at the end.

The test run used 450, 50 and 200 frames.

### The loss

The fit needs one number that says how wrong the model is. That is the
**loss** $L$. For a frame of $N$ atoms it compares the model's energy and
forces with the DFT values:

$$
L(\theta) = w_E \left(\frac{U_\theta - U^{\text{DFT}}}{N}\right)^2
+ \frac{w_F}{3N} \sum_i \left|\mathbf{F}_{i,\theta} - \mathbf{F}_i^{\text{DFT}}\right|^2
$$

The numbers $w_E$ and $w_F$ set how much each part counts. The test run used 1
and 100, so forces counted far more. A low loss means the model's surface
passes close to the DFT points, the fit from
{numref}`line-vs-bends-figure` in many dimensions.

### The training step

Training lowers $L$ by moving the weights a little at a time. Each move uses a
**batch**, a small group of training frames. The software works out how $L$
changes when each weight changes, by going back through the model just as it
does for the forces, and moves every weight a little in the direction that
lowers $L$. {numref}`fit-loop-figure` shows one such step.

```{figure} ../figures/agent_fit_loop.svg
:name: fit-loop-figure

One training step. The test run repeated it 1800 times: 45 batches of 10
frames, for 40 epochs. An epoch is one pass through the training frames.
```

### Watching the error

{numref}`val-curve-figure` shows the force error on the validation frames as
training went on. It fell from 293 to 48 meV/Å. Once the steps were done, the
finished model was run on the test frames. {numref}`force-parity-figure` puts
every predicted force component beside its DFT value.

```{figure} ../figures/agent_val_curve.svg
:name: val-curve-figure
:width: 520px

Force error on the 50 validation frames during the test run, from the training
log.
```

```{figure} ../figures/agent_force_parity.svg
:name: force-parity-figure
:width: 340px

Every force component in the 200 test frames, 5400 points. A perfect model
would put them all on the line. The mean error is 48.2 meV/Å.
```

### How close the ethanol frame came

The ethanol frame from the last section is one of the test frames. DFT gives it
−4214.753 eV. The model's −4215.102 eV is 0.349 eV too low, or 39 meV per atom.
Over all 200 test frames the average is 0.341 eV, or 37.9 meV/atom.

That sounds large, but {numref}`energy-offset-figure` shows what kind of error
it is. Every test frame is too low by nearly the same amount. With that shift
taken away, the average error is 0.012 eV per frame, or 1.3 meV/atom. A
constant shift changes no force, because forces are slopes of $U$.

```{figure} ../figures/agent_energy_offset.svg
:name: energy-offset-figure
:width: 340px

The energies of the 200 test frames. The points follow a line parallel to the
perfect one, shifted down by 0.34 eV.
```

The forces are close too. For the first carbon, the model's
(−2.266, −1.763, −2.638) eV/Å compares with (−2.252, −1.742, −2.537) eV/Å from
DFT.

### Where the baselines come from

The baselines in the ethanol table also come from the training frames. A
hydrogen at −616.8 eV looks wrong, and as a physical energy it is. The nine
baselines add up to the average energy of the training frames, −4214.816 eV.
Every training frame holds the same nine atoms, so the data cannot say how that
average should be split between atomic numbers. The software picks the
smallest set of baselines that adds up to the average, and with one kind of
molecule that makes each atom's baseline proportional to how many of its kind
there are: O 1, C 2, H 6. That is why the share of one atom has
no meaning by itself, and only the total does.

### What a low error does not show

A low error is not the end of the job. In a simulation the atoms can reach
arrangements unlike any in the training frames, so the model also has to be run
in MD, to see whether the molecule stays intact and the energy is conserved. If
a check fails, someone has to decide what to change: more data, a larger model,
longer training. Then the model is trained again, and checked again. Training
an MLIP is a loop.

### What the test run showed

The loop has run once from end to end, by hand, on MD17 ethanol, with a small
model: 500 frames (450 to train on and 50 to validate), 40 epochs, about 2
minutes on a CPU. The MD check ran in OpenMM, for reasons given on the next
page. The agent's run on the next page uses rMD17 instead, a corrected version
of MD17 that comes with official splits and a warning not to train on more
than 1000 frames, because frames from one simulation are correlated.

| Check | Result |
| --- | --- |
| Force error, test frames | 48.2 meV/Å (5.7% of the average DFT force component) |
| Force error, training frames | 46.5 meV/Å |
| Energy error, test frames | 37.9 meV/atom |
| OpenMM MD | 2000 steps of 0.5 fs, started at 500 K |
| Closest pair, final frame | 0.98 Å |

This shows that the loop works. It does not yet show a good model. No accuracy
target has been set, the energy error is nearly all the constant shift
described above and its cause is not known, and the test frames come from the
same simulation as the training frames, so these errors may be optimistic.

## Check yourself

:::{dropdown} What does training an MLIP determine, and what plays the same role in LJ?
It determines the parameters $\theta$. In LJ the same role is played by
$\varepsilon$ and $\sigma$, but there are two of those and 67,472 of these.
:::

:::{dropdown} Where does an MLIP get the bending it needs to follow a curved surface?
From the smooth function $\phi$ in its chains, and from the combine step, which
multiplies gathered numbers together.
:::

:::{dropdown} Why does a hydrogen get a share of about −616 eV?
The baselines only have to add up to the average energy of the training frames.
Every frame holds the same atoms, so the data cannot fix how that average is
split between atomic numbers. Only the total has meaning.
:::

## Next

Every step on this page was run by hand, and a person read the results and
decided what to do next. [Automating the loop with an agent](02_the_agent.md)
hands that deciding to a language model.
