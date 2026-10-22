# A first binding

Every program in the [C++ module](../cpp/index.md) ran on its own, started
from the terminal through its `main`. The home page's script instead
imports the engine into Python and calls it. This chapter builds the
smallest such bridge: it takes the C++ function
`lj_energy(r, epsilon, sigma)` from [Functions](../cpp/02_functions.md),
makes it callable from Python with four extra lines of C++, and follows one
call across.

## What Python can import

Python imports two kinds of module. A `.py` file is Python source, read and
run by Python itself. An **extension module** is a compiled library: machine
code, built by a C or C++ compiler, that Python loads like any other module.
The Python documentation describes it as
[a shared library that exports an initialization function](https://docs.python.org/3/extending/building.html):
when Python imports the module for the first time, it calls that function,
and the function sets up the module's contents. The file must be named after
the module plus a suffix that depends on the Python version and the system,
such as `.cpython-312-darwin.so` for Python 3.12 on a Mac
([defining extension modules](https://docs.python.org/3/c-api/extension-modules.html)).

Writing that initialization function by hand takes a lot of code. pybind11
writes it for you.

## The binding

```{literalinclude} examples/ch01/ljmodule.cpp
:language: cpp
:linenos:
:emphasize-lines: 2, 9-12
:caption: ljmodule.cpp
```

Lines 4–7 are the function from the C++ module, unchanged. The new lines:

- **Line 2** includes pybind11's header.
- **Line 9**, `PYBIND11_MODULE(ljmodule, m)`, creates the function that
  Python calls when it imports the module
  ([pybind11 first steps](https://pybind11.readthedocs.io/en/stable/basics.html)).
  The first argument is the module's name, written without quotes; the
  second, `m`, is the module being built.
- **Line 10** sets the module's description.
- **Line 11**, `m.def(...)`, adds a Python function named `lj_energy` that
  runs the C++ function `&lj_energy` (the `&` takes the function's address,
  as it took a variable's address in chapter 1 of the C++ module). The last
  argument is the function's description for Python's `help()`.

## Build and import

pybind11 itself is a header-only library: compiling with its headers is all
it takes, with no extra library to link
([building manually](https://pybind11.readthedocs.io/en/stable/compiling.html)).
Install it into the Python you will import the module with; the examples here
use the `molsim-docs` conda environment and pybind11 2.13.1, the same version
the engine's build fetches:

```console
$ python3 -m pip install "pybind11==2.13.1"
```

The build is one compiler command, the one from pybind11's documentation for
version 2.13 on macOS:

```console
$ c++ -O3 -Wall -shared -std=c++17 -undefined dynamic_lookup \
      $(python3 -m pybind11 --includes) ljmodule.cpp \
      -o ljmodule$(python3-config --extension-suffix)
```

Each part:

- `-shared` builds a shared library instead of a program: there is no
  `main`, and the result is loaded by another program, here Python.
- `$(python3 -m pybind11 --includes)` runs a small command and pastes its
  output into the line: the folders that hold pybind11's and Python's
  headers.
- `-o ljmodule$(python3-config --extension-suffix)` names the output after
  the module, plus the suffix this Python expects:
  `ljmodule.cpython-312-darwin.so`.
- `-undefined dynamic_lookup` is for macOS only: it lets the library refer to
  Python's own functions, which are found when Python loads it. On Linux,
  leave it out.

The result is a compiled library, and it exports the initialization function
that `PYBIND11_MODULE` wrote, named `PyInit_` plus the module name:

```console
$ file ljmodule.cpython-312-darwin.so
ljmodule.cpython-312-darwin.so: Mach-O 64-bit dynamically linked shared library arm64
$ nm -gU ljmodule.cpython-312-darwin.so | grep PyInit
00000000000007c0 T _PyInit_ljmodule
```

Python, run in the same folder, now imports it like any module:

```{literalinclude} examples/ch01/use_it.py
:language: python
:caption: use_it.py
```

```console
$ python3 use_it.py
-0.3203365942785746
-0.6406731885571492
-0.7736980930559998
```

These are the three values of `parameters.cpp` in the
[Functions](../cpp/02_functions.md#parameters-for-ε-and-σ) chapter, printed with
more digits: `std::cout` shows six significant digits, Python shows enough
digits to identify the number exactly. They are the same numbers. Printed
with 17 digits, the C++ result for the first call is
`-0.32033659427857458`, and Python confirms that this equals
`-0.3203365942785746`.

## One call, step by step

The C++ function takes `double`s; Python passes Python `float`s. pybind11
stands between them ({numref}`pybind-call-figure`):

1. Python calls the function that `m.def` made.
2. pybind11 **converts** each argument to the type the C++ function expects:
   each Python `float` becomes a C++ `double`. A Python `int` is also
   accepted, because it can be converted to a floating-point number
   ([functions](https://pybind11.readthedocs.io/en/stable/advanced/functions.html)).
3. The C++ function runs, as compiled machine code.
4. pybind11 converts the returned `double` back into a Python `float`.

```{figure} ../figures/pybind_call.svg
:name: pybind-call-figure

One call of `ljmodule.lj_energy`. pybind11 converts the arguments on the way
in and the result on the way out; the C++ function never sees a Python
object.
```

If an argument cannot be converted, pybind11 refuses the call before any C++
runs, and Python raises a `TypeError` that lists the types it accepts:

```console
>>> ljmodule.lj_energy("far", 1.0, 1.0)
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
TypeError: lj_energy(): incompatible function arguments. The following argument types are supported:
    1. (arg0: float, arg1: float, arg2: float) -> float

Invoked with: 'far', 1.0, 1.0
```

This is the same mistake as storing text in a `double` in
[A first program](../cpp/01_first_program.md#a-type-is-fixed). There the
compiler caught it before the program existed; here the call is made from
Python, whose types are known only when the line runs, so the check happens
at that moment.

## Names and default values

The error message calls the parameters `arg0`, `arg1`, `arg2`: pybind11 does
not know their names. `py::arg` tells it, and `= 1.0` gives a default value
([keyword arguments](https://pybind11.readthedocs.io/en/stable/basics.html)).
A second module, `ljnamed`, differs from `ljmodule` only in these lines:

```{literalinclude} examples/ch01/ljnamed.cpp
:language: cpp
:lines: 4, 11-15
:caption: ljnamed.cpp (the lines that differ)
```

`namespace py = pybind11;` lets the code write `py::arg` instead of
`pybind11::arg`. Built with the same command, the function now has names and
defaults that Python can use:

```console
>>> import ljnamed
>>> print(ljnamed.lj_energy.__doc__)
lj_energy(r: float, epsilon: float = 1.0, sigma: float = 1.0) -> float

LJ energy of one pair at distance r

>>> ljnamed.lj_energy(1.5)
-0.3203365942785746
>>> ljnamed.lj_energy(1.5, sigma=1.2)
-0.7736980930559998
```

:::{admonition} If you know Python
:class: tip
The [Functions](../cpp/02_functions.md) chapter noted that C++ has no named
arguments. The C++ function still has none: `py::arg` gives names to the
Python side of the bridge only, and pybind11 matches them to positions before
calling C++.
:::

## Check yourself

:::{dropdown} 1. You edit lj_energy in ljmodule.cpp and run use_it.py again without rebuilding. Which version runs?
The old one. Python loads the compiled `.so` file, and the `.so` changes only
when you run the compiler command again, just as `./pair` in the C++ module
runs the last compiled program.
:::

:::{dropdown} 2. What does ljmodule.lj_energy(2, 1, 1) return, and why is an int accepted?
`-0.0615234375`. pybind11 converts each Python `int` to a C++ `double`,
because an integer can be turned into a floating-point number.
:::

:::{dropdown} 3. You build the module with -o ljmodule.so, without the suffix. Can Python 3.12 on this Mac import it?
Yes: Python's list of extension suffixes also includes the plain `.so`. But
the suffix records which Python the file was built for, so `ljmodule.cpython-312-darwin.so`
is the safer name: a different Python version will not mistake it for its own.
:::

:::{dropdown} 4. Why does ljmodule.lj_energy("far", 1.0, 1.0) fail with a TypeError instead of a compile error?
The call is in Python, and Python checks types when the line runs. pybind11
tries to convert `"far"` to a `double`, cannot, and raises the error before
the C++ function is called.
:::

## Next

`ljmodule` makes one function callable. The engine's bridge does the same
thing on a larger scale: whole classes instead of one function, and C++
objects that Python and C++ use at the same time.
[Binding the engine](02_binding_the_engine.md) reads it, and follows the
home page's `sim.run()` across it.
