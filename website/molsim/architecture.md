# A simple molecular dynamics loop

A molecular dynamics (MD) simulation moves atoms by Newton's second law. It
treats each atom $i$ as a point with mass $m_i$, position $\mathbf{r}_i$ and
velocity $\mathbf{v}_i$, and follows classical mechanics: no quantum effects
for the atoms themselves. That is the main simplification MD makes.

The force on each atom comes from the potential energy $U$ of all the atoms
together. The force on atom $i$ points downhill in $U$, the direction in
which moving atom $i$ lowers the energy fastest:

$$
\mathbf{F}_i = -\frac{\partial U}{\partial \mathbf{r}_i}
$$

and the force sets the acceleration, as in Newton's second law:

$$
m_i \frac{d^2 \mathbf{r}_i}{dt^2} = \mathbf{F}_i
$$

For more than two atoms these equations have no solution on paper. The
simulation solves them step by step instead: from the positions and
velocities at time $t$, it computes every force, then the positions and
velocities a short time $\Delta t$ later. Repeating that step thousands of
times produces a **trajectory**: a film of the atoms in motion, like the one
on the [home page](../index.md).

This page builds that loop one part at a time, and runs each part as it comes.

## What the simulation holds

The simulation holds one **state**: the positions and velocities of every
atom, and the box that contains them. Everything else in the loop reads this
state or updates it.

