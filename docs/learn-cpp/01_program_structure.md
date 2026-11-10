# Chapter 1 — Program structure: compile, link, run

Plain version: [01p_program_structure_plain.md](01p_program_structure_plain.md).
Toy programs: [`examples/01/`](examples/01/). Every output below was produced by
running them with Apple clang 17 on this machine.

**After this chapter you can say:**

- why a C++ typo is reported *before* anything runs;
- what every line of a minimal C++ program does: `#include`, `int main`, `std::cout`, `return 0`;
- what `argc`/`argv` hold, and what `return 1` from `main` means;
- the difference between a **declaration** (in a `.h` file) and a **definition** (in a `.cpp` file), and
  why `#include` is plain copy-paste;
- which of two errors you are looking at — a compiler error or a linker error — and what each one means;
- why most modules come as a `.h` + `.cpp` pair, and which files legitimately have only one;
- what is physically inside a `.o` file (a symbol table and relocations), and what the linker changes when
  it turns several of them into one program;
- what `add_library` / `add_executable` in `CMakeLists.txt` are telling the compiler and the linker.

Time: about 40 minutes, including running the examples (§1.3b alone is about 10).

```bash
cd docs/learn-cpp/examples/01
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## 1.0 The one big difference: C++ is translated before it runs

Your `.cpp` file is not the program. It is only the **input** to a translator. The program the
computer runs is a separate file of machine instructions, produced in advance:

```
hello.cpp ──compile──► hello.o ──link──► out/hello ──run──► "Hello from C++"
 (text you write)    (machine code     (one runnable      (only now does
                      for this file)    program file)      anything happen)
```

| Stage | Tool | Input → output |
|---|---|---|
| **compile** | `c++` (clang) | one `.cpp` → one object file `.o` |
| **link** | `c++` again (calls `ld`) | all the `.o` files → one executable |
| **run** | you, `./out/hello` | the executable → whatever it does |

> **Unlike Python:** `python script.py` has no compile or link step you can see. The interpreter reads
> your source and executes it statement by statement, so the `.py` file *is* the program.

For a one-file program, one command does compile and link together:

```bash
c++ -std=c++17 hello.cpp -o out/hello
./out/hello
```
```
Hello from C++
```

`-std=c++17` picks the language version this repo uses (`CMakeLists.txt:5`). `-o out/hello` names the
output file.

### Why it matters: when you find out about a mistake

Use a name that was never defined. `typo.cpp`:
```cpp
#include <iostream>

int main() {
    std::cout << "line 4 runs" << std::endl;
    std::cout << undefinedName << std::endl;
    return 0;
}
```
```
$ c++ -std=c++17 typo.cpp -o out/typo
typo.cpp:5:18: error: use of undeclared identifier 'undefinedName'
    5 |     std::cout << undefinedName << std::endl;
      |                  ^
1 error generated.
```

C++ **never printed "line 4 runs"** — no program was produced at all. The compiler reads the whole file
first, and checks every name and every type before anything exists to run.

> **Unlike Python:** the same typo in a `.py` file prints line 1 first, then stops with `NameError` when
> execution reaches line 2. Python checks a name only when it gets there, so a typo late in a long run
> surfaces only after everything before it has run.

**What it buys:** a whole class of mistakes (typos, wrong types, wrong number of arguments) is caught in
seconds, even on a line that would only run after an hour of simulation. **What it costs:** you must tell
the compiler about every name *before* you use it. That rule is the reason header files exist (§1.3).

---

## 1.1 Anatomy of the smallest program

`hello.cpp`, line by line:

```cpp
1  #include <iostream>
2
3  int main() {
4      std::cout << "Hello from C++" << std::endl;
5      return 0;
6  }
```

| Line | C++ | What it does |
|---|---|---|
| 1 | `#include <iostream>` | paste in the declarations for input/output streams (`std::cout`, `std::cerr`) — §1.3 shows what "paste" means literally |
| 3 | `int main() {` | the **entry point**. Every program has exactly one `main`, and running the program means calling it. `int` is its return type. |
| 3, 6 | `{` … `}` | a **block**. Braces mark where the body starts and ends |
| 4 | `std::cout << "…" << std::endl;` | send text to standard output; `std::endl` ends the line |
| 4 | `std::` | the **namespace** the standard library lives in. `std::cout` = "the `cout` from `std`" |
| 4, 5 | `;` | ends a statement. Forget it and the compiler stops |
| 5 | `return 0;` | the program's **exit status**. `0` = success, anything else = failure |

