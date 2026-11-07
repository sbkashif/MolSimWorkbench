# Reading the engine

[Binding the engine](../pybind11/02_binding_the_engine.md) followed
`sim.run()` as far as the bridge: Python builds the engine's objects, then
makes one call, `runSimulation`, and the whole run happens in C++. This page
continues on the C++ side. It walks the real engine in the order a run uses
it, one class at a time, matching each class to its part of the loop in
[A simple molecular dynamics loop](../molsim/architecture.md), and
names the C++ idea each excerpt relies on, with a link to the chapter that
explains it.

The excerpts are copied from the source files when this site is built, so
they always match the code.

## The classes and who holds whom

Four classes do the work ({numref}`engine-classes-figure`). A
`MolecularDynamics` object runs the loop. It keeps its own copy of the
`Geometry` (the atoms and the box) and shares a `ForceCalculator` with
whoever created it. The `ForceCalculator` keeps a list of energy functions,
each one a `Potential`; `LJPotential` and `DFTPotential` are two kinds of
`Potential`.

```{figure} ../figures/engine_classes.svg
:name: engine-classes-figure

The classes of a running simulation. Arrows between boxes show what one
object holds; "is a" marks the two energy functions that fit the `Potential`
socket (dashed).
```

## Putting a run together

The Python method `System.from_yaml`, read in
[Binding the engine](../pybind11/02_binding_the_engine.md#following-simrun),
is the clearest map of how the pieces connect. In order: the parser reads
the simulation file; in
`lj_particle_box` mode `LJParticleBoxBuilder.createCubicLattice` builds the
`Geometry`; each potential file becomes a `Potential`, added to a
`ForceCalculator`; and a `MolecularDynamics` object receives the geometry,
the force calculator, the parameters and the trajectory file. The command
line program `run_simulation` does the same steps in C++, in the `main`
function of `src/tools/run_simulation.cpp`. The sections below take these
objects one at a time, starting with the atoms.

## The system: Geometry

`Geometry` holds the state of the
[architecture page](../molsim/architecture.md#what-the-simulation-holds)
except the velocities: the type and position of every atom, and the box.

```{literalinclude} ../../src/builder/geometry.h
:language: cpp
:start-at: class Geometry {
:end-before: "#endif // GEOMETRY_H"
```

- A **class** groups data with the functions that work on it; the data is
  `private`, so other code reaches it only through the `public` functions
  ([Classes](../cpp/04_classes.md)).
- Positions are a `std::vector` of `std::array<double, 3>`: a list with one
  entry per atom, each entry three numbers $x, y, z$
  ([Vectors and loops](../cpp/03_vectors_and_loops.md)).
- `getPositions` returns `const std::vector<...>&`: a read-only reference to
  the stored list, so the caller sees the positions without copying them.
  [Functions](../cpp/02_functions.md#what-a-call-does-in-memory) showed that
  an ordinary argument or result is copied; a reference is how C++ avoids
  that copy.

The atoms are placed by `LJParticleBoxBuilder::createCubicLattice`, in
`src/builder/lj_particle_box_builder.cpp`. Its heart is three nested loops,
one per direction:

```{literalinclude} ../../src/builder/lj_particle_box_builder.cpp
:language: cpp
:start-at: // Place particles on cubic lattice with small random offsets
:end-before: std::cout << "Placed "
:dedent: 4
```

Each atom sits at the centre of its grid cell, shifted so that the box is
centred on the origin, plus a small random nudge (`smallDisp`, up to 5% of
the spacing).

## The socket: Potential

The `Potential` class is the socket of the
[plug picture](../molsim/architecture.md#the-energy-function-is-a-plug):

```{literalinclude} ../../src/engine/force_calculator.h
:language: cpp
:start-at: // Base class for all interaction potentials
:end-at: "};"
```

`virtual ... = 0` declares a function without a body: every kind of
`Potential` must provide its own. Code that holds a `Potential` can call
`calculateForces` without knowing which energy function it is talking to
([The plug](../cpp/05_the_plug.md)).

`calculateForcesAndEnergy` is `virtual` too, but it has a body: a default
that calls the other two. A kind of `Potential` may replace it. `LJPotential`
does, because its pair loop yields the forces and the energy in one pass, and
the default would run that loop twice.

`LJPotential` is one such kind. Its declaration starts:

```{literalinclude} ../../src/engine/potentials/lj_potential.h
:language: cpp
:start-at: class LJPotential : public Potential {
:end-before: // LJ-specific methods
```

`: public Potential` says that an `LJPotential` is a `Potential`; `override`
marks each function that fills one of the socket's slots, and the compiler
checks that it really does.

### The pair loop

The work of `LJPotential` is the loop over pairs of atoms, in
`src/engine/potentials/lj_kernels.cpp`. The part for one pair $i, j$:

```{literalinclude} ../../src/engine/potentials/lj_kernels.cpp
:language: cpp
:start-at: double dx = v.x[j] - xi;
:end-at: fzi -= fOverR * dz;
:dedent: 12
```

Line by line, this is the energy function of the architecture page:

- `dx`, `dy`, `dz` are the separation of the pair. The three "minimum image"
  lines replace each by the separation to the nearest copy in the
  [periodic box](../molsim/architecture.md#what-the-simulation-holds).
- Pairs beyond the cutoff are skipped. The test compares $r^2$ with
  $r_c^2$, so no square root is needed.
- `sr6` is $(\sigma/r)^6$ and `sr12` is $(\sigma/r)^{12}$; `eps4` is
  $4\varepsilon$. The energy line is the LJ formula.
- `fOverR` is the size of the force divided by $r$,
  $24\varepsilon\left[2(\sigma/r)^{12} - (\sigma/r)^6\right]/r^2$. Multiplying it
  by the separation gives the force vector on atom $i$, again without a
  square root.

This loop visits every pair, so its cost grows as $N^2$; how it is split
across processor cores is the subject of the parallel programming module.

## The force step: ForceCalculator

`ForceCalculator` holds the list of potentials and adds up their forces and
energies:

```{literalinclude} ../../src/engine/force_calculator.cpp
:language: cpp
:start-at: void ForceCalculator::calculateForces
:end-before: void ForceCalculator::clearPotentials
```

The list is a `std::vector<std::shared_ptr<Potential>>`. A `shared_ptr` is a
pointer that several objects can share, and that deletes the object when
the last one lets go; here it lets the list hold LJ and DFT potentials side
by side, each reached through the `Potential` socket
([The plug](../cpp/05_the_plug.md)). With one potential in the list, as in
every run on these pages, the sum has one term.

## The loop: MolecularDynamics

`MolecularDynamics` owns the loop. `initialize` gives every atom a random
velocity at the target temperature and computes the first forces:

```{literalinclude} ../../src/engine/molecular_dynamics.cpp
:language: cpp
:start-at: void MolecularDynamics::initialize() {
:end-before: void MolecularDynamics::runSimulation() {
```

`runSimulation` then repeats one step per pass. The heart of the loop is
two calls, the integrator and the thermostat:

```{literalinclude} ../../src/engine/molecular_dynamics.cpp
:language: cpp
:start-at: for (currentStep = 0; currentStep < totalSteps; ++currentStep) {
:end-before: // Potential energy at the new positions
:dedent: 4
```

The rest of the pass adds the kinetic energy to the potential energy, which
the force step inside the integrator has already computed, prints a line
every 10 steps, and writes a frame of the trajectory.

**The integrator** is the Velocity-Verlet step of the architecture page,
with every mass equal to 1:

```{literalinclude} ../../src/engine/molecular_dynamics.cpp
:language: cpp
:start-at: void MolecularDynamics::integrateVelocityVerlet() {
:end-before: // Apply periodic boundary conditions to keep all atoms
```

The first loop moves each position by $v\,\Delta t + \tfrac12 F\,\Delta t^2$
and adds half a step of force to each velocity; then the forces are
recomputed at the new positions; the second loop adds the other half. In
between, `applyPeriodicBoundaryConditions` moves any atom that left the box
back in, by one box length, so positions stay between 0 and $L$.

{numref}`engine-loop-figure` draws one pass of `runSimulation` this way.
Compared with the loop of the architecture page ({numref}`md-loop-figure`),
the Integrator box has opened up: it is one function,
`integrateVelocityVerlet`, whose two loops sit on either side of the force
step, because the velocities can only be finished with the forces at the
new positions.

```{figure} ../figures/engine_loop.svg
:name: engine-loop-figure
:width: 100%

One pass of `runSimulation`, one time step. Each box names the engine
function that does that part; both halves of the velocity update happen
inside `integrateVelocityVerlet`, with `calculateForces` called between them.
```

**The thermostat** rescales every velocity by one factor, so that the
temperature is exactly the target:

```{literalinclude} ../../src/engine/molecular_dynamics.cpp
:language: cpp
:start-at: void MolecularDynamics::applyTemperatureControl() {
:end-before: bool MolecularDynamics::exportToPDB
```

The temperature here is computed as on the architecture page,
$T = 2K / (3N)$ with $k_B = 1$.

## Reading the configuration: YAMLParser

`YAMLParser` turns the YAML files into the objects above. It does not read
YAML itself: it runs a small Python script, `yaml_parser.py`, that converts
the file to JSON, and reads the JSON with the nlohmann/json library. So a
run needs Python with the PyYAML package, and that script where the engine
looks for it (the repository's `src/utils/`, or the folder the run starts
in).

For a classical potential file, the LJ branch builds the plug:

```{literalinclude} ../../src/utils/yaml_parser.cpp
:language: cpp
:start-at: if (componentType == "lennard_jones") {
:end-at: return ljPotential;
:dedent: 8
```

It reads the cutoff, ε and σ for each atom type, and the mixing rule, which
sets ε and σ for a pair of two different types. `std::make_shared` creates
the `LJPotential` and the `shared_ptr` that will hold it in the list.

## What the engine leaves out

The engine is small on purpose. These limits are known and stay as they are
on these pages:

- **Every pair is checked** on every step, so the work grows as $N^2$; there
  is no neighbour list yet that would skip far pairs cheaply.
- **The LJ energy is cut, not shifted,** at the cutoff, and no tail
  correction is added by default. `LJPotential` can add the tail correction
  (`setTailCorrection`), but the YAML keys `tail_correction` and
  `shift_potential` are not read by the parser.
- **Every atom has mass 1.**
- **The thermostat is the simplest kind:** an exact rescaling after every
  step, which holds the temperature fixed but does not let it fluctuate as
  it would in a real system at that temperature.
- **The run length** is `total_steps`; `equilibration_steps` and
  `production_steps` in the files are read but not used yet.

## Check yourself

:::{dropdown} 1. You write a new energy function, MorsePotential. Which class must it derive from, and which functions must it provide?
It must derive from `Potential` and provide `calculateEnergy`,
`calculateForces` and `getName`, the three functions declared `= 0`. Nothing
in `ForceCalculator` or `MolecularDynamics` changes.
:::

:::{dropdown} 2. In the pair loop, why is the cutoff tested on r2 and not on r?
Comparing squared distances gives the same answer and avoids a square root
for every pair; the energy and force formulas also need only $r^2$.
:::

:::{dropdown} 3. How many times does one step visit each pair of atoms?
Twice, with the default serial loop. The force call in
`integrateVelocityVerlet` runs the pair loop once, and that loop visits pair
$(i, j)$ once from atom $i$ and once from atom $j$. The potential energy that
`runSimulation` reports comes out of the same pass, through
`calculateForcesAndEnergy`, so it costs no second loop.
:::

:::{dropdown} 4. A run's YAML says tail_correction: true. Is the tail correction applied?
No. The parser does not read that key, so the run uses the default, no tail
correction.
:::

## Next

The pair loop visits every pair of atoms, so its cost grows as $N^2$, and so
far it runs on one core. The
[parallel programming module](../parallel/index.md) first measures how much
of a step that loop takes, then splits it across the cores of the processor.
