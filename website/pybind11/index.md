# About the pybind11 module

The engine is written in C++, and the five-line script on the
[home page](../index.md) drives it from Python. This module explains the
bridge between the two: **pybind11**, a library that makes C++ functions and
classes callable from Python.

The first chapter builds the smallest possible bridge, for the `lj_energy`
function from the C++ module, and calls it from Python. The second reads the
engine's real bridge and follows `sim.run()` from Python into C++. The
first chapter needs the C++ module up to [Functions](../cpp/02_functions.md);
the second also uses classes, which the [C++ module](../cpp/index.md) covers.

As in the C++ module, every example is a file you can build and run, and
every output on the pages comes from running it. Each chapter ends with a few
questions; click a question to see its answer.

Chapters:

1. [A first binding](01_a_first_binding.md)
2. [Binding the engine](02_binding_the_engine.md)
