# Chapter 2 — Variables and types: a variable *is* its storage

Plain version: [02p_variables_and_types_plain.md](02p_variables_and_types_plain.md).
Code: [`examples/02/`](examples/02/) — `mini_lj.cpp` plus small toy programs. Every output below was
produced by compiling and running them with Apple clang 17 on an M1 Mac.

**After this chapter you can say:**

- why `double x = 1.0;` makes a fixed-size piece of memory that can only ever hold a `double`;
- what `double`, `int`, `bool` and `std::size_t` are, how big each one is, and which one to reach for;
- why `1 / 2` is `0` in C++, and why the repo writes `0.5 * dt * dt` and never `1/2 * dt * dt`;
- what `const` and `auto` do, and what they do *not* do;
- why a variable with no starting value is a bug the compiler can only sometimes warn you about;
- how mini_lj computes the Lennard-Jones energy and force for two atoms, and where the same formula lives
  in `lj_potential.cpp`.

## Where we are

Chapter 1 ended on one sentence: in C++, `double x = 1.0;` *is* an 8-byte box with a type painted on it.
Every program in chapter 1 only moved text around; none of them held a number or computed anything. This
chapter makes that sentence concrete, and uses it for mini_lj's first calculation: the Lennard-Jones energy
and force between two atoms at distance 1.2.

Time: about 30 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/02
mkdir -p out          # compiled programs go here (git-ignored)
```

All compile commands in this chapter use `-Wall` ("turn on most warnings") and `-O2` (optimise, as the
repo's Release build does). Warnings matter in this chapter: several of C++'s number traps are only caught
as warnings.

---

## 2.0 A variable is a box

In C++, the name and the storage are one thing. `double x = 1.0;` does three things at once:

1. sets aside 8 bytes of memory (the size of a `double`);
2. records, **for the rest of the program**, that those 8 bytes hold a `double` and are called `x`;
3. writes the value `1.0` into them.

`x = 2.0;` later does not move anything. It overwrites the 8 bytes. And `x = "hello";` is not a
rebinding at all — it would mean "write a piece of text into a box built for a `double`". The compiler
refuses. `examples/02/fixed_type.cpp` tries it (and two more things this chapter covers):

```cpp
5      double x = 1.0;
6      x = "hello";            // error: x is a double, forever
7      const double cutoff = 2.5;
8      cutoff = 3.0;           // error: const cannot be assigned again
9      int n{2.9};             // error: brace initialisation refuses to lose the .9
```
```
$ c++ -std=c++17 -Wall -O2 fixed_type.cpp -o out/fixed_type
fixed_type.cpp:6:9: error: assigning to 'double' from incompatible type 'const char[6]'
    6 |     x = "hello";            // error: x is a double, forever
      |         ^~~~~~~
fixed_type.cpp:8:12: error: cannot assign to variable 'cutoff' with const-qualified type 'const double'
    8 |     cutoff = 3.0;           // error: const cannot be assigned again
      |     ~~~~~~ ^
