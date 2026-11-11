# Binding the engine

[A first binding](01_a_first_binding.md) made one C++ function callable from
Python. The engine needs more: Python must create C++ objects, such as a
`Geometry` or an `LJPotential`, call their functions, and hand them to other
C++ objects. This chapter reads the engine's real bridge,
`src/bindings/molsim_bindings.cpp`, and follows the home page's `sim.run()`
from Python down into C++.

It uses the classes of the [C++ module](../cpp/index.md): an object such as
`geometry` is one value of the class `Geometry`
([Classes](../cpp/04_classes.md)), and the potentials are plugs in the
`Potential` socket ([The plug](../cpp/05_the_plug.md)).

## The layers

The script on the home page passes through four layers
({numref}`pybind-layers-figure`). The top two are Python; the module
`_molsim` is the pybind11 bridge; the engine below it is C++.

```{figure} ../figures/pybind_layers.svg
:name: pybind-layers-figure

The layers between the home-page script and the engine. Each layer calls the
one below it.
```

## The bridge: molsim_bindings.cpp

The engine's binding file starts like `ljmodule.cpp`, with two extra headers:

```{literalinclude} ../../src/bindings/molsim_bindings.cpp
:language: cpp
:start-at: "#include <pybind11/pybind11.h>"
:end-at: "#include <pybind11/stl.h>"
```

```{literalinclude} ../../src/bindings/molsim_bindings.cpp
:language: cpp
:start-at: "PYBIND11_MODULE(_molsim, module)"
:end-at: "module.doc()"
```

