# Chapter 2, plain version — variables and types, told as a robot kitchen

This is a plain-language retelling of [02_variables_and_types.md](02_variables_and_types.md). Same
content, same section numbers, one picture throughout: the robot kitchen from chapter 1. Sections are step tables with three
columns, **Kitchen | C++ | Example**. For full compiler output and exact line numbers, open the technical
version. Code: [`examples/02/`](examples/02/).

**After this chapter you can say:**

- why a C++ variable is a jar on the robot's shelf with its label painted on, and why the label can never be repainted;
- what the four jar sizes this course uses are, and what goes in each;
- why "1 cup divided by 2" can come out as zero cups in C++;
- what a jar sealed with clear tape (`const`) is, and what "same size as that one" (`auto`) means;
- why an empty-looking jar can hold leftovers from an earlier job;
- how mini_lj works out the pull between two atoms.

## Where we are

Chapter 1 ended on one sentence: in C++ every ingredient sits in a jar on the shelf with its label painted
on — `double x = 1.0;` *is* that jar. So far our recipes only put words on the serving tray; none of them
measured anything. This chapter makes the jar picture concrete and uses it for the first real
measurement: how strongly two atoms 1.2 apart pull on each other.

## The kitchen, once

Chapter 1's kitchen (recipe = `.cpp`, translator = compiler, binder = linker, the robot cooking from the
finished book = the running program) gets a shelf of jars the robot works from.

| Kitchen | C++ |
|---|---|
| a **jar on the shelf with its label painted on**: the name, and what it may hold | a variable |
| the jar's size and shape (8 cups, 4 cups, 1 cup) | the type: `double`, `int`, `bool`, `std::size_t` |
| a jar sealed with clear tape: you can look, not change | `const` |
| "make this jar the same kind as the thing going in it" | `auto` |

---

## 2.0 A jar with its label painted on

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the robot puts a jar on the shelf, the label `x — holds one double` is painted on, and the jar is filled | declare and initialise | `double x = 1.0;` |
| 2 | putting something new in empties the jar and refills it. The jar stays where it is; the label stays the same | assignment overwrites | `x = 2.0;` |
| 3 | trying to pour a sentence into a jar painted "double" — the translator refuses before any cooking | type error at compile time | `x = "hello";` → `error: assigning to 'double' from incompatible type 'const char[6]'` |

Why this matters for everything after: if `x` *is* a jar, then `y = x` cannot mean "`y` is another name
for `x`'s jar". It has to mean "put out a second jar labelled `y` and pour a copy in". That is chapter 3.

---

## 2.1 The four jar sizes

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | an 8-cup jar for measured amounts with a decimal point | `double` (8 bytes) | positions, energies, ε, σ |
| 2 | a 4-cup jar for whole counts | `int` (4 bytes) | `int numAtoms = 2;` |
| 3 | a 1-cup jar that holds yes or no | `bool` (1 byte) | `bool tailCorrection = false;` |
| 4 | an 8-cup jar for "how many" and "which position", never negative | `std::size_t` (8 bytes) | sizes of containers (chapter 4) |
| 5 | ask how big a jar is | `sizeof` | `sizeof(double)` prints `8 bytes` |

### The never-negative jar (skip if you will just copy the repo's choice)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the `std::size_t` jar is a counter that only goes up from zero | unsigned | `std::size_t zero = 0;` |
| 2 | take one away from an empty counter and it rolls over to its largest number, like an old car odometer | wraparound | `zero - 1` prints `18446744073709551615` |
| 3 | no alarm goes off | no error, no warning | remember it in chapter 4: `positions.size() - 1` |

---

## 2.2 Filling the jar when you put it out

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | put the jar out and fill it at once | initialise with `=` | `double sigma = 1.0;` |
| 2 | fill it with braces: this form refuses to spill anything | brace initialisation refuses narrowing | `int n{2.9};` → error: `cannot be narrowed to 'int'` |
| 3 | put a jar out **without filling it** | no initial value | `double energy;` |
| 4 | that jar still holds whatever an earlier job left in that spot on the shelf | the memory is not cleared | |
| 5 | add 1.5 cups to the leftovers | `energy += 1.5;` | the translator warns: `variable 'energy' is uninitialized when used here` |
| 6 | look at the result: the book translated for speed gave `nan`, the plain translation gave `1.5`. Same recipe, different result | undefined behaviour | `-O2`: `energy = nan`; `-O0`: `energy = 1.5` |

The habit: **always fill a jar when you put it out.** The repo does: `double totalEnergy = 0.0;`.

---

## 2.3 Dividing: the whole-cups trap

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | dividing two whole-cup jars gives whole cups; anything left over is thrown away | `int / int` is integer division | `1 / 2` prints `0`; `7 / 2` prints `3` |
| 2 | if either amount has a decimal point, you get an exact split | `double / int` → ordinary division | `1.0 / 2` prints `0.5` |
| 3 | thrown away means "toward zero", not "down" | truncation | `-7 / 2` prints `-3`, not `-4` |
| 4 | the remainder | `%` | `7 % 2` prints `1` |
| 5 | squeezing 2.9 cups into a whole-cup jar drops the .9; the translator warns | implicit conversion | `int truncated = 2.9;` → `2`, warning `changes value from 2.9 to 2` |
| 6 | to do it on purpose, say so | `static_cast<int>(x)` | used in chapter 4 |

