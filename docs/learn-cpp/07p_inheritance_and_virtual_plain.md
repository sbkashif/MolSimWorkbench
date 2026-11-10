# Chapter 7, plain version — inheritance and virtual functions, told as a robot kitchen

This is a plain-language retelling of [07_inheritance_and_virtual.md](07_inheritance_and_virtual.md).
Same sections, same order, one picture: the robot kitchen from chapter 1. Code:
[`examples/07/`](examples/07/). For exact line numbers and full compiler output, open the technical
version.

**After this chapter you can say:**

- why the C++ kitchen needs a written station job description before the book can say "station 2: do this";
- what a job description with blank duties is, and why no machine can be built to *be* the job description;
- how the book says "station 2: calculateEnergy" and whichever machine is installed at station 2 does it;
- what `override` checks, and why a copy cut down to the job description's outline cannot do the machine's job;
- what a cookie cutter's "fill these jars first" line is, and what `explicit` refuses;
- how the real kitchen's `ForceCalculator` asks every station for its energy in one loop.

## The kitchen, once more

| Kitchen | C++ |
|---|---|
| a **station job description**: "any machine installed at a Potential station must be able to calculateEnergy, calculateForces, and say its name" | an **abstract base class** (`Potential`) |
| a duty listed with **no instructions**: each machine must bring its own | a **pure virtual** method (`= 0`) |
| a machine built **for** that station, which brings its own way of doing each duty | a **derived class** (`LJPotential : public Potential`) |
| the book says "**station 2: calculateEnergy**" without saying which machine is there, and whichever machine is installed does it its own way | a call through a base pointer, **dynamic dispatch** (`virtual`) |
| an index card with a station's position on it, in the book's list of stations | a `Potential*` pointer |
| the machine's label "I do *this* duty from the job description" | `override` |

---

## Where we are

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | at the end of chapter 6 the start-here page had a geometry cookie and an LJ cookie | two classes | `Geometry geometry; LJPotential lj;` |
| 2 | and asked the LJ cookie for the energy | one call | `lj.calculateEnergy(geometry)` |
| 3 | now we want a second station: a spring pulling every atom to the centre | a second potential | `HarmonicTether` |
| 4 | with only chapter 6, that is a second cookie and a second call on the start-here page, and every new kind of station means rewriting that page | a second variable and call in `main` | |
| 5 | this chapter: one loop over "every station", whichever machine is installed there | inheritance and `virtual` | |

---

## 7.0 The goal: one loop over every station

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | we want the book to say "for every station: calculateEnergy", once, however many stations there are | one loop over all potentials | `for (Potential* p : potentials)` … `p->calculateEnergy(geometry)` (`main.cpp:24-25`) |
| 2 | but a list in the book must hold **one kind of entry** | a vector holds one type | `std::vector<LJPotential>` cannot hold a tether |
| 3 | and the translator checks every instruction before the robot starts: it must know each station *can* do the duty | compile-time checking | |
| 4 | so: write one station job description, and build both machines for it | a common base class | `Potential` |

---

## 7.1 The job description: an abstract base class

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | write the station job description | `class Potential { … };` | `potential.h:9-16` |
| 2 | each duty is marked "each machine does this **its own way**" | `virtual` | `virtual double calculateEnergy(…)` |
| 3 | with no instructions attached: "you must bring your own" | `= 0` (pure virtual) | `… = 0;` |
| 4 | no machine can be built to **be** the job description itself | abstract class: no objects | `Potential p;` → `variable type 'Potential' is an abstract class` |
| 5 | a machine that skips one duty cannot be built either | still abstract | `Unfinished u;` → `unimplemented pure virtual method 'calculateForces'` |
| 6 | the translator refuses before the book is made, not while the robot is cooking | a compile-time error | |
| 7 | the real job description is word for word the same | `src/engine/force_calculator.h:35-48` | |

---

## 7.2 Deriving: `class LJPotential : public Potential`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the LJ cookie cutter says "machines from me are built for the Potential station" | `: public Potential` | `class LJPotential : public Potential {` |
| 2 | "**is a**": an LJ machine *is a* Potential machine | inheritance | |
| 3 | next to each duty, a label: "this is my way of doing *that* duty" | `override` | `double calculateEnergy(…) override;` |
| 4 | the LJ machine may have extra controls the job description never mentions | extra methods | `addAtomParameters`, `setCutoff` |
| 5 | the LJ recipe cards themselves did not change at all | `lj_potential.cpp` is identical to chapter 6 | |
| 6 | every machine here is built to **one** job description only | single inheritance | |

