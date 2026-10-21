# Chapter 5 — Functions, `const`, and headers in practice

Plain version: [05p_functions_and_headers_plain.md](05p_functions_and_headers_plain.md).
Code: [`examples/05/`](examples/05/) (mini_lj step 5, plus small toy programs). Every output below was
produced by compiling and running them with Apple clang 17 on an M1 Mac.

**After this chapter you can say:**

- how to read a C++ function signature — return type, name, typed parameters — and what the compiler
  checks with it;
- how a program is split into a header of declarations and a `.cpp` of definitions, and how the pieces
  are compiled and linked (chapter 1 §1.3, now with your own code);
- for each parameter of `calculatePairForce`, *why* it is passed by value, by `const` reference, or by
  plain reference;
- how a C++ function hands back two results: an out-parameter, or a `std::pair` unpacked with
  `auto [epsilon, sigma]`;
- why `params["LJ"]` will not compile on a `const` map, and what `.at("LJ")` does instead;
- what default arguments and overloading are, and where the repo uses them.

Time: about 30 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/05
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## Where we are

At the end of chapter 4, mini_lj was a single `main`, lines 18 to 75 of `mini_lj.cpp`. It built the lattice, filled a
`std::map` of parameters, and ran the `i < j` pair loop, and it printed `E = -611.491665613` for 125
atoms. But every piece of physics — the minimum image, the mixing rule, the 4ε[(σ/r)¹² − (σ/r)⁶]
formula — was written straight into the body of that loop. You could not test the formula on its own,
reuse it, or even give it a name. And we are about to add forces, which would tangle a second formula
into the same loop.

This chapter pulls the maths out into **named functions in their own files**. That is the
declaration/definition split from chapter 1 §1.3, which was demonstrated there with a toy `greet`
function. Now it is done for real.

---

## 5.0 The split: what moved where

Here is the same program after the split. The lattice moved to its own pair of files, and the pair
maths moved to another:

```
examples/05/
├── lj_math.h      Vec3, struct AtomParameters, and three declarations
├── lj_math.cpp    the bodies of getMixedParameters, calculatePairEnergy, calculatePairForce
├── lattice.h      one declaration: makeCubicLattice
├── lattice.cpp    its body (the chapter 4 lattice code, unchanged)
└── main.cpp       the pair loop, now calling the functions
```

The function names are not invented. `getMixedParameters`, `calculatePairEnergy` and
`calculatePairForce` are the names the repo uses in `src/engine/potentials/lj_potential.cpp` (§5.11).

The loop body, before and after:

```cpp
// chapter 4, main(): the physics is written inline
const Params& pi = params[types[i]];
const Params& pj = params[types[j]];
const double eps = std::sqrt(pi.epsilon * pj.epsilon);   // geometric mixing
const double sig = std::sqrt(pi.sigma * pj.sigma);
const double sr6 = std::pow(sig / r, 6);
energy += 4.0 * eps * (sr6 * sr6 - sr6);
```

```cpp
// chapter 5, main.cpp:32-35: the loop says WHAT, the functions say HOW
auto [epsilon, sigma] = getMixedParameters(params, types[i], types[j]);
energy += calculatePairEnergy(epsilon, sigma, r);
const Vec3 direction = {d[0] / r, d[1] / r, d[2] / r};
calculatePairForce(epsilon, sigma, r, direction, forces[i], forces[j]);
```

The chapter 5 loop also computes **forces**, which chapter 4 did not. The force formula is the one from
chapter 3's `pairForce`, written the way the repo writes it (§5.4).

The rest of this chapter takes those four lines apart.

---

## 5.1 Reading a function signature

A **function** is a named piece of code that takes inputs and may give back a result. A function that
does not belong to any class is a **free function**. All of this chapter's functions are free functions;
chapter 6 puts them inside a class.

Here is one, from `lj_math.cpp`:

```cpp
double calculatePairEnergy(double epsilon, double sigma, double distance) {
    const double sr6 = std::pow(sigma / distance, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}
```