**Our kitchen's habit:** every "half" in the repo is written `0.5`, never `1/2`. `1/2` is two whole-cup jars,
so it is `0`, and `1/2 * dt * dt` would be zero. `src/engine/integrators/velocity_verlet_integrator.cpp:7` writes
`0.5 * dt * dt`; the arithmetic mixing rule in `lj_potential.cpp:350-351` writes `0.5 * (… + …)`.

---

## 2.4 `const`: a jar sealed with clear tape

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | fill the jar, then seal it with clear tape: anyone can see inside, nobody can change it | `const` | `const double cutoff = 2.5;` |
| 2 | someone tries to refill it; the translator stops them | compile error | `cutoff = 3.0;` → `cannot assign to variable 'cutoff' with const-qualified type` |
| 3 | the tape costs nothing while cooking; only the translator checks it | no run-time cost | |

Seen before: `const std::string configPath = argv[1];` in `run_simulation.cpp:16`.

---

## 2.5 `auto`: "same kind of jar as what goes in"

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | "paint the label to match whatever I am pouring in" | `auto` | `auto b = 2.0;` → a `double` jar |
| 2 | the label is painted **once**, by the translator; it never changes after | still fixed | |
| 3 | pour in a whole number and you get a whole-cup jar, with its division trap | type comes from the right side | `auto a = 2;` then `a / 4` prints `0` |
| 4 | the repo uses it when the label would be long and obvious | | `const auto& atoms = geometry.getAtoms();` |

---

## 2.6 The maths drawer: `<cmath>`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | C++ has no "to the power" key; you open the maths drawer | `#include <cmath>` | |
| 2 | "raise to the 6th" | `std::pow(x, 6)` | (σ/r)⁶ |
| 3 | square root, cube root, rounding | `std::sqrt`, `std::cbrt`, `std::round` | chapters 3 and 4 |
| 4 | the drawer's rounding sends halves away from zero | `std::round(2.5)` is 3; `std::round(-2.5)` is -3 | |

---

## 2.7 How many digits go on the serving tray

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the robot writes amounts on the serving tray to 6 digits unless told otherwise | default `std::cout` precision | `1/3` prints `0.333333` |
| 2 | "from now on, write 12 digits" | `std::cout.precision(12);` | `0.333333333333` |
| 3 | the jar's contents did not change, only how many digits were written on the tray | printing only | |

That is why mini_lj prints `-2.21169` while learn-parallel lesson 00 writes `−2.211693`: the same amount,
written out to a different number of digits.

---

## 2.8 Floating-point numbers are not exact (deep dive later)

Omitted from the plain version. The technical version shows that adding the same three numbers in a
different order can give a different answer, which matters once several robots add up a total together
(learn-parallel lesson 04).

---

## 2.9 The mini_lj step: two atoms, one distance

| # | Kitchen | C++ (`examples/02/mini_lj.cpp`) | Example |
|---|---|---|---|
| 1 | open the maths drawer and glue in the serving-tray page | `#include <cmath>`, `#include <iostream>` | lines 2–3 |
| 2 | three 8-cup jars, filled at once: well depth, atom size, distance | `double epsilon = 1.0; double sigma = 1.0; double r = 1.2;` | lines 6–8 |
| 3 | size over distance, to the 6th | `double sr6 = std::pow(sigma / r, 6);` | line 10 |
| 4 | that, squared, for the 12th | `double sr12 = sr6 * sr6;` | line 11 |
| 5 | the energy recipe, with decimal-point numbers | `4.0 * epsilon * (sr12 - sr6)` | line 12 |
| 6 | the pull recipe | `24.0 * epsilon * (2.0 * sr12 - sr6) / r` | line 15 |
| 7 | a whole-cup jar for the count | `int numAtoms = 2;` | line 17 |
| 8 | translate, bind, let the robot cook | `c++ -std=c++17 -Wall -O2 mini_lj.cpp -o out/mini_lj` then `./out/mini_lj` | |
| 9 | the result | `E(1.2) = -0.890965`, `F(1.2) = -2.21169` | negative pull: the atoms draw together |

Same numbers as learn-parallel lesson 00's
[Two actual atoms](../learn-parallel/00_starting_point.md#two-actual-atoms).

---

## 2.10 In the real kitchen

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the repo's own "energy of one pair" recipe | `calculatePairEnergy` | `src/engine/potentials/lj_potential.cpp:360-367` |
| 2 | same three jars, longer names | `sigma_r`, `sigma_r6`, `sigma_r12` | lines 363–365 |
| 3 | same final line, same `4.0` | `return 4.0 * epsilon * (sigma_r12 - sigma_r6);` | line 367 |
| 4 | the pull recipe right below it | `forceMagnitude = 24.0 * …` | line 380 |
| 5 | jars of each size, each filled when put out | `double cutoff = 10.0;`, `int numThreads = 0;`, `bool tailCorrection = false;` | `lj_potential.h:71`, `:75`, `:76` |

---

## Next

mini_lj now measures one pair, but the distance 1.2 was written in by hand. Real atoms have
positions — three numbers each — and the distance has to be worked out from them, across a box that wraps
around. To hand positions to other recipes we need to know what happens when one jar is "given" to
another: since a variable *is* its jar, C++ pours a copy into a new jar by default. Chapter 3 is about
copies (a second jar), references (a second label stuck on the same jar) and pointers (an index card with
a shelf position written on it).
