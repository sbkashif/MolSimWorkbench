# Functions

The program from [A first program](01_first_program.md) computes the LJ
energy at one distance. To get it at four distances, you would copy its two
formula lines four times, changing `r` in each copy; a slip in one copy would
go unnoticed. A **function** holds the formula once, under a name, and runs
it for any distance you hand it.

This chapter writes the formula as a function, gives it parameters for ε and
σ, and looks at what happens in memory when the function runs.

## Writing the formula once

```{literalinclude} examples/ch02/energies.cpp
:language: cpp
:linenos:
:emphasize-lines: 4-7
:caption: energies.cpp
```

Lines 4–7 are the function. {numref}`cpp-function-parts-figure` labels its
parts:

```{figure} ../figures/cpp_function_parts.svg
:name: cpp-function-parts-figure

The parts of a function definition.
```

- **Return type,** `double`: the kind of value the function hands back.
- **Name,** `lj_energy`: how the rest of the program calls it.
- **Parameter,** `double r`: a variable that belongs to the function and
  gets its value from each call.
- **Body,** lines 5–6 between the braces: the formula from chapter 1, now
  written once.
- **`return`**: ends the function and hands the value after it back to the
  caller.

`main` now only calls the function. `lj_energy(1.0)` means: run the body
with `r` set to 1.0, then use the returned value in place of the call.
Line 10 prints that value; lines 11–13 call the same function with other
distances:

```console
$ clang++ -std=c++17 energies.cpp -o energies
$ ./energies
U(1.0)  = 0
U(1.12) = -1
U(1.5)  = -0.320337
U(2.5)  = -0.0163169
```

