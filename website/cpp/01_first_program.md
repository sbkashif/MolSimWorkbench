# A first program

The [module introduction](index.md) set out one running example that grows
by a few lines per chapter until it has the shape of the engine. This chapter
writes its first version: one small C++ program
that computes the Lennard-Jones energy of two atoms 1.5σ apart. On the way
it introduces the three things every C++ program has: a source file, a
compiler, and a `main` function.

## From a text file to a running program

A C++ program starts as a plain text file, here `pair.cpp`. The computer
cannot run that text directly. A **compiler** reads it, checks it, and writes
a new file, the **executable**, which the computer can run
({numref}`cpp-compile-figure`). The compiler used on this page is `clang++`;
`g++` takes the same commands.

```{figure} ../figures/cpp_compile.svg
:name: cpp-compile-figure

The source file `pair.cpp` is compiled into the executable `pair`, which runs
and prints its output. If the source has an error, the compiler stops and no
executable is made.
```

:::{admonition} If you know Python
:class: tip
Python reads and runs `pair.py` in one go: `python pair.py`. C++ splits this
into two separate steps, compile and then run. The compile step is where C++
finds many mistakes, before the program ever runs.
:::

## The program

```{literalinclude} examples/ch01/pair.cpp
:language: cpp
:linenos:
:caption: pair.cpp
```

Read from the top:

- **Lines 1–2.** The program uses two things it does not write itself:
  `std::pow`, which raises a number to a power, and `std::cout`, which
  prints. Both were written by the people who make the compiler, and ship
  with it as the **standard library**. `#include <cmath>` tells the compiler
  "before reading my file, paste in the file named `cmath`", which describes
  `std::pow`; `#include <iostream>` does the same for `std::cout`. For now
  that is all you need: `#include` is how a program gets to use code someone
  else wrote. [What the compiler does](#what-the-compiler-does) below shows
  the pasting happen.
- **Line 4.** `int main()` starts the [main function](https://en.cppreference.com/w/cpp/language/main_function).
  Every C++ program has exactly one, and running the program means running
  `main`, from its opening brace `{` to its closing brace `}` on line 10.
- **Line 5.** `double r = 1.5;` makes a **variable** named `r` that holds the
  number 1.5. `double` is its **type**: a number with a decimal point. Every
  statement ends with a semicolon.
- **Line 6.** `std::pow(1.0 / r, 6)` raises $\sigma/r$ to the sixth power (σ
  is 1 in reduced units). The result is stored in a new variable, `sr6`.
- **Line 7.** Squaring `sr6` gives $(\sigma/r)^{12}$, so this line is the LJ
  formula $4\left[(\sigma/r)^{12} - (\sigma/r)^{6}\right]$ with ε = 1.
- **Line 8.** `std::cout <<` prints each piece in turn: some text, the value
  of `r`, more text, the value of `energy`, and `"\n"`, which ends the line.
- **Line 9.** `return 0;` ends `main` and reports to the system that the
  program finished without a problem.

The `std::` in front of `pow` and `cout` says that both come from the C++
standard library, the code that ships with every compiler.

## Compile and run

In a terminal, in the folder that holds `pair.cpp`:

```console
$ clang++ -std=c++17 pair.cpp -o pair
$ ./pair
U(1.5) = -0.320337
```

The first command compiles: `-std=c++17` selects the 2017 version of the
language, which the engine also uses, and `-o pair` names the executable.
It prints nothing when it succeeds. The second command runs the executable.

To check the value, point at r = 1.5 on the interactive plot in
[The energy function](../molsim/architecture.md#the-energy-function): it
reads `U(1.50) = -0.320`.

Now change line 5 to `double r = 1.122462;`, the bottom of the well on that
plot, and compile and run again. Every change to the source needs a new
compile:

```console
$ clang++ -std=c++17 pair.cpp -o pair
$ ./pair
U(1.12246) = -1
```

`std::cout` prints six significant digits by default, which is why `r` shows
as `1.12246`.

## The parts of a computer this page uses

The rest of this page follows `pair.cpp` through the computer, so it helps
to name the four parts it passes through ({numref}`cpp-computer-figure`):

- **Disk.** Keeps files, such as `pair.cpp` and the executable `pair`, even
  when the power is off.
- **Memory.** The working storage of running programs: a long row of bytes,
  each with a number called its **address**. Much faster than the disk, but
  its contents are lost when the power is off.
- **Processor.** Both stores and works, in two parts. Its **registers** are a
  handful of storage slots inside the processor itself. Its **execution
  unit** runs the program's instructions one after another, and works only on
  values in the registers; in the figure it is running `fmov d0, #1.5`,
  which puts 1.5 into register `d0` (the next section explains such
  instructions). Since the registers are few, the processor keeps copying
  values between memory and registers.
- **Screen.** Where the output appears.

The **operating system** (macOS, Linux or Windows) is the program that
manages these parts for every other program: it starts programs, gives each
one its share of memory, and passes their output to the screen.

```{figure} ../figures/cpp_computer.svg
:name: cpp-computer-figure

The parts of a computer that `pair.cpp` passes through. The executable is
loaded from the disk into memory; the processor copies values between memory
and its registers, and its execution unit computes with them; the output
goes to the screen.
```

## What the compiler does

The single `clang++` command in {numref}`cpp-compile-figure` runs three
stages in a row ({numref}`cpp-build-figure`), and normally you see only the
final executable. Two options give you a preview of the steps in between:
`-E` shows what the first stage makes, and `-c` what the first two make.

| Command | Preview of | What it writes |
| --- | --- | --- |
| `clang++ -E pair.cpp -o pair.ii` | stage 1 | `pair.ii`: C++ text, with the headers pasted in |
| `clang++ -c pair.cpp -o pair.o` | stages 1 and 2 | `pair.o`: machine code for your lines only |
| `clang++ pair.cpp -o pair` | (no preview: all three stages) | `pair`: the executable |

The previews are only for looking inside the process; everyday work uses the
last line. The
numbers below are from a Mac with Apple clang 17; another system gives
different sizes, but the same stages.

```{figure} ../figures/cpp_build.svg
:name: cpp-build-figure

The compile arrow of {numref}`cpp-compile-figure`, opened up. Header files
join at the first stage, libraries at the last; both come with the compiler.
```

**1. Preprocess: paste in the headers.** Each `#include` line is replaced
by the whole text of the file it names. The `-E` preview writes that text
out; nothing has been compiled yet, so `pair.ii` is still C++ that you can
read:

```console
$ clang++ -std=c++17 -E pair.cpp -o pair.ii
$ wc -l pair.cpp pair.ii
      10 pair.cpp
   71811 pair.ii
```

The 10 lines of `pair.cpp` became 71,811: almost all of it is the text of
`cmath` and `iostream`, and of further headers those two include in turn.
Searching `pair.ii` finds the line that describes `pow`:

```console
$ grep -n "extern double pow" pair.ii
5757:extern double pow(double, double);
```

This line is a **declaration**: it says that a function `pow` exists, takes
two `double`s and returns a `double`. It does not contain the steps that
compute the power. A declaration is enough for the next stage to check that
line 6 of `pair.cpp` uses `pow` correctly.

**2. Compile: translate to machine code.** The processor does not read C++.
It runs **instructions**: tiny steps such as "put this number in a
register", "multiply two registers", "copy a register to memory". Compiling
turns each line of C++ into a few instructions, called **machine code**.

The `-c` preview writes the machine code to an **object file**, and
`objdump -d` lists its instructions by name:

```console
$ clang++ -std=c++17 -c pair.cpp -o pair.o
$ objdump -d pair.o
```

The listing is long; two lines of `pair.cpp` are enough to show the pattern.
First, how to read one instruction, using `fmov d0, #1.5`:

- **The first word is the instruction's name**, a short abbreviation:
  `fmov` stands for "floating-point move", that is, put a number with a
  decimal point somewhere.
- **`d0`, `d1`, `d2` are registers** that hold numbers with a decimal point.
- **`#` marks a number written into the instruction itself**, here 1.5.
- **Square brackets mark a place in memory.** `[sp, #0x18]` means "0x18 bytes
  past the address held in `sp`". `sp` and `x29` are registers that hold
  memory addresses; here they point at the stretch of memory where `main`
  keeps its variables. Read any `[...]` as "the place in memory that holds
  this variable".

Line 5, `double r = 1.5;`, becomes two instructions (numbers shortened,
`1.50000000` written as `1.5`):

| Instruction | Name stands for | What it does |
| --- | --- | --- |
| `fmov d0, #1.5` | floating-point move | put 1.5 into register `d0` |
| `stur d0, [x29, #-0x10]` | store register | copy `d0` into the place in memory that holds `r` |

Line 7, `double energy = 4.0 * (sr6 * sr6 - sr6);`, becomes seven:

| Instruction | Name stands for | What it does |
| --- | --- | --- |
| `ldr d0, [sp, #0x18]`, and the same into `d1` and `d2` | load register | copy `sr6` from memory into three registers (three instructions) |
| `fnmsub d1, d0, d1, d2` | floating-point negated multiply-subtract | `d1 = d0 × d1 − d2`, that is `sr6 * sr6 - sr6`, in one instruction |
| `fmov d0, #4.0` | floating-point move | put 4.0 into `d0` |
| `fmul d0, d0, d1` | floating-point multiply | `d0 = 4.0 × d1` |
| `str d0, [sp, #0x10]` | store register | copy the result into the place in memory that holds `energy` |

`stur` and `str` both store; they differ only in how they write the memory
place, which does not matter here.

Every line follows the same pattern: copy values from memory into registers,
compute, copy the result back. The instruction names are those of this
Mac's processor; a different kind of processor gets different instructions
for the same C++ lines.

**3. Link: add the code you did not write.** `pair.o` holds machine code for
your 10 lines only. Line 6 calls `pow`, but the instructions that compute a
power are not in `pair.o`: `#include <cmath>` gave the compiler only the
declaration of `pow`, which says that it exists. The same holds for
`std::cout`. Those instructions are already compiled, in **libraries**:
files of machine code that ship with the system, ready to be joined to
programs. The linker joins `pair.o` to the libraries it needs and writes the
executable:

```console
$ clang++ pair.o -o pair
$ otool -L pair
pair:
	/usr/lib/libc++.1.dylib
	/usr/lib/libSystem.B.dylib
```

`libc++` is the C++ standard library; `libSystem` holds, among other things,
the C math functions such as `pow`. On Linux, `ldd pair` gives the same list
there. The executable is 39 KB; it does not copy those libraries in, it
records which ones to load when it starts.

You will write your own header files later in this module, when the program
grows past one file. That chapter returns to declarations and linking. Here
it is enough that the headers describe the standard library, and the linker
supplies its code.

## What running the program does

`./pair` asks the operating system to run the executable. Following
{numref}`cpp-computer-figure` from left to right: the operating system reads
the file from the disk into memory, loads the two libraries next to it, and
starts the processor on the first instruction of `main`.

Each variable of the program gets a place in memory, at an address, while
`main` runs. This
version of the program prints, for each variable, how many bytes it takes
and at which address it lives:

```{literalinclude} examples/ch01/where.cpp
:language: cpp
:lines: 9-11
:lineno-start: 9
:caption: where.cpp (lines 9–11, added after the print of line 8)
```

`sizeof(r)` is the size of `r` in bytes. `&r` is the address of `r`; a later
chapter covers `&` properly, and here it only asks "where is `r`?".

```console
$ clang++ -std=c++17 where.cpp -o where
$ ./where
U(1.5) = -0.320337
r      is 8 bytes at 0x16d5ce9d0
sr6    is 8 bytes at 0x16d5ce9c8
energy is 8 bytes at 0x16d5ce9c0
```

Addresses are printed in hexadecimal (base 16, marked by `0x`). The three
addresses are 8 apart, and each variable is 8 bytes, so the three sit side
by side in memory ({numref}`cpp-memory-figure`). Every `double` takes 8
bytes, which is why the type had to be known before the program ran: the
compiler reserves the room. Run the program again and the addresses change,
because the operating system places the program at a different spot in
memory on each run; the sizes and the spacing of 8 stay the same.

```{figure} ../figures/cpp_memory.svg
:name: cpp-memory-figure
:width: 600px

The variables of `pair.cpp` in memory during one run. Each small cell is one
byte; each `double` fills eight.
```

When line 8 runs, `std::cout` turns the numbers into text and hands it to
the operating system, which shows it in the terminal. When `main` returns,
the operating system takes the memory back: `r`, `sr6` and `energy` exist
only while the program runs.

## A type is fixed

The type given to a variable when it is made stays with it. `r` was made as a
`double`, so it can only ever hold a number. This version tries to store
text in it:

```{literalinclude} examples/ch01/wrong_type.cpp
:language: cpp
:lines: 5-6
:lineno-start: 5
:caption: wrong_type.cpp (lines 5–6)
```

The compiler refuses, and points at the line and column of the problem:

```console
$ clang++ -std=c++17 wrong_type.cpp -o wrong_type
wrong_type.cpp:6:9: error: assigning to 'double' from incompatible type 'const char[4]'
    6 |     r = "far";
      |         ^~~~~
1 error generated.
```

`const char[4]` is how C++ describes the text `"far"`: three characters plus
one marker for the end. No executable is made, so there is nothing to run
until the error is fixed. This is the red path in {numref}`cpp-compile-figure`.

:::{admonition} If you know Python
:class: tip
In Python, `r = 1.5` and then `r = "far"` is allowed: the name `r` simply
points at a new value of a different kind, and an error, if any, appears only
when a later line tries to do arithmetic with text. In C++ the variable `r`
itself has the type `double`, and the compiler rejects the second line before
the program runs.
:::

## Check yourself

:::{dropdown} 1. You edit pair.cpp and run ./pair without compiling again. Which value does it print, the old one or the new one?
The old one. `./pair` runs the executable, and the executable is only
rewritten when you compile. Edits to `pair.cpp` reach the program only
through `clang++`.
:::

:::{dropdown} 2. Which line would you change to compute the energy at 3σ, and what would the program print?
Line 5: `double r = 3.0;`. It prints `U(3) = -0.00547944`. (This program has
no cutoff, so it gives the formula's value; the simulation, with its cutoff
at 2.5σ, would use zero for this pair.)
:::

:::{dropdown} 3. pair.o calls pow but does not contain it. Which stage puts the steps of pow into the running program, and where do they come from?
The link stage. The steps of `pow` are already compiled, in a library that
ships with the system (`libSystem` on a Mac); the linker records that the
program needs it, and the operating system loads it when the program starts.
`#include <cmath>` only supplied the declaration.
:::

:::{dropdown} 4. Remove the semicolon at the end of line 5 and compile. Does the compiler make an executable?
No. The compiler stops with `error: expected ';' at end of declaration`,
points at line 5, and makes no executable, as for the type error above.
:::

## Next

The program computes one energy for one distance written into the source.
The next chapter, [Functions](02_functions.md), turns the formula into a
**function**, so the same lines can compute the energy at any distance.
