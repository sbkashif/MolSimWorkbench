# Chapter 8, plain version — who clears the counter, and who empties the storeroom

This is a plain-language retelling of [08_ownership_and_lifetime.md](08_ownership_and_lifetime.md). Same
sections, same order, one picture: the robot kitchen from chapter 1. Each section is a step table, **Kitchen | C++ | Example**.
For exact line numbers and full program output, open the technical version. The two sections marked
(deep dive later) there, on cycles and `weak_ptr` and on the bindings layer, are left out here.

## The kitchen, extended

| Kitchen | C++ |
|---|---|
| **counter space** for one sub-recipe; wiped clean the moment that sub-recipe is finished | the **stack**: a function's local variables, destroyed at its closing `}` |
| the **storeroom**: things stay there until someone throws them out | the **heap** |
| the **clean-up step** written at the end of every recipe card for a dish | a **destructor**, `~ClassName()` |
| an **index card with a shelf position** (ch 3) | a pointer `T*` |
| an index card pointing at counter space that has already been wiped | a **dangling pointer** |
| a storeroom item with **one claim ticket**; you may hand the ticket over, never photocopy it | `std::unique_ptr` |
| a storeroom item with a **sign-out sheet**; the last one to sign it back throws it out | `std::shared_ptr` (the number of names on the sheet = `use_count()`) |
| looking at someone else's storeroom item without signing the sheet | a non-owning (borrowed) raw pointer |

---

## Where we are

In chapter 7 the book kept a list of station cards, `{&lj, &tether}`, each pointing at a machine on
`main`'s own counter. Fine, because `main`'s counter stays in use until the end. But the real kitchen's
YAML parser builds the machines inside *its own* sub-recipe and hands them back. When that sub-recipe finishes,
its counter is wiped. What do the index cards point at then?

---

## 8.0 The question C++ makes you answer: who throws it out, and when?

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | every jar the robot puts out has to be cleared away at some point, and something has to decide when | every object has a lifetime | |
| 2 | a jar on a sub-recipe's counter space is cleared when that sub-recipe finishes | automatic storage | §8.1 |
| 3 | a box in the storeroom stays until someone throws it out | dynamic storage | §8.2 |
| 4 | a C++ jar's lifetime is set by **where you put it**, not by who points at it | storage has a lifetime fixed by its scope | `double cutoff = 2.5;` lives until its `}` |
| 5 | nobody counts who is using a jar unless you ask for a sign-out sheet | no counting by default | §8.6 is the opt-in |

---

## 8.1 The closing brace destroys: destructors

| # | Kitchen | C++ | Example (`scope.cpp`) |
|---|---|---|---|
| 1 | a recipe step in brackets has its own patch of counter | a **scope** `{ … }` | the inner block in `main` |
| 2 | put two jars on that patch | create objects in the scope | `Noisy b("b"); Noisy c("c");` |
| 3 | reach the closing bracket: the patch is wiped, last jar first | destroyed at `}`, reverse order | `destroy c` then `destroy b` |
| 4 | each jar's clean-up step runs as it goes | the destructor | `~Noisy()` prints `destroy` |
| 5 | the jar on `main`'s own counter goes when `main` ends | `a` dies at `main`'s `}` | `destroy a` is the last line |

**RAII**, in kitchen words: *whoever takes something out must put it back in their own clean-up step*.
A row of jars (`std::vector`) that borrowed storeroom shelves returns them in its clean-up step. The trajectory writer
closes its file in its clean-up step (`trajectory_writer.cpp:51-53`). Nothing has to be remembered: every
pair of brackets runs the clean-up step for everything on that patch, in reverse order.

---

## 8.2 Two places to keep things: the stack and the heap

| | counter space (stack) | storeroom (heap) |
|---|---|---|
| how you get some | just put a jar down: `LJPotential lj;` | ask the storeroom: `std::make_shared<LJPotential>()` |
| when it is cleared | when this sub-recipe finishes, automatically | when the owner throws it out |
| cost to get | nothing | a trip to the storeroom |
| still there after the sub-recipe returns? | **no** | yes |

A `std::vector` is both: a small **label card** on the counter that says where its row of jars is in the
storeroom. The 4096 positions are in the storeroom. When the card is cleared away, its clean-up step
empties those storeroom shelves too.

---

