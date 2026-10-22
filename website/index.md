# MolSimWorkbench

*Prepared as part of the [US-RSE Mentorship Program](https://us-rse.org/wg/mentorship-program), with mentorship from [Chris Neely](https://github.com/cjneely10).*

Programming tutorials are hard to follow without using them to build an application, and
simulation theory is hard to grasp without writing the code. This website is developed to learn the two together, on one running example: a molecular dynamics
simulation of Lennard-Jones atoms.

::::{grid} 1 1 2 2
:gutter: 3

:::{grid-item-card} Basic simulation algorithm, ready for C++
:link: molsim/architecture
:link-type: doc

What the simulation holds, the loop each time step runs, and the
Lennard-Jones energy function, laid out step by step so each one can
become C++ later.
:::

:::{grid-item-card} Learn C++ programming, through a molecular simulation app
:link: cpp/01_first_program
:link-type: doc

C++ from a first program to loops on many cores, each idea arriving where
the simulation needs it.
:::
::::

These pages grew out of my 2018 course project for ChE 8450, Multiscale
Modelling, at Clemson University, a Lennard-Jones simulator written in
Python ([Lennard-Jones](https://github.com/sbkashif/Lennard-Jones)). For now
they stay with Lennard-Jones; I will add more complicated energy functions as I work them out by hand.

Below is a real run: 512 atoms start on a grid and settle into a liquid.
Python is the engine's easy interface; the five lines of Python on the left
are all it takes to start the run, and these pages open up everything they
set in motion.

```{image} figures/cover.webp
:alt: A five-line Python script and an excerpt of its YAML settings file, which names the Lennard-Jones potential file; beside them, the trajectory it produced, in which 512 Lennard-Jones atoms start on a grid and settle into an equilibrium liquid.
:align: center
:target: _images/cover.webp
```

## Behind `sim.run()`

The last line of the script hands control to the C++ engine, which runs
**the loop** of every molecular dynamics simulation: one turn per time step,
thousands of turns per run.

Each call below is made by the one above it, and the pages follow them in
the same order, from the top down. Click a call to see its code; click a
concept to go to the page that explains it.

::::{container} concept-stack
- [`sim.run()`](pybind11/02_binding_the_engine.md#following-simrun)\
  Python calls into C++ through the compiled module `_molsim`.
  {bdg-link-warning}`pybind11 <pybind11/02_binding_the_engine.html>`
  - [`MolecularDynamics::runSimulation()`](engine/reading_the_engine.md#the-loop-moleculardynamics)\
    One pass of the loop per time step.
    {bdg-link-primary}`C++ <cpp/04_classes.html>`
    - [`integrateVelocityVerlet()`](engine/reading_the_engine.md#the-loop-moleculardynamics)\
      Moves every atom by its velocity and force.
      {bdg-link-info}`Molecular simulation <molsim/architecture.html#the-loop>`
      - [`ForceCalculator::calculateForces()`](engine/reading_the_engine.md#the-force-step-forcecalculator)\
        Adds up the forces of every
        energy function. {bdg-link-primary}`C++ <cpp/05_the_plug.html>`
        - [`LJPotential::calculateForces()`](engine/reading_the_engine.md#the-socket-potential)\
          The energy function, here
          Lennard-Jones.
          {bdg-link-info}`Molecular simulation <molsim/architecture.html#the-energy-function>`
          - [`ljComputeGather(view, backend, numThreads)`](engine/reading_the_engine.md#the-pair-loop)\
            The loop over every
            pair of atoms, serial or on several threads.
            {bdg-link-danger}`Parallel programming: CPU cores <parallel/index.html>`
::::

## What is here

The walkthrough has six modules, each built on the same simulation, and one page
between them that reads the engine itself:

- **Molecular simulation.** How atoms in a box move: an energy function,
  forces, and a time step. The energy function is the part you choose; these
  pages use Lennard-Jones (LJ).
- **C++.** The language of the engine, from a first program to the classes
  the engine is made of.
- **pybind11.** How Python code calls the C++ engine.
- **Reading the engine** (one page). The real classes, in the order a run
  uses them.
- **Parallel programming: CPU cores.** Making the force loop, the slowest
  part, run on several cores.
- **GPU programming** (being written). Running the same loop on a graphics
  card, on thousands of threads.
- **Testing** (coming later). Checking that every change still gives the right
  forces.

```{toctree}
:caption: Molecular simulation
:maxdepth: 1
:hidden:

molsim/architecture
molsim/running
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
