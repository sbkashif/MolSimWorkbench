# The same module, in your browser

[Binding the engine](02_binding_the_engine.md) followed `sim.run()` from a
Python script, through the pybind11 module `_molsim`, into the C++ engine.
That path needs Python and a compiled engine on your computer. Yet the
**Run** button on [A simple molecular dynamics loop](../molsim/architecture.md)
runs the same files with nothing installed. This page shows what runs them:
the same script, the same `molsimworkbench.py` and the same pybind11 module,
compiled for the browser.

## What happens when you press Run

Run passes through five layers ({numref}`browser-layers-figure`). The top two
are JavaScript, the language web pages are programmed in, and exist only in
the browser. The bottom three are the layers of the previous page.

```{figure} ../figures/pybind_browser_layers.svg
:name: browser-layers-figure

The layers a Run on a page goes through. Each layer calls the one below it.
The bottom three are the same as when you run `run.py` on your computer.
```

**The page.** `engine_run.js` is the page's own code. When you press Run, it
takes the two YAML files as they are in the boxes and posts them to a
**Web Worker**: a second thread that the browser runs beside the page. The
run happens in the worker, so the page keeps scrolling and drawing while the
engine works. Stop does not ask the engine to stop; it ends the worker, and
the next Run starts a new one.

**The worker.** `engine_worker.js` loads **Pyodide**, which is CPython 3.12,
the same Python you run on your computer, compiled to **WebAssembly**:
a form of machine code that browsers run. Pyodide is about 6 MB, fetched once
per visit, as soon as you move the pointer onto the controls. The worker then
writes four files into a folder `/run` that exists only inside Pyodide:

```
/run/molsimworkbench.py
/run/_molsim.cpython-312-wasm32-emscripten.so
/run/data/lj_bulk_simulation.yaml
/run/data/potentials/lj_bulk.yaml
```

The two YAML files are the ones you edited. The other two are the
repository's own `molsimworkbench.py` and the engine module. The worker then
runs the lines of `run.py`:

```python
sim = System.from_yaml("data/lj_bulk_simulation.yaml",
                       trajectory_output="traj.xyz", trajectory_every=...)
print("final energy:", sim.run())
```

**Python, pybind11, C++.** From here on it is the previous page.
`from_yaml` builds the `Geometry`, the `LJPotential` and the
`MolecularDynamics` through `_molsim`, then calls `runSimulation`, and all
1000 steps run inside that one call, in C++. The lines the engine prints go
back to the page as it prints them, which is what fills the Simulator panel.

**Back to the page.** When the run ends, the worker reads `traj.xyz`, packs
every position of every frame into one array of numbers and sends it to the
page. The movie is drawn from that array: it is the engine's trajectory, not
an animation made up to look like one.

## The one change: reading YAML

The module in the browser is built from the same C++ files as on your
computer, with one difference in behaviour. On a computer the engine reads a
YAML file by starting a separate program, `python yaml_parser.py`
(`yaml_parser.cpp:81`). A web page cannot start programs, so that line cannot
run in the browser.

Instead, Python hands C++ a function to call. When `molsimworkbench.py` finds
it is running in a browser, it passes one of its own functions to the engine:

```python
if sys.platform == "emscripten":
    def _load_yaml(path):
        with open(path) as file:
            return json.dumps(yaml.safe_load(file))

    _molsim.setYAMLLoader(_load_yaml)
```

`setYAMLLoader` is bound like any other function in `molsim_bindings.cpp`, but
its argument is unusual: on the C++ side it is a `std::function`, a value that
holds something callable. The line `#include <pybind11/functional.h>` at the
top of the binding file lets pybind11 wrap a Python function as one. From then
on, whenever the C++ parser needs a YAML file, it calls `_load_yaml`, so C++
calls back into Python, and Python reads the file with the PyYAML library that
the worker loaded.

## Is it really the engine?

The check is the one from the end of the previous page: run the same
configuration both ways and compare. The run on
[A simple molecular dynamics loop](../molsim/architecture.md), with its files
unchanged, ends at

```
browser:   final energy: -458.60397439333104
computer:  final energy: -458.60397439333326
```

The two agree to 14 digits, and every step line the engine prints is the
same. Two builds of the same C++ code by different compilers often differ in
the last few digits like this, because a compiler may order or combine a few
floating-point operations differently; a different engine would not agree in
the first digit after a thousand steps.

::::{admonition} Detour: where the browser module comes from
:class: detour

The script `scripts/wasm/build_pyodide.sh` compiles the engine's C++ files,
and the same `molsim_bindings.cpp`, with **Emscripten**, a C++ compiler whose
output is WebAssembly instead of machine code for your processor. It compiles
against Pyodide's copy of the Python headers, so the result is a Python
extension module for Pyodide, just as the normal build makes one for your
Python. The file is 651 KB and is part of the website. It is rebuilt by hand
whenever the engine's C++ changes.
::::

## Check yourself

:::{dropdown} 1. Why does the run happen in a Web Worker instead of on the page itself?
A run takes the whole of the thread it runs on until `runSimulation` returns.
On the page's own thread, the page could not scroll, draw or respond to Stop
during the run. In a worker the page stays free, and Stop can end the worker.
:::

:::{dropdown} 2. Which layers of a browser run are C++, and which are new compared with running run.py on your computer?
The engine is C++, reached through the pybind11 module, as on your computer.
The new layers are the two JavaScript ones, the page and the worker, plus
Pyodide in place of your installed Python.
:::

:::{dropdown} 3. Why does the engine need setYAMLLoader in the browser but not on your computer?
On a computer the engine reads YAML by starting `python yaml_parser.py`, and a
web page cannot start programs. `setYAMLLoader` gives the engine a Python
function to call instead, which reads the file with the Python that is already
running.
:::

## Next

On both your computer and in the browser, Python's part ends at one call,
`runSimulation`, and every step of the run happens inside it, in C++.
[Reading the engine](../engine/reading_the_engine.md) opens that call: the
objects `from_yaml` built, one class at a time, in the order a run uses them.