## 8.3 The dangling pointer

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a sub-recipe puts a jar on its counter and hands back a card with **that counter's position** | `return &cutoff;` | `dangling.cpp` |
| 2 | the translator notices and grumbles, but still makes the book | a **warning**, not an error | `warning: address of stack memory associated with local variable 'cutoff' returned` |
| 3 | the sub-recipe ends; its counter is wiped and reused by the next sub-recipe | the frame is popped | `otherWork()` uses the same space |
| 4 | the robot follows the card | `*p` | printed `0` (and `2.14436e-314` at `-O0`), not `2.5` |
| 5 | whatever happens next is not defined by any rule | **undefined behaviour** | wrong number, garbage, or a crash |

### Chapter 7's list, moved into a sub-recipe

| # | Kitchen | C++ | Example (`dangling_potentials.cpp`) |
|---|---|---|---|
| 1 | a sub-recipe builds two machines on **its own counter** | locals `lj`, `tether` | `buildPotentials()` |
| 2 | it hands back a list of cards pointing at them | `std::vector<Potential*>` of `&lj, &tether` | `return potentials;` |
| 3 | its counter is wiped as it finishes | destructors run | `[LJPotential destroyed]` printed **before** `[back in main…]` |
| 4 | the translator did **not** grumble this time: the cards were inside a list | no warning | compiled cleanly with `-Wall` |
| 5 | the book says "station 1: getName", and the card leads to a wiped counter | `p->getName()` on a dead object | `exit=139`: a segmentation fault |

To hand machines back from a sub-recipe, they must live in the **storeroom**, with an owner responsible for
them.

---

## 8.4 `new` and `delete`, and why the repo never writes them

| # | Kitchen | C++ | Example (`raw_new.cpp`) |
|---|---|---|---|
| 1 | fetch a box from the storeroom by hand; get a card with its shelf | `new Noisy(name)` | `construct kept`, `construct forgotten` |
| 2 | throw it out by hand | `delete kept;` | `destroy kept` |
| 3 | forget to throw one out | no `delete` | `forgotten` is never destroyed: a **leak** |
| 4 | the card does not say whose job the throwing-out is | a raw `T*` carries no ownership | the real problem |

Ways to get it wrong by hand: forget (leak), throw out twice (crash), use after throwing out (dangling),
or something burns between fetching and throwing out and the throwing-out step is skipped (leak). The fix
is to make **the throwing-out part of a label's own clean-up step**. That label is a smart pointer.

`grep -rnwE 'new|delete' src` finds the words only in comments. The repo never fetches by hand.

---

## 8.5 `std::unique_ptr`: exactly one owner

| # | Kitchen | C++ | Example (`smart.cpp`, first half) |
|---|---|---|---|
| 1 | fetch a box and take **its one claim ticket** | `std::make_unique<Noisy>("u")` | `construct u` |
| 2 | try to photocopy the ticket | copy a `unique_ptr` | does not compile |
| 3 | hand the ticket to another owner; your hands are now empty | `std::move(u)` | `u is empty after the move` |
| 4 | the ticket-holder's sub-recipe ends: its clean-up step throws the box out | `moved` dies at `}` | `destroy u` |

In the repo, the MD driver is the only one who needs the trajectory writer, so it holds it with one ticket
(`molecular_dynamics.h:123`, made at `molecular_dynamics.cpp:92`). When the driver is cleared away, the
writer goes, and the writer's clean-up closes the file.

---

## 8.6 `std::shared_ptr`: counted owners

| # | Kitchen | C++ | Example (`smart.cpp`, second half) |
|---|---|---|---|
| 1 | fetch a box with a **sign-out sheet**; your name is on it | `std::make_shared<Noisy>("s")` | `use_count = 1` |
| 2 | a second owner signs too | copy the `shared_ptr` | `use_count = 2` |
| 3 | the second owner's sub-recipe ends; it signs the box back | the copy dies | `use_count = 1`, box stays |
| 4 | you sign it back: last name off the sheet, so you throw it out | `first.reset()` | `destroy s` |
| 5 | reach into the box through the card | `->` | `lj->setCutoff(2.5)` |

The counting happens only for boxes you chose to give a sign-out sheet. Copying a `shared_ptr` never
copies the box: it adds one more name to the **same** box's sheet.

---

## 8.7 The mini_lj step: build the potentials in a function

