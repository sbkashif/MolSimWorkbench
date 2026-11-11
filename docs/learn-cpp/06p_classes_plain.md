# Chapter 6, plain version — classes, told as a robot kitchen

This is a plain-language retelling of [06_classes.md](06_classes.md). Same sections, same order, one
picture: the robot kitchen from chapter 1. Code: [`examples/06/`](examples/06/). For exact line numbers and full compiler
output, open the technical version.

**After this chapter you can say:**

- what a cookie cutter, a cookie, and the cards taped to the cutter are in C++;
- what a cookie's private drawer and its serving hatch are;
- what a "look, don't touch" recipe is, and why a taped-shut cookie only accepts those;
- why the hatch hands out a view of the jar, not a copy of it;
- where the recipes for a cookie can be written down;
- how to read `geometry.h` and most of `lj_potential.h`.

## The kitchen, once more

| Kitchen | C++ |
|---|---|
| a **cookie cutter**: the shape every cookie of this kind will have | a **class** |
| a **cookie** stamped out by the cutter | an **object** (instance) |
| the jars built into every cookie, each labelled with what it holds | **data members** |
| the recipe cards taped to the cutter, which work on one cookie at a time | **methods** |
| the cookie's **serving hatch**: what anyone outside may ask for | `public:` |
| the cookie's **private drawer**: only the recipes taped to its cutter may open it | `private:` |
| a recipe card marked "look, don't touch" | a **`const` method** |
| what happens the moment a new cookie is stamped out | the **constructor** |

---

## Where we are

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | at the end of chapter 5 every recipe had its own card | named functions in their own files | `calculatePairEnergy`, `getMixedParameters` |
| 2 | but the start-here page carried five loose jars to every recipe it called | five loose variables in `main` | `positions`, `types`, `box`, `params`, `cutoff` |
| 3 | the box size even came back in a jar you had to set out beforehand | out-parameter | `double& box` |
| 4 | nothing tied the box to the atoms it belongs to | no link | someone could add atoms and forget the box |
| 5 | this chapter: two cookie cutters. One makes a "geometry" cookie holding atoms **and** box; one makes an "LJ" cookie holding parameters, cutoff **and** the LJ recipes | two classes | `Geometry`, `LJPotential` |

---

## 6.0 Describing a cookie cutter

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | describe the cookie cutter | `class Geometry { … };` | `geometry.h:11-26` |
| 2 | list every jar the cookie will carry, **with what it holds**, up front | data members, typed | `std::vector<Vec3> atomCoordinates;` |
| 3 | tape the recipe cards to the cutter | methods | `void addAtom(…);` |
| 4 | the cutter's shape is fixed before any cookie is stamped: every jar is listed in the description, and no cookie can grow an extra jar later | members declared in the class body | you cannot add a member to one object later |
| 5 | the description ends with a full stop after the closing bracket | `};` | forget it, and the error appears on the next line |

---

## 6.1 Every object has its own members

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | stamp out two cookies | two objects | `Geometry a; Geometry b;` |
| 2 | each cookie has its **own** jars | each object has its own members | `a.addAtom(…)` leaves `b` empty |
| 3 | the cutter can say "every new cookie's box jar starts with zeros" | default member initializer | `Vec3 boxDimensions = {0.0, 0.0, 0.0};` |
| 4 | the starting value is poured into each cookie's own jar; no jar is shared between cookies | one per object | |
| 5 | a jar given no starting value may hold leftovers from an earlier job | an uninitialized `double` holds garbage | that is why the repo writes `double cutoff = 10.0;` |

---

## 6.2 `public` and `private`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the serving hatch: anyone can ask for these | `public:` | `c.set(2.5)`, `c.get()` |
| 2 | the private drawer: only this cookie's own recipes may open it | `private:` | `double cutoff = 10.0;` |
| 3 | someone outside reaches into the drawer | outside code uses a private member | `c.cutoff = -1.0;` |
| 4 | the translator refuses: the book is never made | compile error | `'cutoff' is a private member of 'Cutoff'` |
| 5 | what the drawer buys: the cookie keeps its own rules, e.g. "every atom has both a type and a position" | an invariant | `Geometry` has no way to add one without the other |
| 6 | `struct` is a cutter whose jars are all at the hatch by default; `class` starts with them in the drawer | default access | `struct AtomParameters` |

---

## 6.3 Methods, and the invisible `this`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | ask one cookie to run one of its recipes | call a method | `geometry.addAtom("LJ", p);` |
| 2 | inside the recipe, "the atom-types jar" means **this cookie's** jar | members by bare name | `atomTypes.push_back(atomType);` |
| 3 | the recipe carries an invisible index card with the cookie's shelf position | `this` (a pointer) | `atomTypes` is short for `this->atomTypes` |
| 4 | the ingredient and the jar have the same name, "cutoff" | a parameter shadows a member | `void setCutoff(double cutoff)` |
| 5 | "pour cutoff into cutoff" pours the ingredient into itself; the cookie's jar is untouched | self-assignment | prints `a: 10` |
| 6 | "pour cutoff into **this cookie's** cutoff jar" | `this->cutoff = cutoff;` | prints `b: 2.5` |

---

## 6.4 `const` methods

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a recipe card marked "**look, don't touch**": it only reads this cookie's jars | `const` after `()` | `getNumberOfAtoms() const` |
| 2 | a cookie sealed in clear tape only accepts "look, don't touch" recipes | through `const Geometry&`, only `const` methods | `calculateEnergy(const Geometry& geometry)` |
| 3 | a recipe that only reads, but never wrote "look, don't touch" on its card, is refused | missing `const` | `'this' argument … has type 'const Atoms', but function is not marked const` |
| 4 | the translator reads the card's marking, not the steps | the promise is in the signature | |
| 5 | chapter 5's card box refused `[]` for the same reason: `[]` may write a new card | `operator[]` is not `const` | the chapter 5 error now reads as plain English |