The module is named `_molsim`; the leading underscore is a Python convention
for "internal: use it through something else", here through
`molsimworkbench.py`. Instead of `m.def` for a function, the file mostly
uses `py::class_`, which makes a C++ class available as a Python class
([object-oriented code](https://pybind11.readthedocs.io/en/stable/classes.html)).
The `Geometry` binding shows the pattern:

```{literalinclude} ../../src/bindings/molsim_bindings.cpp
:language: cpp
:start-at: "py::class_<Geometry>(module"
:end-before: "py::class_<Potential, std::shared_ptr<Potential>>"
```

- `py::class_<Geometry>(module, "Geometry")` creates a Python class named
  `Geometry` that stands for the C++ class `Geometry`.
- `.def(py::init<>())` lets Python create one: `_molsim.Geometry()` runs the
  C++ constructor, the function that sets up a new object.
- Each `.def("name", &Geometry::name)` makes one C++ member function callable
  from Python, exactly like `m.def` did for `lj_energy`.
  `py::return_value_policy::copy` tells pybind11 to give Python its own copy
  of the returned value
  ([return value policies](https://pybind11.readthedocs.io/en/stable/advanced/functions.html)).

Some functions take or return a list of numbers, such as a position with
three coordinates. The last header, `pybind11/stl.h`, converts C++
containers such as `std::vector` and `std::array` to and from Python lists,
copying them on every crossing
([STL containers](https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html)).
The middle one, `pybind11/functional.h`, is for the browser and comes up on
[the next page](03_in_your_browser.md).

## Building a pair from Python

With these classes, Python can build the two-atom system of the C++ module
by hand, one engine object at a time:

```{literalinclude} examples/ch02/pair_by_hand.py
:language: python
:lines: 6-
:caption: pair_by_hand.py
```

```console
$ python3 pair_by_hand.py
energy: -0.32033659427857464
forces: [[1.1580288310461555, 0.0, 0.0], [-1.1580288310461555, 0.0, 0.0]]
```

The energy is U(1.5) again: the engine's LJ code gives the same value as
`lj_energy` in its first 16 digits. The forces come back as a Python list of
lists, converted by `stl.h` from the C++ `std::vector<std::array<double, 3>>`.
They are equal and opposite: at 1.5σ the atoms attract, so the first atom is
pulled toward +x and the second toward −x.

Two lines need a closer look. `geometry.addAtom("LJ", [0.0, 0.0, 0.0])` passes
a Python list where the C++ function expects a `std::array<double, 3>`;
`stl.h` converts it. And `forces.addPotential(lj)` hands a Python-created
object to a C++ object, which keeps it. That second one is the subject of the
next section.

## Who owns the potential

`ForceCalculator` keeps its energy functions in a list of
`std::shared_ptr<Potential>`. Recall from [The plug](../cpp/05_the_plug.md)
that a `std::shared_ptr` counts its owners: the object it points to is
deleted only when the last `shared_ptr` to it is gone. The binding declares the same kind of owner for
the potentials:

```{literalinclude} ../../src/bindings/molsim_bindings.cpp
:language: cpp
:start-at: "py::class_<Potential, std::shared_ptr<Potential>>"
:end-at: "py::class_<LJPotential, Potential, std::shared_ptr<LJPotential>>"
```

The second template argument is the **holder type**: how pybind11 owns the
C++ object behind a Python object. Without it, the holder is a
`std::unique_ptr`, a single owner, and the C++ object is deleted when Python
drops its last reference
([smart pointers](https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html)).
With `std::shared_ptr`, the Python object and the `ForceCalculator` share
ownership ({numref}`pybind-shared-figure`).

```{figure} ../figures/pybind_shared.svg
:name: pybind-shared-figure

After `forces.addPotential(lj)`, one `LJPotential` has two owners.
```

This script deletes Python's name for the potential after adding it, and
then uses it through the `ForceCalculator`:

```{literalinclude} examples/ch02/shared_owner.py
:language: python
:lines: 8-
:caption: shared_owner.py
```

```console
$ python3 shared_owner.py
energy: -0.32033659427857464
TypeError: addPotential(): incompatible function arguments. The following argument types are supported:
    1. (self: _molsim.ForceCalculator, arg0: _molsim.Potential) -> None

Invoked with: <_molsim.ForceCalculator object at 0x10379e730>, 5
```

The energy is still computed after `del lj`, because the `ForceCalculator`
still owns the potential. The last lines show the type check from the first
chapter at work on a class: `addPotential` accepts any `Potential`, such as
an `LJPotential`, and refuses the number 5. `self` is the object the
function is called on, here `forces`.

## How Python finds _molsim

`_molsim` is built by CMake, not by a hand-typed command. `CMakeLists.txt`
calls pybind11's CMake function `pybind11_add_module` with the name
`_molsim` and the source `src/bindings/molsim_bindings.cpp`, then links the
engine's libraries into it. The function adds a library target to build from
the listed sources and sets the Python-specific compiler and linker options,
the ones typed by hand in the first chapter
([building with CMake](https://pybind11.readthedocs.io/en/stable/compiling.html)).
The result lands in `build/`, as `build/_molsim.cpython-312-darwin.so` on
this Mac.

Python does not look in `build/` by itself. `molsimworkbench.py` adds it:

```{literalinclude} ../../molsimworkbench.py
:language: python
:pyobject: _load_bindings
```

`_load_bindings` first tries a plain `import _molsim`. If that fails, it adds
`build/` and the folders that some build tools put inside it (`Debug`,
`Release`, …) to `sys.path`, the list of folders Python searches when
importing, and tries again. The `sys.path.insert(0, ...)` line at the top of
each example in this chapter does the same for `molsimworkbench.py` itself,
which lives at the top of the repository.

## Following sim.run()

`System.from_yaml` reads the configuration and builds the engine objects,
calling C++ through `_molsim` at every step:

```{literalinclude} ../../molsimworkbench.py
:language: python
:pyobject: System.from_yaml
```

Each `_molsim.` name is a class from the binding file: the YAML reader, the
lattice builder, the `ForceCalculator` and its potentials, and the
`MolecularDynamics` driver. `System.run` then hands control to C++:

```{literalinclude} ../../molsimworkbench.py
:language: python
:pyobject: System.run
```

`runSimulation` is a single call from Python, and the whole loop of
[A simple molecular dynamics loop](../molsim/architecture.md) runs inside
it, in C++: 2500 steps of forces, Velocity-Verlet and thermostat, with no
Python in between. This is why the Python layer costs almost nothing here:
it crosses the bridge a handful of times to set things up, then once to run.
The home page's "Behind sim.run()" call stack is this path.

The home-page script, with three lines added to show what it built, run from
the top folder of the repository, where `data/` is:

```{literalinclude} examples/ch02/run_system.py
:language: python
:lines: 7-
:caption: run_system.py
```

```console
$ python3 website/pybind11/examples/ch02/run_system.py
bindings loaded from: _molsim.cpython-312-darwin.so
...
Trajectory output set to traj.xyz (XYZ format), writing every 1 steps.
type of sim._dynamics: <class '_molsim.MolecularDynamics'>
...
Step 2490, Potential Energy: -265.692, Kinetic Energy: 187.5, Total Energy: -78.1925, Temperature: 1
final energy: -79.82
```

(Each `...` stands for lines that the C++ engine prints while it reads the
files and runs.) The command-line program `run_simulation`, given the same
configuration, prints `Final energy: -79.817` and writes a trajectory that is
identical byte for byte (`cmp` finds no difference): the Python layer adds no
physics of its own.

```console
$ build/run_simulation data/lj_play_simulation.yaml cli.xyz
...
Final energy: -79.817
$ cmp traj.xyz cli.xyz
```

### When the layers disagree

That last check has not always passed. The command-line program and
`System.from_yaml` each assemble the engine objects themselves, so they can
drift apart. In this version of the code, `from_yaml` gained one line:

```python
config.simulationParams.usePBC = True
```

The command-line program had always switched on periodic boundaries for the
LJ box; `from_yaml` did not, so Python runs simulated the atoms without a
periodic box, and their trajectories differed from the command line's. The
fix is the line above, and the check that caught it is the one in the
previous section: run both on the same configuration and compare the
results. Whenever Python and C++ build the same thing separately, they need
a test that compares them.

## Check yourself

:::{dropdown} 1. Why does shared_owner.py still compute an energy after del lj?
The `ForceCalculator` holds its own `std::shared_ptr` to the potential. `del
lj` drops Python's owner only; the object is deleted when its last owner
lets go, and the `ForceCalculator` has not.
:::

:::{dropdown} 2. The forces in pair_by_hand.py come back as a Python list of lists. Which line of molsim_bindings.cpp makes that possible?
`#include <pybind11/stl.h>`. It converts the C++
`std::vector<std::array<double, 3>>` into a Python list of lists, copying it.
:::

:::{dropdown} 3. You run a script from a folder where molsimworkbench.py cannot find build/. What happens, and what would you check first?
The import of `_molsim` fails with an `ImportError` (`ModuleNotFoundError`).
Check that the project has been built, so that `build/_molsim*.so` exists,
and that `molsimworkbench.py` is the one at the top of the repository, next
to `build/`.
:::

:::{dropdown} 4. sim.run() simulates 2500 steps. How many times does that call cross from Python into C++?
Three: `initialize`, `runSimulation` and `getCurrentEnergy`. All 2500 steps
run inside the one `runSimulation` call, in C++.
:::

## Next

On your computer, this path needs Python and the compiled module, yet the Run
buttons on these pages need neither. [The same module, in your
browser](03_in_your_browser.md) shows what runs them: the bottom three of these
layers, with two JavaScript layers on top in place of your script.
