# Chapter 1, plain version — program structure, told as a robot kitchen

This is a plain-language retelling of [01_program_structure.md](01_program_structure.md). It has the same
content and tells it as one story: a kitchen run by a robot. Each section is a step table with three
columns, **Kitchen | C++ | Example**. For exact line numbers and full compiler output, open the original.

## The kitchen, once

You write recipes in English. The kitchen robot cannot read English; it only follows instructions written in
its own robot code. So before the robot can cook anything, your recipes have to be translated and assembled
into a book the robot can use.

| Kitchen | C++ |
|---|---|
| a recipe you write in English | your source file (`.cpp`) |
| the translator who turns one English recipe into robot code | the **compiler** |
| one translated recipe card, in robot code | an **object file** (`.o`) |
| the binder who clips all the translated cards into one book | the **linker** |
| the finished robot book | the **executable** (`out/hello`, `build/run_simulation`) |
| switching the robot on and handing it the book | running the program |
| a recipe that calls for a **sub-recipe** ("make the dough — see the dough recipe") | a function calling another function |
| a sub-recipe's **summary card**: "pipe a name in icing: needs one name; hands nothing back" — no steps | a **declaration**, in a header file (`.h`) |
| the sub-recipe written out in full, step by step | a **definition**, in a `.cpp` file |
| the standard cookbook that comes with the kitchen (dough, icing basics, serving) | the C++ standard library (`std::string`, `std::cout`, …) |
| photocopying a card and gluing it into your recipe | `#include` |
| "see page __" written in a recipe, with the page number left blank | a call the compiler could not finish (a **relocation**) |
| the page number the binder writes into that blank | the function's **address** |

---

## 1.0 Translate first, cook later

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | you write a recipe in English | you write source code | `hello.cpp` |
| 2 | the translator reads the **whole** recipe and writes it out in robot code | compile | `c++ -std=c++17 -c hello.cpp` |
| 3 | the binder clips the translated cards into one book | link | (one command does steps 2 and 3 for a single file) |
| 4 | the robot cooks from the book | run | `./out/hello` → `Hello from C++` |

The robot never sees your English. It only ever sees the finished book.

### The misspelled ingredient

You write "add 2 cups of flurp" in the middle of a recipe.

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the translator reaches "flurp" and finds no such ingredient | the compiler meets a name it was never told about | `std::cout << undefinedName` |
| 2 | it refuses to translate the recipe, and points at the exact line | compiler error with file, line, column | `typo.cpp:5:18: error: use of undeclared identifier 'undefinedName'` |
| 3 | no translated card, so no book | no executable is produced | |
| 4 | the robot cooks **nothing** — not even the steps before "flurp" | nothing runs; `"line 4 runs"` is never printed | |

The translator is strict: it refuses any word it has not been told about. You find the mistake before any
cooking starts. That one rule explains the rest of this chapter.

---

## 1.1 The smallest recipe

```cpp
#include <iostream>

int main() {
    std::cout << "Hello from C++" << std::endl;
    return 0;
}
```

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | glue in the "how to put food on the serving tray" page from the standard cookbook | include a standard header | `#include <iostream>` |
| 2 | "**start here**": the robot always opens the book at this page | the entry point | `int main() {` |
| 3 | the steps of this page sit between two brackets | a block | `{` … `}` |
| 4 | put this plate on the serving tray | print to standard output | `std::cout << "Hello from C++"` |
| 5 | `std::` = "from the standard cookbook" | a namespace | `std::cout` |
| 6 | every step ends with a full stop | every statement ends with `;` | `return 0;` |
| 7 | when finished, the robot hands back a slip: 0 = "all fine", anything else = "something went wrong" | exit status | `return 0;` → `echo $?` prints `0` |

---

## 1.2 Our kitchen's front door: `run_simulation.cpp`

When you switch the robot on, you can hand it an order slip with a few words written on it. Running
`build/run_simulation data/lj_bulk_simulation.yaml` hands it a slip naming the config file.