| Part | C++ | What it means |
|---|---|---|
| return type | `double` before the name | **required**; the compiler checks every `return` against it |
| name | `calculatePairEnergy` | the repo uses camelCase |
| parameters | `double epsilon, …` — type, then name | each parameter **must** have a type |
| body | `{ … }` | the code that runs on each call |
| "no result" | return type `void` | nothing comes back |

The first line, without the body, is the **signature**: return type, name, and parameter types.
The compiler uses it to check every call. If you call `calculatePairEnergy(1.0, 1.0)` with two
arguments, or pass a string, the program does not compile.

### Declaration and definition, for real

Chapter 1 §1.3 split a toy `greet` into `greet.h` (the declaration) and `greet.cpp` (the definition).
mini_lj now does the same. `lj_math.h:16-22`:

```cpp
16  // Declarations only. The bodies are in lj_math.cpp.
17  std::pair<double, double> getMixedParameters(const std::map<std::string, AtomParameters>& params,
18                                               const std::string& type1,
19                                               const std::string& type2);
20  double calculatePairEnergy(double epsilon, double sigma, double distance);
21  void calculatePairForce(double epsilon, double sigma, double distance,
22                          const Vec3& direction, Vec3& force1, Vec3& force2);
```

Each line is a signature followed by `;` instead of a body. That is a **declaration**: "a function with
this name and these types exists somewhere". The same signatures, each followed by `{ … }`, are the
**definitions** in `lj_math.cpp`.

`main.cpp` includes `lj_math.h`, so the compiler can check every call in `main.cpp` against the
declarations. It never needs to see the bodies.

> **Unlike Python:** a `def` is a statement that runs and creates a function object; type hints on it are
> never checked, and there is no separate "promise" that a function exists. In C++ the declaration *is*
> that promise, and the compiler rejects any call whose argument count or types do not match it.

---

## 5.2 Building a program from several files

One command compiles all three `.cpp` files and links them:

```bash
c++ -std=c++17 -Wall -O2 main.cpp lj_math.cpp lattice.cpp -o out/mini_lj
```

`-Wall` turns on the compiler's common warnings. `-O2` turns on optimization (chapter 10 measures what it
buys). Headers are **not** on the command line. They only exist pasted into a `.cpp` (chapter 1 §1.3).

The same build, one stage at a time, as in chapter 1:

```bash
c++ -std=c++17 -Wall -O2 -c lj_math.cpp -o out/lj_math.o
c++ -std=c++17 -Wall -O2 -c lattice.cpp -o out/lattice.o
c++ -std=c++17 -Wall -O2 -c main.cpp    -o out/main.o
c++ out/main.o out/lattice.o -o out/broken        # forgot lj_math.o on purpose
```
```
Undefined symbols for architecture arm64:
  "calculatePairForce(double, double, double, std::__1::array<double, 3ul> const&, std::__1::array<double, 3ul>&, std::__1::array<double, 3ul>&)", referenced from:
      _main in main.o
  "getMixedParameters(std::__1::map<std::__1::basic_string<char, …>, AtomParameters, …> const&, std::__1::basic_string<char, …> const&, std::__1::basic_string<char, …> const&)", referenced from:
      _main in main.o
  "calculatePairEnergy(double, double, double)", referenced from:
      _main in main.o
ld: symbol(s) not found for architecture arm64
clang++: error: linker command failed with exit code 1 (use -v to see invocation)
```

(The `getMixedParameters` line is shortened with `…`. In full it spells out every template argument of
`std::map` and `std::string`, and is almost 600 characters long.)

This is chapter 1's linker error again, but now you can read every part of it. `main.o` compiled fine,
because the declarations were enough. Then the linker went looking for three bodies and found none. The
names it lists are the full signatures, parameter types included. `Vec3` appears as
`std::__1::array<double, 3ul>`, because `using Vec3 = …` only gives a second name to an existing type
(chapter 3). `std::__1::` is an inner namespace the standard library on macOS keeps its names in, and
`3ul` is the number 3 written as an `unsigned long`. §5.9 explains why the parameter types are part of
the name.

---

## 5.3 What may go in a header