The four values are the curve on the
[interactive plot](../molsim/architecture.md#the-energy-function): zero at
σ, −1 at the bottom of the well, and close to zero at the cutoff.

:::{admonition} If you know Python
:class: tip
The same function in Python is `def lj_energy(r):` with the body below it.
C++ also states the type of the parameter (`double r`) and of the result
(`double` before the name). The compiler then checks every call: calling
`lj_energy("far")` is a compile error, not a surprise when the program runs.
:::

## Define before use

The compiler reads a file from top to bottom, and it must have seen a
function before a line calls it. This version puts `lj_energy` after `main`:

```{literalinclude} examples/ch02/after_main.cpp
:language: cpp
:linenos:
:caption: after_main.cpp
```

```console
$ clang++ -std=c++17 after_main.cpp -o after_main
after_main.cpp:5:18: error: use of undeclared identifier 'lj_energy'
    5 |     std::cout << lj_energy(1.5) << "\n";
      |                  ^
1 error generated.
```

At line 5 the compiler has not yet met `lj_energy`, so the name means
nothing to it. This is also why `#include` lines go at the top: in chapter
1, the preview of the first stage showed that `#include <cmath>` places the
line

```cpp
extern double pow(double, double);
```

above your code. That line is a **declaration**: it tells the compiler that a
function `pow` exists, what it takes and what it returns, without its body.
A declaration is enough for a call to compile; the body comes from the
library at the link stage. You will write declarations of your own when the
program is split into several files, in a later chapter.

## Parameters for ε and σ

`lj_energy` assumes ε = σ = 1. Two more parameters let each call choose them:

```{literalinclude} examples/ch02/parameters.cpp
:language: cpp
:lines: 4-7
:lineno-start: 4
:caption: parameters.cpp (the function)
```

The body changes in two places: `sigma / r` replaces `1.0 / r`, and the
result is multiplied by `epsilon`. A call now passes three **arguments**, the
values for the three parameters, matched by position: the first goes to `r`,
the second to `epsilon`, the third to `sigma`.

```{literalinclude} examples/ch02/parameters.cpp
:language: cpp
:lines: 9-14
:lineno-start: 9
:caption: parameters.cpp (main)
```

```console
$ ./parameters
eps=1, sigma=1:   U(1.5) = -0.320337
eps=2, sigma=1:   U(1.5) = -0.640673
eps=1, sigma=1.2: U(1.5) = -0.773698
```

Doubling ε doubles the energy. Set the sliders of the interactive plot to
ε = 1 and σ = 1.2 and point at r = 1.5 to see the third value.

Calling the function with the wrong number of arguments is a compile error,
and the compiler shows the definition it expected:

```console
$ clang++ -std=c++17 wrong_count.cpp -o wrong_count
wrong_count.cpp:10:18: error: no matching function for call to 'lj_energy'
   10 |     std::cout << lj_energy(1.5) << "\n";
      |                  ^~~~~~~~~
wrong_count.cpp:4:8: note: candidate function not viable: requires 3 arguments, but 1 was provided
    4 | double lj_energy(double r, double epsilon, double sigma) {
      |        ^         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1 error generated.
```

:::{admonition} If you know Python
:class: tip
Python lets you name arguments in a call, as in `lj_energy(1.5, sigma=1.2)`.
C++ has no named arguments: every argument is matched by its position.
:::

## What a call does in memory

Chapter 1 showed that each variable has its own place in memory. A function's
parameters and variables are no exception, and they get their places when
the function is called. This version prints the address of `r` in `main` and
in `lj_energy`, and changes `r` inside the function:

```{literalinclude} examples/ch02/copies.cpp
:language: cpp
:linenos:
:emphasize-lines: 5-6, 13, 15
:caption: copies.cpp
```

```console
$ ./copies
in main: r is at 0x16b17e900
  inside lj_energy: r is at 0x16b17e8b8
in main: r is still 1.5
```

The two `r`s are at different addresses: they are two variables that happen
to share a name. The call **copies** the value of `main`'s `r` into the
function's own `r` ({numref}`cpp-call-figure`). Line 6 changes the copy, so
`main`'s `r` stays 1.5. When the function returns, its result is copied back
into `energy`, and the function's variables are released; the next call
gets a fresh set.

```{figure} ../figures/cpp_call.svg
:name: cpp-call-figure

One call of `lj_energy`. The arguments are copied into the function's own
variables; the result is copied back into `main`'s `energy`.
```

Copying a few numbers costs nothing. Copying a list of 512 atoms on every
call would; a later chapter shows how a function can work on the caller's
variable directly instead of on a copy.

:::{admonition} If you know Python
:class: tip
For numbers, Python behaves the same way: assigning `r = 99.0` inside a
function does not change the caller's `r`.
:::

## Check yourself

:::{dropdown} 1. What does lj_energy(2.0, 1.0, 1.0) return?
`-0.0615234`. At 2σ the pair is past the bottom of the well, on the long,
shallow part of the curve.
:::

:::{dropdown} 2. You meant ε = 1.2 and σ = 1.0 at r = 1.5, but wrote lj_energy(1.0, 1.2, 1.5). Does the compiler catch it, and what do you get?
The compiler accepts it: all three arguments are `double`s, so the types
match. The function receives r = 1.0, ε = 1.2 and σ = 1.5, and returns
`568.107`, a pair pushed deep into the wall. The intended call
`lj_energy(1.5, 1.2, 1.0)` gives `-0.384404`. Arguments are matched only by
position, so their order is yours to get right.
:::

:::{dropdown} 3. In copies.cpp, line 6 sets r to 99.0. Why does main still print 1.5?
The call copied `main`'s `r` into the function's own `r`, a separate
variable at a different address. Line 6 changes only that copy.
:::

:::{dropdown} 4. after_main.cpp fails with "use of undeclared identifier". Name two ways to fix it.
Move the whole function above `main`; or keep it below and put a
declaration, `double lj_energy(double r);`, above `main`. The declaration is
the same kind of line that `#include <cmath>` provides for `pow`.
:::

## Next

`main` still lists every distance by hand. The next chapter,
[Vectors and loops](03_vectors_and_loops.md), stores many values in one
variable, so that a few lines can work through all of them.