A typical run here has 512 atoms in a cube. The numbers are in
**reduced units**: lengths are measured in σ (the atom's size, defined below),
energies in ε (the strength of attraction between two atoms), and every atom
has mass 1. Temperature is measured in units of ε divided by Boltzmann's
constant, so T = 1.0 means a typical thermal energy equal to the attraction
between two atoms. Reduced units keep the numbers near 1, and one run then
describes every substance that the same energy function fits, from argon to
krypton, after converting back to real units.

A box with hard walls would put many atoms next to a wall, and a real liquid
has no walls. The simulation avoids walls with a **periodic box**: the box is
surrounded on every side by copies of itself ({numref}`periodic-box-figure`).
An atom that leaves through one face comes back in through the opposite face,
and two atoms interact through whichever copy of the pair is nearest. This is
called the **minimum image** rule.

```{figure} ../figures/periodic_box.svg
:name: periodic-box-figure
:width: 340px

A periodic box in two dimensions. The solid square is the real box; the
dashed squares are copies of it. The left atom and the right atom interact
through the nearest copy (solid line), not straight across the box (dotted
line).
```

## The loop

Each time step runs the same four parts in the same order
({numref}`md-loop-figure`). Grey boxes are the parts of the loop, arrows show
what one part hands to the next, and the teal box is the energy function,
described in the next section.

```{figure} ../figures/md_loop.svg
:name: md-loop-figure

One time step of the simulation. The force step asks the energy function
(teal) for the force on every atom; the dashed boxes are other energy
functions that fit the same place.
```

1. **System.** The current positions and velocities.
2. **Force step.** The force on every atom, computed from the positions by the
   energy function.
3. **Integrator.** Moves every atom forward by one time step using its
   velocity and force: this is the step-by-step solution of Newton's equation
   from the top of the page. The integrator here is **Velocity-Verlet**. It
   moves each position using the current velocity and force,

   $$
   \mathbf{r}_i(t+\Delta t) = \mathbf{r}_i(t) + \mathbf{v}_i(t)\,\Delta t
       + \frac{\mathbf{F}_i(t)}{2m_i}\,\Delta t^2 ,
   $$

   recomputes the forces at the new positions, and then updates each
   velocity with the average of the old and new forces:

   $$
   \mathbf{v}_i(t+\Delta t) = \mathbf{v}_i(t)
       + \frac{\mathbf{F}_i(t) + \mathbf{F}_i(t+\Delta t)}{2m_i}\,\Delta t .
   $$
4. **Thermostat.** Holds the temperature at its target. This engine does it
   in the simplest way: after every step it scales all velocities by one
   factor, so that the kinetic energy matches the target temperature exactly.

The loop then starts again from the new state. The time step is short (0.001
in reduced units here), so 4000 steps cover 4 time units.

## Running the loop

That is the whole algorithm, and it can run now. A run is described by up to
three files in YAML, a plain-text format of `key: value` lines where indentation
shows nesting ({numref}`run-layers-figure`):

```{figure} ../figures/run_layers.svg
:name: run-layers-figure

The configuration files of a run. The simulation file names the others; the
geometry file is used only when the engine does not build the atoms itself.
```

**The simulation file** sets the run itself. An excerpt of
`data/lj_bulk_simulation.yaml`, with its comments removed:

```yaml
simulation:
  type: "dynamics"
  system_mode: "lj_particle_box"
  timestep: 0.001
  temperature: 1.0
  random_seed: 42

  lj_box:
    num_particles: 125
    density: 0.8
    initialization: "cubic_lattice"
    random_seed: 42

potential_config_files:
  - "potentials/lj_bulk.yaml"
```

All numbers are in reduced units (the comment on `timestep` in the file
still says picoseconds). `system_mode: "lj_particle_box"` tells the engine
to build the atoms itself: it places `num_particles` atoms on a cubic grid
inside a cube whose side $L$ gives the requested density,
$L = (N / \rho)^{1/3}$. For 125 atoms at density 0.8 that is
$L = 5.386$. Each atom is nudged off its grid point by up to 5% of the grid
spacing, so that the start is not perfectly symmetric.

**The potential file** chooses the [energy function](#the-energy-function):
here `potentials/lj_bulk.yaml`, LJ with ε = σ = 1 and a cutoff of 2.5.

**The geometry file** lists atoms one by one, for runs that do not use
`lj_particle_box`. Every run on these pages does use it, so none of them needs
a geometry file.

### Starting a run

A short Python script, `run.py`, starts the run through the `molsimworkbench`
module:

```python
from molsimworkbench import System

sim = System.from_yaml("data/lj_bulk_simulation.yaml",
                       trajectory_output="traj.xyz")
print("final energy:", sim.run())
```

`System.from_yaml` reads the files and builds the system; `sim.run()` runs the
loop and returns the final energy. These lines only hand the work to the C++
engine; the [pybind11 module](../pybind11/index.md) explains how.

You can run it here. The boxes hold the two real files, and **Run** does what
`run.py` does, then plays the trajectory as a movie, with every atom drawn at
its diameter σ. The engine is compiled to run inside your browser, so it runs
on your own computer and nothing is sent to a server. The first Run of a visit
downloads Python for the browser, which takes a few seconds. Your edits are
gone when you reload the page.

```{engine-run}
```

The run is **reproducible**. Press Run twice and you get the same lines: the
`random_seed` under `simulation` fixes the starting velocities, and the one
under `lj_box` fixes the small nudges off the grid. With `random_seed: 0` under
`simulation`, the velocities are drawn afresh and every run differs.

:::{admonition} How long a run is
:class: note
This file runs 1000 steps of 0.001, one unit of time. That is the engine's
default length, because the file sets no `total_steps`. The file's
`equilibration_steps` and `production_steps` are not used yet. To run longer,
add `total_steps` under `simulation`, as the 512-atom run below does
(`total_steps: 4000`, with `num_particles: 512`).
:::

### What comes out

**Printed lines.** Every 10 steps the engine prints the potential energy, the
kinetic energy, their sum, and the temperature. (The `Å` after the box length
is a leftover label; the length is in units of σ.) The kinetic energy stays at
exactly 187.5: the thermostat rescales the velocities after every step so that
the temperature is 1, and for 125 atoms that means
$\tfrac{3}{2} \times 125 \times 1 = 187.5$. Because the thermostat adds and
removes energy, the total energy is not constant in these runs.

**The trajectory** records every atom at each saved step, and it is what the
movie plays. It is in **XYZ** format, plain text that viewers such as VMD read
directly. Each frame is a block of lines: the number of atoms, a comment, then
one line per atom:

```text
125
Step 0, PE: -611.552758, KE: 187.500000
   LJ    3.264187    3.197090    3.263010    0.589169   -0.455788    1.221319
```

On each atom line come the atom type, its position $x, y, z$, and its velocity
$v_x, v_y, v_z$. In the movie, an atom that leaves through a face comes back
through the opposite one, as the
[periodic box](#what-the-simulation-holds) rule says.

## The energy function

The **energy function** takes the positions of all atoms and returns two
things: the total potential energy $U$, and the force on every atom,
$\mathbf{F}_i = -\partial U / \partial \mathbf{r}_i$ as defined at the top of
the page.

The energy function used throughout these pages is the **Lennard-Jones (LJ)
potential**. It gives the energy of one pair of atoms at distance $r$:

$$
U(r) = 4\varepsilon \left[ \left(\frac{\sigma}{r}\right)^{12}
                          - \left(\frac{\sigma}{r}\right)^{6} \right]
$$

The first term pushes atoms apart when they overlap; the second pulls them
together at longer range. The total energy is the sum of $U(r)$ over every
pair of atoms.

The plot below draws $U(r)$ for one pair, with the pair itself above it.
Each slider is one variable, explained in its own subsection below: σ, ε,
and the thermal energy $k_B T$. Point at the curve to read its value and
move the right-hand atom to that distance.

```{lj-explorer}
```

### σ: the size of an atom

Two atoms whose centres are σ apart just touch: the curve crosses zero at
$r = \sigma$. They rest a little farther apart, at
$2^{1/6}\sigma \approx 1.12\sigma$, the lowest point of the curve. The
atoms drawn above the plot have diameter σ: point at $r = \sigma$ and they
touch. Moving the σ slider grows the atoms and moves both distances
together. The wall at short distance is steep but not vertical, so the atoms
are soft spheres: a hard collision can push two of them slightly closer than
σ, at a high cost in energy (about 2ε at 0.95σ).

### ε: the barrier to pulling apart

Read the curve as a valley. A pair resting at the bottom has energy −ε. To
pull apart, the pair must climb back up to $U = 0$ on the far side of the
valley, where the atoms are far enough apart to stop feeling each other: an
**energy barrier** of height ε.

The energy for the climb comes from motion. As two atoms move apart, the
attraction slows them down, and their energy of motion (kinetic energy)
turns into potential energy, like a ball rolling up a slope; the total stays
the same. In the plot the ball rides on the curve at the pair's potential
energy, and the dashed line is the pair's total energy: the gap between them
is the energy of motion. If the dashed line is below zero, the ball stops
partway up and rolls back, and the two atoms vibrate but stay together. If
it is above zero, the ball rolls out of the valley and the atoms fly apart.

### $k_B T$: the thermal energy

How much energy of motion a pair has depends on the temperature. In the
simulation, temperature is the average kinetic energy of the atoms: each
atom carries on average $\tfrac{3}{2} k_B T$ of it, where $k_B$ is
Boltzmann's constant (the engine computes the temperature exactly this way,
from the total kinetic energy). The product $k_B T$ is the **thermal
energy**, and it is the yardstick to hold against ε. In reduced units
$k_B = 1$, so the thermal energy is simply the temperature: T = 1.0 means
$k_B T = \varepsilon$.

The thermal-energy slider sets the pair's energy to $k_B T$ above the bottom
of the well: it moves the dashed line. Below ε the pair stays together;
above ε it breaks apart.

A real pair does not keep a fixed energy. Collisions with other atoms keep
trading energy, so at any moment some pairs have more than the average and
some less. When $k_B T$ is well below ε, a pair rarely gets enough to cross
the barrier; when it is near or above ε, pairs break apart often. The share
of pairs with enough energy falls roughly as $e^{-\varepsilon / k_B T}$, so
what decides the behaviour is the ratio $k_B T / \varepsilon$, not either
number alone. That is why reduced temperature is measured in units of ε.

Because the energy is close to zero far away, the simulation skips every pair
farther apart than a **cutoff**, $r_c = 2.5\sigma$ here. Beyond the cutoff
the energy is set to zero; the curve is not shifted, so it jumps from about
$-0.016\varepsilon$ to zero at $r_c$.

## What ε and σ do to many atoms

The sliders showed ε and σ for two atoms. With many atoms their effect shows
directly. {numref}`run-sigma-epsilon-figure` shows four runs,
each with 512 atoms in the same box (density 0.3 for σ = 1), at T = 1, for
10,000 steps of 0.004, changing only ε or σ:

- **ε = 0.5 and ε = 2.** The ratio $k_B T / \varepsilon$ is 2 and 0.5. With
  the weaker attraction, pairs break apart as fast as they form and the atoms
  spread through the box: each atom has on average 4.1 others within 1.5σ.
  With the stronger attraction, the atoms pull together into a dense region
  and leave empty space beside it: 10.1 neighbours within 1.5σ.
- **σ = 0.8 and σ = 1.4.** The same number of atoms fills 8% of the box
  volume with the small atoms and 43% with the large ones.

```{figure} ../figures/run_sigma_epsilon_runs.svg
:name: run-sigma-epsilon-figure

Four runs of the engine, after 10,000 steps. Each panel is a slice 3σ thick
seen from above, with every atom drawn to scale (diameter σ). The left two
differ only in ε (σ = 1), the right two only in σ (ε = 0.5).
```

Each of those runs is the same two files with different settings, and you can
run your own in the boxes under [Starting a run](#starting-a-run). The engine
reads the files every time it starts, so there is nothing to rebuild. Keep
`random_seed` the same when you compare two runs, so that your change is the
only difference between them.

A sparse, cold box is a good first try: `density: 0.1`, the first `temperature`
at 0.6, and `total_steps: 6000` added under `simulation`. In the movie the atoms
no longer fill the box; they gather into clumps with empty space between them.

## Your own slider

The interactive plot above has sliders for ε, σ and $k_B T$. A loop over one setting is the same idea,
and you can build it yourself: every stop along the way is a real simulation.
`sweep.py` runs the engine once per value and plots the average potential energy
per atom. Press **Run**, then change `SETTING` and `VALUES` at the top and press
it again. The first Run also downloads a plotting library.

```{engine-python}
```

Sweeps of `temperature` and `epsilon` give two different-looking curves, but
they are one curve. {numref}`run-collapse-figure` shows four sweeps of 512 atoms
for 6000 steps, with the energy measured in units of ε. On the left, raising $T$
and lowering ε give the same energy at the same ratio $k_B T / \varepsilon$, to
within 0.02ε. On the right, a denser box and larger atoms give the same energy
at the same $\rho \sigma^3$, to within 0.05ε, with the cutoff kept at 2.5σ.

```{figure} ../figures/run_collapse.svg
:name: run-collapse-figure

Potential energy per atom, in units of ε, from four sweeps of the engine. Left:
at density 0.8, changing $T$ or ε. Right: at $T = 1$, changing the density or σ.
Each pair of sweeps falls on one curve.
```

So four settings are really two numbers: how hot the atoms are compared with
how strongly they attract, and how tightly they are packed compared with their
size. That is why reduced units lose nothing: a run at ε = 1 and σ = 1 stands
for every LJ fluid with the same two numbers.

## From a grid to an equilibrium liquid

A longer run of 512 atoms ends as a liquid at **equilibrium**: a
state whose averages no longer change, only fluctuate. Two measurements from
that run, 4000 steps long, check both halves of the claim.

**The potential energy settles.** {numref}`run-energy-figure` shows the
potential energy per atom during the run. On the grid it is −4.91ε. As the
atoms start moving it first rises, to −4.30ε at step 80, then falls and
settles. After t = 1 it fluctuates around −5.10ε, by about 0.04ε, with no
trend. The first unit of time is the run forgetting its starting grid;
averages are taken only after it.

```{figure} ../figures/run_energy.svg
:name: run-energy-figure
:width: 600px

Potential energy per atom in the 512-atom run. The shaded part is the
settling; afterwards the energy fluctuates around a steady average (dashed).
```

**The atoms keep moving.** In a solid, each atom vibrates around a fixed
place; in a liquid, atoms wander away for good. The **mean-square
displacement** (MSD) tells the two apart: the average over all atoms of the
squared distance each has travelled since the start. In a solid it levels
off at a small value. In {numref}`run-msd-figure` it grows in a straight
line, by 0.40σ² per unit of time. At t = 4 it is 1.65σ²: a typical atom has
travelled 1.29σ, farther than the 1.08σ between neighbours on the starting
grid.

```{figure} ../figures/run_msd.svg
:name: run-msd-figure
:width: 600px

Mean-square displacement in the 512-atom run. It keeps rising in a straight
line, past the squared spacing between neighbours (dashed).
```

Atoms that cross a face of the box are followed through it, so this is the
distance really travelled.

This agrees with the published phase diagram of the LJ fluid. T = 1.0 lies
between the triple point, about 0.68
([Khrapak and Morfill](https://ar5iv.labs.arxiv.org/html/1104.2754)), and
the critical point, 1.19 for this cutoff
([Wilding 1995](https://arxiv.org/abs/cond-mat/9503145)), so a liquid can
exist there; and density 0.8 is below the density at which the liquid
freezes at this temperature, about 0.91 (from the freezing line in the same
Khrapak and Morfill paper). The starting grid was never a crystal of LJ
atoms, only a convenient way to place them without overlaps.

## One material, two states

Reduced units turn back into a real material once you choose its ε and σ. Argon
is the classic choice: [Rahman's 1964 simulation](https://doi.org/10.1103/PhysRev.136.A405)
of liquid argon used $\varepsilon / k_B$ = 120 K and σ = 3.4 Å. With those values
$T = 1$ means 120 K, and density 0.82 means 1.38 g/cm³, close to
[real liquid argon at its boiling point](https://lar.bnl.gov/properties/index.html),
1.396 g/cm³. His run was 864 atoms at 94.4 K and 1.374 g/cm³.

Each button below runs the engine on the same 125 atoms (ε = σ = 1, 2500 steps),
changing only the conditions: $T$ and the density. The material stays the
same; the box shrinks or grows to give the density.

```{engine-states}
```

| Button | $T$ | Density | As argon | Neighbours within 1.5σ | MSD over t = 4 |
|---|---|---|---|---|---|
| Liquid argon at 94.4 K | 0.79 | 0.81 | 94.4 K (−179 °C) | 11.5 | 1.1σ² |
| Compressed gas | 2.50 | 0.10 | 300 K (27 °C) | 1.3 | 51σ² |

**Liquid argon at 94.4 K** uses Rahman's temperature and density, his cutoff of 2.25σ and his
steps of 10⁻¹⁴ s, with 125 atoms where he used 864. It is not an exact repeat:
this engine holds the temperature with a thermostat and integrates with velocity
Verlet, where Rahman let his system run freely and used a predictor-corrector.
Over six seeds it still gives neighbours at 3.6 to 3.7 Å and a diffusion constant
of about 2.4 × 10⁻⁵ cm²/s, against his 3.7 Å and 2.43 × 10⁻⁵ cm²/s, so the smaller
box loses little. Rahman reports 45 seconds per step on his CDC 3600; 864 atoms take
under 10 milliseconds per step here.

The table averages three runs with different seeds, and takes the MSD from the
second half of each run, without any drift of the whole box.

At $T = 0.79$, between the triple and critical points, the atoms keep moving past
each other: an atom keeps about half of its neighbours over $t = 4$. The second run
is above the critical point, where liquid and gas are no longer distinct, and at
this low density the atoms behave as a gas: an atom has only about one close
neighbour.

There is no solid button. Lennard-Jones atoms do form a stable crystal below the
triple point, but near a density of 0.96, and this engine starts from a simple
cubic grid at a lower density. Cooled to $T = 0.3$ at density 0.82, those atoms
jam into a packing that is barely more ordered than the liquid, so calling it a
solid would claim more than the run shows.

The density is fixed, so the liquid
at 94.4 K is under pressure: at atmospheric pressure argon boils at 87.3 K. The gas
run is 0.17 g/cm³, which the ideal gas law puts at roughly 100 times atmospheric
pressure, as in a gas cylinder.

Other materials are other values of ε and σ. Attraction between krypton atoms is
about five times that between neon atoms, and krypton boils at −153 °C where neon
boils at −246 °C.

## The energy function is a plug

The loop in {numref}`md-loop-figure` needs only two things from the energy
function: the energy, and the force on every atom, both computed from the
positions. Nothing else in the loop depends on how the energy function gets
them. The integrator moves atoms by their forces whether the forces came from
a two-line formula or from a quantum calculation.

So the energy function is a **plug**. Any energy function that answers those
two questions fits the same place:

- **LJ.** A formula for each pair of atoms; fast, and a good model for simple
  liquids such as argon.
- **Density functional theory (DFT).** Computes the energy from the electrons.
  Far more accurate for chemistry, and far slower.
- **Machine-learned potentials.** Fitted to DFT results, to get close to DFT
  accuracy at a fraction of the cost.

The force step can also hold several plugs at once and add their forces
together, for example one energy function for bonds and another for the
attraction between molecules.

### Choosing the plug

The simulation file names the file that holds the energy function:

```yaml
potential_config_files:
  - "potentials/lj_bulk.yaml"
```

and that file says which energy function to build, with its parameters:

```yaml
components:
  - type: "lennard_jones"
    cutoff: 2.5
    atom_parameters:
      "LJ": { epsilon: 1.0, sigma: 1.0 }
```

Pointing `potential_config_files` at a different file swaps the plug. The
rest of the loop is unchanged.

## Where MolSimWorkbench does it

Each part of {numref}`md-loop-figure` is one C++ class in the engine:

| Part of the loop | Class |
| --- | --- |
| System (positions, box) | `Geometry` |
| Force step | `ForceCalculator`, which holds a list of energy functions and adds up their forces |
| Energy function (the plug) | `Potential`; the LJ plug is `LJPotential`, the DFT plug is `DFTPotential` |
| Integrator and thermostat | `MolecularDynamics`, in `integrateVelocityVerlet` and `applyTemperatureControl` |

`Potential` is the shape of the socket. It declares two functions,
`calculateEnergy` and `calculateForces`, and every plug provides its own
version of both. How C++ expresses "every plug provides its own version" is
the subject of the C++ module.

## Check yourself

:::{dropdown} 1. An atom sits 0.1σ inside the left face of the box, and another sits 0.1σ inside the right face. The box is 8.6σ wide. How far apart are they, for the force calculation?
0.2σ. Through the nearest copy, the pair is only 0.1σ + 0.1σ apart; the
minimum image rule uses that distance, not the 8.4σ straight across the box.
:::

:::{dropdown} 2. Two atoms are 3σ apart. What LJ energy does the simulation give the pair?
Zero. 3σ is beyond the cutoff of 2.5σ, so the pair is skipped. (The formula
itself would give about $-0.005\varepsilon$.)
:::

:::{dropdown} 3. You replace LJ with a machine-learned energy function. Which parts of the loop in the figure have to change?
Only the plug. The force step still asks for energy and forces, and the
integrator and thermostat still work on positions, velocities and forces.
:::

:::{dropdown} 4. You set num_particles to 1000 and keep density 0.8. How long is the side of the box?
$L = (1000 / 0.8)^{1/3} = 10.77$, in units of σ.
:::

:::{dropdown} 5. A run has 512 atoms at T = 1. What kinetic energy does it print on every line?
768: the thermostat holds the kinetic energy at
$\tfrac{3}{2} N k_B T = \tfrac{3}{2} \times 512 \times 1$.
:::

:::{dropdown} 6. You press Run twice with random_seed: 42. Do the two runs differ? And with random_seed: 0?
With 42, no: the seed fixes every random number, so the runs are identical.
With 0, yes: the starting velocities are drawn afresh on each run.
:::

:::{dropdown} 7. Why are the averages of the 512-atom run taken only after t = 1?
Until then the run is still forgetting its starting grid: the potential
energy is still drifting ({numref}`run-energy-figure`). Averages over that
part would describe the grid, not the liquid.
:::

:::{dropdown} 8. At ε = 1 and T = 1 the energy per atom is about −5.1. What is it at ε = 2 and T = 2?
About −10.2. The ratio $k_B T / \varepsilon$ is still 1, so the energy in units
of ε is the same, −5.1, and one unit of ε is now 2.
:::

## Next

Everything on this page went through one call, `sim.run()`, and all the work of
that call happened inside the engine. Changing a setting only takes you so far:
to change what the engine does, or to write your own, you need the language it
is written in. The [C++ module](../cpp/index.md) starts from a ten-line program
that computes one LJ energy, the formula on this page, and adds a few lines per chapter until it reaches the classes the engine is built from.