`lj_math.h` contains three kinds of thing:

| Kind | Example | Safe in a header? |
|---|---|---|
| a type alias | `using Vec3 = std::array<double, 3>;` | yes |
| a type definition | `struct AtomParameters { double epsilon; double sigma; };` | yes |
| a function declaration | `double calculatePairEnergy(double, double, double);` | yes |
| a function **body** | `double twice(double x) { return 2.0 * x; }` | **only if marked `inline`** |

The rule behind the table is called the **one-definition rule**. A program may contain many identical
*declarations* of a function, but only **one definition**. A header is pasted into every `.cpp` that
includes it. So a function body in a header gets compiled once per `.cpp`, and the linker sees several
copies.

`examples/05/twice.h` puts two bodies in a header, one plain and one marked `inline`:

```cpp
double twice(double x) { return 2.0 * x; }
inline double thrice(double x) { return 3.0 * x; }
```

`twice_a.cpp` and `twice_b.cpp` both include it and call both functions:

```bash
c++ -std=c++17 -Wall -O2 -c twice_a.cpp -o out/twice_a.o
c++ -std=c++17 -Wall -O2 -c twice_b.cpp -o out/twice_b.o
c++ out/twice_a.o out/twice_b.o -o out/twice
```
```
duplicate symbol 'twice(double)' in:
    .../examples/05/out/twice_a.o
    .../examples/05/out/twice_b.o
ld: 1 duplicate symbols
clang++: error: linker command failed with exit code 1 (use -v to see invocation)
```

(Paths shortened.) Only `twice` is reported. `inline` tells the linker: "you will see this body in
several object files; they are all the same, so keep one". With `inline` added to `twice` as well, the
program links and prints `3 4.5 50`.

You will rarely write `inline` yourself in this repo. Chapter 6 shows the common case: a method body
written *inside* a class definition is automatically inline. That is why
`getName() const override { return "Lennard-Jones"; }` can sit in `lj_potential.h`.

The include guard (`#ifndef LJ_MATH_H`) does **not** prevent this error. The guard stops one `.cpp`
from pasting the same header twice. Here, two *different* `.cpp` files each paste it once, which is
normal.

---

## 5.4 Choosing how each parameter is passed

In C++ **you choose, per parameter**, how an argument is passed, and the choice is written in the
signature (chapter 3):

| Written as | Meaning |
|---|---|
| `double x` | **by value**: the function gets its own copy; nothing it does reaches the caller |
| `const Vec3& v` | **by `const` reference**: the caller's object, read-only, no copy |
| `Vec3& v` | **by reference**: the caller's object, and the function may change it |

> **Unlike Python:** a call never copies its arguments; each parameter is a new name for the caller's
> object, and whether the function can change it depends on the object (a list can be mutated, a float
> cannot). In C++ the signature decides, so you can read from the declaration alone which arguments a
> function may modify.

Now the six parameters of `calculatePairForce` (`lj_math.cpp:18-27`):

```cpp
void calculatePairForce(double epsilon, double sigma, double distance,
                        const Vec3& direction, Vec3& force1, Vec3& force2) {
    const double sr6 = std::pow(sigma / distance, 6);
    const double sr12 = sr6 * sr6;
    const double forceMagnitude = 24.0 * epsilon * (2.0 * sr12 - sr6) / distance;
    for (int k = 0; k < 3; ++k) {
        force1[k] -= forceMagnitude * direction[k];
        force2[k] += forceMagnitude * direction[k];
    }
}
```

| Parameter | Passed as | Why |
|---|---|---|
| `epsilon`, `sigma`, `distance` | `double`, by value | 8 bytes. Copying it is as cheap as passing its address, and the function cannot disturb the caller |
| `direction` | `const Vec3&` | an input. 24 bytes; the repo's habit is `const &` for anything bigger than one number. `const` means the body cannot change it |
| `force1`, `force2` | `Vec3&` | **outputs**. The function adds into the caller's `forces[i]` and `forces[j]` |
| (return type) | `void` | nothing to return: the results went out through `force1` and `force2` |

