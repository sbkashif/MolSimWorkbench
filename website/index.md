# MolSimWorkbench

*Prepared as part of the [US-RSE Mentorship Program](https://us-rse.org/wg/mentorship-program),
with mentorship from [Chris Neely](https://github.com/cjneely10).*

MolSimWorkbench teaches programming through one running example: a molecular
dynamics simulation, which follows atoms as they move by Newton's laws. The
engine that runs it is written in C++, and Python drives it. The course
builds that engine up from its parts and shows the programming each part
needs.

Below is a real run. A five-line Python script reads a settings file and
runs the engine; the film is the result: 512 atoms start on a grid and settle
into a liquid at equilibrium.

```{image} figures/cover.webp
:alt: A five-line Python script and an excerpt of its YAML settings file; beside them, the trajectory it produced, in which 512 Lennard-Jones atoms start on a grid and settle into an equilibrium liquid.
:align: center
:target: _images/cover.webp
```

## Behind `sim.run()`

The last line of the script hands control to the C++ engine, which runs
**the loop** of every molecular dynamics simulation: one turn per time step,
thousands of turns per run.

```{figure} figures/home_loop.svg
:width: 100%

One turn of the loop. Each step names the engine function that does it.
```

Each call below is made by the one above it. Click a call to see its code in
[Reading the engine](engine/reading_the_engine.md); click a concept to go to
the page that teaches it.

::::{container} concept-stack
- [`sim.run()`](engine/reading_the_engine.md#putting-a-run-together)\
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
            {bdg-link-danger}`Parallel programming <parallel/index.html>`
::::

Alongside the engine, tests such as `TEST(LJKernels, NetForceIsZero)` check
that every change still gives the right forces.

## The course

The course has five modules, each built on the same simulation:

- **Molecular simulation.** How atoms in a box move: an energy function,
  forces, and a time step. The energy function is the part you choose; the
  course uses Lennard-Jones (LJ).
- **C++.** The language of the engine, from a first program to the classes
  the engine is made of.
- **pybind11.** How Python code calls the C++ engine.
- **Parallel programming.** Making the force loop, the slowest part, run on
  several cores.
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
:caption: The engine in code
:maxdepth: 1
:hidden:

engine/reading_the_engine
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
:caption: Parallel programming
:maxdepth: 1
:hidden:

parallel/index
parallel/01_measure_first
parallel/02_threads_and_races
parallel/03_openmp_and_speedup
```