> **Unlike Python:** indentation means nothing to the compiler. Only the braces decide what is inside a
> block; you could write the whole program on one line.

The exit status is not decoration. Your shell sees it, and so do CTest, CI, and any script that runs the
program:

```bash
./out/hello; echo "exit=$?"
```
```
Hello from C++
exit=0
```

---

## 1.2 Our line: the entry point of `run_simulation`

Open `src/tools/run_simulation.cpp`. It has the same shape as `hello.cpp`, plus command-line arguments
and error handling. Here are lines 10–17 and 74–78:

```cpp
10  int main(int argc, char* argv[]) {
11      if (argc < 2) {
12          std::cerr << "Usage: run_simulation <config_path> [trajectory_output]" << std::endl;
13          return 1;
14      }
15
16      const std::string configPath = argv[1];
17      const std::string trajectoryOutput = (argc >= 3) ? argv[2] : "trajectory.xyz";
    ...
74      } catch (const std::exception& error) {
75          std::cerr << "Simulation failed: " << error.what() << std::endl;
76          return 1;
77      }
78  }
```

| Line | C++ | What it does |
|---|---|---|
| 10 | `int main(int argc, char* argv[])` | this form of `main` receives the command line |
| 10 | `argc`, `argv` | **arg**ument **c**ount, and the **arg**ument **v**alues |
| 11 | `if (argc < 2)` | a condition; the parentheses are required |
| 12 | `std::cerr` | standard **error**, a separate stream from standard output |
| 13 | `return 1;` | leave `main` with a failure status |
| 16 | `const std::string configPath = argv[1];` | a **typed** variable; `const` means it can never be reassigned, and the compiler enforces that |
| 17 | `cond ? a : b` | the **conditional operator**: `a` if `cond` is true, else `b` |
| 74 | `catch (const std::exception& error)` | catch any standard exception thrown inside the `try` block |
| 75 | `error.what()` | the exception's message |

`argv[0]` is the program's own name, so one user argument means `argc == 2`.
`examples/01/args.cpp` is this same skeleton cut down to 13 lines. Run it both ways:

```bash
c++ -std=c++17 args.cpp -o out/args
./out/args;        echo "exit=$?"
./out/args Salman; echo "exit=$?"
```
```
Usage: args <name>
exit=1
argc = 2
Hello, Salman
exit=0
```

### Run the real one

`run_simulation` must be started from the **repo root**, not from `examples/01/`. Its config path is relative
(`data/…`), and its YAML loader calls a helper script by a relative path too. Started from the wrong folder,
it does not say "file not found"; it fails with a confusing parse error:

```
Simulation failed: Failed to parse YAML file: [json.exception.parse_error.101] parse error at line 1, column 1: ...
```

From the repo root (the second argument keeps the trajectory file inside the git-ignored `build/`):

```bash
cd ~/codes/MolSimWorkbench
./build/run_simulation;  echo "exit=$?"
./build/run_simulation data/lj_bulk_simulation.yaml build/trajectory.xyz
```
```
Usage: run_simulation <config_path> [trajectory_output]
exit=1
Starting to parse simulation config: data/lj_bulk_simulation.yaml
...
Simulation complete.
Final energy: -461.986
Trajectory written to: build/trajectory.xyz
```

The first run takes line 13's exit: no config named, so `return 1`. The second gets past it and runs a whole
simulation. The final energy changes from run to run, because the starting velocities are random.

Leave `char* argv[]` (a pointer to text) and `const std::exception&` (a reference) as unexplained shapes for
now. Chapter 3 is about exactly those two symbols, `*` and `&`.

---

## 1.3 Two kinds of file: declare in `.h`, define in `.cpp`

§1.0 ended with a rule: the compiler must have seen a name before you use it. In a one-file program that
just means writing things in order. A real program spans many files, and **the compiler works on one
`.cpp` file at a time and remembers nothing between files.** So how can `main.cpp` call a function written
in `greet.cpp`?