The **missing `const`** on `force1` and `force2` is how you spot an output when reading C++. The
explainer primer makes the same point for `calculateForces` ("The out-parameter convention" in
[`docs/explainer/00_cpp_primer.md`](../explainer/00_cpp_primer.md)).

The rule of thumb the repo follows:

- a number (`double`, `int`, `std::size_t`, `bool`): by value;
- anything else you only read (`std::string`, `Vec3`, `std::vector`, `std::map`, `Geometry`):
  `const T&`;
- anything you write into: `T&`.

### `const` inside the body

`const double sr6 = …` uses `const` on a local variable (chapter 2): it is set once and never changes.
The compiler refuses any later assignment. Most locals in mini_lj and the repo are written this way. It
costs nothing at run time, and it tells the reader "this value is final".

---

## 5.5 Getting results out

A C++ function returns **at most one value**. There are three ways to deliver results, and mini_lj uses
all three.

**Return by value.** `calculatePairEnergy` returns a `double`. `makeCubicLattice` returns a whole
`std::vector<Vec3>`:

```cpp
// lattice.cpp:6, 13, 22
std::vector<Vec3> makeCubicLattice(std::size_t numAtoms, double density, unsigned seed, double& box) {
    std::vector<Vec3> positions;
    ...
    return positions;
}
```

Returning a 125-element vector might look like a 3,000-byte copy. It is not. The compiler builds the
vector directly in the caller's variable (this is called **copy elision**). When it cannot do that, it
**moves** the vector instead: the new vector takes over the old one's storage, which costs a few
pointer copies no matter how many atoms there are. Either way, returning a container by value is cheap,
and it is the normal style.

**Out-parameter, to add into existing storage.** `calculatePairForce` does not *produce* a force, it
*adds* one into `forces[i]` and `forces[j]`, which are being summed over every pair. A return value
cannot do that. A `Vec3&` parameter can.

**Out-parameter, for a second result.** `makeCubicLattice` also needs to report the box length it
computed. It already returns the positions, so the box comes out through `double& box`:

```cpp
// main.cpp:14-15
double box = 0.0;
const std::vector<Vec3> positions = makeCubicLattice(numAtoms, 0.8, 42, box);
```

This works, but it is awkward. The caller must create `box` before the call, and nothing in the call
line shows that `box` is being written. Hold on to this; chapter 6 fixes it.

---

## 5.6 Two results in one: `std::pair` and `auto [a, b]`

`getMixedParameters` needs to give back two numbers, ε and σ. C++ has a small standard type for this: **`std::pair<A, B>`**, from `<utility>`, which holds one `A` and one `B`.

```cpp
// lj_math.cpp:5-11
std::pair<double, double> getMixedParameters(const std::map<std::string, AtomParameters>& params,
                                             const std::string& type1,
                                             const std::string& type2) {
    const AtomParameters& p1 = params.at(type1);
    const AtomParameters& p2 = params.at(type2);
    return {std::sqrt(p1.epsilon * p2.epsilon), std::sqrt(p1.sigma * p2.sigma)};
}
```

`return {a, b};` builds the pair from the two values in the braces. The compiler knows the return type
is `std::pair<double, double>`, so you do not have to repeat it.

The caller unpacks it with a **structured binding** (C++17):

```cpp
// main.cpp:32
auto [epsilon, sigma] = getMixedParameters(params, types[i], types[j]);
```

| | C++ |
|---|---|
| return two values | `return {eps, sig};` with return type `std::pair<double, double>` |
| unpack at the call | `auto [epsilon, sigma] = f(…);` |
| without unpacking | `auto p = f(…); p.first; p.second;` |

The square brackets are required, and so is `auto`. The compiler works out that `epsilon` and `sigma`
are both `double`. The names are yours to choose; they match by position.

---

## 5.7 `const` meets the map: `[]` versus `.at()`

In `main.cpp:17` the parameter map is `const`, because nothing should change it once it is filled:

```cpp
const std::map<std::string, AtomParameters> params = {{"LJ", {1.0, 1.0}}};
```

