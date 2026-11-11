# Chapter 3, plain version — copies, references, pointers, told as a robot kitchen

This is a plain-language retelling of
[03_copies_references_pointers.md](03_copies_references_pointers.md). Same content, same section numbers,
one picture throughout: the robot kitchen from chapter 1. Sections are step tables with three columns, **Kitchen | C++ |
Example**. For full compiler output and exact line numbers, open the technical version. Code:
[`examples/03/`](examples/03/).

**After this chapter you can say:**

- why "give `b` what `a` has" pours a copy into a second jar, and why that is C++'s default;
- what a second label on the same jar (a reference) is, and what a second label under clear tape
  (`const&`) is for;
- how a recipe hands back results by writing into the caller's own jars (out-parameters);
- what an index card with a shelf position on it (a pointer) is, and what a blank card (`nullptr`) is;
- what a tray of exactly three cups (`std::array<double, 3>`) is;
- how to read `char* argv[]` and `const std::exception& error`.

## Where we are

At the end of chapter 2, mini_lj measured the pull between two atoms — but the distance 1.2 was written in
by hand. Real atoms have positions, three numbers each, and the distance must be worked out from them
across a box that wraps around. As soon as we hand positions to other recipes, we meet chapter 2's rule: a
variable *is* its jar, so "giving" a jar to someone pours a **copy**. This chapter is about when you want
that copy, and the two ways around it: second labels and shelf-position cards.

## The kitchen, once

| Kitchen | C++ |
|---|---|
| a jar with its label painted on | a variable (chapter 2) |
| pouring a jar's contents into a second, new jar | a copy: `b = a` |
| a **second label stuck on the same jar** | a reference: `double& c = a;` |
| a second label, with the jar under clear tape for whoever uses that label | a const reference: `const Vec3& v` |
| an **index card with a shelf position written on it**; it can be rewritten | a pointer: `double* p = &a;` |
| a blank index card | `nullptr` |
| a fixed tray of exactly three cups | `std::array<double, 3>` |
| "call that tray a Vec3" | `using Vec3 = std::array<double, 3>;` |

---

## 3.0 C++ copies by default

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | "b = a" puts out a **new jar** `b` and pours a copy of `a` into it | copy | `double b = a;` |
| 2 | change what is in jar `b`; jar `a` is untouched | independent | `b = 2.0` → prints `a = 1, b = 2` |
| 3 | a tray of three cups copies all three cups | `std::array` copies whole | `q = p; q[0] = 99.0` → `p[0] = 1, q[0] = 99` |
| 4 | copying is what happens unless you ask for something else; to share one jar you must ask for it (a second label, next section) | | |

Copying is safe — nobody can change your jar behind your back — but pouring a 4096-jar shelf into a new
shelf every time you hand it over would waste the afternoon.

---

## 3.1 A reference: a second label on the same jar

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | stick a second label `c` on jar `a`. No new jar | reference | `double& c = a;` |
| 2 | fill "jar c": you filled jar `a`, because it is the same jar | write through | `c = 3.0` → `a = 3, c = 3` |
| 3 | same for a tray | | `r[0] = 99.0` → `p[0] = 99` |
| 4 | the label is stuck on for good. "c = other" does **not** move the label; it pours `other`'s contents into the jar `c` is stuck on | never re-pointed | `c = other;` → `a = 7`; later `other = 8` leaves `a = 7` |
| 5 | you cannot have a second label stuck on nothing | must be initialised | `double& c;` alone does not compile |

---

## 3.2 Handing a jar to a recipe

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the recipe is handed a **copy jar**; whatever it does stays in its own jar | pass by value | `void doubleByValue(double x)` |
| 2 | after it finishes, your jar is unchanged | | `after doubleByValue: v = 1.5` |
| 3 | the recipe is allowed to use **your own jar** (it gets a second label on it) | pass by reference | `void doubleByReference(double& x)` |
| 4 | after it finishes, your jar has changed | | `after doubleByReference: v = 3` |
| 5 | the order looks the same both times; only the sub-recipe's summary card tells you which | call site looks identical | `doubleByValue(v)` vs `doubleByReference(v)` |

