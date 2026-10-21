# Many values: vectors and loops

At the end of [Functions](02_functions.md), `main` still names every
distance by hand: one line per call. A simulation has hundreds of atoms and
thousands of distances. This chapter keeps many values in one variable, a
**vector**, and works through all of them with a **loop**. It ends with the
positions of several atoms, stored the way the engine stores them.

## A vector holds many values

```{literalinclude} examples/ch03/table.cpp
:language: cpp
:lines: 10-17
:lineno-start: 10
:emphasize-lines: 2-3
:caption: table.cpp (main; lj_energy as in chapter 2)
```

- **Line 3 of the file,** `#include <vector>`, brings in the standard
  library's vector, as `<cmath>` brought in `pow`.
- **Line 11** makes `distances`, a `std::vector<double>`: a vector whose
  elements are all `double`s. The type in the angle brackets is the type of
  every element; a vector holds one type only. The braces give its first
  four elements.
- **Line 12** reads one element and the count. Elements are numbered from 0,
  so `distances[0]` is the first one, 1.0, and `distances[3]` is the last.
  `distances.size()` is the number of elements, 4. The dot means "ask
  `distances` for": `size` is a function that belongs to the vector, a kind
  the next chapter covers.

```console
$ ./table
first: 1, count: 4
U(1) = 0
U(1.12246) = -1
U(1.5) = -0.320337
U(2.5) = -0.0163169
```

## A loop runs the same lines for every element

Lines 13–15 are a **loop**:

```cpp
    for (double r : distances) {
        std::cout << "U(" << r << ") = " << lj_energy(r, 1.0, 1.0) << "\n";
    }
```

Read it as "for each `r` in `distances`, run the lines in the braces". The
first time through, `r` is `distances[0]`; the second time, `distances[1]`;
and so on until the last element. The four printed lines above come from
this one call line. Adding a fifth distance to line 11 adds a fifth line of
output, with no other change.

:::{admonition} If you know Python
:class: tip
`for (double r : distances)` is C++ for `for r in distances:`. The braces do
the job of Python's indentation. A Python list may mix numbers and text; a
`std::vector<double>` holds only `double`s, which is what lets the compiler
place them side by side in memory (below).
:::

## Building a vector, and finding the lowest energy

A vector can start empty and grow. This program fills it with 211 distances,
from 0.9σ to 3.0σ in steps of 0.01σ, and then looks for the distance with
the lowest energy:

```{literalinclude} examples/ch03/scan.cpp
:language: cpp
:lines: 10-29
:lineno-start: 10
:caption: scan.cpp (main)
```

- **Line 11** makes an empty vector.
- **Lines 12–14** are the second kind of loop, a counting loop:
  `for (int i = 0; i <= 210; i++)` sets `i` to 0, runs the braces as long as
  `i <= 210` holds, and adds 1 to `i` (`i++`) after each pass. `int` is the
  type for whole numbers. `push_back` adds one element at the end of the
  vector, so after the loop it holds 211 elements.
- **Line 16** prints the last element as `distances[distances.size() - 1]`:
  with 211 elements numbered from 0, the last is number 210.
- **Lines 18–26** walk through every distance and keep the lowest energy
  seen so far. `if (u < best_u) { ... }` runs its braces only when the
  condition holds.

```console
$ ./scan
211 distances, from 0.9 to 3
lowest energy -0.999824 at r = 1.12
```