fixed_type.cpp:7:18: note: variable 'cutoff' declared const here
...
fixed_type.cpp:9:11: error: type 'double' cannot be narrowed to 'int' in initializer list [-Wc++11-narrowing]
...
1 warning and 3 errors generated.
```

(`...` marks lines cut from the full output.) Line 6 is today's topic; lines 8 and 9 come back in §2.2 and
§2.4. `'const char[6]'` is how C++ names the text `"hello"`: six characters, the five letters plus a hidden
end marker.

| Question | C++ answer |
|---|---|
| what `x` is | the **storage** itself: a fixed-size piece of memory |
| where the type lives | on the variable; fixed when the program is compiled |
| `x = <new value>` | overwrites the bytes in place |
| `x = "hello"` after `double x = 1.0;` | compile error |
| when a type mistake is found | before anything runs (chapter 1, §1.0) |

A **type** is the compiler's answer to two questions about a piece of memory: how many bytes it takes, and
what the bits in those bytes mean. Writing the type in front of the name is called a **declaration** of the
variable. (This is the same word as chapter 1's function declaration: "this name exists, and here is its
type.") A language where every variable's type is fixed at compile time is called **statically typed**.

> **Unlike Python:** there a name and the object it names are separate. `x = 1.0` ties the name `x` to a
> float object stored elsewhere, and `x = "hello"` just moves the tag to a string, so `type(x)` can change
> from line to line. The type travels with the object and is checked only when the code runs (**dynamically
> typed**).

**What it buys:** the compiler knows the exact size and meaning of every variable, so it can lay them out
in memory and emit the exact machine instruction for `x * y` (a floating-point multiply) with no checking
at run time. That is a large part of why tight numeric loops in C++ are fast.
**What it costs:** you must say what every variable is, and you cannot reuse one name for different kinds
of thing.

This one difference — the variable *is* the box — is what chapter 3 builds on: if `x` is a box, then
`y = x` has to mean "make another box and copy the contents".

---

## 2.1 The four types this course uses

C++ has many number types. This repo, and this course, use four.

| Type | Holds | Size here | Used for |
|---|---|---|---|
| `double` | a real number, about 16 significant digits | 8 bytes | positions, energies, forces, ε, σ |
| `int` | a whole number, roughly ±2.1 billion | 4 bytes | small counts, loop counters over x/y/z |
| `bool` | `true` or `false` | 1 byte | flags, e.g. `tailCorrection` |
| `std::size_t` | a whole number **≥ 0**, very large | 8 bytes | sizes and indices of containers |

`sizeof(x)` asks the compiler how many bytes `x` takes. `examples/02/types.cpp` prints it:

```bash
c++ -std=c++17 -Wall -O2 types.cpp -o out/types
./out/types
```
```
sizeof(double)      = 8 bytes
sizeof(int)         = 4 bytes
sizeof(bool)        = 1 byte
sizeof(std::size_t) = 8 bytes
```

(The compile step also prints one warning; it belongs to §2.3 and is shown there.)

A few notes on those names:

- **`double`** is short for "double-precision floating point". There is also `float` (4 bytes, about 7
  digits); this repo never uses it for physics.
- **`std::size_t`** lives in the `std` namespace like `std::cout`, and comes from the header `<cstddef>`.
  It is **unsigned** — it cannot be negative. It is the type the standard library uses for "how many
  things" and "which position". The repo sometimes writes it without `std::` (`size_t`); on this compiler
  both spellings name the same type.
- A **literal** is a value written directly in the code. `2` is an `int` literal; `2.0` is a `double`
  literal; `true` is a `bool` literal. The decimal point decides. This matters a lot in §2.3.

### Why `std::size_t` and not `int` (skip if you will just copy the repo's choice)

A container in chapter 4 reports its size as a `std::size_t`. Using the same type for your loop index
avoids mixing a signed and an unsigned number in one comparison, which compilers warn about (chapter 4
shows the warning). But unsigned numbers have one trap
that `int` does not: they cannot go below zero, so subtracting past zero **wraps around** to the largest
possible value. From the same run of `types.cpp`:

```cpp
std::size_t zero = 0;
std::cout << "zero - 1  = " << zero - 1 << std::endl;   // unsigned: wraps around
```
```
zero - 1  = 18446744073709551615
```

That is 2⁶⁴ − 1. No error, no warning. Keep it in mind for chapter 4, where `positions.size() - 1` appears.

> **Unlike Python:** an `int` there has no fixed size. It grows as needed and never wraps or overflows, so
> the size of a whole number is something Python hides completely. In C++ every integer type has a fixed
> width, and running off either end is your problem.

---

## 2.2 Giving a variable its first value

**Initialisation** means giving a variable its value at the moment it is created. C++ has several
spellings; the repo uses two.

| Spelling | Example | Notes |
|---|---|---|
| `T name = value;` | `double sigma = 1.0;` | the usual one |
| `T name{value};` or `T name = {…};` | `std::array<double, 3> b = {0.0, 0.0, 0.0};` | **brace initialisation**; used for groups of values (chapter 3) and it refuses to lose information (below) |
| `T name;` | `double energy;` | **no value at all** — see below |

Brace initialisation refuses a **narrowing** conversion — one that would lose information, like putting
`2.9` into an `int`. That was the third error in §2.0:

```
fixed_type.cpp:9:11: error: type 'double' cannot be narrowed to 'int' in initializer list [-Wc++11-narrowing]
```

The `=` form would have accepted it with only a warning and silently kept `2` (§2.3).

### A variable with no value holds whatever was there

C++ lets you declare `double energy;` with no value. The 8 bytes are set aside, but **nothing is written into them**.
They hold whatever bits were left in that memory by earlier code. `examples/02/uninit.cpp`:

```cpp
5      double energy;              // no "= 0.0"
6      energy += 1.5;              // adds to garbage
7      std::cout << "energy = " << energy << std::endl;
```

`energy += 1.5` means `energy = energy + 1.5`. Compile and run it:

```
$ c++ -std=c++17 -Wall -O2 uninit.cpp -o out/uninit
uninit.cpp:6:5: warning: variable 'energy' is uninitialized when used here [-Wuninitialized]
    6 |     energy += 1.5;              // adds to garbage
      |     ^~~~~~