And `getMixedParameters` receives it as `const std::map<…>&`. That is why its body uses
`params.at(type1)`, not `params[type1]` as chapter 4 did.

Chapter 4 warned that `[]` on a map **inserts** a key that is missing. `examples/05/const_map.cpp`
shows the consequence of a typo:

```cpp
std::map<std::string, AtomParameters> params = {{"LJ", {1.0, 1.0}}};
const double eps = params["Lj"].epsilon;      // typo: "Lj"
...
params.at("Xe");                               // inside try { } catch (const std::out_of_range& error)
```
```
params["Lj"].epsilon = 0
map size is now 2
params.at("Xe") threw: map::at:  key not found
```

`params["Lj"]` silently created a new entry with ε = 0 and returned it. An LJ simulation with a
mistyped atom name would run, with zero interaction for that type, and no error. `.at()` refuses
instead: it throws an exception, `std::out_of_range`. (The `try`/`catch` is the one you saw in chapter 1
§1.2.)

Because `[]` might insert, it is not allowed on a `const` map at all. `examples/05/const_map_error.cpp`
tries:

```cpp
double firstEpsilon(const std::map<std::string, double>& epsilons) {
    return epsilons["LJ"];
}
```
```
const_map_error.cpp:6:20: error: no viable overloaded operator[] for type 'const std::map<std::string, double>' (aka 'const map<basic_string<char>, double>')
    6 |     return epsilons["LJ"];
      |            ~~~~~~~~^~~~~
/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/map:1162:38: note: candidate function not viable: 'this' argument has type 'const std::map<std::string, double>' (aka 'const map<basic_string<char>, double>'), but method is not marked const
 1162 |   _LIBCPP_HIDE_FROM_ABI mapped_type& operator[](const key_type& __k);
      |                                      ^
```

(One more identical note follows, for the second version of `operator[]`.)

| Lookup | Missing key | Allowed on a `const` map? |
|---|---|---|
| `m[key]` | **inserts** a default entry, returns it | no |
| `m.at(key)` | throws `std::out_of_range` | yes |
| `m.find(key)` | returns `m.end()`, a "not found" marker | yes |

Read the note in that error: *"'this' argument has type 'const std::map…', but method is not marked
const"*. `operator[]` is a **method** of the map class, and it is "not marked const". You cannot fully
read that sentence yet. Chapter 6 §6.4 explains `const` methods, and then this note becomes plain
English.

`const` spreads this way on purpose. Because `params` is `const`, every function that receives it must
take it as `const &`, and inside those functions only read-only operations are allowed. You find out at
compile time, not after a run with a silently wrong ε.

---

## 5.8 Default arguments (brief)

A parameter can have a **default value**, used when the caller leaves it out. `examples/05/default_args.cpp`:

```cpp
double scaledCutoff(double sigma, double factor = 2.5) {
    return factor * sigma;
}

std::cout << scaledCutoff(1.0) << std::endl;        // factor = 2.5
std::cout << scaledCutoff(1.0, 3.0) << std::endl;   // factor = 3.0
```
```
2.5
3
```

Arguments match parameters strictly by position, so you can leave out only trailing parameters. The
default is written **once**, in the declaration, not again in the definition.

> **Unlike Python:** there are no keyword arguments. You cannot skip `factor` and set a later parameter by
> name; to change the last of several defaults, you must spell out every argument before it.

The repo does exactly that. `src/engine/molecular_dynamics.h:67` declares:

```cpp
void setBox(double a, double b, double c, double alpha = 90.0, double beta = 90.0, double gamma = 90.0);
```

and `src/engine/molecular_dynamics.cpp:28` defines it without the defaults:

```cpp
void MolecularDynamics::setBox(double a, double b, double c, double alpha, double beta, double gamma) {
```

So `setBox(10, 10, 10)` gives a box with 90° angles. Ignore the `MolecularDynamics::` prefix for now;
chapter 6 explains it.

---

## 5.9 Overloading: one name, several functions (deep dive later)

In C++, two functions may share a name as long as their **parameter types differ**. This is
**overloading**. At each call, the compiler picks the version whose parameters match the arguments.

