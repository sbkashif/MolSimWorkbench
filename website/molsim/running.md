# Running the minimal simulation algorithm

[A minimal molecular simulation algorithm](architecture.md) described the loop
that moves the atoms. This page runs it: the files that describe a run, how
to start one, what comes out, and how to check that the run on the home page
really ends as an equilibrium liquid.

The code on this page is Python, and works the same in a script or in a
Jupyter notebook.

:::{admonition} Before you start
:class: note
Install MolSimWorkbench with the
[installation instructions](https://github.com/sbkashif/MolSimWorkbench#build)
in the repository's README. All commands on this page run from the top folder
of the repository.
:::

:::{admonition} In a Jupyter notebook
:class: tip
Each code block on this page can be a cell of its own. Keep the result of
`sim.run()` in a variable and print it in the next cell: the engine's own
printed lines reach the notebook by a separate route and can mix with
Python's output in the same cell.
:::

## Three configuration files

A run is described by up to three files in YAML, the plain-text format of
`key: value` lines met on the architecture page
({numref}`run-layers-figure`):

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

**The potential file** chooses the energy function, as shown in
[Choosing the plug](architecture.md#choosing-the-plug): here
`potentials/lj_bulk.yaml`, LJ with ε = σ = 1 and a cutoff of 2.5.

**The geometry file** lists atoms one by one, for runs that do not use
`lj_particle_box`. `data/lj_pair_simulation.yaml` leaves out `system_mode`
and names one with `geometry_file: "geometry/lj_pair.yaml"`:

```yaml
molecule:
  name: "LJ pair"
  atoms:
    - { type: "LJ", position: [0.0, 0.0, 0.0] }
    - { type: "LJ", position: [1.5, 0.0, 0.0] }

box:
  type: "cubic"
  dimensions: [10.0, 10.0, 10.0]
```

Two atoms of type `LJ`, the type that `potentials/lj_bulk.yaml` gives
ε = σ = 1, 1.5σ apart in a box 10σ wide. Their energy is
$U(1.5) = -0.320$, the value at r = 1.5 on the interactive plot in
[The energy function](architecture.md#the-energy-function). Every other run
on these pages uses `lj_particle_box`.

## Starting a run

A short Python script starts the run, through the `molsimworkbench` module.
Saved as `run.py` in the top folder of the repository:

```python
from molsimworkbench import System

sim = System.from_yaml("data/lj_bulk_simulation.yaml",
                       trajectory_output="traj.xyz")
print("final energy:", sim.run())
```

`System.from_yaml` reads the three files and builds the system;
`sim.run()` runs the loop and returns the final energy. While it runs, the
engine prints a line every 10 steps:

```console
$ python run.py
...
Creating LJ particle box: N=125, rho=0.8, L=5.38609 Å
...
Step 0, Potential Energy: -611.553, Kinetic Energy: 187.5, Total Energy: -424.053, Temperature: 1
Step 10, Potential Energy: -608.269, Kinetic Energy: 187.5, Total Energy: -420.769, Temperature: 1
...
final energy: -458.60397439333326
```

(The `Å` after the box length is a leftover label; the length is in units
of σ.) The Python lines hand the work to the C++ engine; the pybind11 module
explains how.

:::{admonition} Without Python
:class: note
The engine also comes as a command-line program,
`build/run_simulation data/lj_bulk_simulation.yaml traj.xyz`. It runs the
same C++ code and writes the same trajectory, byte for byte.
:::

The run is also **reproducible**. The `random_seed` under `simulation` fixes
the starting velocities, and the one under `lj_box` fixes the small nudges
off the grid, so running again gives the same file. With `random_seed: 0`
under `simulation`, the starting velocities are drawn afresh on every run,
and each run gives a different trajectory.

:::{admonition} How long a run is
:class: note
This file runs 1000 steps of 0.001, one unit of time. That comes from the
engine's default length, because the file sets no `total_steps`. The file's
`equilibration_steps` and `production_steps` are printed at the start but
not used yet. To run longer, add `total_steps` under `simulation`, as the
home-page run does (`total_steps: 4000`, with `num_particles: 512`).
:::

## What comes out

**Printed lines.** Every 10 steps the engine prints the potential energy,
the kinetic energy, their sum, and the temperature. The kinetic energy stays
at exactly 187.5: the thermostat rescales the velocities after every step so
that the temperature is 1, and for 125 atoms that means
$\tfrac{3}{2} \times 125 \times 1 = 187.5$. Because the thermostat adds and
removes energy, the total energy is not constant in these runs.

**The trajectory** is in **XYZ** format, a plain-text format that viewers
such as VMD read directly (`vmd traj.xyz`). Each frame is a block of lines:
the number of atoms, a comment, then one line per atom:

```text
125
Step 0, PE: -611.552758, KE: 187.500000
   LJ    3.264187    3.197090    3.263010    0.589169   -0.455788    1.221319
```

On each atom line come the atom type, its position $x, y, z$, and its
velocity $v_x, v_y, v_z$. This run writes a frame after every step, plus
one at the start and one at the end: 1002 frames for 1000 steps.

**Reading a trajectory** takes a few lines of Python. This function reads
every frame into one NumPy array:

```{literalinclude} examples/read_xyz.py
:language: python
:pyobject: read_xyz
```

```console
$ python read_xyz.py traj.xyz
frames, atoms, coordinates: (1002, 125, 3)
atom 0, first frame: [-2.122491 -2.188536 -2.124306]
atom 0, last frame:  [2.893976 3.145653 3.373486]
```

The first frame has coordinates between $-L/2$ and $L/2$, as the grid is
built around the centre of the box. After the first step the engine keeps
every atom between 0 and $L$, moving an atom by one box length when it
leaves through a face, as the [periodic box](architecture.md#what-the-simulation-holds)
rule says. So coordinates jump by $L$ between frames now and then; the next
section undoes those jumps before measuring how far atoms travel.

## From a grid to an equilibrium liquid

The home page claims that its run ends as a liquid at **equilibrium**: a
state whose averages no longer change, only fluctuate. Two measurements
from that run, 512 atoms for 4000 steps, check both halves of the claim.

**The potential energy settles.** {numref}`run-energy-figure` shows the
potential energy per atom during the run. On the grid it is −4.91ε. As the
atoms start moving it first rises, to −4.30ε at step 80, then falls and
settles. After t = 1 it fluctuates around −5.10ε, by about 0.04ε, with no
trend. The first unit of time is the run forgetting its starting grid;
averages are taken only after it.

```{figure} ../figures/run_energy.svg
:name: run-energy-figure
:width: 600px

Potential energy per atom in the home-page run. The shaded part is the
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

Mean-square displacement in the home-page run. It keeps rising in a straight
line, past the squared spacing between neighbours (dashed).
```

The MSD needs the jumps by $L$ removed first. `msd.py` does it by taking each
frame-to-frame step through the nearest copy of the box, then adding the
steps up:

```{literalinclude} examples/msd.py
:language: python
:start-after: from read_xyz import read_xyz
```

On the 125-atom run of this page:

```console
$ python msd.py traj.xyz 5.38609 0.001
t = 0.25: MSD = 0.076
t =  0.5: MSD = 0.187
t =  1.0: MSD = 0.416
```

This agrees with the published phase diagram of the LJ fluid. T = 1.0 lies
between the triple point, about 0.68
([Khrapak and Morfill](https://ar5iv.labs.arxiv.org/html/1104.2754)), and
the critical point, 1.19 for this cutoff
([Wilding 1995](https://arxiv.org/abs/cond-mat/9503145)), so a liquid can
exist there; and density 0.8 is below the density at which the liquid
freezes at this temperature, about 0.91 (from the freezing line in the same
Khrapak and Morfill paper). The starting grid was never a crystal of LJ
atoms, only a convenient way to place them without overlaps.

## What ε and σ do to many atoms

The architecture page described ε and σ for
[two atoms](architecture.md#σ-the-size-of-an-atom). With many atoms their
effect shows directly. {numref}`run-sigma-epsilon-figure` shows four runs,
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

## Check yourself

:::{dropdown} 1. You set num_particles to 1000 and keep density 0.8. How long is the side of the box?
$L = (1000 / 0.8)^{1/3} = 10.77$, in units of σ.
:::

:::{dropdown} 2. The home-page run has 512 atoms at T = 1. What kinetic energy does it print on every line?
768: the thermostat holds the kinetic energy at
$\tfrac{3}{2} N k_B T = \tfrac{3}{2} \times 512 \times 1$.
:::

:::{dropdown} 3. You run run.py twice with random_seed: 42. Do the two trajectories differ? And with random_seed: 0?
With 42, no: the seed fixes every random number, so the two files are
identical byte for byte. With 0, yes: the starting velocities are drawn
afresh on each run.
:::

:::{dropdown} 4. Why are the averages of the home-page run taken only after t = 1?
Until then the run is still forgetting its starting grid: the potential
energy is still drifting ({numref}`run-energy-figure`). Averages over that
part would describe the grid, not the liquid.
:::

## Next

Everything on this page went through one call, `sim.run()`, and everything
that call did happened inside the engine, in C++. To see how the loop of
the architecture page works inside the engine, the next step is the
[C++ module](../cpp/index.md), because the engine is C++ code. It starts from a ten-line program that computes one LJ energy, the
formula on the architecture page, and adds a few lines per chapter until it
reaches the classes the engine is built from.