uninit.cpp:5:18: note: initialize the variable 'energy' to silence this warning
    5 |     double energy;              // no "= 0.0"
      |                  ^
      |                   = 0.0
1 warning generated.
$ ./out/uninit
energy = nan
```

Now compile the same file without optimisation (`-O0`) and run it:

```
$ c++ -std=c++17 -O0 uninit.cpp -o out/uninit0
$ ./out/uninit0
energy = 1.5
```

Same source, two different answers. Reading a variable before giving it a value is **undefined
behaviour**: the C++ rules say nothing about what happens, so the compiler may produce anything, and
different settings produce different things. `nan` ("not a number") is what the optimised build happened
to give. The unoptimised build happened to find zero bits in that memory, which looks correct and is the
most dangerous outcome of all.

Here the compiler warned, because the mistake was two lines apart. In longer code it often cannot. The
habit that prevents it: **always give a variable a value when you create it.** The repo does this
everywhere an accumulator starts — `double totalEnergy = 0.0;` at `src/engine/potentials/lj_potential.cpp:278`.

> **Unlike Python:** a name there cannot exist without an object; using one before assigning it raises
> `NameError`, every time. C++ will happily hand you the leftover bits, and the result can even look right.

---

## 2.3 Arithmetic: the integer division trap

Arithmetic on `double`s uses `+ - * /`, with the usual precedence. There is no `**`
operator (§2.6 covers powers). The trap is **division of two integers**. `examples/02/types.cpp` again:

```cpp
std::cout << "1 / 2     = " << 1 / 2 << std::endl;
std::cout << "1.0 / 2   = " << 1.0 / 2 << std::endl;
std::cout << "7 / 2     = " << 7 / 2 << std::endl;
std::cout << "7 % 2     = " << 7 % 2 << std::endl;
std::cout << "-7 / 2    = " << -7 / 2 << std::endl;
```
```
1 / 2     = 0
1.0 / 2   = 0.5
7 / 2     = 3
7 % 2     = 1
-7 / 2    = -3
```

The rule: **the types of the two operands decide the operation.** `int / int` is integer division: the
answer is an `int`, and the fractional part is thrown away. If either side is a `double`, the other side is
converted to `double` first and you get ordinary division.

| Expression | Result | Why |
|---|---|---|
| `1 / 2` | `0` | both `int`: integer division |
| `1.0 / 2` | `0.5` | one side is `double`: ordinary division |
| `-7 / 2` | `-3` | integer division drops the fraction: rounds **toward zero** |
| `7 % 2` | `1` | `%` is the remainder of integer division |

> **Unlike Python:** in Python 3, `/` always gives a float (`1 / 2` is `0.5`), and integer division is a
> separate operator, `//`, which rounds **down** (`-7 // 2` is `-4`). In C++ the operand types choose the
> operation silently, and negative results round toward zero, so a line that looks like Python's `/` can be
> integer division and does not even match Python's `//`.

### Our lines: why the repo writes `0.5 *`

Every "half" in the repo is written as the `double` literal `0.5`:

```cpp
// src/engine/integrators/velocity_verlet_integrator.cpp:7
const double dtSquaredHalf = 0.5 * dt * dt;

// src/engine/potentials/lj_potential.cpp:358-359  (arithmetic mixing rule)
mixedEpsilon = 0.5 * (params1.epsilon + params2.epsilon);
mixedSigma = 0.5 * (params1.sigma + params2.sigma);
```

If someone "tidied" that line to `1/2 * dt * dt`, the compiler would evaluate `1/2` first — two `int`s —
get `0`, and every position update would silently use zero. No error, no warning. Writing `0.5`, or
`1.0 / 2.0`, makes the number a `double` from the start.

The same rule protects mini_lj's formula. `4.0 * epsilon` and `24.0 * epsilon` use `double` literals. Here
`epsilon` is already a `double`, so `4 * epsilon` would also work — but writing `4.0` means the line is
right no matter what the other operands are.

### A double squeezed into an int

Going the other way, putting a `double` into an `int` drops the fractional part. `types.cpp` does it on
line 22, and this is the warning its compile step printed:

```
types.cpp:22:21: warning: implicit conversion from 'double' to 'int' changes value from 2.9 to 2 [-Wliteral-conversion]
   22 |     int truncated = 2.9;   // a double squeezed into an int: the .9 is dropped
      |         ~~~~~~~~~   ^~~
```
```
int truncated = 2.9 -> 2
```

An **implicit conversion** is one the compiler does for you without being asked. When you *want* the
conversion, write it out: `static_cast<int>(x)` means "convert `x` to `int`, and I mean it". Chapter 4
uses exactly that to turn a cube root into a whole number of atoms per side.

---

## 2.4 `const`: a box that cannot be written again

`const` in front of a type means: this variable gets its value when it is created, and can never be
assigned again. That was the second error in §2.0:

```
fixed_type.cpp:8:12: error: cannot assign to variable 'cutoff' with const-qualified type 'const double'
```

You have already seen it: `const std::string configPath = argv[1];` at `src/tools/run_simulation.cpp:16`.

**What it buys:** the reader knows at a glance that the value never changes after this line, and the
compiler holds you to it. **What it costs:** nothing at run time. `const` is checked by the compiler and
leaves no trace in the program.

`const` matters much more in chapter 3, where it decides whether a function may change what you hand it.
mini_lj chapter 2 does not use it yet; chapter 3's version marks every value that never changes.

---

## 2.5 `auto`: let the compiler write the type

`auto` in place of a type means "give this variable the type of whatever is on the right". It is decided
**once, when compiling**, and then fixed like any other type. It does not make C++ dynamic.
`examples/02/auto_demo.cpp`:

```cpp
auto a = 2;       // int
auto b = 2.0;     // double
auto c = a / 4;   // int / int -> int
auto d = b / 4;   // double / int -> double
```
```
$ c++ -std=c++17 -Wall -O2 auto_demo.cpp -o out/auto_demo && ./out/auto_demo
a/4 = 0, b/4 = 0.5
sizeof(a) = 4, sizeof(b) = 8
```

`auto` copies §2.3's trap along with the type: `a` is an `int` because `2` is, so `a / 4` is integer
division.

