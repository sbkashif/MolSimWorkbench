# Chapter 5, plain version — functions, `const`, and headers, told as a robot kitchen

This is a plain-language retelling of [05_functions_and_headers.md](05_functions_and_headers.md). Same
sections, same order, one picture: the robot kitchen from chapter 1. Code: [`examples/05/`](examples/05/). For exact line
numbers and full compiler output, open the technical version.

**After this chapter you can say:**

- how to read a recipe's heading: what it makes, its name, and the ingredients it takes;
- how the summary cards (`.h`) and the full sub-recipes (`.cpp`) are now split for mini_lj itself;
- for each ingredient of `calculatePairForce`, whether the robot following that recipe gets a copy jar, looks at your jar
  through clear tape, or uses your own jar;
- two ways a recipe hands back two things;
- why the card box (`std::map`) refuses `[]` when it is sealed with clear tape, and what `.at()` does;
- what default ingredients and two recipes with one name are.

## The kitchen, once more

| Kitchen | C++ |
|---|---|
| a **recipe** (a sub-recipe the robot can be told to follow): a name, a list of ingredients handed in, steps, and the dish it produces | a **function**: name, parameters, body, return type |
| the recipe's heading line, without the steps | the function's **signature** / **declaration** |
| a sheet of summary cards: each sub-recipe's name, what it needs, what it hands back | a header file (`.h`) |
| the full recipe card | the **definition**, in a `.cpp` |
| handed a **fresh copy jar** — the robot can do anything with it, yours is safe | pass **by value** (`double x`) |
| allowed to look at **your jar**, sealed with clear tape | pass **by `const` reference** (`const Vec3& v`) |
| allowed to use **your own jar**, and put things in it | pass **by reference** (`Vec3& v`) |
| a tray with two compartments | `std::pair` |
| the card box with alphabetical tabs | `std::map` |

---

## Where we are

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | at the end of chapter 4 the whole meal was one long page of steps | one `main`, lines 18 to 75 | `examples/04/mini_lj.cpp` |
| 2 | it made the right dish | the right answer | `E = -611.491665613` for 125 atoms |
| 3 | but how to mix two atom types, and the LJ formula itself, were written into the middle of the "for every pair" step | the physics is inline in the loop | `4.0 * eps * (sr6 * sr6 - sr6)` inside the `j` loop |
| 4 | you cannot try out one part on its own, or give it a name | no reusable pieces | |
| 5 | and we are about to add a second formula, for forces | forces would tangle the loop further | |
| 6 | this chapter: cut the page into **named sub-recipes**, each written out in its own file, with summary cards | functions in their own files | chapter 1 §1.3's summary-card / sub-recipe split, now for real |

---

## 5.0 The split: what moved where

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a sheet of summary cards for the LJ sub-recipes | `lj_math.h` | three declarations, plus `Vec3` and `AtomParameters` |
| 2 | the LJ recipe cards | `lj_math.cpp` | `getMixedParameters`, `calculatePairEnergy`, `calculatePairForce` |
| 3 | a summary card and a full sub-recipe for laying out the atoms | `lattice.h`, `lattice.cpp` | `makeCubicLattice` |
| 4 | the start-here page now just says what to do, in order | `main.cpp` | "mix the parameters, add the pair energy, add the pair force" |
| 5 | the recipe names are the ones the real kitchen uses | repo names | `lj_potential.cpp` has the same three |
| 6 | a new dish is added: forces | `forces` vector | the force formula from chapter 3 |

---

## 5.1 Reading a function signature

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the recipe heading names the **dish it makes** first | return type | `double` |
| 2 | then the recipe's name | function name | `calculatePairEnergy` |
| 3 | then each ingredient, **with what kind of thing it must be** | typed parameters | `(double epsilon, double sigma, double distance)` |
| 4 | then the steps | the body | `{ … return 4.0 * epsilon * (…); }` |
| 5 | a recipe that makes no dish (it only changes things on the counter) | return type `void` | `void calculatePairForce(…)` |
| 6 | the translator checks every step that calls for a sub-recipe against its summary card: right number and kind of ingredients? | the compiler checks every call | two arguments instead of three: does not compile |