> **Unlike Python:** a second `def pair_energy(...)` silently **replaces** the first, because the name is
> just rebound. In C++ both versions exist side by side, so when you read a call you must check the
> argument types to know which body runs.

`examples/05/overload.cpp`:

```cpp
double pairEnergy(double r) { ... }                // takes a distance
double pairEnergy(const Vec3& d) {                 // takes a separation vector
    return pairEnergy(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
}
...
std::cout << "pairEnergy(1.2) = " << pairEnergy(1.2) << std::endl;
std::cout << "pairEnergy(d)   = " << pairEnergy(d) << std::endl;      // d = {1.2, 0, 0}
```
```
pairEnergy(1.2) = -0.890965
pairEnergy(d)   = -0.890965
```

(The same `E(1.2)` as chapter 2.)

How can the linker tell two `pairEnergy`s apart? Because, in C++, the name stored in the object file
includes the parameter types. `nm` lists the names in an object file:

```bash
c++ -std=c++17 -O2 -c overload.cpp -o out/overload.o
nm out/overload.o | grep pairEnergy        # raw names
nm -C out/overload.o | grep pairEnergy     # -C: decode them
```
```
000000000000002c T __Z10pairEnergyRKNSt3__15arrayIdLm3EEE
0000000000000000 T __Z10pairEnergyd
000000000000002c T pairEnergy(std::__1::array<double, 3ul> const&)
0000000000000000 T pairEnergy(double)
```

`__Z10pairEnergyd` is "a function named `pairEnergy` (10 letters) taking a `d`ouble". Encoding the
types into the name is called **name mangling**. It is why the linker errors in §5.2 and chapter 1
printed full signatures: the linker was looking for a mangled name, and decoded it to show you.

Two places in the repo use overloading:

- `MolecularDynamics::setBox` has two versions, `molecular_dynamics.h:67-68`. One takes six numbers,
  the other a `SimulationBox`. The second just unpacks the box and calls the first
  (`molecular_dynamics.cpp:44-46`).
- `DFTPotential` declares two `calculateForces`: one fills a `std::vector<std::array<double, 3>>&`
  (`dft_potential.h:16`), the other returns a flat `std::vector<double>` (`dft_potential.h:20`).
  `dft_potential.cpp:16` is the first calling the second.

What it costs: when a call matches two versions about equally well, the compiler reports the call as
*ambiguous* and you must convert an argument by hand. The full matching rules are long; you will not
need them to read this repo.

---

## 5.10 The mini_lj step

`main.cpp` now reads as a list of steps. Lines 14-37:

```cpp
14      double box = 0.0;
15      const std::vector<Vec3> positions = makeCubicLattice(numAtoms, 0.8, 42, box);
16      const std::vector<std::string> types(positions.size(), "LJ");
17      const std::map<std::string, AtomParameters> params = {{"LJ", {1.0, 1.0}}};
18
19      double energy = 0.0;
20      std::vector<Vec3> forces(positions.size(), Vec3{0.0, 0.0, 0.0});
21
22      for (std::size_t i = 0; i < positions.size(); ++i) {
23          for (std::size_t j = i + 1; j < positions.size(); ++j) {
24              Vec3 d;
25              for (int k = 0; k < 3; ++k) {
26                  d[k] = positions[j][k] - positions[i][k];
27                  d[k] -= box * std::round(d[k] / box);
28              }
29              const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
30              if (r > cutoff) continue;
31
32              auto [epsilon, sigma] = getMixedParameters(params, types[i], types[j]);
33              energy += calculatePairEnergy(epsilon, sigma, r);
34              const Vec3 direction = {d[0] / r, d[1] / r, d[2] / r};
35              calculatePairForce(epsilon, sigma, r, direction, forces[i], forces[j]);
36          }
37      }
```

Line 16 uses the `vector(n, value)` constructor from chapter 4: 125 copies of `"LJ"`. Line 20 does the
same with a zero force per atom. After the loop, `main` adds up all the forces into `net`.