The repo uses `auto` where the type is long and obvious from the right-hand side —
`const auto& atoms = geometry.getAtoms();` at `src/engine/potentials/lj_potential.cpp:280` (the `&` is
chapter 3) — and spells out `double` everywhere the type is the point. The parallel course's glossary
entry is a one-line reminder: [`auto`](../learn-parallel/GLOSSARY.md#c-you-will-meet-in-the-exercises).

---

## 2.6 Maths functions: `<cmath>`

C++ has no `**` operator. Powers, square roots and rounding are functions in the header `<cmath>`, all in
the `std` namespace.

| C++ (`#include <cmath>`) | What it does | Used in this course for |
|---|---|---|
| `std::pow(x, 6)` | x raised to the power 6 | (σ/r)⁶ — this chapter |
| `std::sqrt(x)` | square root | distance from dx, dy, dz — chapter 4 |
| `std::round(x)` | nearest whole number; halves round away from zero (`2.5` → `3.0`) | the minimum image — chapter 3 |
| `std::cbrt(x)` | cube root | box length from volume — chapter 4 |

> **Unlike Python:** `round(2.5)` there is `2`, because Python rounds halves to even. `std::round(2.5)` is
> `3.0`. It never matters for our box arithmetic, but they are not the same function.

`std::pow` with a whole-number exponent is a function call doing general-purpose work. The repo's fast
kernels avoid it by multiplying instead: `sr2 * sr2 * sr2` for (σ/r)⁶. The reference code keeps
`std::pow` because it is the clearest way to write the formula. Chapter 3's mini_lj switches to the
multiplied form.

---

## 2.7 Printing numbers: `std::cout` shows 6 digits

`std::cout << x` prints a `double` with **6 significant digits** unless told otherwise.
From `types.cpp`:

```cpp
double third = 1.0 / 3.0;
std::cout << "1/3 default   : " << third << std::endl;
std::cout.precision(12);
std::cout << "1/3 precision12: " << third << std::endl;
```
```
1/3 default   : 0.333333
1/3 precision12: 0.333333333333
```

The value inside is identical. Only the printing changed. `std::cout.precision(12)` stays in force for
every later `<<` until changed again.

This is why mini_lj's chapter-2 output shows `-0.890965` (6 digits) while
[learn-parallel lesson 00](../learn-parallel/00_starting_point.md#two-actual-atoms) shows `−2.211693`
(7 digits) for the force. Same number, different printing. Chapter 4's mini_lj sets precision 12 so its
energy can be compared digit for digit with the repo's benchmark.

---

## 2.8 Floating-point numbers are not exact (deep dive later)

A `double` stores a binary fraction. Most decimal numbers, like 0.1, have no exact binary form, so each
one is rounded to the nearest number a `double` can hold. Every arithmetic step rounds again.
`examples/02/float_order.cpp` (with precision 17):

```
$ c++ -std=c++17 -Wall -O2 float_order.cpp -o out/float_order && ./out/float_order
0.1 + 0.2        = 0.30000000000000004
(0.1 + 0.2 == 0.3) is false
(1e16 + 1) - 1e16 = 0
(1e16 - 1e16) + 1 = 1
```

The first two lines are the familiar `0.1 + 0.2` surprise. The last two are the important ones: **the same three numbers, added in a
different order, give a different answer.** At 10¹⁶ the gap between neighbouring `double`s is 2, so the
`+ 1` is rounded away.

Two consequences you will meet later:

- results from code that is "the same maths in a different order" are compared with a tolerance, not
  `==`. The repo's benchmark reports `max rel 1.490e-11` between backends, not "equal";
- a parallel sum adds partial results in a different order from the serial loop, so its last digits can
  differ. [learn-parallel lesson 04](../learn-parallel/04_threads_by_hand.md) and the glossary entry
  [Floating-point is not associative](../learn-parallel/GLOSSARY.md#measuring) pick this up.

---

## 2.9 The mini_lj step: two atoms, one distance

This is the first version of the program the whole track grows. It computes the Lennard-Jones energy
U(r) = 4ε[(σ/r)¹² − (σ/r)⁶] and force F(r) = (24ε/r)[2(σ/r)¹² − (σ/r)⁶] for two atoms a distance `r`
apart. (What ε and σ mean physically is in
[learn-parallel lesson 00](../learn-parallel/00_starting_point.md#what-σ-and-ε-actually-mean); here they
are just two `double`s.)

`examples/02/mini_lj.cpp`:

```cpp
 1  // mini_lj, chapter 2: two atoms, one distance. Just variables, types and arithmetic.
 2  #include <cmath>
 3  #include <iostream>
 4
 5  int main() {
 6      double epsilon = 1.0;   // well depth (reduced units)
 7      double sigma = 1.0;     // size of the atom
 8      double r = 1.2;         // distance between the two atoms
 9
10      double sr6 = std::pow(sigma / r, 6);
11      double sr12 = sr6 * sr6;
12      double energy = 4.0 * epsilon * (sr12 - sr6);
13
14      // Size of the push. Positive = the atoms push apart, negative = they pull together.
15      double force = 24.0 * epsilon * (2.0 * sr12 - sr6) / r;
16
17      int numAtoms = 2;
18      std::cout << "atoms  = " << numAtoms << std::endl;
19      std::cout << "E(1.2) = " << energy << std::endl;
20      std::cout << "F(1.2) = " << force << std::endl;
21      return 0;
22  }
```

| Line | What it does | Chapter-2 idea |
|---|---|---|
| 2 | brings in `std::pow` | §2.6 |
| 6–8 | three 8-byte boxes, each given a value at creation | §2.0, §2.2 |
| 10 | `sigma / r` is `double / double` = ordinary division; then raised to the 6th power | §2.3, §2.6 |
| 11 | (σ/r)¹² as (σ/r)⁶ squared — one multiply instead of a second `pow` | §2.6 |
| 12, 15 | `4.0`, `24.0`, `2.0` are `double` literals | §2.3 |
| 17 | a count is an `int`, not a `double` | §2.1 |
| 18–20 | default 6-digit printing | §2.7 |

```bash
c++ -std=c++17 -Wall -O2 mini_lj.cpp -o out/mini_lj
./out/mini_lj
```
```
atoms  = 2
E(1.2) = -0.890965
F(1.2) = -2.21169
```

These are the same two numbers as learn-parallel lesson 00's
[Two actual atoms](../learn-parallel/00_starting_point.md#two-actual-atoms): U = −0.890965 and
F = −2.211693. The force is negative: at r = 1.2 the atoms pull together.

Nothing on lines 6–8 or 10–15 changes after it is set. In chapter 3 they all become `const`.

---

## 2.10 In the real code

The repo's version of lines 10–12 is `LJPotential::calculatePairEnergy`,
`src/engine/potentials/lj_potential.cpp:368-375`:

```cpp
368  double LJPotential::calculatePairEnergy(double epsilon, double sigma, double distance) {
369      if (distance <= 0.0) return 0.0;
370
371      double sigma_r = sigma / distance;
372      double sigma_r6 = std::pow(sigma_r, 6);
373      double sigma_r12 = sigma_r6 * sigma_r6;
374
375      return 4.0 * epsilon * (sigma_r12 - sigma_r6);
```

| Line | Read it as | mini_lj |
|---|---|---|
| 368 | a function returning a `double`, taking three `double`s (functions are chapter 5) | `epsilon`, `sigma`, `r` on lines 6–8 |
| 369 | guard: a zero distance would divide by zero; `0.0` is a `double` literal | — |
| 371–373 | the same three boxes, the same `std::pow` | lines 10–11 |
| 375 | the same formula, with the same `4.0` | line 12 |

The force version, `calculatePairForce`, starts at line 378 and has
`double forceMagnitude = 24.0 * epsilon * (2.0 * sigma_r12 - sigma_r6) / distance;` at line 388 — mini_lj's
line 15 with longer names. Its parameter list is the subject of chapter 3.

The class header shows each of this chapter's types once, `src/engine/potentials/lj_potential.h:73-78`:

```cpp
73      double cutoff = 10.0;  // Default cutoff
74      std::string mixingRule = "geometric";  // Default mixing rule
75
76      LJBackend backend;
77      int numThreads = 0;
78      bool tailCorrection = false;
```

Each line is a variable with a fixed type and a starting value: a `double`, a `std::string` (a piece of
text; chapter 4), an `int`, a `bool`. They sit inside a class, which chapter 6 explains; the way each one is
declared is exactly this chapter's. Line 76 is the exception — it has no `= …`. `LJBackend` is an `enum
class` (a named set of choices; see the glossary section
[C++ you will meet in `lj_kernels`](../learn-parallel/GLOSSARY.md#c-you-will-meet-in-lj_kernelshcpp)),
and the class's constructor gives it its value. That is chapter 6's topic.

---

## Next

mini_lj now computes one pair, but the distance `r = 1.2` was typed in by hand. Real atoms have
positions — three numbers each — and the distance must be worked out from them, across a periodic box.
To hand positions to functions we first need to know what C++ does when one variable is assigned to
another or passed to a function: since a variable *is* its box, the default is to **copy**. Chapter 3 is
about copies, and the two ways around them: references and pointers.

> **Unlike Python:** `b = a` there never copies; it ties a second name to the same object. Chapter 3 shows
> why the C++ default is the opposite.
