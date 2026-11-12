# MolSimWorkbench

*Prepared by [Salman Bin Kashif](https://sbkashif.github.io/) as part of the [US-RSE Mentorship Program](https://us-rse.org/wg/mentorship-program), with mentorship from [Chris Neely](https://github.com/cjneely10), and with the help of an AI assistant (see the [AI usage statement](#ai-usage-statement)).*

Molecular simulations have become a routine part of materials discovery
pipelines. While modern molecular simulation packages have a low barrier to
entry, realistic systems that give insights relevant to wet-lab work are hardly
ever simulated with default options and standard methods. Simulating a complex
system, or analyzing it, needs an understanding of what its critical parts do
and of the theory behind them. That level of understanding usually comes from
working the methods out on paper and in code. Additionally, analyzing
simulation data creatively needs more than beginner-level programming. But programming has a
problem of its own: it is hard to learn without something real to build. This
website teaches both at once.

:::::{admonition} Learning approach: read, play, take apart and code your own
:class: tip

::::{grid} 1 2 2 2
:gutter: 2

:::{grid-item-card} 1. Read
:link: "#getting-started"
:link-type: url

A brief text and figures, from the ground up.
:::

:::{grid-item-card} 2. Play
:link: "#what-to-look-for"
:link-type: url

Explore the settings in the built-in simulator.
:::

:::{grid-item-card} 3. Take apart
:link: "#behind-the-buttons"
:link-type: url

Read the code, one part at a time.
:::

:::{grid-item-card} 4. Code your own
:link: cpp/05_the_plug
:link-type: doc

Plug in your own energy function, integrator or thermostat.
:::
::::
:::::

The engine can also grow beyond these steps, with advanced sampling
methods, machine learning methods and automated workflows built on it. This release covers the
basic molecular dynamics loop, basic programming, parallelization (a serial
code is hardly practical for real systems) and an agentic AI workflow. I chose
C++ for the engine backend because I wanted to learn it myself; a simpler version in Python is coming
soon.

## Getting started

We start with one of the simplest materials to simulate: **argon**, the liquid
in [pioneering work by Aneesur Rahman in 1964](https://doi.org/10.1103/PhysRev.136.A405),
the first molecular dynamics simulation of a realistic liquid. Argon can be
modelled well with a simple equation called the Lennard-Jones potential, which
treats each atom as a soft sphere with two settings: how big it is and how
sticky it is.

A simulation of argon is a
{ref}`simple loop <behind-sim-run>`. It starts with a set of atoms at given
positions. From the positions, the Lennard-Jones potential gives the potential
energy. Differentiating the energy gives the force on each atom, and
integrating the forces over a short time step gives the new positions.
Repeating this many times gives a trajectory: how the atoms move over time.

The simulator below holds only 125 atoms, a nanoscale block of liquid argon;
a beaker of the real liquid holds about 10²⁴. Yet 125 atoms are enough: one
small region of a liquid looks much like any other, so the properties
calculated from how these atoms move are the same ones measured on real liquid
argon at the same temperature and density.

Press **Liquid argon at 94.4 K** to rerun Rahman's liquid. The atoms start from
a solid-like grid and settle into a liquid; the simulator then shows two
values, compared against those Rahman published.

```{engine-play}
```

## Reading the Simulator panel

When you run the simulator, it prints the energies of the configuration at
each step, and the Visualization panel shows the configuration itself. You can ignore the energies for now. What matters is what the
configurations add up to: a property of the material is an average over many
configurations, and the table at the end of the run reports two such
properties, one structural and one dynamic. Each row averages over a longer
stretch of the run than the one before, so the values settle as you read down:

- **Nearest neighbours:** the distance at which atoms most often sit next to each
  other.
- **Diffusion constant:** how quickly atoms wander away from where they were. The
  larger it is, the faster they spread.

Rahman checked both against experiment: the arrangement of neighbours with
X-ray scattering on liquid argon, and the diffusion with measured values. So
the table checks your run against his, and through him against real argon.

## What to look for

Now play with the parameter space: change the temperature and density, or
invent a material of your own by changing the size and stickiness of its atoms.
Try these six experiments in order.

### Change the temperature

**1. Heat a liquid.** Press **Liquid argon at 94.4 K**. Then set How hot to
300 K and press **Run custom**. Watch the Visualization panel, and read the last
row of the table in the Simulator panel. *Will the atoms spread apart as they
heat up?*

:::{dropdown} What happens
No. They stay just as crowded, but they move faster: the diffusion constant
rises about four times.

They cannot spread apart because the simulation keeps the number of atoms, the
size of the box and the temperature fixed, the conditions known as the NVT
ensemble. It is like heating liquid argon in a sealed, rigid container: the
atoms have nowhere to go, so the density stays the same however hot it gets.
In an open container they could leave, and argon would boil away at 87 K, long
before 300 K. A real sealed container would need to hold an enormous pressure
to keep argon this crowded at 300 K.
:::

### Change the density

**2. Give the atoms room.** Keep How hot at 300 K, set How crowded to
0.17 g/cm³ and run it. *What do the atoms do with the extra space?*

:::{dropdown} What happens
They spread out and become a gas. Liquid and gas are names for what atoms do,
so what you see is what tells them apart:

| Run | How hot | How crowded | What the atoms do | Diffusion constant (× 10⁻⁵ cm²/s) |
|---|---|---|---|---|
| Liquid argon at 94.4 K | 94.4 K | 1.374 g/cm³ | Stay packed together and jostle past each other | About 2 |
| Experiment 1 | 300 K | 1.36 g/cm³ | Stay packed together and move faster | About 10 |
| This run | 300 K | 0.17 g/cm³ | Stay far apart and fly across the box | About 90 |

In a liquid, the atoms push past each other, which is what lets a liquid flow.
In a gas, they rarely meet, which is why a gas spreads to fill its container
and can be squeezed.
:::

**3. Condense a gas.** With How crowded still at 0.17 g/cm³, set How hot to
96 K and run it, then again at 36 K. *Where do the atoms go as the gas cools?*

:::{dropdown} What happens
They stop filling the box evenly. By 36 K they have gathered into clumps with
empty space between them, and the diffusion constant has fallen from about
90 × 10⁻⁵ to about 5 × 10⁻⁵ cm²/s, because an atom in a clump cannot wander
far. The attraction between the atoms was always there; the hot atoms simply
moved too fast to stay together. A real gas starts to condense the same way.
:::

### Change the material

**4. Change the stickiness.** With How crowded still at 0.17 g/cm³, set How
hot to 120 K. Run it with How sticky at 60 K, then at 360 K. *At the same
temperature, which material is a gas?*

:::{dropdown} What happens
The less sticky one. At 60 K the atoms spread through the box; at 360 K they
clump, and the diffusion constant is about six times smaller. Stickier atoms
need more heat to pull apart, so the material boils at a higher temperature.
Real materials show the same thing: the attraction between krypton atoms is
about five times that between neon atoms, and krypton boils at −153 °C, where
neon boils at −246 °C.
:::

**5. Heat against stickiness.** Press **Reset**, then **Run custom**. Now set
How hot to 240 K and How sticky to 240 K, and run again. *This material is
hotter, but also stickier. Which effect wins?*

:::{dropdown} What happens
Neither. The atoms make the same pattern in both runs, and only move a little
faster in the second. What decides the state is the heat compared with the
stickiness, not either one alone.
:::

**6. Size against space.** Press **Reset**, set How big to 4.76 Å and run it.
Press **Reset** again, set How crowded to 1.38 g/cm³ and run it. *Bigger atoms,
or the same atoms in a smaller box: which is more crowded?*

:::{dropdown} What happens
They come out the same: a crowded liquid, with a diffusion constant of about 3
to 4 × 10⁻⁵ cm²/s in both. The box looks smaller in the second run, but the
atoms fill the same share of it. What decides the state is the size of the
atoms compared with the space they have.
:::

So two contests decide what a material does: heat against stickiness, and
atom size against space. Change either balance, and you change whether it is
a liquid or a gas.

## Behind the buttons

Each run writes your slider settings into the config files as config values,
with some unit conversions, then runs this short Python script.

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

(behind-sim-run)=
## Behind `sim.run()`

The last line of the script hands control to the C++ engine, which runs **the
loop** of every molecular dynamics simulation: one turn per time step, thousands
of turns per run. Here is that loop twice, as physics and as the code that does
it. Each call leads to the one below it (small helpers in between are left
out), and the highlighted part is the loop;
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
  - [`MolecularDynamics::​runSimulation()`](engine/reading_the_engine.md#the-loop-moleculardynamics)\
    One pass of the simulation loop per time step.
    - [`VelocityVerletIntegrator::​step()`](engine/reading_the_engine.md#the-loop-moleculardynamics)\
      Moves every atom by its velocity and force.
      - [`ForceCalculator::​calculateForcesAndEnergy()`](engine/reading_the_engine.md#the-force-step-forcecalculator)\
        Adds up the forces of every
        energy function.
        - [`LJPotential::​calculateForcesAndEnergy()`](engine/reading_the_engine.md#the-socket-potential)\
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

## History

- **November 2026:** A new section,
  [agentic AI in a molecular simulation workflow](agent/01_training_an_mlip.md),
  hands the decisions in training a machine learning interatomic potential to
  a language model. For now it runs its simulations in OpenMM, until our
  simulator supports MLIPs.
- **October 2026:** Turned into a website, with a built-in simulator to explore
  the parameter space and detailed lessons on molecular simulation and
  programming.
- **September 2025:** Grown into a general molecular simulation workflow, with
  a Python interface and config files. It is built so that any part of a
  molecular simulation, such as the energy function, the integrator or the
  thermostat, can be swapped for a new one, so that a new concept can be
  learned by adding it.
- **April 2025:** Rewritten in C++ with AI agents, as an application for
  learning basic C++
  ([repository](https://github.com/sbkashif/lennard-jones-cpp)).
- **March 2018:** The code started as a class project, a Lennard-Jones simulation
  in Python for a course on multiscale modelling at Clemson University, taught
  by [Sapna Sarupria](https://www.linkedin.com/in/sarupria/)
  ([repository](https://github.com/sbkashif/Lennard-Jones)).

(ai-usage-statement)=
:::{admonition} AI usage statement
:class: tip

The simulation engine powering the interactive sliders and examples on this
website was built by repurposing my own Python code from a class project. Turning that code into the engine, and then
into this interactive website, relied heavily on an AI assistant, Claude (by
Anthropic). However, both the code and the pages were revised substantially
after the AI's first drafts, as the commit history shows.

My aim in building this simulator and website is to learn new skills from the
ground up, by building applications related to my field. My own research
projects have had little room for new and advanced concepts, such as agentic
workflows, given the time a rigorous study needs. So I built this
*workbench* to learn such concepts hands-on, and used AI heavily to reduce the
logistical friction: first to build working end-to-end applications, then to
learn the basics interactively, and to write the webpages. I read the code
and the pages, had the assistant quiz me on them, and revised the pages
several times until they matched my own understanding, presentation style and
language. What you read here is my own understanding, written down with AI
help.

While I have revised much of the content, I am still far from vetting every
line, so many sections are marked *Draft*. The pages may still contain
mistakes. If you find one, please open an issue on the
[repository](https://github.com/sbkashif/MolSimWorkbench).
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
pybind11/03_in_your_browser
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

```{toctree}
:caption: Agentic AI in a molecular simulation workflow
:maxdepth: 1
:hidden:

agent/01_training_an_mlip
agent/02_the_agent
agent/03_first_run
agent/04_the_code
```