```bash
c++ -std=c++17 -Wall -O2 main.cpp lj_math.cpp lattice.cpp -o out/mini_lj
./out/mini_lj
```
```
N = 125
E = -611.491665613
force on atom 0 = (-4.10635870563, 9.86021608902, -13.8938854543)
net force       = (-4.4408920985e-14, -1.33226762955e-14, 3.81916720471e-14)
```

The energy is **the same as chapter 4**, to all 12 digits. Moving code into functions changed the shape,
not the result. And `./out/mini_lj 4096` gives `E = -20105.493949`, the same value as chapter 4 and the
repo's `bench_lj` reference.

The net force is a free physics check. `calculatePairForce` subtracts a force from atom *i* and adds the
same force to atom *j* (Newton's third law), so the total over all atoms should be zero. It is zero up
to about 10⁻¹⁴: rounding error from adding thousands of `double`s in a different order.

---

## 5.11 In the real code

Open `src/engine/potentials/lj_potential.cpp`. The repo's reference loop calls the same three functions
the same way, lines 328-330:

```cpp
328              auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
329              std::array<double, 3> direction = {dx/distance, dy/distance, dz/distance};
330              calculatePairForce(epsilon, sigma, distance, direction, forces[i], forces[j]);
```

and line 291 is the energy version, `auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);`
followed by `totalEnergy += calculatePairEnergy(epsilon, sigma, distance);`.

The functions themselves, lines 335-386, shortened:

```cpp
335  std::pair<double, double> LJPotential::getMixedParameters(const std::string& type1, const std::string& type2) {
336      if (atomParameters.find(type1) == atomParameters.end() ||
337          atomParameters.find(type2) == atomParameters.end()) {
338          return {0.0, 0.0};  // Return zero parameters if missing
339      }
340
341      const auto& params1 = atomParameters[type1];
342      const auto& params2 = atomParameters[type2];
         ...
357      return {mixedEpsilon, mixedSigma};
358  }
359
360  double LJPotential::calculatePairEnergy(double epsilon, double sigma, double distance) {
361      if (distance <= 0.0) return 0.0;
         ...
367      return 4.0 * epsilon * (sigma_r12 - sigma_r6);
368  }
369
370  void LJPotential::calculatePairForce(double epsilon, double sigma, double distance,
371                                      const std::array<double, 3>& direction,
372                                      std::array<double, 3>& force1,
373                                      std::array<double, 3>& force2) {
         ...
380      double forceMagnitude = 24.0 * epsilon * (2.0 * sigma_r12 - sigma_r6) / distance;
381
382      for (int i = 0; i < 3; ++i) {
383          force1[i] -= forceMagnitude * direction[i];
384          force2[i] += forceMagnitude * direction[i];
385      }
386  }
```

| Line | What you now recognise |
|---|---|
| 335 | returns `std::pair<double, double>`; the strings come in by `const &` |
| 336-339 | `.find(key) == .end()` is the "not found" test from §5.7. A missing type gives ε = σ = 0 instead of an exception — a different choice from mini_lj's `.at()` |
| 338, 357 | `return {a, b};` builds the pair |
| 341-342 | `[]` is safe here, because 336-339 already proved both keys exist |
| 360 | the same signature as mini_lj's `calculatePairEnergy`: three `double`s by value, returns `double` |
| 370-373 | the same six parameters as §5.4: three by value, `direction` by `const &`, two outputs by plain `&` |
| 383-384 | Newton's third law, written into the caller's force slots |

One thing is different, and you can now see exactly what it is: every name has an `LJPotential::`
prefix, and `getMixedParameters` takes only the two type names. It does not need the map passed in. The
map, `atomParameters`, lives *inside* the `LJPotential` object, and so does `cutoff`. That is chapter 6.

---

## Next

mini_lj now has named functions in their own files, but look at what `main` carries around: `positions`,
`types`, `box` (returned through an out-parameter), `params` and `cutoff` — five loose variables,
passed separately into every call. Nothing ties `box` to `positions`: a caller can add atoms and forget
the box, or hand `getMixedParameters` the wrong map. Chapter 6 bundles the data together with the
functions that use it, into a **class**.