| # | Kitchen | C++ (`src/tools/run_simulation.cpp`) |
|---|---|---|
| 1 | the robot receives the order slip: how many words, and the words themselves | `int main(int argc, char* argv[])` (line 10) |
| 2 | word 0 is always the robot's own name | `argv[0]` |
| 3 | no dish named? write a complaint on the **error board**, not on the serving tray | `std::cerr << "Usage: …"` (line 12) |
| 4 | hand back the slip saying "failed" | `return 1;` (line 13) |
| 5 | copy the dish name onto a card that can never be rewritten | `const std::string configPath = argv[1];` (line 16) |
| 6 | "take word 2 if there is one, otherwise use the house default" | `(argc >= 3) ? argv[2] : "trajectory.xyz"` (line 17) |
| 7 | if anything burns during cooking, catch it and write what burned on the error board | `catch (const std::exception& error)` (line 73) |

Try it: `./out/args` prints `Usage: args <name>` and hands back `1`. `./out/args Salman` prints `argc = 2`
then `Hello, Salman` and hands back `0`.

**Run the real robot.** Switch it on from the kitchen's front door, the **repo root**, not from
`examples/01/`. Its order slip names a file by a path that only makes sense from there:

```bash
# from the repo root
./build/run_simulation;  echo "exit=$?"
./build/run_simulation data/lj_bulk_simulation.yaml build/trajectory.xyz
```

With no slip it writes `Usage: …` on the error board and hands back `1`. With the slip it cooks a whole
simulation and ends with `Simulation complete.` Started from the wrong folder, it cannot find its files and
writes a confusing `Failed to parse YAML file` on the error board instead.


---

## 1.3 A recipe with sub-recipes: `.h` and `.cpp`

Think of the recipe for **name cookies**: cookies with someone's name piped on in icing. Written out, it is
mostly references to other recipes:

```
NAME COOKIES                                   ← main.cpp
  1. Make the dough.        → see "Basic dough", standard cookbook
  2. Bake 10 minutes.
  3. Pipe the name "Salman" on each cookie.
                            → see "Pipe a name in icing", my own recipe (greet.cpp)
```

```
PIPE A NAME IN ICING                           ← greet.cpp
  needs: one name
  1. Fill the piping bag.
  2. Write "Hello, " and then the name.
```