### Declaration and definition, for real

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the summary card gives the heading only, ending in a full stop | declaration | `double calculatePairEnergy(double epsilon, double sigma, double distance);` |
| 2 | the recipe card repeats the heading, then the steps | definition | same line, then `{ … }` |
| 3 | `main.cpp` glues in a photocopy of the summary cards | `#include "lj_math.h"` | |
| 4 | the translator checks each "see …" step in `main` against the summary cards, never seeing the full sub-recipes | `main.cpp` compiles alone | |

---

## 5.2 Building a program from several files

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | translate all three recipes and bind them into one book | one command | `c++ -std=c++17 -Wall -O2 main.cpp lj_math.cpp lattice.cpp -o out/mini_lj` |
| 2 | summary cards are never handed to the translator on their own | headers are not on the command line | |
| 3 | or translate each recipe separately | `-c` | `c++ … -c lj_math.cpp -o out/lj_math.o` |
| 4 | then forget to hand the binder the LJ recipe cards | link without `lj_math.o` | `c++ out/main.o out/lattice.o -o out/broken` |
| 5 | the binder: "three 'see page __' blanks, and no pages for any of them" | `Undefined symbols` | `"calculatePairEnergy(double, double, double)", referenced from: _main in main.o` |
| 6 | the binder names each missing recipe with its **full ingredient list** | the full signature is part of the name | see §5.9 |

---

## 5.3 What may go in a header

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the summary sheet may describe the **shapes of trays** | type aliases and structs | `using Vec3 = …;`, `struct AtomParameters {…};` |
| 2 | the summary sheet may list sub-recipe headings | declarations | `double calculatePairEnergy(…);` |
| 3 | the summary sheet should **not** contain a full sub-recipe | no function bodies | |
| 4 | why: every recipe that glues in the sheet gets its own translated copy of that sub-recipe, and the binder finds two pages with the same title | the one-definition rule | `duplicate symbol 'twice(double)'` |
| 5 | unless the sheet marks it "identical everywhere, keep any one copy" | `inline` | `inline double thrice(double x) {…}` links fine |
| 6 | the "already glued in" note does not help here: it stops one recipe gluing the sheet twice, not two recipes each gluing it once | include guards do not prevent duplicate definitions | |
| 7 | chapter 6: short recipes written **inside** a cookie-cutter description are automatically "keep one" | in-class method bodies are inline | `getName() const override { return "Lennard-Jones"; }` |

---

## 5.4 Choosing how each parameter is passed

When the robot is told to follow a recipe, every ingredient has to be handed in somehow. The recipe
heading says, for each ingredient, which of three ways it is handed over: a fresh copy jar, your jar under
clear tape, or your own jar to use.

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | three small numbers: the robot gets **fresh copy jars** — as cheap as pointing at yours, and yours is safe | by value | `double epsilon, double sigma, double distance` |
| 2 | the direction: the robot may **look at your jar through clear tape**, no copy made | by `const` reference | `const Vec3& direction` |
| 3 | the two force jars: the robot **uses your own jars** and adds into them | by reference | `Vec3& force1, Vec3& force2` |
| 4 | the recipe makes no dish of its own; its work is what it put in your jars | `void` | |
| 5 | how to spot an output when reading: **no clear tape** on a `&` | missing `const` | `force1`, `force2` |
| 6 | a jar that is filled once and then taped shut | `const` local | `const double sr6 = …;` |

The repo's habit: a single number by value; anything bigger that is only read, by `const &`; anything
written into, by `&`.

---

## 5.5 Getting results out

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a recipe hands back **one dish** | return by value | `calculatePairEnergy` returns a `double` |
| 2 | even a big dish (a whole row of jars) is handed back cheaply: the robot builds it right on the shelf where you will keep it, or slides the whole row across | copy elision / move | `makeCubicLattice` returns `std::vector<Vec3>` |
| 3 | to **add** to a running total in your jar, the robot must use your jar | out-parameter for accumulation | `force1[k] -= …` adds into `forces[i]` |
| 4 | a second result, the box size, goes into a jar you set out beforehand | out-parameter for a second result | `double box = 0.0; makeCubicLattice(…, box);` |
| 5 | awkward: nothing in the step that calls the sub-recipe shows that the box jar gets filled | the itch | chapter 6 fixes it |

---

## 5.6 Two results in one: `std::pair` and `auto [a, b]`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the mixing recipe must hand back two things, ε and σ | two results | |
| 2 | it hands back **one tray with two compartments** | `std::pair<double, double>` | `return {mixedEps, mixedSig};` |
| 3 | at the counter you empty the tray into two labelled jars in one move | structured binding | `auto [epsilon, sigma] = getMixedParameters(…);` |
| 4 | or keep the tray and reach into compartment one or two | `.first`, `.second` | `p.first` |