| # | Kitchen | C++ | Example (`examples/08/`) |
|---|---|---|---|
| 1 | a **station rack** holds the list of stations, with its name on each machine's sign-out sheet | `ForceCalculator` with `std::vector<std::shared_ptr<Potential>>` | `force_calculator.h:16` |
| 2 | asking the rack for "total energy" asks every station in turn | its `calculateEnergy` loop | `force_calculator.cpp:5` |
| 3 | the LJ machine now puts a note on the serving tray when it is cleared away | `~LJPotential()` prints | `[LJPotential destroyed]` |
| 4 | a sub-recipe fetches the LJ machine from the storeroom and signs for it | `auto lj = std::make_shared<LJPotential>();` | `main.cpp:14` |
| 5 | it gives the machine to the rack, which signs too | `addPotential(lj)` | `use_count = 2` |
| 6 | the sub-recipe ends and signs back; the rack's signature keeps the machine | local `lj` dies | count 1; station survives |
| 7 | `main` receives the rack (itself a storeroom item with a sheet) | `auto forceCalculator = buildForceCalculator();` | `main.cpp:29` |
| 8 | `main` ends: the rack is cleared, its list is cleared, the last signature comes off | destructor chain | `[LJPotential destroyed]` **after** `[end of main]` |

Real output (`./out/mini_lj`):

```
[LJPotential constructed, cutoff = 10]
[leaving buildForceCalculator; lj use_count = 2]
total E = -568.06281728
force on atom 0 = (-3.89410964909, 10.0790696534, -13.6814548653)
[end of main]
[LJPotential destroyed]
```

The total is chapter 7's. Only who is responsible for the machines changed. The force on atom 0 now
includes the tether's pull, because the rack adds every station's forces together.

---

## 8.8 Borrowing: raw pointers that do not own

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the fast force loop gets **cards with shelf positions** of rows of jars someone else owns | `LJKernelView`: `const double* x` | `lj_kernels.h:27` |
| 2 | the jars belong to the LJ machine | `std::vector<double> sx, …` members | `lj_potential.h:89` |
| 3 | first make each row the right length, **then** write the cards | `sx.resize(n)` before `view.x = sx.data()` | `lj_potential.cpp:99`, then `:136` |
| 4 | the cards are thrown away when this sub-recipe ends, long before the machine is | `LJKernelView view;` is a local | `lj_potential.cpp:165` |
| 5 | rule: **never follow a card after the owner has moved or emptied that shelf** | a borrowed pointer must not outlive the storage | |

### Why the order matters

| # | Kitchen | C++ | Example (`stale_data.cpp`) |
|---|---|---|---|
| 1 | a row of 4 jars; write a card with its shelf position | `const double* x = sx.data();` | `x == sx.data(): 1` |
| 2 | the row must grow to 1000; it no longer fits, so the whole row moves to a longer shelf and the old shelf is emptied | `sx.resize(1000)` | `x == sx.data(): 0` |
| 3 | follow the old card | `x[0]` | `5.88185e-310`, not `1` |

### Catching it with AddressSanitizer (skip if you only read code)

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | fit the robot with a shelf monitor that checks every reach onto every shelf | `-fsanitize=address` | `c++ -g -fsanitize=address stale_data.cpp …` |
| 2 | the monitor stops the robot at the bad reach, and says which step emptied the shelf | the report | `heap-use-after-free` at `stale_data.cpp:13`, freed at `stale_data.cpp:10` |

---

## 8.9 In the real code

| # | Kitchen | C++ (repo) | Line |
|---|---|---|---|
| 1 | `main` fetches a station rack from the storeroom | `auto forceCalculator = std::make_shared<ForceCalculator>();` | `run_simulation.cpp:43` |
| 2 | for each potential file, the parser's sub-recipe fetches a machine and hands it back | `parser.parsePotential(potentialFile)` | `run_simulation.cpp:45` |
| 3 | inside the parser: fetch, set up, hand back | `auto ljPotential = std::make_shared<LJPotential>();` … `return ljPotential;` | `yaml_parser.cpp:339`, `:359` |
| 4 | the rack signs for each machine | `std::vector<std::shared_ptr<Potential>> potentials;` | `force_calculator.h:31` |
| 5 | the MD driver also signs for the rack | `this->forceCalculator = forceCalc;` | `molecular_dynamics.cpp:209` |
| 6 | so the driver's clean-up step has nothing to throw out by hand | `// No need to delete forceCalculator as it's a shared_ptr` | `molecular_dynamics.cpp:26` |

The parser does exactly what `dangling_potentials.cpp` got wrong, but right: the machine is in the
storeroom with a signature on it, so the parser's counter can be wiped safely.

---

## Next

`mini_lj` now builds its machines in a sub-recipe and keeps them safely in the storeroom, the same way
the repo does. Nobody has yet timed how long "every station: calculateEnergy" takes, and one robot is
doing all of it while the kitchen has room for eight. Chapter 9 times it and brings in more robots.