---

## 3.3 A second label under clear tape: `const T&`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the recipe gets a second label on your jar, but through that label the jar is under clear tape: look, do not change | const reference | `double firstComponent(const Vec3& v)` |
| 2 | no copy is poured, and nothing can be spilled | no copy, no change | |
| 3 | the recipe tries to refill it; the translator stops it | compile error | `v[0] = 0.0;` → `cannot assign to return value because function 'operator[]' returns a const value` |
| 4 | **the kitchen's rule:** small jars are handed as copies; big things to read get a taped second label; things the recipe must fill get an untaped second label | choosing a parameter | `double x` / `const Vec3& v` / `Vec3& v` |

---

## 3.4 A tray of three cups: `std::array`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a tray with exactly three cups, each holding one `double`; the tray cannot grow | `std::array<double, 3>` | `std::array<double, 3> p = {1.0, 2.0, 3.0};` |
| 2 | cup number 0, 1, 2 | indexing | `p[0]` |
| 3 | the tray takes exactly the space of three cups, 24 bytes | `sizeof` | `sizeof(v) = 24` |
| 4 | give the tray a short name | type alias | `using Vec3 = std::array<double, 3>;` |
| 5 | do something for cup 0, 1, 2 in turn | counting loop | `for (int k = 0; k < 3; ++k)`: start `k` at 0, keep going while `k < 3`, add 1 after each pass |

---

## 3.5 The mini_lj step: positions, a box, and recipes that use labels

The box is 5 long and wraps around, so atom `b` at 4.0 also has copies at −1.0, 9.0, … The nearest copy
counts ("minimum image").

| # | Kitchen | C++ (`examples/03/mini_lj.cpp`) | Example |
|---|---|---|---|
| 1 | two trays of three cups: the atoms' positions | `Vec3 a = {0.2, 0.0, 0.0}; Vec3 b = {4.0, 0.0, 0.0};` | lines 36–37 |
| 2 | a recipe reads both trays (taped labels) and hands back a **new** tray: b minus a | `Vec3 displacement(const Vec3& a, const Vec3& b)` | `raw dx = 3.8` |
| 3 | a recipe gets an untaped label on that tray and fixes it in place: "use the nearest copy" | `void applyMinimumImage(Vec3& d, double box)` | `imaged dx = -1.2` — and no `=` at the call |
| 4 | two empty force trays on the counter | `Vec3 fa = {0.0, 0.0, 0.0}; Vec3 fb = …` | lines 44–45 |
| 5 | the force recipe gets untaped labels on both force trays and **adds into** them | out-parameters `Vec3& f1, Vec3& f2` | `pairForce(1.0, 1.0, d, fa, fb);` |
| 6 | one spring, two ends: one tray gets minus, the other plus | `f1[k] -= …; f2[k] += …;` | `force on a = -2.21169`, `force on b = 2.21169` |