---

## 5.7 `const` meets the map: `[]` versus `.at()`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the parameter card box is sealed with clear tape: nobody adds or changes cards | `const std::map<…>` | `main.cpp:17` |
| 2 | `[]` means "find the card, and **if there is none, write a blank one and file it**" | `operator[]` inserts | a typo `"Lj"` gives ε = 0 and the box now has 2 cards |
| 3 | so `[]` is refused on a taped box | compile error | `no viable overloaded operator[] for type 'const std::map…'` |
| 4 | `.at()` means "find the card, and **if there is none, raise the alarm**" | throws `std::out_of_range` | `map::at:  key not found` |
| 5 | `.find()` means "tell me whether the card is there" | returns `end()` if missing | the repo uses this |
| 6 | the error note says `operator[]` "is not marked const" — a recipe that has not promised to leave the box alone | a non-`const` method | chapter 6 §6.4 |

---

## 5.8 Default arguments (brief)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | "a pinch of salt, unless you say otherwise" | default argument | `double factor = 2.5` |
| 2 | calling for the sub-recipe without it uses the default | | `scaledCutoff(1.0)` → `2.5` |
| 3 | calling for it with a value overrides the default | | `scaledCutoff(1.0, 3.0)` → `3` |
| 4 | the default is written **on the summary card only**, not again in the full sub-recipe | default in the declaration | `molecular_dynamics.h:84` has `alpha = 90.0`; `.cpp:30` does not |
| 5 | no "salt=…" by name: ingredients are handed in strictly in order | arguments match by position only | |

---

## 5.9 Overloading: one name, several functions (deep dive later)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | two recipes may share a name if they take **different kinds of ingredients** | overloading | `pairEnergy(double)`, `pairEnergy(const Vec3&)` |
| 2 | the translator picks the right card by looking at what you handed over | overload resolution | both print `-0.890965` |
| 3 | the binder files each card under name **plus** ingredient list | name mangling | `__Z10pairEnergyd` = `pairEnergy(double)` |
| 4 | the real kitchen does this twice | | `MolecularDynamics::setBox`, `DFTPotential::calculateForces` |

---

## 5.10 The mini_lj step

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the start-here page now reads as steps: lay out atoms, set the card box, loop over pairs, call the recipes | `main.cpp:14-37` | |
| 2 | bind the book | compile | `c++ -std=c++17 -Wall -O2 main.cpp lj_math.cpp lattice.cpp -o out/mini_lj` |
| 3 | same dish as chapter 4, to 12 digits | | `E = -611.491665613` |
| 4 | the new dish | | `force on atom 0 = (-4.10635870563, 9.86021608902, -13.8938854543)` |
| 5 | check: every push on one atom is a pull on another, so the total is zero | Newton's third law | `net force` about `1e-14` (rounding) |
| 6 | 4096 atoms: still the same as chapter 4 | | `E = -20105.493949` |

---

## 5.11 In the real code

`src/engine/potentials/lj_potential.cpp`:

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the real force loop calls the same three recipes the same way | lines 291, 328-330 | `auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);` |
| 2 | the mixing recipe first checks the card box has both cards, and hands back an empty tray if not | `.find(…) == .end()`, lines 336-339 | `return {0.0, 0.0};` |
| 3 | after that check, `[]` cannot write a blank card | lines 341-342 | |
| 4 | the force recipe takes the same six ingredients, the same three ways | lines 370-373 | |
| 5 | one difference: every recipe name has a prefix, and the mixing recipe is **not handed the card box** | `LJPotential::getMixedParameters(type1, type2)` | the card box lives inside the LJ station itself — chapter 6 |

---

## Next

| # | Kitchen | C++ |
|---|---|---|
| 1 | the recipes have names and cards now, but the start-here page has to carry five loose jars to every recipe it calls | `positions`, `types`, `box`, `params`, `cutoff` in `main` |
| 2 | nothing ties the box-size jar to the atom jars: someone can add atoms and forget the box, or hand over the wrong card box | no link between related data |
| 3 | chapter 6: a **cookie cutter** that stamps out one object holding the jars *and* the recipes that use them | a **class** |