The answer is to split "this exists" from "this is how it works":

| | **Declaration** | **Definition** |
|---|---|---|
| says | "a function `greet` exists; it takes a string and returns nothing" | the actual body |
| lives in | a **header**, `greet.h` | a **source file**, `greet.cpp` |
| how many | can be seen any number of times | must exist **exactly once** in the whole program |
| who needs it | the **compiler**, to check your call | the **linker**, to connect your call to real code |

`examples/01/` has the three files:

```cpp
// greet.h
#ifndef GREET_H
#define GREET_H

#include <string>

// Declaration: "a function called greet exists, takes a string, returns nothing."
void greet(const std::string& name);

#endif
```
```cpp
// greet.cpp
#include "greet.h"
#include <iostream>

// Definition: the actual body.
void greet(const std::string& name) {
    std::cout << "Hello, " << name << std::endl;
}
```
```cpp
// main.cpp
#include "greet.h"

int main() {
    greet("Salman");
    return 0;
}
```

`void` means "returns nothing".

### `#include` is copy-paste

Before the compiler proper starts, a step called the **preprocessor** literally replaces the `#include` line with the
full text of that file — and of every file *it* includes. You can see the result with `-E`:

```bash
c++ -std=c++17 -E main.cpp | wc -l
```
```
   46151
```

Our 6-line `main.cpp` reaches the compiler as **46,151 lines**, almost all of it from `<string>`.
(`hello.cpp`, with `<iostream>`, becomes 70,991.) That is why C++ builds are slow, and why CMake
recompiles only the files that changed.

Copy-paste creates a hazard: if two headers both include the same header, its text gets pasted twice into
one `.cpp`. For `greet.h` that happens to be harmless: a function **declaration** is only a promise, and C++
allows the same promise to be repeated. But most real headers also **define** something, such as a `struct`
or a `class`, and a definition may appear only once per `.cpp`. A header holding a struct, pasted twice:

```cpp
// noguard_struct.h  (no include guard)
struct Params { double epsilon; double sigma; };
```
```
In file included from b.cpp:2:
./noguard_struct.h:1:8: error: redefinition of 'Params'
    1 | struct Params { double epsilon; double sigma; };
      |        ^
```

Every header in this repo defines classes (`lj_potential.h` defines `class LJPotential`), so every one needs
protecting. The three `#` lines in `greet.h` are the standard protection, and the habit is to add them to
every header, whether or not it would break today:

```cpp
#ifndef GREET_H     // "if GREET_H has not been defined yet…"
#define GREET_H     //  …define it now, and keep this text
...
#endif              // end of the guarded region
```

The second paste finds `GREET_H` already defined and skips everything up to `#endif`. This is an
**include guard**, and every header in this repo starts with one. `src/engine/potentials/lj_potential.h`:

```cpp
1  #ifndef LJ_POTENTIAL_H
2  #define LJ_POTENTIAL_H
```

> **Unlike Python:** `import` runs a module once, caches it in `sys.modules`, and hands you a name bound
> to it, so importing twice is free and safe. `#include` pastes text every time, into every file that asks.
> An include guard does by hand, for one header, what `sys.modules` does for you automatically.

### Compile separately, link together

Now build the three files the long way, one stage at a time:

```bash
c++ -std=c++17 -c greet.cpp -o out/greet.o      # compile only (-c): greet.cpp -> greet.o
c++ -std=c++17 -c main.cpp  -o out/main.o       # compile only:      main.cpp  -> main.o
c++ out/main.o out/greet.o -o out/greeter       # link the two .o files into one program
./out/greeter
```
```
Hello, Salman
```

`main.cpp` compiled successfully **without ever seeing `greet`'s body**. The declaration was enough for
the compiler to check the call (right name, right argument type). It left a note in `main.o`: "needs
`greet` — someone fill this in." Filling in those notes is the linker's job.

Now leave out `greet.o` from the link step:

```bash
c++ out/main.o -o out/broken
```
```
Undefined symbols for architecture arm64:
  "greet(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&)", referenced from:
      _main in main.o
ld: symbol(s) not found for architecture arm64
clang++: error: linker command failed with exit code 1 (use -v to see invocation)
```

