# A minimal molecular simulation algorithm

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

This page describes the parts of that loop and how they connect. It uses no
code. The next page runs the loop with MolSimWorkbench from Python; the C++
module after it walks through the language the engine is written in.

## What the simulation holds

The simulation holds one **state**: the positions and velocities of every
atom, and the box that contains them. Everything else in the loop reads this
state or updates it.

The run on the home page has 512 atoms in a cube. The numbers are in
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
in reduced units for the home-page run), so 4000 steps cover 4 time units.

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

A run is described by a configuration file in YAML, a plain-text format of
`key: value` lines where indentation shows nesting. The simulation file names
the file that holds the energy function:

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

## Next

The loop is now a figure and a table of classes, but nothing has moved yet.
[Running the minimal simulation algorithm](running.md) puts it to work: three configuration files, one Python
call, and a check that the atoms on the home page really end as a liquid.
