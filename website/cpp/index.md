# About the C++ module

[Running a simulation](../molsim/running.md) did everything from Python, yet
the work inside `sim.run()` is done by an engine written in C++.

::::{admonition} Detour: why the engine is not Python
:class: detour

This project was for me to learn C++. It also has a practical reason: plain
Python is slow for the one loop that matters. Every step of a run visits every pair of atoms, and this
is how slow that gets. The same pass over all pairs of the home page's 512
atoms, written once in plain Python and once in C++
(`examples/why_cpp/pairs.py` and `pairs.cpp`), gives the same energy and very
different times:

```console
$ python3 pairs.py
python: U = -2695.565361   57.2 ms per pass
$ g++ -O3 -std=c++17 pairs.cpp -o pairs && ./pairs
c++:    U = -2695.565361   1.81 ms per pass
```

The C++ pass is about 30 times faster (Python 3.13, and g++ 8.5 with `-O3`,
which asks the compiler to
[optimise for speed](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html),
both on one [core](../parallel/01_measure_first.md#cores-and-threads) of an
AMD EPYC 7763). The home page's run makes 4000 passes: about four minutes of
this loop in Python, about seven seconds in C++. A C++
[compiler](01_first_program.md#from-a-text-file-to-a-running-program) turns
the loop into the
[processor's own instructions](01_first_program.md#what-the-compiler-does)
once, before the program runs. Python turns it into
[bytecode](https://docs.python.org/3/glossary.html#term-bytecode), its own
instruction list, and a program called the interpreter then carries out
those instructions one at a time, on every one of the 130,816 pair visits of
every pass.

The linked terms each get a proper introduction later: the compiler and the
processor's instructions in [A first program](01_first_program.md), cores
in the parallel module. Here only the two times matter.
::::

So the engine computes in C++, and Python only sets up the run and starts
it. This module covers enough of the language to read the engine, one
chapter at a time, with one running example: a small program that computes
the Lennard-Jones energy of two atoms. Each chapter adds a few lines to it.
Chapter 5 arrives at the shape of the real engine's energy-function plug
from [A minimal molecular simulation algorithm](../molsim/architecture.md),
and chapter 6 splits the program into files, the way the engine is
organised.

Every example is a complete file you can compile and run. The outputs shown
on the pages come from running those files. Each chapter ends with a few
questions; click a question to see its answer.

Chapters:

1. [A first program](01_first_program.md)
2. [Functions](02_functions.md)
3. [Many values: vectors and loops](03_vectors_and_loops.md)
4. [Classes](04_classes.md)
5. [The plug: inheritance and virtual functions](05_the_plug.md)
6. [Several files: headers, source files and linking](06_several_files.md)

## Next

Start with [A first program](01_first_program.md). You need a terminal and a
C++ compiler; the chapter shows the commands that compile and run the
program.