### Telling the two errors apart

| | Compiler error (§1.0) | Linker error (above) |
|---|---|---|
| looks like | `file.cpp:5:18: error: …` — a file, line and column | `Undefined symbols…`, `ld:`, `linker command failed` — **no line number** |
| means | this file uses something it was never told about | something was *declared* but its *definition* never reached the link |
| usual fix | `#include` the right header, fix the typo | add the missing `.cpp` to the build |

The long name in the linker message is `greet(const std::string&)`, written out in full:
`std::string` is shorthand for `std::basic_string<char, …>`.

---

## 1.3b Behind the scenes: what the compiler and the linker actually produce

§1.3 said the compiler "leaves a note" in `main.o` and the linker "fills it in". This section opens the files
and shows you the note. Run these from `examples/01/` after the two `c++ -c` commands above.

### Four stages, not two

`c++` is a driver: it runs several programs in a row. `-save-temps` keeps every intermediate file:

```bash
c++ -std=c++17 -save-temps -c main.cpp
```

| Stage | Program | Output file | Size / lines | What happened |
|---|---|---|---|---|
| 1. preprocess | clang | `main.ii` | 46,151 lines | every `#include` pasted in, every `#ifndef` resolved. Still C++ text. |
| 2. compile | clang | `main.bc`, then `main.s` | 235 lines of assembly | names checked, types checked, then each function translated to machine instructions, written as text |
| 3. assemble | clang's built-in assembler | `main.o` | 2,504 bytes | the assembly text turned into binary machine code, plus a table of names |
| 4. link | `ld` | `out/greeter` | 40,456 bytes | all `.o` files joined; every name given a final address |

Stages 1–3 are what people mean by "compiling", and they happen **once per `.cpp` file, alone**. Stage 4
happens **once per program, over all files together**. That split is the whole difference between the two
errors in §1.3. You can watch the driver launch these programs with `-v`:

```bash
c++ -std=c++17 -v main.cpp greet.cpp -o out/greeter2
```
```
 "/Library/Developer/CommandLineTools/usr/bin/clang" -cc1 -triple arm64-apple-macosx15.0.0 ...   ← main.cpp
 "/Library/Developer/CommandLineTools/usr/bin/clang" -cc1 -triple arm64-apple-macosx15.0.0 ...   ← greet.cpp
 "/Library/Developer/CommandLineTools/usr/bin/ld" -demangle ... -arch arm64 ...                  ← the linker
```

Two separate compiler runs, one per file, neither aware of the other. Then one linker run.

### Inside `main.o`: the call to `greet` has no destination yet

An object file carries a **symbol table**: the list of names it *defines* and the names it *needs*. `nm`
prints it. `c++filt` turns the compiler's encoded names back into readable C++:

```bash
nm out/main.o | c++filt
```
```
                 U greet(std::__1::basic_string<char, ...> const&)
...
0000000000000000 T _main
                 U _strlen
...
```

- **`T`** = defined here, in the code ("text") section. `main` lives in `main.o`, at offset 0.
- **`U`** = **undefined**: used here, defined somewhere else. `greet` is a `U`. So is `strlen`, which lives in
  the C library.

Now the other file:

```bash
nm out/greet.o | grep greet
```
```
0000000000000000 T __Z5greetRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE
```

`greet.o` *defines* it (`T`). This long string is the **mangled name**: C++ encodes the parameter types into
the symbol (`5greet` = a 5-letter name `greet`, `RK…basic_string…` = takes a `const` reference to a
`std::string`). That is how the linker can tell `greet(std::string)` from a different `greet(int)`.

Now look at the machine code where `main` calls `greet`. `objdump -d -r` disassembles it and shows the
**relocations**, the notes the compiler left for the linker:

```bash
objdump -d -r --no-show-raw-insn out/main.o
```
```
      24:      	ldr	x0, [sp, #0x8]
      28:      	bl	0x28 <ltmp0+0x28>
		0000000000000028:  ARM64_RELOC_BRANCH26	__Z5greetRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE
```

Read it line by line:

- `ldr x0, …` puts into register `x0` the address of the `std::string` that the previous instructions built
  from `"Salman"`. On this CPU the first argument of a call travels in `x0`; that is how `greet` receives `name`.