---

## 7.3 Calling through a base pointer

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the book keeps a list of **index cards with station positions** (where each machine is installed), not the machines themselves | `std::vector<Potential*>` | `{&lj, &tether}` |
| 2 | an LJ machine's position counts as a Potential station's position, because the LJ machine was built for that station | automatic conversion | `LJPotential*` → `Potential*` |
| 3 | for each card: "**station 2: calculateEnergy**" | `p->calculateEnergy(geometry)` | |
| 4 | the book does not say which machine is there; whichever machine is installed does it **its** way | dynamic dispatch, at run time | first LJ, then tether |
| 5 | a duty *not* marked "its own way" is done the job description's way, whichever machine is installed | non-virtual: chosen by the pointer's type | `p->name() = PlainBase` |

---

## 7.4 Through a `Potential*`, only `Potential`'s methods

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | through a station card, the book may only ask for duties in the job description | only base methods through a base pointer | |
| 2 | "station 1: set your cutoff" is refused: that is not on the job description | compile error | `no member named 'setCutoff' in 'Potential'` |
| 3 | so set up the LJ machine **by name** first, then add its station card to the list | configure through the concrete type | `lj.setCutoff(2.5);` then `{&lj, …}` |
| 4 | the real YAML reader does the same | `yaml_parser.cpp:339-359` | |

---

## 7.5 The right duties are not enough: built for the station, or not at all

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a machine that can do all three duties but was **never built for** the Potential station | a class with the right methods, no base | `class Duck` |
| 2 | cannot be installed at a Potential station | compile error | `cannot initialize a variable of type 'Potential *' with an rvalue of type 'Duck *'` |
| 3 | in the real kitchen: the machine-learning machine was never built for the Potential station | `class MachineLearningPotential {` with no base | `machine_learning_potential.h:6` |
| 4 | and is not even in the production plan, so never translated | not in `CMakeLists.txt` | a placeholder |

---

## 7.6 `override` catches near misses

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the job says "say your name, **look, don't touch**"; the machine's card says "say your name" without the "look, don't touch" | a missing `const` | `std::string getName()` |
| 2 | that is a **different** duty with the same name; the station call still gets the job description's default | not an override | `b->getName() = unnamed` |
| 3 | with `-Wall`, the translator mutters a warning; without it, silence | `-Woverloaded-virtual` | |
| 4 | if the machine's card says "this is my way of doing *that* duty", the translator checks, and refuses | `override` → error | `non-virtual member function marked 'override' hides virtual member function` |
| 5 | so every real machine carries the label | `override` on every implementation | |

---

## 7.7 Why a pointer, not a value: slicing

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | instead of a station card, the book keeps a **copy of the machine squeezed into a box shaped like the job description** | a base-class **value** | `Base copy = d;` |
| 2 | everything that does not fit the box is cut away, including what kind of machine it was | slicing | `copy.name() = Base` |
| 3 | a station card still leads to the real machine | pointer or reference | `ref.name() = Derived` |
| 4 | a list of cut-down copies vs a list of station cards | `std::vector<Base>` vs `std::vector<Base*>` | `values[0].name() = Base`, `pointers[0]->name() = Derived` |
| 5 | rule: "its own way" only works through a station card or a second label | virtual dispatch needs a pointer or reference | |

---

## 7.8 A second potential: `HarmonicTether`

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a second machine, built for the same station: pulls each atom towards the centre | `class HarmonicTether : public Potential` | ½k\|r\|² |
| 2 | the cutter's stamping step says "**fill the k jar with the k ingredient**", before anything else | initializer list | `HarmonicTether(double k) : k(k) {}` |
| 3 | in front of the brackets only a jar can stand; inside, `k` is the ingredient | | no `this->` needed |
| 4 | filling a jar as it is made vs making it, then pouring over the leftovers | initialize vs assign | required for `const` and reference jars, and for the base part |
| 5 | without `explicit`, handing over just the number 0.1 where a tether is wanted quietly stamps out a tether | implicit conversion | `report(0.1)` → `LooseTether with k = 0.1` |
| 6 | with `explicit`, you must say "a tether, please" | `explicit` | `StrictTether t = 0.1;` → `no viable conversion` |

