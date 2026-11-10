# Chapter 4, plain version — containers, told as a robot kitchen

This is a plain-language retelling of [04_containers.md](04_containers.md). Same content, same section
numbers, one picture throughout: the robot kitchen from chapter 1. Sections are step tables with three columns, **Kitchen | C++ |
Example**. For full output and exact line numbers, open the technical version. Code:
[`examples/04/`](examples/04/).

**After this chapter you can say:**

- what a growing row of identical jars (`std::vector`) is, and what happens when it outgrows its shelf;
- why asking for jar number 5 in a row of 2 does not raise an alarm unless you use `.at`;
- what a card box with alphabetical tabs (`std::map`) is, and why looking up a missing tab **adds a blank
  card** instead of complaining;
- what a `struct` is: a labelled tray with named cups;
- how a double loop visits every pair of atoms exactly once;
- that mini_lj now gives the same energy as the repo for 4096 atoms.

## Where we are

At the end of chapter 3, mini_lj had two atoms — two named trays, `a` and `b` — and one force recipe. That
does not scale: a real system has thousands of atoms, which cannot be thousands of named trays, and each
atom has a **type** that decides its ε and σ. This chapter brings in a growing row of trays
(`std::vector`) for "one per atom", a card box with tabs (`std::map`) for "parameters per type name", and a
loop over every pair. At the end, mini_lj computes the same energy as the repo for 4096 atoms.

## The kitchen, once

| Kitchen | C++ |
|---|---|
| a **row of identical jars that can grow**; if it outgrows its shelf, the whole row is moved to a longer shelf | `std::vector<T>` |
| a fixed tray of exactly three cups (chapter 3) | `std::array<double, 3>` |
| a **card box with alphabetical tabs**; finding a card means flipping through tabs | `std::map<K, V>` |
| a strip of letter tiles you can rearrange | `std::string` |
| a tray with named cups ("epsilon", "sigma") | `struct` |

---

## 4.0 Three rules every C++ container follows

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | every jar in a row is the same kind: a row of `double` jars holds only `double`s | one element type | `std::vector<double>` |
| 2 | "give `b` what `a` has" builds a whole second row and pours a copy into every jar (chapter 3) | copying a container copies everything | `ys = xs` |
| 3 | so recipes are handed a taped second label on the row, never a copy | `const std::vector<…>&` | `getPositions()` in `geometry.h:20` |

---

## 4.1 `std::vector`: a growing row of jars

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | an empty row | `std::vector<double> xs;` | `xs.size() = 0` |
| 2 | put a new jar at the end of the row | `xs.push_back(1.0);` | then `xs.push_back(2.0);` |
| 3 | count the jars | `xs.size()` | `size = 2` |
| 4 | jar number 1 | `xs[1]` | `xs[1] = 2` |
| 5 | "four jars, each holding 0.0" — round brackets: how many, of what | `std::vector<double> zeros(4, 0.0);` | `zeros.size() = 4` |
| 6 | "these jars: 4 and 0.0" — braces: the contents themselves | `std::vector<double> listed = {4, 0.0};` | `listed.size() = 2` |
| 7 | copy the row, change the copy; the original row is untouched | `ys = xs; ys[0] = 99.0;` | `xs[0] = 1, ys[0] = 99` |
| 8 | a row of three-cup trays: one tray per atom | `std::vector<Vec3> positions` | `positions[1][0] = 1.2` |

---

## 4.2 Asking for a jar that is not there

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | `[]`: walk 5 jars along without checking the row is that long. You grab whatever is there | no bounds check, undefined behaviour | `xs[5]` on 2 jars |
| 2 | `.at()`: count the row first, and raise the alarm if it is too short | checked; throws | `xs.at(5) threw std::out_of_range` |
| 3 | the kitchen's busy loops use `[]`, because the loop already stops at the end of the row | | `i < positions.size()` |

### Counting down from an empty row (skip if you never subtract from a size)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the jar count is a never-negative counter (chapter 2) | `size()` is `std::size_t` | |
| 2 | "one less than the number of jars" on an empty row rolls over | wraparound | `empty.size() - 1 = 18446744073709551615` |
| 3 | so the pair loop writes "start one after i" rather than "stop one before the end" | `j = i + 1` | |

---

