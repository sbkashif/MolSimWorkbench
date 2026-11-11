# MolSimWorkbench

*Prepared by [Salman Bin Kashif](https://sbkashif.github.io/) as part of the [US-RSE Mentorship Program](https://us-rse.org/wg/mentorship-program), with mentorship from [Chris Neely](https://github.com/cjneely10).*

Simulation theory is hard to grasp without writing the code, and programming
tutorials are hard to follow without building an application with them.
This website teaches the two together by programming a basic molecular dynamics
simulator.

We start with one of the simplest materials to simulate: **argon**, the liquid in
Aneesur Rahman's pioneering 1964 simulation. It can be modeled effectively with a
simple equation called the Lennard-Jones potential, which treats each atom as a hard
ball with two settings: how big it is and how sticky it is.

The simulator below starts from a solid-like grid of 125 argon atoms, which settles
into a liquid as the simulation equilibrates. It then shows two properties of that
liquid beside the values Rahman published. After that you can change the temperature
and density, or try your own hypothetical material, described by the same equation,
by changing the properties of the atoms.

```{engine-play}
```

## Reading the Simulator panel

The lines that scroll past are the engine reporting on each step of the run. You can
ignore the energies for now: the next page explains them. A table appears when the
run ends, showing two measurements. They wobble while the run is short, then stay
close to one value:

- **Nearest neighbours:** the distance at which atoms most often sit next to each
  other.
- **Diffusion constant:** how quickly atoms wander away from where they were. The
  larger it is, the faster they spread.

## What to look for

Try these six experiments in order.

### Change the temperature

**1. Heat a liquid.** Press **Liquid argon at 94.4 K**. Then set How hot to 300 K and
press **Run custom**. Watch the Visualization panel, and read the last row of the
table in the Simulator panel. *Will the atoms spread apart as they heat up?*

:::{dropdown} What happens
They stay just as crowded, because the box does not change. What changes is the
motion: the atoms move faster, and the diffusion constant rises about four times. In an open container argon would boil away long before 300 K; here the
box keeps it crowded.
:::

### Change the density

**2. Give the atoms room.** Keep How hot at 300 K, set How crowded to 0.17 g/cm³ and
run it. *What do the atoms do with the extra space?*

:::{dropdown} What happens
They spread out and become a gas. Liquid and gas are names for what atoms do, so
what you see is what tells them apart:

| Run | How hot | How crowded | What the atoms do | Diffusion constant (× 10⁻⁵ cm²/s) |
|---|---|---|---|---|
| Liquid argon at 94.4 K | 94.4 K | 1.374 g/cm³ | Stay packed together and jostle past each other | About 2 |
| Experiment 1 | 300 K | 1.36 g/cm³ | Stay packed together and move faster | About 10 |
| This run | 300 K | 0.17 g/cm³ | Stay far apart and fly across the box | About 90 |

In the liquid, the atoms push past each other, which is what lets a liquid flow. At
−179 °C argon stays liquid only under pressure. In the gas, the atoms rarely meet,
which is why a gas spreads to fill its container and can be squeezed.
:::

**3. Condense a gas.** With How crowded still at 0.17 g/cm³, set How hot to 96 K and
run it, and again at 36 K. *Where do the atoms go as the gas cools?*

:::{dropdown} What happens
They stop filling the box evenly. By 36 K the atoms have gathered into clumps with
empty space between them, and the diffusion constant has fallen from about 90 × 10⁻⁵ to about
5 × 10⁻⁵ cm²/s, because an atom in a clump cannot wander far. The attraction was always there, but the hot atoms moved too
fast to stay together. A real gas starts to condense the same way.
:::

### Change the material

**4. Change the stickiness.** With How crowded still at 0.17 g/cm³, set How hot to 120 K.
Run it with How sticky at 60 K, then at 360 K. *At the same temperature, which
material is a gas?*

:::{dropdown} What happens
The less sticky one. At 60 K the atoms spread through the box; at 360 K they
clump, and the diffusion constant is about six times smaller. Stickier atoms need
more heat to pull apart, so the material boils at a higher temperature. Real materials show the same
thing: attraction between krypton atoms is about five times that between neon
atoms, and krypton boils at −153 °C where neon boils at −246 °C.
:::

**5. Heat against stickiness.** Press **Reset**, then **Run custom**. Now set How hot
to 240 K and How sticky to 240 K and run again. *This material is hotter, but also
stickier. Which effect wins?*

:::{dropdown} What happens
Neither. The atoms make the same pattern in both runs, and only move a little faster
in the second. The state depends on heat compared with stickiness, not on either one
alone.
:::

**6. Size against space.** Press **Reset**, set How big to 4.76 Å and run it. Press
**Reset** again, set How crowded to 1.38 g/cm³ and run it. *Bigger atoms, or the same atoms
in a smaller box: which is more crowded?*

:::{dropdown} What happens
They come out the same: a crowded liquid, with a diffusion constant of about 3 to
4 × 10⁻⁵ cm²/s in both. The box looks smaller in the second run, but the atoms fill the
same share of it. The state depends on atom size compared with the space available.
:::

So two contests decide what a material does: heat against stickiness, and atom size
against space. Change those, and you change whether it is a liquid or a gas.

## Behind the buttons

Each run writes your settings into the settings files, then runs this short
Python script. The sliders use real units, with stickiness written as a temperature.
The engine works in reduced units, so each setting goes in divided by argon's value:
96 K becomes `temperature: 0.8`, which is the number you see in the Simulator panel.

```python
from molsimworkbench import System

sim = System.from_yaml("data/lj_play_simulation.yaml",
                       trajectory_output="traj.xyz")
sim.run()
```

`System.from_yaml` reads the settings and builds the atoms in their box.
`sim.run()` then runs the simulation: the engine prints the lines you saw in the
Simulator panel and saves the atom positions to `traj.xyz`, which the
Visualization panel plays back. This page then builds the table at the end of the
Simulator panel from `traj.xyz`: the nearest-neighbour distance from how often pairs
of atoms sit at each distance, and the diffusion constant from how fast the atoms
spread from where they were.

## Behind `sim.run()`

The last line of the script hands control to the C++ engine, which runs **the
loop** of every molecular dynamics simulation: one turn per time step, thousands
of turns per run. Here is that loop twice, as physics and as the code that does
it. Each call is made by the one above it, and the highlighted part is the loop;
click a call to see its code.

:::::{grid} 1 1 2 2
:gutter: 3

::::{grid-item}
:class: home-half

**Physics**

```{image} figures/loop_tall.svg
:alt: The simulation loop. The system goes to the force step, which asks the energy function for the force on every atom; the integrator moves the atoms and the thermostat holds the temperature, then the loop starts again. A small curve beside the force step shows the energy function, with faint curves behind it for the others that could be swapped in.
```
::::

::::{grid-item}
:class: home-half

**Programming**

:::{container} concept-stack
- [`sim.run()`](pybind11/02_binding_the_engine.md#following-simrun)\
  Python calls into C++ through the compiled module
  [`_molsim`](pybind11/02_binding_the_engine.md#the-bridge-molsim_bindingscpp).
  - [`MolecularDynamics::runSimulation()`](engine/reading_the_engine.md#the-loop-moleculardynamics)\
    One pass of the simulation loop per time step.
    - [`integrateVelocityVerlet()`](engine/reading_the_engine.md#the-loop-moleculardynamics)\
      Moves every atom by its velocity and force.
      - [`ForceCalculator::calculateForces()`](engine/reading_the_engine.md#the-force-step-forcecalculator)\
        Adds up the forces of every
        energy function.
        - [`LJPotential::calculateForces()`](engine/reading_the_engine.md#the-socket-potential)\
          The energy function, here
          Lennard-Jones.
          - [`ljComputeGather(view, backend, numThreads)`](engine/reading_the_engine.md#the-pair-loop)\
            The loop over every
            pair of atoms, serial or on several threads.
:::
::::

:::::

:::{container} concept-link
Each part of the loop gets its own explanation on the
{bdg-link-info}`simple molecular dynamics loop <molsim/architecture.html>` page,
with the equation and the code behind it. Then you can code your own: a different
system, a different integrator, a different energy function. For that, learn
{bdg-link-primary}`C++ programming <cpp/index.html>`.
:::

```{toctree}
:caption: Molecular simulation
:maxdepth: 1
:hidden:

molsim/architecture
```

```{toctree}
:caption: C++
:maxdepth: 1
:hidden:

cpp/index
cpp/01_first_program
cpp/02_functions
cpp/03_vectors_and_loops
cpp/04_classes
cpp/05_the_plug
cpp/06_several_files
```

```{toctree}
:caption: pybind11
:maxdepth: 1
:hidden:

pybind11/index
pybind11/01_a_first_binding
pybind11/02_binding_the_engine
```

```{toctree}
:caption: The engine in code
:maxdepth: 1
:hidden:

engine/reading_the_engine
```

```{toctree}
:caption: Parallel programming: CPU cores
:maxdepth: 1
:hidden:

parallel/index
parallel/01_measure_first
parallel/02_threads_and_races
parallel/03_openmp_and_speedup
```

```{toctree}
:caption: GPU programming
:maxdepth: 1
:hidden:

gpu/index
```