The scan finds the bottom of the well at 1.12σ, the
$2^{1/6}\sigma \approx 1.1225\sigma$ of the
[architecture page](../molsim/architecture.md#the-energy-function), to the
0.01σ spacing of the scan; the energy there is within 0.0002ε of −ε.

## Where the elements live

[A first program](01_first_program.md) printed the addresses of three
variables of `main` and found them 8 bytes apart. A vector's elements are
also side by side, 8 bytes apart for `double`s:

```{literalinclude} examples/ch03/side_by_side.cpp
:language: cpp
:lines: 6-9
:lineno-start: 6
:caption: side_by_side.cpp (the loop)
```

```console
$ ./side_by_side
distances[0] = 1 at 0x600003e49200
distances[1] = 1.12246 at 0x600003e49208
distances[2] = 1.5 at 0x600003e49210
distances[3] = 2.5 at 0x600003e49218
```

The counting loop's `i` has type `std::size_t`, the type `size()` returns: a
whole number that cannot be negative.

The addresses start with `0x6000...`, not the `0x16...` of chapter 1's
variables. The vector variable itself is small (24 bytes on this Mac) and
sits with `main`'s other variables; it records where its elements are and
how many there are. The elements live in another part of memory, which the
vector asks the operating system for while the program runs, and can ask
for more of when `push_back` needs room ({numref}`cpp3-vector-figure`).

```{figure} ../figures/cpp3_vector.svg
:name: cpp3-vector-figure

The vector `distances`: a 24-byte record among `main`'s variables, and its
four elements side by side elsewhere in memory, 8 bytes apart.
```

## Reading past the end

`distances` has four elements, numbered 0 to 3. Asking for number 10 is a
mistake, and the two ways of reading an element handle it differently:

```{literalinclude} examples/ch03/past_end.cpp
:language: cpp
:lines: 6-7
:lineno-start: 6
:caption: past_end.cpp
```

```console
$ ./past_end
distances[10]    = 0
libc++abi: terminating due to uncaught exception of type std::out_of_range: vector
distances.at(10) = 
```

`distances[10]` does not check: it reads whatever lies in memory where
element 10 would be, and the program carries on with that value. Here it
printed 0; another run or another computer may print anything, or crash. The
C++ standard calls this **undefined behaviour**: the language makes no
promise about what happens. `distances.at(10)` checks the number first and
stops the program with an `out_of_range` error. (The two lines of output
appear in this order because the error message is printed at once, while
`std::cout` holds back its last line until the program ends.)

`[ ]` is faster and is what the engine uses inside its loops; `.at()` is the
safer choice while a program is being written.

:::{admonition} If you know Python
:class: tip
In Python, `distances[10]` always raises an `IndexError`, and
`distances[-1]` is the last element. In C++ `[ ]` never checks, and there
are no negative numbers for counting from the end: the last element is
`distances[distances.size() - 1]`.
:::

## Positions of atoms

An atom's position is three numbers, x, y and z. `std::array<double, 3>`
holds exactly three `double`s; unlike a vector, its size is fixed when it is
made and is part of its type. A vector of them holds the positions of all
atoms ({numref}`cpp3-positions-figure`).

```{figure} ../figures/cpp3_positions.svg
:name: cpp3-positions-figure
:width: 620px

`std::vector<std::array<double, 3>>`: one `std::array<double, 3>` per atom.
Two numbers pick one value: which atom, then which coordinate.
```

This program places three atoms, computes the distance of every pair, and
adds up their LJ energies:

```{literalinclude} examples/ch03/atoms.cpp
:language: cpp
:lines: 11-36
:lineno-start: 11
:caption: atoms.cpp (lj_energy as before)
```

- **Lines 11–16** are a function that takes two positions and returns the
  distance between them, $\sqrt{\Delta x^2 + \Delta y^2 + \Delta z^2}$.
  `std::sqrt` comes from `<cmath>`; `<array>` provides `std::array`.
- **Lines 19–23** place atom 0 at the origin, atom 1 at x = 1.5 and atom 2 at
  y = 1.2.
- **Lines 25–26** are two loops, one inside the other. The outer loop picks
  atom `i`; the inner loop starts at `j = i + 1`, so each pair is visited
  once: 0–1, 0–2, 1–2, and never 1–0 again.
- **Line 31,** `total += u`, is short for `total = total + u`.

```console
$ ./atoms
pair 0-1: r = 1.5, U = -0.320337
pair 0-2: r = 1.2, U = -0.890965
pair 1-2: r = 1.92094, U = -0.0780279
total U = -1.28933
```

This is the sum over pairs from the
[architecture page](../molsim/architecture.md#the-energy-function), for
three atoms. With N atoms the two loops visit $N(N-1)/2$ pairs, so the work
grows with the square of N: 512 atoms make 130,816 pairs.

### Where the engine uses it

The engine stores positions in exactly this type. Its class `Geometry` keeps
the atom positions as a `std::vector<std::array<double, 3>>`, and
`ForceCalculator::calculateForces` fills a variable of the same type with
the force on every atom, one `std::array<double, 3>` of x, y and z
components per atom. (These are written with `&` and `const` in the engine,
which later chapters explain.)

## Check yourself

:::{dropdown} 1. A vector has 512 elements. What is the number of its last element, and what does distances.at(512) do?
511, because counting starts at 0. `distances.at(512)` stops the program
with an `out_of_range` error; `distances[512]` would read past the end
without any check.
:::

:::{dropdown} 2. In scan.cpp, the loop runs i from 0 to 210. Which distance does i = 22 give, and what is the energy there?
0.9 + 0.01 × 22 = 1.12, the distance the scan reports as the lowest, with
energy −0.999824.
:::

:::{dropdown} 3. In atoms.cpp, why does the inner loop start at j = i + 1 and not at j = 0?
Starting at 0 would visit every pair twice (0–1 and 1–0) and would also pair
each atom with itself (0–0), at distance 0, where the LJ formula divides by
zero. Starting at `i + 1` visits each pair exactly once.
:::

:::{dropdown} 4. How many pairs do the two loops visit for 4 atoms?
6: 0–1, 0–2, 0–3, 1–2, 1–3, 2–3, which is 4 × 3 / 2.
:::

## Next

`lj_energy` takes ε and σ on every call, and the positions sit in a loose
vector. The next chapter bundles values and the functions that work on them
into one new type, a **class**, the building block the engine is made of.