## 4.3 Tray or row?

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a tray has exactly three cups, decided when the recipe is written | `std::array<double, 3>`: size fixed | one position, one force, the box |
| 2 | a row's length is decided while cooking | `std::vector`: size at run time | one entry per atom |
| 3 | the row's own label (where the row is, how long, how much shelf is reserved) is the same size however many jars there are | `sizeof` a vector is 24 | `sizeof(xs) = 24, sizeof(zeros) = 24` for 2 and 4 jars |
| 4 | a row of trays: x y z, x y z, x y z, all side by side on one shelf | "array of structures" (AoS) | `std::vector<std::array<double, 3>>` |

Whether one row of trays (AoS) or three separate rows of x, y and z (SoA) is faster is measured in
[learn-parallel lesson 02](../learn-parallel/02_serial_refactor.md); the glossary's
[Memory and layout](../learn-parallel/GLOSSARY.md#memory-and-layout) uses a filing-cabinet picture for it.

---

## 4.4 `std::string`: letter tiles

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a strip of letter tiles that the string owns; you can swap a tile | `std::string` is mutable | `copy[0] = 'X'` (chapter 3) |
| 2 | copying the strip makes a new strip | `t = s` copies | |
| 3 | a row of name strips, one per atom | `std::vector<std::string> types;` | every atom is `"LJ"` |

---

## 4.5 `struct`: a tray with named cups

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | design a tray with two named cups, "epsilon" and "sigma", both `double` | declare a `struct` | `struct Params { double epsilon; double sigma; };` |
| 2 | fill both cups in order | brace initialisation | `Params p = {1.0, 1.0};` |
| 3 | reach for one cup by name | `.` | `p.epsilon` |
| 4 | the tray design is fixed: no adding cups later | fixed fields | `p.extra = 1.0;` does not compile |
| 5 | the repo's tray of the same design | `AtomParameters` | `lj_potential.h:67-70` |

---

## 4.6 `std::map`: a card box with tabs — and its trap

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a card box: each tab is a type name, each card holds that type's parameters | `std::map<std::string, Params>` | `params` |
| 2 | file a card under "LJ" | `params["LJ"] = {1.0, 1.0};` | `size = 1` |
| 3 | **the trap:** open the box at tab "Ar" with `[]`. There is no such tab, so a **blank card is filed** under "Ar", and you are handed it | `[]` inserts a default | `params["Ar"].epsilon = 0, size now = 2` |
| 4 | `.at`: look, and raise the alarm if the tab is missing | throws | `params.at("Ne") threw std::out_of_range` |
| 5 | `.find`: look without filing anything; "the back of the box" means not found | `find(k) == end()` | `Xe not found, size still 2` |
| 6 | the tabs are always in alphabetical order | sorted by key | prints `Ar LJ` |
| 7 | finding a card means flipping through tabs, comparing names — slower than counting along a row | tree lookup | glossary: [Red-black tree](../learn-parallel/GLOSSARY.md#memory-and-layout) |

A typo in an atom type does not fail loudly. It quietly files a blank card, and that atom pulls on nothing.

---

## 4.7 Loops: every pair once

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | for each atom i… | outer loop | `for (std::size_t i = 0; i < positions.size(); ++i)` |
| 2 | …pair it with every atom **after** it | inner loop starts at `i + 1` | `for (std::size_t j = i + 1; …)` |
| 3 | never an atom with itself; never the same pair twice | the [triangular loop](../learn-parallel/GLOSSARY.md#the-two-ways-to-write-the-loop) | 125 atoms → 7750 pairs |
| 4 | count with the same never-negative counter the row uses | `std::size_t` index | mixing `int` and `size_t`: `-Wextra` warns `comparison of integers of different signs` |
| 5 | "skip the rest of this pair, go to the next" | `continue` | pairs beyond the cutoff |
| 6 | "for each card in the box", looking through a taped label | range-for | `for (const auto& entry : params)` |

---

## 4.8 Laying out atoms on a grid

| # | Kitchen | C++ (`examples/04/mini_lj.cpp`) | Example |
|---|---|---|---|
| 1 | how many atoms: from the order slip if given, otherwise 125 | `std::stoul(argv[1])` or `125` | line 19 |
| 2 | box size from density | `std::cbrt(numAtoms / density)` | line 24 |
| 3 | atoms per side: cube root, add a half, drop the fraction = round to nearest | `static_cast<int>(std::cbrt(numAtoms) + 0.5)` | line 25 |
| 4 | three nested loops over the grid, stopping once enough atoms are placed | `for … for … for …` with `&&` | lines 32–34 |
| 5 | grid point, centred, plus a small random nudge | `(i + 0.5) * spacing - box / 2.0 + jitter(rng)` | lines 35–37 |
| 6 | put a new tray at the end of the row, and a name strip "LJ" | `positions.push_back({x, y, z}); types.push_back("LJ");` | lines 38–39 |

### The dice (skip if you do not care how the nudge is made)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a set of dice that always rolls the same sequence for the same starting number | `std::mt19937 rng(42);` | the Mersenne Twister, started from 42 |
| 2 | turn each roll into a number between −5% and +5% of the spacing | `std::uniform_real_distribution<double> jitter(lo, hi)` | `jitter(rng)` |
| 3 | the repo uses the same dice with the same starting number, rolled in the same order | same seed | so every atom lands in exactly the same place |

---

## 4.9 The pair loop

| # | Kitchen | C++ (`examples/04/mini_lj.cpp`) | Example |
|---|---|---|---|
| 1 | one card in the box: "LJ" → ε = 1, σ = 1 | `params["LJ"] = {1.0, 1.0};` | line 44 |
| 2 | two empty jars, filled with zero at once: total energy, pairs inside | `double energy = 0.0; std::size_t inside = 0;` | lines 47–48 |
| 3 | for every pair: the gap, wrapped to the nearest copy (chapter 3) | `d[k] = …; d[k] -= box * std::round(d[k] / box);` | lines 52–55 |
| 4 | the distance | `std::sqrt(…)` | line 56 |
| 5 | too far apart: next pair | `if (r > cutoff) continue;` | line 57 |
| 6 | look up each atom's card by its type name, through a taped label | `const Params& pi = params[types[i]];` | lines 60–61 — safe only because "LJ" is in the box |
| 7 | mix the two cards | `std::sqrt(pi.epsilon * pj.epsilon)` | lines 62–63 |
| 8 | chapter 2's energy recipe, added to the total | `energy += 4.0 * eps * (sr6 * sr6 - sr6);` | line 65 |

---

## 4.10 Run it: 125 atoms, then 4096

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | translate, bind, let the robot cook | `c++ -std=c++17 -Wall -O2 mini_lj.cpp -o out/mini_lj` | |
| 2 | 125 atoms | `./out/mini_lj` | `pairs = 7750, inside cutoff = 3474`, `E = -611.491665613` |
| 3 | 4096 atoms | `./out/mini_lj 4096` | `pairs = 8386560, inside cutoff = 113910`, `E = -20105.493949` |
| 4 | only 1.4% of pairs are close enough to matter; the rest are measured and thrown away | | the fact the parallel course starts from |
| 5 | the repo's own benchmark, same system | `build/benchmarks/bench_lj 4096 3` | `reference … -20105.493949019` |
| 6 | every digit mini_lj prints matches | | 75 lines of one-file C++ = the repo's reference energy |

---

## 4.11 In the real kitchen

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the repo's shelf of atoms: a row of name strips, a row of position trays, one box tray | `atomTypes`, `atomCoordinates`, `boxDimensions` | `geometry.h:30-32` |
| 2 | handed out under a taped second label | `const std::vector<…>& getPositions() const` | `geometry.h:19-20` |
| 3 | the repo's grid recipe: same dice, same loops, same nudge | `createCubicLattice` | `lj_particle_box_builder.cpp:29-50` |
| 4 | one `addAtom` puts a jar at the end of **both** rows | `geometry.addAtom("LJ", {x, y, z});` | line 46 |
| 5 | the repo checks the tab exists **before** opening the box with `[]`, so no blank card is ever filed | `atomParameters.find(type1) == atomParameters.end()` then `atomParameters[type1]` | `lj_potential.cpp:344-350` |
| 6 | preparing the force row: wrong length → resize it with zero trays; right length → zero every tray through an untaped label | `forces.resize(…)`, `for (auto& force : forces)` | `lj_potential.cpp:309-315` |

---

## 4.12 When a vector grows, its elements move (deep dive later)

Omitted from the plain version, except for the picture: when the row of jars outgrows its shelf, the whole
row is carried to a longer shelf. Any index card that still has the old shelf position written on it now
points at an empty shelf (`storage moved while growing: yes`). Chapter 8 returns to this.

---

## Next

mini_lj now gets the right answer — the repo's reference energy for 4096 atoms — but look at the
recipe. It is one long page, `main` from line 18 to line 75: reading the order, laying out the grid,
filling the card box, the pair loop, the wrap-around, the mixing and the energy formula, all tangled
together. You cannot try out the energy formula on its own, or reuse the grid recipe anywhere else.
Chapter 5 cuts the maths out into named sub-recipes, each with a summary card and a full recipe file —
`lj_math.{h,cpp}` and `lattice.{h,cpp}` — chapter 1's summary-card / sub-recipe split, used for real.