---

## 7.9 The virtual destructor (brief)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the clean-up when a machine is removed | the destructor `~Potential()` | chapter 8 |
| 2 | if the book removes a machine through its station card, the clean-up must also be "its own way", or only the job-description part is cleared | `virtual ~Potential()` | |
| 3 | the real kitchen removes machines through their station cards | `ForceCalculator` holds base-class smart pointers | |
| 4 | the primer has a runnable demonstration | [`00_cpp_primer.md`](../explainer/00_cpp_primer.md), "Why the base class marks its destructor `virtual`" | |

---

## 7.10 The mini_lj step

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | new: the job description, and the tether machine | `potential.h`, `tether.{h,cpp}` | |
| 2 | changed: the LJ cutter now says "built for the Potential station", and the start-here page has one loop | `lj_potential.h`, `main.cpp` | |
| 3 | bind the book | | `c++ … main.cpp geometry.cpp lattice.cpp lj_potential.cpp tether.cpp -o out/mini_lj` |
| 4 | each station reports, and the loop adds them up | | `Lennard-Jones: -611.491665613`, `Harmonic tether: 43.428848333`, `total: -568.06281728` |
| 5 | a third station would be one more machine and one more station card; the loop does not change | | |

---

## 7.11 How dispatch works: the vtable (deep dive later)

The technical version explains the hidden table behind "station 2: calculateEnergy" (each object carries
one extra 8-byte pointer: `sizeof(WithVirtual) = 16` vs `sizeof(Plain) = 8`). It is a "(deep dive later)"
section and is omitted here.

---

## 7.12 Two more hierarchies in the repo: integrators and thermostats (skip if you only care about potentials)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | two more job descriptions: one for the machine that moves atoms forward in time, one for the machine that holds the temperature | `class Integrator`, `class Thermostat` | `integrator.h:13`, `thermostat.h:15` |
| 2 | one duty with no instructions: "take one step" | `virtual void step(MDState&, double) = 0;` | `integrator.h:18` |
| 3 | one duty **with** default instructions: "before the step, do nothing" | a virtual with a body | `Thermostat::beforeStep`, `thermostat.h:20-22` |
| 4 | the rescaling machine keeps the default; the Langevin machine brings its own | `override` on `afterStep` only, or on both | `velocity_rescale_thermostat.h:22`, `langevin_thermostat.h:29-30` |
| 5 | a clerk who reads the machine's name off the order card and fetches that machine; "none" means an empty station | `makeThermostat(name, …)` returns `nullptr` for `"none"` | `thermostat.h:37` |

---

## 7.13 In the real code

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the real job description | `class Potential`, `force_calculator.h:35-48` | |
| 2 | a note "a station job called Potential exists" before its full text | forward declaration | `class Potential;` (line 11) |
| 3 | two machines built for it | `lj_potential.h:14-23`, `dft_potential.h:10-17` | same three duties, same `override`s |
| 4 | the DFT machine cannot be stamped out without a parameter file | constructor with a parameter | `DFTPotential(const std::string& parametersFile)` |
| 5 | the real loop: "every station: calculateEnergy", add it up | `force_calculator.cpp:13-19` | `totalEnergy += potential->calculateEnergy(geometry);` |
| 6 | a new kind of machine means installing it at a new station, not rewriting the loop | the **seam** | [GLOSSARY: Seam](../learn-parallel/GLOSSARY.md#design-vocabulary) |

---

## Next

| # | Kitchen | C++ |
|---|---|---|
| 1 | the book's list holds the station cards of two machines sitting on `main`'s own counter | `{&lj, &tether}` point at `main`'s variables |
| 2 | that is safe while `main` is running | they live until `main` returns |
| 3 | the real kitchen builds machines inside a sub-recipe and hands them back | `YAMLParser::parsePotential` returns the potential |
| 4 | but a sub-recipe's counter is wiped when it finishes: a station card pointing there would point at nothing | a local dies when its function returns |
| 5 | chapter 8: who owns each machine, and how long it stays | ownership, lifetime, `std::shared_ptr` |
