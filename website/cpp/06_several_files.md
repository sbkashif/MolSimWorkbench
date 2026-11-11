# Several files: headers, source files and linking

Every program so far is one file. The engine's `src/` folder holds 33 files: mostly
one header and one source file per class, plus the command-line tool and
the Python bindings. This chapter splits the
`LJPotential` of [Classes](04_classes.md) into a header, a source file and a
`main`, compiles them separately and links them. It pays off two promises:
[A first program](01_first_program.md) showed that `#include <cmath>` pastes
in the **declaration** of `pow`, and [Functions](02_functions.md) promised
declarations of your own.

## Three files

The **header** `lj.h` declares the class: its members and the functions it
has, without their code.

```{literalinclude} examples/ch06/lj.h
:language: cpp
:linenos:
:caption: lj.h
```

The **source file** `lj.cpp` holds the code, the **definitions**. It
includes its own header, then writes each function out in full.
`LJPotential::energy` means "the function `energy` of the class
`LJPotential`"; `::` joins the class name and the member name, as in
`std::pow`.

```{literalinclude} examples/ch06/lj.cpp
:language: cpp
:linenos:
:caption: lj.cpp
```

`main.cpp` includes the header and uses the class. It never sees the
formula:

```{literalinclude} examples/ch06/main.cpp
:language: cpp
:linenos:
:caption: main.cpp
```

`#include "lj.h"` uses quotes, not angle brackets. Quotes tell the compiler
to look first in the folder of the file being compiled; angle brackets, as in
`<cmath>`, look in the folders where the compiler keeps the standard
library's headers (and in any folder added with the `-I` option).

## Compile each file, then link

Chapter 1's preview `-c` runs the first two stages and stops before linking.
With several files, that is the normal way to work: each source file becomes
its own object file, and one last command links them
({numref}`cpp6-files-figure`).

```console
$ clang++ -std=c++17 -c lj.cpp -o lj.o
$ clang++ -std=c++17 -c main.cpp -o main.o
$ clang++ lj.o main.o -o pair
$ ./pair
U(1.5) = -0.320337
```

```{figure} ../figures/cpp6_files.svg
:name: cpp6-files-figure

The header is pasted into both source files. Each source file compiles on
its own into an object file; the linker matches every function that
`main.o` needs with the object file that has it, and writes the executable.
```

`main.cpp` compiled without the code of `energy`: the declaration in `lj.h`
was enough for the compiler to check the call, as `pow`'s declaration was in
chapter 1. The object files record the difference. Listing their names, as
chapter 1 did for `pow`, shows `U` (undefined, needed) in `main.o` and `T`
(code present) in `lj.o`:

```console
$ nm main.o | c++filt | grep LJ
                 U LJPotential::LJPotential(double, double, double)
                 U LJPotential::energy(double) const
$ nm lj.o | c++filt | grep LJ
0000000000000038 T LJPotential::LJPotential(double, double, double)
0000000000000000 T LJPotential::LJPotential(double, double, double)
000000000000007c T LJPotential::energy(double) const
```

(`c++filt` turns the compiler's internal names back into readable C++. The
constructor appears twice because the compiler emits two versions of it for
its own reasons; both are the same code here.)

Splitting pays off as the program grows. Changing the formula changes only
`lj.cpp`, so only `lj.o` is recompiled before linking again; every other
file's object file is reused. The single command
`clang++ -std=c++17 lj.cpp main.cpp -o pair` also works: it runs the same
steps without keeping the object files.

## A missing definition is a link error

Linking `main.o` alone leaves its needs unmatched:

```console
$ clang++ main.o -o pair
Undefined symbols for architecture arm64:
  "LJPotential::LJPotential(double, double, double)", referenced from:
      _main in main.o
  "LJPotential::energy(double) const", referenced from:
      _main in main.o
ld: symbol(s) not found for architecture arm64
clang++: error: linker command failed with exit code 1 (use -v to see invocation)
```

This error comes from `ld`, the linker, not from the compiler: every file
compiled, but a function that was declared was never defined in anything
given to the linker. The message names what is missing and who needs it.
The usual cause is a source file left out of the link command, or, in a
CMake project, left out of its file list.

## Include guards

Lines 1, 2 and 16 of `lj.h` are an **include guard**. A header is often
included more than once into the same source file, usually indirectly. In
the engine, for example, `lj_potential.h` includes both
`force_calculator.h` and `geometry.h`, and `force_calculator.h` includes
`geometry.h` again. Without a guard, the class would be pasted, and so
defined, twice. This copy of `lj.h` has its guard lines removed, and
`main.cpp` includes it twice:

```console
$ clang++ -std=c++17 -c main.cpp -o main.o
In file included from main.cpp:4:
./lj.h:3:7: error: redefinition of 'LJPotential'
    3 | class LJPotential {
      |       ^
main.cpp:3:10: note: './lj.h' included multiple times, additional include site here
    3 | #include "lj.h"
      |          ^
main.cpp:4:10: note: './lj.h' included multiple times, additional include site here
    4 | #include "lj.h"
      |          ^
./lj.h:3:7: note: unguarded header; consider using #ifdef guards or #pragma once
```

The guard works in the preprocessing stage, before any compiling.
`#ifndef LJ_H` means "only if the name `LJ_H` is not yet defined, keep the
lines up to `#endif`"; `#define LJ_H` then defines it. The first inclusion
keeps the class and defines `LJ_H`; every later inclusion finds `LJ_H`
defined and drops the whole header. `#pragma once` at the top of a header
does the same in one line; it is not part of the C++ standard, but every
major compiler supports it, as clang's note suggests. The engine uses guards:
`lj_potential.h` starts with `#ifndef LJ_POTENTIAL_H`, `geometry.h` with
`#ifndef GEOMETRY_H`.

## How the engine builds: CMake

Typing one command per file does not scale to dozens of files. The engine lists
its files in `CMakeLists.txt` at the repository root, and **CMake** turns
that list into the compile and link commands. Three of its entries:

```cmake
add_library(EngineLib STATIC
    src/engine/force_calculator.cpp
    src/engine/molecular_dynamics.cpp
    src/engine/potentials/lj_potential.cpp
    src/engine/potentials/lj_kernels.cpp
    src/engine/potentials/dft_potential.cpp
)
target_link_libraries(EngineLib PUBLIC BuilderLib)

add_executable(run_simulation
    src/tools/run_simulation.cpp
)
target_link_libraries(run_simulation PRIVATE BuilderLib EngineLib)
```

- **`add_library(EngineLib STATIC ...)`** compiles each listed source file
  into an object file and packs them into one **static library**, the file
  `libEngineLib.a`: a bundle of object files, ready to link. Listing its
  contents shows one object file per source file:

  ```console
  $ ar t build/libEngineLib.a
  __.SYMDEF
  force_calculator.cpp.o
  molecular_dynamics.cpp.o
  lj_potential.cpp.o
  lj_kernels.cpp.o
  dft_potential.cpp.o
  ```

  (`__.SYMDEF` is an index of the names the library has, for the linker.)
- **`add_executable(run_simulation ...)`** compiles the command-line tool's
  one source file, which holds its `main`.
- **`target_link_libraries`** gives the linker the libraries to match
  against: `run_simulation`'s needs are met by `BuilderLib` (geometry,
  builders, file readers) and `EngineLib` (the force calculator, the
  potentials, the MD loop).

A source file missing from `add_library` produces exactly the link error
above, naming the functions it should have provided. Headers are not listed:
they reach the compiler through `#include`.

## Check yourself

:::{dropdown} 1. You change the formula in lj.cpp. Which commands must run again before ./pair shows the change?
`clang++ -std=c++17 -c lj.cpp -o lj.o` and the link, `clang++ lj.o main.o -o pair`.
`main.o` does not change: `main.cpp` and `lj.h` are untouched.
:::

:::{dropdown} 2. main.cpp compiles without error but the link fails with "Undefined symbols ... LJPotential::energy". What is missing?
The definition of `energy`: the object file that has it, `lj.o`, was not
given to the linker, or `lj.cpp` does not define it. The declaration in `lj.h`
was enough to compile `main.cpp`, so the compiler could not notice.
:::

:::{dropdown} 3. Why does the engine's geometry.h need an include guard even though no file writes #include "geometry.h" twice?
Because it arrives twice indirectly: `lj_potential.h` includes
`force_calculator.h`, which includes `geometry.h`, and then includes
`geometry.h` itself. Without the guard, `Geometry` would be defined twice in
the same source file.
:::

## Next

This is the last chapter of the C++ module. The language you have now
(functions, vectors, classes, the plug, headers and linking) is the language
the engine is written in.

Every program in this module was linked into an executable with its own
`main`, started from the terminal. The home page starts the engine
differently: a Python script imports it and calls it. The
[pybind11 module](../pybind11/index.md) builds that bridge, first for the
`lj_energy` function of [Functions](02_functions.md), then for the engine's
classes.