In the toy program this is exactly what happens: `main.cpp` builds a string from `"Salman"` (step 1, using
the standard cookbook's string recipe) and then calls `greet` (step 3), whose recipe is in `greet.cpp`.

The problem: the translator works on **one recipe at a time** and forgets everything between them. While it
translates *Name cookies*, it has never seen *Pipe a name in icing*. So how can it check step 3?

The answer is a small **summary card** for the sub-recipe, kept in `greet.h`:

```
PIPE A NAME IN ICING — summary
  needs: one name.   hands back: nothing.
```

No steps, only what it needs and what it gives back. *Name cookies* glues in a photocopy of that card, not
of the whole sub-recipe.

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | write the sub-recipe's summary card: its name, what it needs, what it hands back | a declaration in a header | `greet.h`: `void greet(const std::string& name);` |
| 2 | write the sub-recipe out in full, once, in its own file | the definition | `greet.cpp`: `void greet(…) { std::cout << … }` |
| 3 | *Name cookies* glues in a **photocopy of the summary card** | `#include "greet.h"` in `main.cpp` | |
| 4 | translating *Name cookies*, the translator checks step 3 against the card: is "pipe a name" a real recipe? is it given exactly one name? It does not need the piping steps for that | `main.cpp` compiles **without** seeing `greet`'s body | `c++ -c main.cpp` |
| 5 | it writes step 3 in robot code as "pipe the name: **see page __**", leaving the page number blank, because it has no idea where that recipe will end up in the book | `main.o` contains a call to `greet` with no destination yet | |
| 6 | separately, the translator turns *Pipe a name in icing* into its own robot pages | `greet.o` | `c++ -c greet.cpp` |
| 7 | the binder puts all pages into one book, numbers them, finds *Pipe a name* on page 47, and writes **47** into every blank "see page __" that asks for it | linking | `c++ out/main.o out/greet.o -o out/greeter` |

### Why photocopy only the card, not the whole sub-recipe?

- **The check needs only the card.** To confirm that "pipe the name Salman" is a valid step, the translator
  needs the sub-recipe's name and what it takes. How the piping is done does not matter to *Name cookies*.
- **The book must contain each recipe once.** Suppose *Name cookies*, *Birthday cake* and *Gift box* all pipe
  names, and each glued in the *full* piping recipe. Each would be translated with its own copy of the piping
  pages, and the binder would find three different "Pipe a name" pages and refuse: *which one does "see page
  __" mean?* With summary cards only, there is exactly one piping recipe, translated once, and every blank
  points at it.

### `#include` is a photocopier

`#include` glues a full copy of the card into your recipe, plus every card *that* card glued in. Our 6-line
`main.cpp` reaches the translator as **46,151 lines**, almost all of it from the standard cookbook's
`<string>` card (the standard cards are big). That is why translating takes time, and why the build only
re-translates recipes that changed.

The danger: if two cards both glue in the same card, your recipe ends up with it twice. For the piping
card that happens to be harmless: it is only a promise ("this sub-recipe exists"), and repeating a promise is
allowed. But most cards also **describe the shape of something**, such as a tray with two compartments
(`struct Params { double epsilon; double sigma; };`), and a shape may be described only once per recipe. Glued
in twice, the translator stops: `error: redefinition of 'Params'`. So every card carries a note at the top,
whether or not it would break today:

| # | Kitchen | C++ |
|---|---|---|
| 1 | "if this card has not been glued in yet…" | `#ifndef GREET_H` |
| 2 | "…mark it as glued in now" | `#define GREET_H` |
| 3 | (the summary itself) | `void greet(…);` |
| 4 | "end of the protected part" | `#endif` |

The second photocopy sees the mark and is skipped. This is an **include guard**. `lj_potential.h` opens
with one on lines 1–2.

### Two different complaints

| | the translator complains (compiler error) | the binder complains (linker error) |
|---|---|---|
| kitchen | "this recipe calls for a sub-recipe I have **no summary card** for" | "there is a 'see page __' for *Pipe a name*, but **no piping pages in the pile**" |
| real message | `typo.cpp:5:18: error: use of undeclared identifier …` | `Undefined symbols … "greet(…)", referenced from: _main in main.o` |
| points at a line? | yes | **no** — nothing is wrong on any one line; a whole recipe's pages are missing |
| fix | glue in the right card (`#include`) or fix the spelling | give the binder the missing pages (add the `.cpp` to the build) |

---

## 1.3b Behind the scenes: the blank page number, for real

The technical chapter opens the actual files. Here is the same thing in the kitchen.

| # | Kitchen | C++ | What you see |
|---|---|---|---|
| 1 | every translated recipe starts with a list: "recipes I **contain**" and "recipes I **need** from elsewhere" | the **symbol table** of a `.o` file | `nm out/main.o` |
| 2 | *Name cookies*' list: contains *main*; needs *Pipe a name*, and several standard-cookbook recipes (the string, the serving tray) | `T _main`, `U greet(…)`, `U … basic_string …`, `U _strlen` (`T` = here, `U` = needed) | |
| 3 | the piping recipe's list: contains *Pipe a name* | `T __Z5greet…` in `greet.o` | `nm out/greet.o` |
| 4 | the recipe's title is written in full robot spelling, including what it takes, so "pipe a name" and "pipe a number" can never be mixed up | a **mangled name**: `__Z5greetRK…basic_string…` | `c++filt` turns it back into `greet(std::string const&)` |
| 5 | in *Name cookies*, step 3's "see page __" is a placeholder pointing at itself, with a sticky note: "fill in: Pipe a name" | `bl 0x28` (jumps to its own address) plus `ARM64_RELOC_BRANCH26 __Z5greet…` | `objdump -d -r out/main.o` |
| 6 | the binder lays out the book: *Name cookies* on page `0x1000004e8`, *Pipe a name* on page `0x100000688` | layout | `nm out/greeter` |
| 7 | it matches "need *Pipe a name*" with "contains *Pipe a name*" — same full robot spelling | symbol resolution | |
| 8 | it writes the page number into the blank and removes the sticky note | relocation: `bl 0x28` becomes `bl 0x100000688` | `objdump -d out/greeter` |
| 9 | the standard-cookbook recipes (dough, serving tray) were also "needed", yet you never handed the binder those pages: the kitchen's own cookbook is added to every book automatically | `std::string`, `std::cout`, `strlen` resolved from the standard library | the driver adds `libc++` to every link |

So the linker error from §1.3 is just step 7 failing: a "need" on one list with no matching "contains" on
any list. The binder only ever reads these lists and blanks, never your English, which is why it cannot
point at a line.

---

## 1.3c Does every sub-recipe need both a card and a full recipe?

No. The pair has a reason, and the reason tells you when it applies.

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | a summary card alone lets the translator check the step, but the binder still needs real pages to point "see page __" at | a declaration gets you through compiling, never through linking | without `greet.cpp`: `Undefined symbols … greet` |
| 2 | why not glue the **full** sub-recipe into every recipe that uses it? Each recipe would be translated with its own copy, and the binder would find several pages with the same title | a body in a header is compiled into every file that includes it | `duplicate symbol 'twice(double)'` (`examples/05/twice.h`) |
| 3 | so: cards go wherever they are needed; each full sub-recipe is written once, in its own file | `.h` = promises, pasted everywhere; `.cpp` = bodies, compiled once | |
| 4 | bonus: change a full sub-recipe and only it is re-translated; change a card and every recipe that glued it in is re-translated | rebuild cost | |

When the pair is **not** needed:

| # | Kitchen | C++ | Example |
|---|---|---|---|
| 1 | the start-here page: nobody else ever asks for it, so it needs no card | a `.cpp` with no `.h` | `run_simulation.cpp`, `molsim_bindings.cpp` |
| 2 | a job description with no steps of its own ("whoever stands here must be able to calculateEnergy") | a class whose functions are `= 0`, apart from one short default written on the card | `class Potential` in `force_calculator.h` (chapter 7) |
| 3 | a one-line sub-recipe written right on the card, marked "identical everywhere, keep any one copy" | a body inside a class is automatically `inline` | getters in `geometry.h` |
| 4 | a recipe with a blank ("___ cake"): it must be seen in full wherever the blank gets filled in | templates live in headers | chapter 9's `timing.h` |
| 5 | cards for sub-recipes nobody ever wrote, and nobody uses | dead headers | `src/engine/minimizers/*.h` |

**Rule of thumb:** if other recipes call for your sub-recipe, give it a summary card (`.h`) and write it out
once in full (`.cpp`). The start-here page needs no card. Short, identical-everywhere sub-recipes and recipes
with blanks can live entirely on the card.

---

## 1.4 The production plan: `CMakeLists.txt` (skip if you never edit the build)

Nobody hands 20 recipes to the translator one by one. `CMakeLists.txt` is the production plan: which
recipes to translate, and which cards go into which book.

| # | Kitchen | CMake | Example |
|---|---|---|---|
| 1 | "translate these recipes and keep their cards together in one folder" | `add_library(… STATIC …)` | `EngineLib`: `force_calculator.cpp`, `lj_potential.cpp`, … |
| 2 | "translate this recipe and make it a finished book" | `add_executable(…)` | `run_simulation` from `run_simulation.cpp` |
| 3 | "when binding that book, also clip in these folders of cards" | `target_link_libraries(…)` | `run_simulation` ← `BuilderLib EngineLib` |
| 4 | summary cards never appear in the plan | headers are not listed | they only exist glued into recipes |

The `CLAUDE.md` rule "new source files must be added to the list, there is no glob" is a binder problem.
If you write a new recipe `new_thing.cpp` and forget to put it in the plan, every recipe that glues in its
summary card still translates fine. Then the binder finds a "see page __" with no pages to point it at.

---

## Next

You can now write a recipe, get it translated, and have it bound into a book the robot can cook from.
Chapter 2 looks inside a recipe at the most basic thing in it: the ingredients and where they are kept.
In C++ every ingredient sits in a **jar on the shelf with its label painted on**: the label says what the jar
holds, and it can never be repainted. Chapter 2 also starts `mini_lj`, the small program this whole track
grows: two atoms and the distance between them.