- `bl` is ARM's "branch with link" instruction: *jump to a function, then come back*. Its target is
  `0x28`, its own address. In other words it points at itself. That is a **placeholder**: the compiler did not
  know where `greet` would end up, so it wrote a dummy target.
- The indented line under it is the **relocation**: "at offset `0x28`, patch this `bl` so it jumps to the symbol
  `__Z5greet…`." This is the "sticky note" from §1.3, as it actually exists in the file.

### After linking: the placeholder becomes an address

```bash
c++ out/main.o out/greet.o -o out/greeter
nm out/greeter | grep -i "greet\| _main"
objdump -d --no-show-raw-insn out/greeter | grep -A12 "<_main>:"
```
```
0000000100000688 T __Z5greetRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE
00000001000004e8 T _main

00000001000004e8 <_main>:
...
10000050c:     	ldr	x0, [sp, #0x8]
100000510:     	bl	0x100000688 <__Z5greetRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE>
```

The linker did three things:

1. **Layout.** It placed `main`'s code at address `0x1000004e8` and `greet`'s code at `0x100000688` in one
   program.
2. **Symbol resolution.** It matched `main.o`'s `U greet(…)` with `greet.o`'s `T greet(…)`. Same mangled name,
   so they are the same function.
3. **Relocation.** It patched the `bl` instruction: the dummy `0x28` is now `0x100000688`, the real address of
   `greet`. The relocation note is gone, because there is nothing left to fill in.

Compare the same instruction before and after:

| | `main.o` (compiler output) | `out/greeter` (linker output) |
|---|---|---|
| the call | `bl 0x28` (points at itself) | `bl 0x100000688` |
| note attached | `ARM64_RELOC_BRANCH26 __Z5greet…` | none |
| `greet` in symbol table | `U` (needed) | `T` (defined, with an address) |

### What each error really is, now that you have seen the files

| Error | What the tool was doing when it failed |
|---|---|
| **compiler**: `use of undeclared identifier 'undefinedName'` | stage 2, reading *one* file's text. It met a name that no line of that file (including everything `#include` pasted in) had declared. It could not even write a `U` entry, because it did not know what kind of thing the name was. |
| **linker**: `Undefined symbols … "greet(…)", referenced from: _main in main.o` | stage 4. `main.o` had a `U greet(…)` entry and a relocation pointing at it, and **no** `.o` file on the command line had a matching `T`. The relocation could not be patched. |

The linker message is the symbol table speaking: "a `U` with no `T` to match it". That is why it has no line
number. The linker never sees your source lines, only symbol names and relocations.

`std::string`'s own functions, `strlen`, and the `std::cout` machinery are also `U`s in `main.o`. Nobody listed
a `.o` for them, yet the link succeeded. The driver adds the C++ standard library (`libc++`) and the system
library to every link automatically, and they supply those `T`s.

---

## 1.3c Does every file need both a `.h` and a `.cpp`?

No. The pair is a convention with a reason behind it, and the reason tells you when it applies.

### A declaration alone gets you through compiling, never through linking

`greet.h` only promises that `greet` exists. That is enough for the compiler to check `greet("Salman")` in
`main.cpp`. But the program needs real machine code to jump to, and machine code only comes from compiling a
**body**. §1.3b showed it: `main.o` has `U greet` (needed), and only `greet.o` has `T greet` (defined). Leave
out `greet.cpp` and the link fails with `Undefined symbols … greet`.

### Why not put the body in the header and skip the `.cpp`?

Because `#include` pastes the header into **every** `.cpp` that includes it. A body in a header is then
compiled once per including file, and the linker finds several definitions of one function. Chapter 5's toy
shows it. `examples/05/twice.h` has a body, and two files include it:

```cpp
double twice(double x) { return 2.0 * x; }
```
```bash
cd docs/learn-cpp/examples/05 && mkdir -p out
c++ -std=c++17 twice_a.cpp twice_b.cpp -o out/t
```
```
duplicate symbol 'twice(double)' in:
    .../twice_b-ab4603.o
    .../twice_a-8a1de0.o
ld: 1 duplicate symbols
```

So each file gets one job:

| File | Holds | Pasted into | Compiled |
|---|---|---|---|
| `.h` | promises: declarations, class shapes | every file that uses them | never on its own |
| `.cpp` | the bodies, written **once** | nothing | once, into one `.o` |

A practical bonus: change a body in `greet.cpp` and only that file is recompiled. Change `greet.h` and every
file that includes it is recompiled.

### The exceptions, in this repo

| Case | Example | Why |
|---|---|---|
| `.cpp` with no `.h` | `src/tools/run_simulation.cpp`, `src/bindings/molsim_bindings.cpp` | no other file calls their functions, so nobody needs the promise. `main` is called by the operating system, never by your code |
| `.h` with no `.cpp`, on purpose | `class Potential` in `src/engine/force_calculator.h` | its functions are all `= 0` ("no body here; every subclass must supply one"), so there is nothing to compile. Chapter 7 |
| body inside a `.h`, on purpose | the getters in `src/builder/geometry.h`, e.g. `getAtoms() const { return atomTypes; }` | a body written inside a class is automatically `inline`: "identical everywhere, keep any one copy", so the linker does not complain. Chapter 5 shows `inline` beside `twice` |
| `.h` with no `.cpp`, on purpose | templates, e.g. chapter 9's `timing.h` | a template is a recipe with a blank; the compiler must see the whole body wherever the blank is filled in. Chapter 9 |
| `.h` with no `.cpp`, by accident | `src/engine/minimizers/*.h` | promises nobody keeps. Nothing includes them, so they never cause a link error. Dead code |

**Rule of thumb:** if other files call your functions, give them a `.h` with the promises and a `.cpp` with
the one copy of the bodies. A file nobody calls into (`main`) needs no `.h`. Things that are only shape,
`inline`, or templates can live entirely in a `.h`.

---

## 1.4 Our build: what `CMakeLists.txt` is saying (skip if you never edit the build)

Nobody types those `c++ -c` commands for a real project. `CMakeLists.txt` lists the files, and CMake
generates the commands. Read it with §1.3 in mind (`CMakeLists.txt:68-102`, shortened):

```cmake
add_library(BuilderLib STATIC                     # compile these .cpp files, bundle the .o files
    src/builder/geometry.cpp
    src/builder/molecule_builder.cpp
    ...
)
add_library(EngineLib STATIC
    src/engine/force_calculator.cpp
    src/engine/molecular_dynamics.cpp
    src/engine/potentials/lj_potential.cpp
    ...
)
target_link_libraries(EngineLib PUBLIC BuilderLib)

add_executable(run_simulation                     # compile this file and make a program
    src/tools/run_simulation.cpp
)
target_link_libraries(run_simulation PRIVATE BuilderLib EngineLib)   # link step: add these .o bundles
```

| CMake | §1.3 equivalent |
|---|---|
| `add_library(X STATIC a.cpp b.cpp)` | `c++ -c a.cpp`, `c++ -c b.cpp`, then bundle `a.o b.o` into one archive file |
| `add_executable(run_simulation run_simulation.cpp)` | `c++ -c run_simulation.cpp`, then a link step that produces a program |
| `target_link_libraries(run_simulation … EngineLib)` | put EngineLib's `.o` files on that link line |

Notice there are no header files in those lists. Headers are never compiled on their own; they only exist
pasted into some `.cpp`.

Now one rule from `CLAUDE.md` makes sense mechanically: *"New source files must be added to the relevant
`add_library`/`add_executable` list — there is no glob."* If you write `src/engine/new_thing.cpp` and forget
to list it, every file that includes `new_thing.h` **still compiles** — the declarations are there. The
definitions never reach the link, and you get exactly the `Undefined symbols` error from §1.3.

---

## Next

Chapter 2, variables and types. In C++, `double x = 1.0;` *is* an 8-byte box with a type painted on it,
and that is where chapter 3's copies, references and pointers come from.

> **Unlike Python:** there, `x = 1.0` only sticks a name tag on a float object somewhere else. Chapter 2
> shows what changes when the name *is* the storage.

Chapter 2 is also where `mini_lj` begins: the small Lennard-Jones program the rest of this track grows,
one idea per chapter, until in chapter 10 it turns out to be the repo's own reference loop. It starts with
two atoms, one distance, and a handful of `double` variables.