---

## 6.5 Getters that return `const T&`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the hatch hands out a **view of the cookie's own jar**, sealed with clear tape | return `const T&` | `const std::vector<Vec3>& getPositions() const` |
| 2 | no copy is poured | reference | |
| 3 | nobody outside can add to the jar through the hatch | `const` | `getPositions().push_back(…)` does not compile |
| 4 | receiving it: stick a second label on the viewed jar | `const auto& positions = …` | no copy |
| 5 | receiving it with plain `auto`: pour the whole jar into a new one | `auto positions = …` | copies 4096 positions every call |
| 6 | the view is only good while the cookie is still on the counter | reference lifetime | chapter 8 |

---

## 6.6 Constructors

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a step that runs **the moment a cookie is stamped out** | constructor | `LJPotential::LJPotential() { … }` |
| 2 | it has the cutter's name and makes no dish | no return type | `LJPotential();` |
| 3 | by the time it runs, every jar already exists and the cutter's starting values are already poured | default member initializers run first | prints `[LJPotential constructed, cutoff = 10]` |
| 4 | only afterwards does the start-here page change the cutoff | `lj.setCutoff(2.5);` | |
| 5 | a cutter with no stamping step gets an empty one for free | compiler-written default constructor | `Geometry` in mini_lj; `Geometry::Geometry() {}` in the repo |

---

## 6.7 Where a method's body lives

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | short sub-recipes written out in full right on the cutter's description, in the header | body inside the class, in the header | `getPositions() const { return atomCoordinates; }` |
| 2 | those are automatically "same recipe everywhere, keep one" | in-class bodies are `inline` | no duplicate-symbol error (chapter 5 §5.3) |
| 3 | longer recipes go on their own card in the recipe file, titled with the cutter's name | `Class::method` in the `.cpp` | `void Geometry::addAtom(…) { … }` |
| 4 | `Geometry::` = "the addAtom that belongs to the geometry cutter" | scope resolution `::` | same `::` as `std::cout` |
| 5 | leave off the title, and it is an unrelated recipe that cannot open this cookie's drawer | a free function | |

---

## 6.8 A private nested struct

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the little ε/σ tray moves **into the LJ cookie's private drawer** | nested `struct` under `private:` | `LJPotential::AtomParameters` |
| 2 | outsiders hand ingredients through the hatch and never see how they are stored | public `addAtomParameters` | `lj.addAtomParameters("LJ", 1.0, 1.0);` |
| 3 | the cutter's designer can rearrange the drawer later without anyone outside noticing | encapsulation | the repo stores flat tables for the parallel kernels |
| 4 | the three LJ helper recipes go in the drawer too | private methods | `calculatePairForce` |

---

## 6.9 The mini_lj step

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the lattice recipe now hands back a whole geometry cookie, box included | `Geometry makeCubicLattice(…)` | the `double& box` jar is gone |
| 2 | the pair loop moves into the LJ cookie's own recipes | `LJPotential::calculateEnergy` | |
| 3 | the mixing recipe no longer needs the card box handed over: it is in the cookie's drawer | member `atomParameters` | `getMixedParameters(atoms[i], atoms[j])` |
| 4 | the geometry cookie is taped shut; every recipe used on it is "look, don't touch" | `const Geometry geometry` | `getAtoms`, `getPositions`, `getBoxDimensions` |
| 5 | the LJ cookie is not taped: the start-here page sets it up | `LJPotential lj;` | `addAtomParameters`, `setCutoff` |
| 6 | run it | | `[LJPotential constructed, cutoff = 10]`, `E = -611.491665613`, same force on atom 0 |

---

## 6.10 In the real code

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the real geometry cutter | `src/builder/geometry.h:12-33` | |
| 2 | its hatch: add an atom, count atoms, view the jars, look up one atom, move one atom, set and view the box | public methods | `setCoordinates` is the one way to move an atom |
| 3 | its drawer: the same three jars as mini_lj | lines 30-32 | |
| 4 | the real LJ cutter's drawer has mini_lj's jars, plus a mixing-rule jar | `lj_potential.h:63-72` | `std::string mixingRule = "geometric";` |
| 5 | changing the cutoff also flips a "tables out of date" flag, so the fast force loops rebuild their tables | `setCutoff` sets `tablesDirty = true` (line 27) | |
| 6 | that only works because nobody can reach into the drawer and change the cutoff directly | `private` | `lj.cutoff = 3.0` would skip the flag |
| 7 | the primer's walk through `lj_potential.h` should now read as review (its line numbers are from an older, shorter version) | [`docs/explainer/00_cpp_primer.md`](../explainer/00_cpp_primer.md) | |

The technical version also has a "(deep dive later)" section on why `calculateEnergy` is not a
"look, don't touch" recipe in the repo. It is omitted here.

---

## Next

| # | Kitchen | C++ |
|---|---|---|
| 1 | the start-here page now has a geometry cookie and an LJ cookie, and asks the LJ cookie for the energy | `lj.calculateEnergy(geometry)` |
| 2 | add a second kind of force, a spring pulling every atom to the centre, and the start-here page needs a second cookie and a second call | a second class, a second call in `main` |
| 3 | every new kind of force means rewriting the start-here page | editing `main` |
| 4 | chapter 7: a **station job description**, so the book can just say "station 2: calculateEnergy" and whichever machine is installed at station 2 does it | inheritance and `virtual` |
