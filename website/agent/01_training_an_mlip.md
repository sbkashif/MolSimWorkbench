# Use case: training an MLIP

:::{admonition} Draft
:class: warning
This section is being built. Every number on this page comes from one small
test run, made by hand before the agent existed.
:::

This section builds an agent for a molecular simulation task. It stands on its
own: this page sets out the task, and the next three make the case for an
agent, show what ours did, and show how it was built.

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
and σ ([Jones 1924](https://doi.org/10.1098/rspa.1924.0082)). That suits argon, but LJ has no term for a chemical bond.
Classical force fields add fixed bond terms, but those bonds cannot break or
form. Density functional theory (DFT) can describe one, because it computes the
energy from the electrons ([Hohenberg and Kohn 1964](https://doi.org/10.1103/PhysRev.136.B864);
[Kohn and Sham 1965](https://doi.org/10.1103/PhysRev.140.A1133)), but it is far too slow to call at every step of a
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
$E_i$. The shares add up to $U$. This split into atomic shares is how neural
network potentials were first made to work for large systems
([Behler and Parrinello 2007](https://doi.org/10.1103/PhysRevLett.98.146401)).

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
set of values gives one energy, and another set gives a different one.

### Neural network basics

A neural network is a function: numbers go in, and a number comes out. Its
parameters decide what comes out. Let us look at the function first, using the
small network in {numref}`nn-basics-figure`.

```{figure} ../figures/agent_nn_basics.svg
:name: nn-basics-figure
:width: 520px

A small neural network. Each circle is one number, and each line is one
weight: 12 + 4 = 16 in all.
```

Three numbers go in: $x_1$, $x_2$, $x_3$. The first **step** turns them into
$m = 4$ new numbers, $y_1$ to $y_4$. Each $y$ is a weighted sum of all three
$x$'s, passed through a simple function $\phi$:

$$
\begin{aligned}
y_1 &= \phi(W_{11}x_1 + W_{12}x_2 + W_{13}x_3)\\
y_2 &= \phi(W_{21}x_1 + W_{22}x_2 + W_{23}x_3)\\
y_3 &= \phi(W_{31}x_1 + W_{32}x_2 + W_{33}x_3)\\
y_4 &= \phi(W_{41}x_1 + W_{42}x_2 + W_{43}x_3)
\end{aligned}
$$

The numbers $W$ are the **weights**. Each $y$ has its own three, one for each
$x$: the first index says which $y$ the weight belongs to, the second which $x$
it multiplies. So this step has $4 \times 3 = 12$ weights. No $y$ appears on
the right-hand side: none depends on another, and each is made from the $x$'s
alone.

The second step turns the four $y$'s into the output, with four weights of its
own, called $V$ only to tell them apart from the first step's $W$:

$$
\text{out} = V_1 y_1 + V_2 y_2 + V_3 y_3 + V_4 y_4
$$

It has to wait for all four $y$'s, because its sum uses every one. The last
step leaves $\phi$ out. That makes $12 + 4 = 16$ weights in all.

Larger networks repeat the same step: each step takes the numbers the last one
made, so the steps run one after another, as a chain. In general a step takes
$n$ numbers and makes $m$, each a weighted sum of all $n$ passed through
$\phi$, with $m \times n$ weights:

$$
y_k = \phi\Big(\sum_{j=1}^{n} W_{kj}\, x_j\Big)
$$

The function $\phi$ matters. To see why, give the same network one number in,
$x$, instead of three, and see what shapes its output can take against points
on a 2D plot ({numref}`line-vs-bends-figure`).

Without $\phi$, each $y_k = W_k x$, and

$$
\text{out} = V_1 W_1 x + V_2 W_2 x + V_3 W_3 x + V_4 W_4 x
= (V_1 W_1 + V_2 W_2 + V_3 W_3 + V_4 W_4)\, x
$$

The bracket is just one number, so the output is a straight line, and the
weights only set its tilt. A straight line can
only cut through the middle of points that curve, above some and below others
(panel a).

Now put $\phi$ back, with a constant $b_k$ in each sum:
$y_k = \phi(W_k x + b_k)$. Inside the brackets, $W_k x + b_k$ is a straight
line: negative on one side of the point where it crosses zero, positive on the
other (panel a of {numref}`phi-figure`). The $\phi$ an MLIP uses, called SiLU
([Hendrycks and Gimpel 2016](https://arxiv.org/abs/1606.08415); [Elfwing et al. 2018](https://doi.org/10.1016/j.neunet.2017.12.012)),
presses large negative numbers to near zero and leaves large positive ones
nearly as they are, with a smooth turn in between, where it dips a little below
zero (to −0.28 at most). So the line comes out with one bend, near the point
where $W_k x + b_k = 0$, that is at $x = -b_k / W_k$ (panel b). Changing $b_k$
moves the bend along $x$: making it more negative lowers the whole line, so it
crosses zero further to the right, and the bend moves with it. The sign of
$W_k$ sets which way it turns, and its size how sharply (panel c). This is also why the last step leaves $\phi$ out:
through it, the output could never go below −0.28.

```{figure} ../figures/agent_phi.svg
:name: phi-figure

How $\phi$ turns a straight line into one bend, here with $W = 2$ and
$b = 0$. (a) The line $Wx + b$; ▼ marks where it crosses zero. (b) After
$\phi$, the negative half is pressed to near zero (arrows) and the positive
half is nearly kept. (c) Changing $b$ moves the bend; flipping $W$ turns it the
other way.
```

The output adds the bent pieces up, so their bends add up too.

So $m$, the number of $y$'s, sets how many bends the output can have. With
$m = 1$ there is one bend, and the output follows one part of the points but
not the rest (panel b). With $m = 4$ there are four, and the output can twist
down and up again to follow them all (panel c). More $y$'s allow more detail.

A simpler $\phi$, one that sets negative numbers to zero
([Nair and Hinton 2010](https://www.cs.toronto.edu/~hinton/absps/reluICML.pdf)), makes sharp corners
instead of rounded ones. An MLIP uses the smooth one, because the forces are
slopes of the energy and a corner would make them jump.

```{figure} ../figures/agent_line_vs_bends.svg
:name: line-vs-bends-figure

The same made-up points, against the network of {numref}`nn-basics-figure`
with one number in. (a) Without $\phi$, other weights and constants only tilt
or shift the line. (b) $m = 1$: one bend. (c) $m = 4$: four bends, enough to
follow the points.
```

The weights and constants set where the bends land and how sharply they turn,
so a different set gives a different curve; $m$ sets how many bends there can
be. In an MLIP, the weights make up almost all of the parameters $\theta$ from
the last section. Unlike $\varepsilon$ and $\sigma$, no single weight means
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

LJ would build this surface from one fixed curve shape, the pair well, scaled
for each pair type and applied to every pair: its two numbers can only make the
well deeper or move it further out ({numref}`lj-family-pes-figure`, panel a).
An MLIP fits the surface to the points, the way the chain in
{numref}`line-vs-bends-figure` fits a curve, so it is not limited to sums of
one curve.

A trained MLIP is, in effect, a rebuilt copy of the DFT energy function. Give it
an arrangement of ethanol's atoms, and it returns roughly the energy and forces
DFT would give, without doing the electronic calculation. That is what makes
long MD runs with near-DFT forces affordable. Panels b and c are two views of
that rebuilt surface. It is only as good as the frames it was built from: near
the kinds of arrangements it was trained on it can be trusted, and far from
them it has nothing to go on. How close the copy came is the subject of
[How an MLIP is trained](#how-an-mlip-is-trained).

```{figure} ../figures/agent_lj_family_vs_pes.svg
:name: lj-family-pes-figure

(a) The LJ pair curve. A larger $\varepsilon$ makes the well deeper and a
larger $\sigma$ moves it further out, but the shape stays the same. (b) The
energy of the model trained on this page as two things change: the C–O bond
length, and the twist of the O–H bond about it (the H–O–C–C angle). Every other
atom stays where it was in one test frame. (c) The same twist, but with every
other atom free to settle at each step. Both ranges stay inside what the
training frames cover.
```

:::{admonition} Reading {numref}`lj-family-pes-figure`
:class: warning
The model only maps positions to an energy: give it the positions of a frame's
atoms, and it returns one number. It has no parameter for a bond length or a twist.
Panels b and c project its output onto two coordinates we chose because they
are already understood, to see whether it behaves as the physics says it
should. Here it does. The energy is lowest at a C–O length of about 1.42 Å,
close to the 1.431 Å measured for ethanol
([NIST CCCBDB](https://cccbdb.nist.gov/expgeom2x.asp?casno=64175&charge=0)). With the other atoms free to settle (panel c), the
twist has three minima, at 60°, 180° and 300°: the trans form, whose measured
geometry NIST lists, and two gauche forms, seen in ethanol's microwave
spectrum ([Kakar and Quade 1980](https://doi.org/10.1063/1.439723)). With the other atoms held (panel b),
trans rises to 0.16 eV, because they stay as they were in a frame at gauche
(twist 61°).

Such probes show what the model outputs, not how it gets there. Its 67,472
parameters are all known, stored in the model file, but no single one means
anything by itself. Working out what a trained network has learned is a
research field of its own, interpretable machine learning
([Oviedo et al. 2022](https://doi.org/10.1021/accountsmr.1c00244)). This page does not go there.
:::

{numref}`lj-family-pes-figure` is what a trained model gives. How the MLIP
turns positions into an energy, stage by stage, is in the detour below, and the
rest of the page does not need it.

::::{dropdown} Detour: from a neural network to a real MLIP
:class-container: detour-dropdown

We now have both pieces. A neural network can bend to fit points, and ethanol
gives the points to fit: frames and their DFT energies. But not all 27 numbers
of a frame matter. Moving the whole molecule along $x$, $y$ or $z$ changes the
numbers but not the energy, and so does turning it about any of three axes.
Setting those 3 + 3 aside leaves 21 numbers that the energy really depends on.
A chain that reads the 27 coordinates as they are would not know that, and
would have to learn it from the data. Nor does it help to pick 21 numbers by
hand, such as bond lengths and angles: they can be picked in many ways, and
every choice is tied to one molecule. So an MLIP builds three things into the
function itself:

- **Where the molecule sits.** Moving the whole molecule must not change the
  energy.
- **Size and order.** The same function must work for any number of atoms, in
  any order. Swap two hydrogens in the list, and the molecule has not changed.
- **Turning.** Turning the whole molecule must not change the energy.

Part of the answer is the one in {numref}`lj-mlip-figure`: an MLIP works one
atom at a time. Each atom's neighbours, $\mathcal{N}_i$, give that atom's share
$E_i$, and the shares add up to $U$, so any number of atoms works. But
$\mathcal{N}_i$ is still not a fixed list of numbers, and the other two
problems remain. So an MLIP wraps chains in stages that fix all three. The
MLIP used here is called **MACE** ([Batatia et al. 2022](https://arxiv.org/abs/2206.07697); code at
[github.com/ACEsuit/mace](https://github.com/ACEsuit/mace)). {numref}`mace-steps-figure` shows its stages for one atom,
with the real number of weights in each stage of the model trained for this page.

```{figure} ../figures/agent_mace_steps.svg
:name: mace-steps-figure

The stages of the trained MACE model for atom $i$. Stages 2 and 3 together
make one round. The stages marked φ(Wx) contain a chain like the one in
{numref}`nn-basics-figure`.
```

**Stage 1: Describe.** Each neighbour is described relative to atom $i$, so where the
molecule sits no longer matters. Its distance becomes 8 numbers, each a smooth
wave, damped so it fades to zero at the cutoff of 4 Å
([Gasteiger et al. 2020](https://arxiv.org/abs/2003.03123)), so a neighbour drifting out of
range changes the energy smoothly. Its direction becomes 16 numbers, in four
groups of 1, 3, 5 and 7, that turn with the molecule in a fixed, known way
([Geiger and Smidt 2022](https://arxiv.org/abs/2207.09453)). Both
come from fixed formulas, so neither needs weights. The weights in this stage
belong to the atomic number: it becomes a list of 32 numbers, one list each for
H, C and O, so 3 × 32 = 96 weights, learned so the model can tell carbon from
hydrogen.

**Stage 2: Gather.** Here the chain from the basics appears. A chain of four steps,
8 → 64 → 64 → 64 → 128 numbers, takes a neighbour's 8 distance numbers and
returns 128 numbers. It has 8×64 + 64×64 + 64×64 + 64×128 = 16,896 weights.
The 128 numbers it returns are 32 × 4: one for each of the neighbour's 32
numbers and each of the four groups of direction numbers. They set how much
each of those counts at this distance. The weighted results
are then added up over all neighbours. A sum does not care how many terms it
has or in what order they come, which solves the second problem.

The rest of this stage's weights are plain weighted sums that mix the 32
numbers, as in the basics but without $\phi$. One 32 × 32 mix comes before
gathering (1,024 weights), and one after it for each of the four groups
(4 × 1,024 = 4,096). A last mix, with a separate 32 × 32 for each group and
each atomic number, lets the atom's own atomic number count
(4 × 3 × 1,024 = 12,288). With the chain, that makes 34,304.

**Stage 3: Combine.** A sum over neighbours only adds up what each neighbour gives on
its own. The energy also depends on how neighbours sit relative to each other,
for example the H–C–H angle. So this stage multiplies the gathered numbers, up
to three at a time, an idea MACE takes from the atomic cluster expansion
([Drautz 2019](https://doi.org/10.1103/PhysRevB.99.014104)). A product of two sums holds terms with two neighbours at
once, and through their direction numbers, the angle between them. These
products are a second source of bending, besides $\phi$. Because the direction
numbers turn in a known way, the products can be arranged so that the 32
numbers kept for the atom do not change at all when the molecule turns. That
solves the third problem. There are 28 such products, each with its own weight
for each of the 32 numbers and each atomic number (28 × 32 × 3 = 2,688), and a
final 32 × 32 mix (1,024), so 3,712 weights.

**Round 2.** Stages 2 and 3 run again on the 32 numbers from round 1. Each
neighbour has already gathered from its own neighbours, so information now
reaches farther than one cutoff. Round 2 has a chain and a combine stage of its
own, with the same counts as round 1. Only its last atomic-number mix differs. It acts on the 32 numbers the atom
kept from round 1, not on all four groups, so it is smaller
(3 × 1,024 = 3,072), and its result is added to the round's output, so what
round 1 found carries through.

**Stage 4: Read out.** Each round leaves 32 numbers per atom. Together these 64
numbers, written $\mathbf{h}_i$, describe the surroundings of atom $i$. Round
1's numbers go through one weighted sum, round 2's through a small chain with
$\phi$, and the two results add up to the atom's share, $E_i$, apart from a
fixed baseline described further on.

**From one ethanol frame to an energy**

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
2.15 Å from the first carbon, so $\mathcal{N}_i$ holds all eight other atoms.
In this frame no two atoms are more than 3.70 Å apart, so the same is true of
every atom.

**64 numbers.** Stages 1 to 3 and round 2 turn $\mathcal{N}_i$ into
$\mathbf{h}_i$, which starts 0.295, 0.089, −0.143. No single one of these has a
physical meaning.

**One share, then the total.** The readout turns $\mathbf{h}_i$ into this
carbon's share, $E_i = -205.800$ eV. The same path for each of the nine atoms
gives nine shares, and they add up to $U = -4210.064$ eV, the model's energy
for this frame.

The shares look large because the readout does not produce them whole. Each
atomic number gets a fixed **baseline**, and the readout supplies a small
**correction** on top, scaled by a fixed factor of 1.18 (set from the typical force size in the
training data):

| Atom | Baseline (eV) | Correction (eV) | Share $E_i$ (eV) |
| --- | ---: | ---: | ---: |
| C | −205.348 | −0.452 | −205.800 |
| C | −205.348 | −0.247 | −205.595 |
| O | −102.674 | −0.249 | −102.923 |
| H | −616.043 | +0.067 | −615.976 |
| H | −616.043 | +0.141 | −615.902 |
| H | −616.043 | +0.026 | −616.017 |
| H | −616.043 | +0.016 | −616.027 |
| H | −616.043 | +0.046 | −615.997 |
| H | −616.043 | +0.217 | −615.826 |
| **Total** | **−4209.629** | **−0.435** | **−4210.064** |

Each entry is rounded, so a column can differ from its total in the last digit
or two. A hydrogen at −616.0 eV looks wrong; the end of
this detour explains why.

**The forces.** The force on the first carbon is minus the slope of $U$ with
respect to that atom's position. The software works this slope out exactly, by
going back through every step of the model
([Rumelhart et al. 1986](https://doi.org/10.1038/323533a0)), which is why every step has to be
smooth. The model gives (0.985, 1.430, −1.120) eV/Å.

So an MLIP gets an energy like this: each atom's neighbours go into the network,
one number comes out per atom, and the numbers add up. Whether −4210.064 eV is
right can only be told by comparing it with DFT for the same frame. That
comparison is what training is built on.

**Where the baselines come from**

The baselines in the ethanol table also come from the training frames. A
hydrogen at −616.0 eV looks wrong, and as a physical energy it is. The nine
baselines add up to the average energy of the training frames, −4209.629 eV.
Every training frame holds the same nine atoms, so the data cannot say how that
average should be split between atomic numbers. The software picks the
smallest set of baselines that adds up to the average, and with one kind of
molecule that makes each atom's baseline proportional to how many of its kind
there are: O 1, C 2, H 6. That is why the share of one atom has
no meaning by itself, and only the total does.
::::

## How an MLIP is trained

Start from where the last section left the MLIP: a function from positions to
an energy. Give it the positions of the atoms in an ethanol frame, and it
returns one energy, $U_\theta$. The forces are the slopes of that energy,
$\mathbf{F}_{i,\theta} = -\partial U_\theta / \partial \mathbf{r}_i$. The
subscript $\theta$ is a reminder that both depend on the 67,472 parameters.

We want this function to give the energy and forces DFT would give. So we start
from a random guess for the 67,472 numbers and adjust them until the model's
output matches DFT on examples where the answer is already known: here, ethanol
frames from rMD17, a public dataset
([Christensen and von Lilienfeld 2020](https://doi.org/10.1088/2632-2153/abba6f)).
Each frame is a snapshot from a simulation, taken from an earlier dataset, MD17
([Chmiela et al. 2017](https://doi.org/10.1126/sciadv.1603015)), with its DFT
energy and forces computed again with tighter numerical settings
([data release](https://doi.org/10.6084/m9.figshare.12672038)). This is called
**training** the model.

The model is meant to be right beyond this data too, for arrangements it has
never seen. So we train it on one part of the data and keep the rest aside, to
check that the training worked and that the model can be trusted on frames it
was not fitted to. This matters because a model can match every frame it was
fitted to and still be wrong in between, the way a curve with many
bends can pass through every point and swing wildly between them. The
frames are split into three sets with a job each. Most of them, the **training frames**, are what the parameters are fitted to.
A few, the **validation frames**, are set aside and checked as training goes
on: if their error stops falling while the error on the training frames keeps
falling, the model is learning those particular frames rather than ethanol. A
last set, the **test frames**, is checked only once, at the very end, so the
final error comes from frames that played no part in any choice. The test run
took 500 frames at random from rMD17's official training set, and MACE set 50
of them aside to validate, leaving 450 to train. The 200 test frames are the
first 200 of rMD17's official test set.

Training then goes round a cycle ({numref}`fit-loop-figure`). Take a small
group of training frames, a **batch**. Predict their energies and forces with
the current parameters. Compare them with DFT. Adjust the parameters a little
to close the gap. Then go round again with a different batch: the next group of
training frames. Once every training frame has been used, that is one
**epoch**. The frames are then shuffled into new batches, and the cycle goes
through them again. Two parts of the
cycle need explaining: the comparison, which is the loss, and the adjustment,
which is the training step.

```{figure} ../figures/agent_fit_loop.svg
:name: fit-loop-figure

The training cycle. One round is one training step. The test run went round
1800 times: 45 batches of 10 frames make one epoch, repeated 40 times.
```

### How training can go wrong

The validation frames are there because training can fail in two opposite
ways. {numref}`train-wrong-figure` shows both, with small networks like the one
in {numref}`nn-basics-figure`, really trained, on made-up points: training
points (filled), which the model is fitted to, and validation points (open),
kept aside half-way between them.

- **Too simple** (panel a). A network with one $y$ has one bend, so it cannot
  follow the points. It misses the training points and the validation points
  alike: its error is high on both (0.53 and 0.17).
- **About right** (panel b). With four $y$'s, the model follows the shape of the
  points without chasing each one. Its error is low on both sets (0.015 and
  0.021), and about the same: what it learned from the training points carries
  over to the points in between.
- **Too flexible** (panel c). Two steps of 64 numbers each, trained for 30,000
  epochs, have enough bends to pass through every training point, noise
  included. The training error falls to almost zero, but the curve wiggles
  between the points, and the validation error ends up nearly twice that of
  (b), at 0.038.

Panel d shows how (c) got there. For the first 150 epochs both errors fall
together. After that the validation error turns up while the training error
keeps falling: from there on, the model is learning the noise in its training
points, not the curve. That is the sign the validation frames are watched for.
A common remedy is to keep the parameters from where the validation error was
lowest; others are more data, or a smaller model.

```{figure} ../figures/agent_train_wrong.svg
:name: train-wrong-figure

Small networks trained on made-up points. Filled: training points. Open:
validation points, kept aside. (a) One $y$: too simple. (b) Four $y$'s: about
right. (c) Two steps of 64 numbers each: too flexible. (d) The training and
validation error of (c) over 30,000 epochs; the dashed line marks where the
validation error was lowest, at epoch 150.
```

A third way is quieter: a model can be right on every frame it has seen and
wrong on arrangements unlike any of them. Validation and test frames come from
the same data, so they cannot show it.
[What a low error does not show](#what-a-low-error-does-not-show) comes back to
it.

### The loss

The **loss** $L$ compares the model's energy and forces for a frame of $N$
atoms with the DFT values:

$$
L(\theta) = w_E \left(\frac{U_\theta - U^{\text{DFT}}}{N}\right)^2
+ \frac{w_F}{3N} \sum_i \left|\mathbf{F}_{i,\theta} - \mathbf{F}_i^{\text{DFT}}\right|^2
$$

The numbers $w_E$ and $w_F$ set how much each part counts. The test run used 1
and 100, so forces counted far more. A low loss means the model's surface
passes close to the DFT points, the fit from
{numref}`line-vs-bends-figure` in many dimensions.

### The training step

The training step is the "Adjust θ" box of {numref}`fit-loop-figure`. For one
batch, the software works out how $L$ changes when each parameter changes, by going back through every step of the model
([Rumelhart et al. 1986](https://doi.org/10.1038/323533a0)), and moves every parameter a little in the direction
that lowers $L$. The run used the Adam method for these moves
([Kingma and Ba 2014](https://arxiv.org/abs/1412.6980)), in its AMSGrad form
([Reddi et al. 2018](https://arxiv.org/abs/1904.09237)).

### Watching the error

{numref}`val-curve-figure` shows the force error on the validation frames as
training went on. It fell from 326 to 39 meV/Å, with two bumps on the way, and
was lowest at the last check: no sign yet of the turn-up in panel d of
{numref}`train-wrong-figure`. Once the steps were done, the
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
would put them all on the line. The mean error is 41.0 meV/Å.
```

### How close the ethanol frame came

The ethanol frame of {numref}`ethanol-frame-figure` is one of the test frames.
DFT gives it −4209.890 eV, and the trained model −4210.064 eV: 0.174 eV too low,
or 19 meV per atom.
Over all 200 test frames the average is 0.180 eV, or 20.0 meV/atom.

That sounds large, but {numref}`energy-offset-figure` shows what kind of error
it is. Every test frame is too low by nearly the same amount. With that shift
taken away, the average error is 0.011 eV per frame, or 1.2 meV/atom. A
constant shift changes no force, because forces are slopes of $U$.

```{figure} ../figures/agent_energy_offset.svg
:name: energy-offset-figure
:width: 340px

The energies of the 200 test frames. The points follow a line parallel to the
perfect one, shifted down by 0.18 eV.
```

The forces are close too, though not every component equally. For the first
carbon, the model's (0.985, 1.430, −1.120) eV/Å compares with
(1.015, 1.263, −1.083) eV/Å from DFT. The middle component is 0.17 eV/Å off,
about four times the average force error.

### What a low error does not show

A low test error says the model matches DFT on frames like the ones it was
trained on. It does not say what the model does when it is used. In MD the
model decides where the atoms go next, step after step, and small errors in
the forces can carry the atoms into arrangements the training frames never
covered. Every frame here comes from one simulation of one molecule, and test
frames from the same simulation share its gaps, so their error cannot show
this. Away from its data, the model can fail in three ways that an MD run shows
directly:

- two atoms pushed closer together than any real bond allows;
- a bond broken, or a new one formed;
- the total energy drifting, in a run with no thermostat, where it should stay
  constant.

None of these stops the run with an error. They have to be looked for.

Benchmarks of MLIPs have found that a low force error on test frames does not
guarantee a stable MD run ([Fu et al. 2023](https://arxiv.org/abs/2210.07237);
[Stocker et al. 2022](https://doi.org/10.1088/2632-2153/ac9955)). So the model
also has to be run in MD, and the run checked.

### One pass round the loop, by hand

Training an MLIP therefore does not end with the test error. It goes: data,
train, check the errors, check in MD, and then decide. Is the model good
enough, or should something change and the model be trained again? Everything
on this page came from one pass through these steps, made by hand:

| Step | What was done | What came out |
| --- | --- | --- |
| 1. Data | 450 rMD17 frames to train on, 50 to validate, 200 to test | |
| 2. Train | MACE, 40 epochs, about 2 minutes on a CPU | validation force error fell from 326 to 39 meV/Å |
| 3. Check the errors | the finished model on the 200 test frames | force error 41.0 meV/Å (4.8% of the average DFT force component), against 37.8 on the training frames; energy error 20.0 meV/atom |
| 4. Check in MD | 2000 steps of 0.5 fs in OpenMM ([Eastman et al. 2024](https://doi.org/10.1021/acs.jpcb.3c06662)), started at 500 K | closest two atoms in the last frame: the O–H bond, at 0.98 Å |
| 5. Decide | read steps 3 and 4 against a target: is the force error low enough, and does MD hold? | if yes, stop; if not, change one thing (more frames, a wider model, longer training) and go back to step 2. Not done here: no target had been set |

Some of this is good. The forces are close to DFT, and the error on the test
frames is about the same as on the training frames, so there is no sign of the
model learning its training frames rather than ethanol. In MD the closest two
atoms at the end were the oxygen and its hydrogen, at about their bond length
(0.971 Å measured, [NIST CCCBDB](https://cccbdb.nist.gov/expgeom2x.asp?casno=64175&charge=0)), so nothing had crashed together.

Some of this is not. The energy error is nearly all the constant shift, and its
cause is not known. No target was set, so nothing says whether 41.0 meV/Å is
good enough. And the test frames come from the same simulation as the training
frames, so these errors may be optimistic.

So the loop has run once, and stopped at step 5. Step 5 is what makes it a
loop: it needs a target to judge against, a choice of what to change, and then
steps 2 to 4 again, as many times as it takes. By hand, that means a person
reading the numbers each time, deciding, and starting the next training.

The next page hands that reading and deciding to an agent, and gives it what
this pass lacked: a target, 20 meV/Å for the force error, and limits for the MD
check.

Both pages use the same data, rMD17 ethanol. Its
[data release](https://doi.org/10.6084/m9.figshare.12672038) comes with official
splits and a warning not to train on more than 1000 frames, because frames from
one simulation are not independent of each other.

## Check yourself

:::{dropdown} What does training an MLIP determine, and what plays the same role in LJ?
It determines the parameters $\theta$. In LJ the same role is played by
$\varepsilon$ and $\sigma$, but there are two of those and 67,472 of these.
:::

:::{dropdown} Where does an MLIP get the bending it needs to follow a curved surface?
From the smooth function $\phi$ in its chains, and from the combine stage, which
multiplies gathered numbers together.
:::

:::{dropdown} Why does a hydrogen get a share of about −616 eV?
The baselines only have to add up to the average energy of the training frames.
Every frame holds the same atoms, so the data cannot fix how that average is
split between atomic numbers. Only the total has meaning.
:::

## Next

Every step on this page was run by hand, and a person read the results and
decided what to do next. [The case for an agent](02_the_agent.md)
hands that deciding to an agent, a program built around a language model.

## References

| | Reference | Used for |
| --- | --- | --- |
| 1 | J. E. Jones, "On the determination of molecular fields. II. From the equation of state of a gas", *Proc. R. Soc. Lond. A* **106**, 463 (1924). [doi:10.1098/rspa.1924.0082](https://doi.org/10.1098/rspa.1924.0082) | the LJ formula |
| 2 | P. Hohenberg and W. Kohn, "Inhomogeneous electron gas", *Phys. Rev.* **136**, B864 (1964). [doi:10.1103/PhysRev.136.B864](https://doi.org/10.1103/PhysRev.136.B864) | DFT |
| 3 | W. Kohn and L. J. Sham, "Self-consistent equations including exchange and correlation effects", *Phys. Rev.* **140**, A1133 (1965). [doi:10.1103/PhysRev.140.A1133](https://doi.org/10.1103/PhysRev.140.A1133) | DFT |
| 4 | J. Behler and M. Parrinello, "Generalized neural-network representation of high-dimensional potential-energy surfaces", *Phys. Rev. Lett.* **98**, 146401 (2007). [doi:10.1103/PhysRevLett.98.146401](https://doi.org/10.1103/PhysRevLett.98.146401) | energy as a sum of atomic shares |
| 5 | D. Hendrycks and K. Gimpel, "Gaussian error linear units (GELUs)" (2016). [arXiv:1606.08415](https://arxiv.org/abs/1606.08415) | the SiLU $\phi$ |
| 6 | S. Elfwing, E. Uchibe and K. Doya, "Sigmoid-weighted linear units for neural network function approximation in reinforcement learning", *Neural Networks* **107**, 3 (2018). [doi:10.1016/j.neunet.2017.12.012](https://doi.org/10.1016/j.neunet.2017.12.012) | the name SiLU |
| 7 | V. Nair and G. E. Hinton, "Rectified linear units improve restricted Boltzmann machines", *ICML* (2010). [PDF](https://www.cs.toronto.edu/~hinton/absps/reluICML.pdf) | the sharp-cornered $\phi$ |
| 8 | I. Batatia, D. P. Kovács, G. N. C. Simm, C. Ortner and G. Csányi, "MACE: Higher order equivariant message passing neural networks for fast and accurate force fields", *NeurIPS* (2022). [arXiv:2206.07697](https://arxiv.org/abs/2206.07697) | MACE |
| 9 | MACE source code. [github.com/ACEsuit/mace](https://github.com/ACEsuit/mace) | the program used to train |
| 10 | J. Gasteiger, J. Groß and S. Günnemann, "Directional message passing for molecular graphs", *ICLR* (2020). [arXiv:2003.03123](https://arxiv.org/abs/2003.03123) | the smooth distance waves |
| 11 | M. Geiger and T. Smidt, "e3nn: Euclidean neural networks" (2022). [arXiv:2207.09453](https://arxiv.org/abs/2207.09453) | direction numbers that turn with the molecule |
| 12 | R. Drautz, "Atomic cluster expansion for accurate and transferable interatomic potentials", *Phys. Rev. B* **99**, 014104 (2019). [doi:10.1103/PhysRevB.99.014104](https://doi.org/10.1103/PhysRevB.99.014104) | products of gathered numbers |
| 13 | D. E. Rumelhart, G. E. Hinton and R. J. Williams, "Learning representations by back-propagating errors", *Nature* **323**, 533 (1986). [doi:10.1038/323533a0](https://doi.org/10.1038/323533a0) | slopes by going back through the model |
| 14 | S. Chmiela, A. Tkatchenko, H. E. Sauceda, I. Poltavsky, K. T. Schütt and K.-R. Müller, "Machine learning of accurate energy-conserving molecular force fields", *Sci. Adv.* **3**, e1603015 (2017). [doi:10.1126/sciadv.1603015](https://doi.org/10.1126/sciadv.1603015) | MD17 |
| 15 | A. S. Christensen and O. A. von Lilienfeld, "On the role of gradients for machine learning of molecular energies and forces", *Mach. Learn.: Sci. Technol.* **1**, 045018 (2020). [doi:10.1088/2632-2153/abba6f](https://doi.org/10.1088/2632-2153/abba6f) | rMD17 |
| 16 | A. S. Christensen and O. A. von Lilienfeld, "Revised MD17 dataset (rMD17)", figshare (2020). [doi:10.6084/m9.figshare.12672038](https://doi.org/10.6084/m9.figshare.12672038) | rMD17 data, how it was recomputed, and its 1000-frame warning |
| 17 | D. P. Kingma and J. Ba, "Adam: A method for stochastic optimization" (2014). [arXiv:1412.6980](https://arxiv.org/abs/1412.6980) | the training moves |
| 18 | S. J. Reddi, S. Kale and S. Kumar, "On the convergence of Adam and beyond", *ICLR* (2018). [arXiv:1904.09237](https://arxiv.org/abs/1904.09237) | AMSGrad |
| 19 | P. Eastman et al., "OpenMM 8: Molecular dynamics simulation with machine learning potentials", *J. Phys. Chem. B* **128**, 109 (2024). [doi:10.1021/acs.jpcb.3c06662](https://doi.org/10.1021/acs.jpcb.3c06662) | the MD check |
| 20 | F. Oviedo, J. L. Ferres, T. Buonassisi and K. T. Butler, "Interpretable and explainable machine learning for materials science and chemistry", *Acc. Mater. Res.* **3**, 597 (2022). [doi:10.1021/accountsmr.1c00244](https://doi.org/10.1021/accountsmr.1c00244) | interpretable machine learning |
| 21 | NIST Computational Chemistry Comparison and Benchmark Database (CCCBDB), "Experimental geometry data: ethanol", NIST Standard Reference Database 101. [cccbdb.nist.gov](https://cccbdb.nist.gov/expgeom2x.asp?casno=64175&charge=0) | the measured C–O bond length and the trans form |
| 22 | R. K. Kakar and C. R. Quade, "Microwave rotational spectrum and internal rotation in gauche ethyl alcohol", *J. Chem. Phys.* **72**, 4300 (1980). [doi:10.1063/1.439723](https://doi.org/10.1063/1.439723) | the gauche form of ethanol |
| 23 | X. Fu, Z. Wu, W. Wang, T. Xie, S. Keten, R. Gomez-Bombarelli and T. Jaakkola, "Forces are not enough: Benchmark and critical evaluation for machine learning force fields with molecular simulations", *Trans. Mach. Learn. Res.* (2023). [arXiv:2210.07237](https://arxiv.org/abs/2210.07237) | low force error, unstable MD |
| 24 | S. Stocker, J. Gasteiger, F. Becker, S. Günnemann and J. T. Margraf, "How robust are modern graph neural network potentials in long and hot molecular dynamics simulations?", *Mach. Learn.: Sci. Technol.* **3**, 045010 (2022). [doi:10.1088/2632-2153/ac9955](https://doi.org/10.1088/2632-2153/ac9955) | low force error, unstable MD |