Same size as chapter 2's `F(1.2) = -2.21169`; now with a direction. Why minus on one end and plus on the
other: [learn-parallel lesson 00](../learn-parallel/00_starting_point.md#why-one-minus-and-one-plus).

---

## 3.6 Pointers: an index card with a shelf position

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | write jar `a`'s shelf position on an index card | `&a` (address-of); `double* p = &a;` | |
| 2 | walk to the shelf position on the card and look in the jar | `*p` (dereference) | `*p = 1` |
| 3 | walk there and refill that jar | `*p = 10.0;` | `a = 10` |
| 4 | rub out the card and write jar `b`'s position instead — a second label could never do this | re-point | `p = &b;` → `*p = 2` |
| 5 | a blank card: it points nowhere. Never walk to a blank card | `nullptr` | `p is nullptr now` |
| 6 | the card for the first cup of a tray; "two cups along from there" | `v.data()`, `first[2]` | `first[2] = 6` |
| 7 | the card itself is small: 8 bytes, whatever it points at | `sizeof(double*)` | `8` |
| 8 | the shelf position changes every time the robot is switched on, but cups on one tray are always 8 apart | addresses | `&v[0] = 0x16d6c6b20, &v[1] = 0x16d6c6b28` |
| 9 | the card has no idea how many cups are on the tray. "Seven cups along" walks off the tray | no bounds check | `first[7]` would compile, and read whatever is there |

### `&` and `*` each mean two things

| Symbol | when painting a label (a type) | when cooking (an expression) |
|---|---|---|
| `&` | "this is a second label": `double& c` | "write down this jar's shelf position": `&a` |
| `*` | "this is an index card": `double* p` | "go to the shelf on this card": `*p` |

### Second label or index card?

| | copy jar | second label | index card |
|---|---|---|---|
| a new jar? | yes | no | a small card, not a jar |
| can be moved to another jar? | — | never | yes |
| can be blank? | — | no | yes |
| kitchen rule | small things | **use this when you can** | when you must: blank is allowed, or you walk along a tray |

---

## 3.7 Which part is taped? (skip if you only read code)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the card can be rewritten, the jar can be refilled | `double* p` | |
| 2 | the jars it points at are under tape; the card can still be rewritten | `const double* p` | the kernels' input arrays |
| 3 | the card is laminated (cannot be rewritten); the jar can be refilled | `double* const p` | rare |

Read the label right to left: "p is a pointer to double that is const".

---

## 3.8 In the real kitchen

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the order slip is a stack of index cards, one per word; each card points at the first letter of a word | `char* argv[]` | `run_simulation.cpp:10` |
| 2 | follow card 1 to its first letter | `argv[1][0]`, `*argv[1]` | `word[0] = c`, `*word = c` |
| 3 | copy the letters into the kitchen's own string jar; never touch the card again | copy into `std::string` | `run_simulation.cpp:16`; changing the copy gave `Xonfig.yaml`, `argv[1]` stayed `config.yaml` |
| 4 | the burnt-dish report is read through a taped second label: not copied, not changed | `catch (const std::exception& error)` | `run_simulation.cpp:74` |
| 5 | the repo's force recipe: three numbers as copies, the direction under tape, two force trays to add into | `calculatePairForce(double, double, double, const std::array<double,3>&, std::array<double,3>&, std::array<double,3>&)` | `lj_potential.h:99-102` |
| 6 | it is handed atom i's and atom j's own force trays from the shelf | `calculatePairForce(…, forces[i], forces[j]);` | `lj_potential.cpp:330` |
| 7 | every potential's job description: read the atoms under tape, fill the caller's force shelf | `calculateForces(const Geometry&, std::vector<…>& forces)` | `force_calculator.h:43-44` |
| 8 | a taped second label on the whole positions shelf, so nothing is copied before the loop | `const auto& positions = geometry.getPositions();` | `lj_potential.cpp:273` |

---

## 3.9 Raw pointers in the parallel kernels (deep dive later)

Omitted from the plain version. In short: the parallel kernels are handed index cards
(`const double* x`) to the first cup of long rows of x, y and z values, made with `.data()` in
`lj_potential.cpp:136-137`. Chapter 4 introduces those rows; chapter 8 explains when such cards go stale.

---

## Next

mini_lj now has two atoms with real positions — two named trays, `a` and `b` — and its recipes
take them by label. That does not scale: a real system has thousands of atoms, which cannot be thousands
of named trays, and the single pair (a, b) has to become every pair (i, j), each with its ε and σ looked
up by the atoms' type. Chapter 4 brings in a growing row of jars (`std::vector`), a card box with tabs
(`std::map`), and a loop over every pair.
