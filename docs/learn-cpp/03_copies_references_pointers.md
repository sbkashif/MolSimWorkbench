# Chapter 3 — Copies, references, pointers

Plain version: [03p_copies_references_pointers_plain.md](03p_copies_references_pointers_plain.md).
Code: [`examples/03/`](examples/03/) — `mini_lj.cpp` plus small toy programs. Every output below was
produced by compiling and running them with Apple clang 17 on an M1 Mac.

**After this chapter you can say:**

- why `b = a` in C++ makes an independent copy, and what that costs and buys;
- what a **reference** `T&` is (a second name for an existing variable), and why `const T&` is the
  standard way to hand a function something to read;
- how a function returns results through **out-parameters** — `Vec3& f1` — and why `calculatePairForce`
  in the repo is shaped that way;
- what a **pointer** `T*` is, and what `&x`, `*p`, `p[i]` and `nullptr` do;
- what `std::array<double, 3>` is and what `using Vec3 = …` adds;
- how to read `char* argv[]` and `catch (const std::exception& error)`, the two shapes chapter 1 left
  unexplained.

## Where we are

At the end of chapter 2, mini_lj computed the energy and force for two atoms — but the distance `r = 1.2`
was typed in by hand. Real atoms have positions, three numbers each, and the distance must be worked out
from them across a periodic box. The moment we write functions that take positions, we meet C++'s default
from chapter 2: a variable is its box, so every assignment and every function argument is a **copy**.
This chapter is about when you want that copy, and the two ways around it: references and pointers.

Time: about 35 minutes, including running the examples. This is the most important chapter of the track.

```bash
cd docs/learn-cpp/examples/03
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## 3.0 Assignment copies

In C++ (chapter 2) a variable *is* a box. `b = a` cannot tie a second name to `a`'s box, because `b` is
already its own box. So it **copies the contents** of `a` into `b`. Afterwards the two have nothing to do
with each other. `examples/03/copy_vs_ref.cpp`, parts 1 and 3:

```cpp
double a = 1.0;
double b = a;        // b is a new variable holding a copy of a's value
b = 2.0;
std::cout << "copy:      a = " << a << ", b = " << b << std::endl;

std::array<double, 3> p = {1.0, 2.0, 3.0};
std::array<double, 3> q = p;
q[0] = 99.0;
std::cout << "array copy: p[0] = " << p[0] << ", q[0] = " << q[0] << std::endl;
```
```
$ c++ -std=c++17 -Wall -O2 copy_vs_ref.cpp -o out/copy_vs_ref && ./out/copy_vs_ref
copy:      a = 1, b = 2
...
array copy: p[0] = 1, q[0] = 99
...
```

(`std::array<double, 3>` is three `double`s in a row; §3.4 explains it. `...` marks output lines shown later.)

`q[0] = 99.0` did **not** change `p`. For every standard type in this course (numbers, `std::array`,
`std::vector`, `std::string`, `std::map`), copying copies everything inside.

| You write | What happens |
|---|---|
| `b = a` | a second **box**, filled with a copy of `a`'s contents |
| change `b` afterwards | `a` never sees it |
| want an independent copy | the default — nothing to ask |
| want a second name for the same thing | ask: a reference, `T&` (§3.1) |

> **Unlike Python:** there `b = a` never copies; it ties a second name to the same object. Whether a later
> change shows through depends on whether you **mutate** the object (`b[0] = 99.0` changes `a` too) or
> **rebind** the name (`b = 2.0` leaves `a` alone), and an independent copy has to be asked for with
> `list(a)` or `copy.deepcopy(a)`. C++ swaps the defaults: copying is free to ask for, sharing must be
> requested with `&`. That one fact explains most of the `&` you will see in the repo.

**What copying buys:** no surprise aliasing. A function handed a copy cannot change your data behind your
back. **What it costs:** time and memory. Copying three `double`s is nothing. Copying a `Geometry` with
4096 positions every time you call a function would be real work — and a function that is *meant* to fill
in your data cannot, if it only gets a copy.

---

## 3.1 A reference: a second name for the same box

A **reference** is declared with `&` after the type: `double& c = a;` reads "`c` is a reference to a
`double`, and it refers to `a`". From then on, `c` is simply another name for `a`'s box. No new box is
made. Parts 2 and 4 of `copy_vs_ref.cpp`:

```cpp
double& c = a;       // c IS a
c = 3.0;
std::cout << "reference: a = " << a << ", c = " << c << std::endl;

std::array<double, 3>& r = p;
r[0] = 99.0;
std::cout << "array ref:  p[0] = " << p[0] << std::endl;

double other = 7.0;
c = other;           // a becomes 7; c still names a
other = 8.0;
std::cout << "c = other:  a = " << a << ", other = " << other << std::endl;
```
```
reference: a = 3, c = 3
array ref:  p[0] = 99
c = other:  a = 7, other = 8
```

Three rules, all visible above:

1. **Writing through a reference writes the original.** `c = 3.0` changed `a`. It works for a plain
   `double` as well as for an array.
2. **A reference is tied once, when it is created, and never re-tied.** `c = other` did *not* make `c` a
   name for `other`. It copied `other`'s value (7) into the box `c` names, which is `a`. When `other`
   changed to 8 afterwards, `a` stayed 7. Every `=` on a reference writes into the original.
3. **A reference must be tied to something when it is created.** `double& c;` on its own is a compile error.
   There is no "empty" reference.

| Question | C++ reference |
|---|---|
| make a second name | `double& c = a;` |
| `c = <value>` | writes into `a` |
| works for a plain `double`? | yes |
| can be re-pointed later? | never |
| can refer to nothing? | no |

The symbol `&` here is part of the **type**: `double&` is "reference to `double`". It means something
different in an expression (§3.6). Spacing does not matter: `double& c`, `double &c` and `double & c` are
the same. The repo writes `double& c`.

---

## 3.2 Passing to a function: copy or the caller's own box

A function's **parameters** follow the same rule as `=`. A parameter declared `double x` is a new box,
filled with a copy of the argument. A parameter declared `double& x` is a reference to the caller's box.
`examples/03/pass_by.cpp`:

```cpp
void doubleByValue(double x) { x *= 2.0; }
void doubleByReference(double& x) { x *= 2.0; }

int main() {
    double v = 1.5;
    doubleByValue(v);
    std::cout << "after doubleByValue:     v = " << v << std::endl;
    doubleByReference(v);
    std::cout << "after doubleByReference: v = " << v << std::endl;
```
```
$ c++ -std=c++17 -Wall -O2 pass_by.cpp -o out/pass_by && ./out/pass_by
after doubleByValue:     v = 1.5
after doubleByReference: v = 3
```

(`void` = returns nothing, chapter 1. `x *= 2.0` = `x = x * 2.0`.)

**Pass by value** (`double x`): the function works on its own copy. The caller's `v` is untouched.
**Pass by reference** (`double& x`): `x` is another name for the caller's `v`, so the change is visible
after the call.

| You want… | C++ |
|---|---|
| the function cannot affect my data | `void f(double x)` / `void f(Vec3 v)` — by value |
| the function fills in my data | `void f(Vec3& v)` — by reference |
| the function reads a big object, no copy | `void f(const Vec3& v)` — §3.3 |
| the function changes my `double` | `void f(double& x)` |

> **Unlike Python:** Python has a single passing rule: the parameter is a new name tied to the caller's
> object, so mutating it shows outside and rebinding it (`x = …`) never does. Floats are immutable, so a
> function can never change the caller's float. In C++ the declaration picks copy or share for each
> parameter, for any type, and through a reference every `=` writes into the caller's box.

The call site looks the same in all cases: `doubleByValue(v)` and `doubleByReference(v)`. You can only
tell which one you get by reading the function's declaration. That is one more reason headers matter.

---

## 3.3 `const T&`: read it, do not copy it, do not touch it

Passing by reference avoids the copy, but it also lets the function change your data. Most functions only
want to **read**. Combine chapter 2's `const` with a reference and you get exactly that:
`const Vec3& v` means "`v` is another name for the caller's box, and through this name it can only be
read". `examples/03/const_ref_error.cpp` tries to write through one:

```cpp
double firstComponent(const Vec3& v) {
    v[0] = 0.0;          // error: v is read-only here
    return v[0];
}
```
```
$ c++ -std=c++17 -Wall -O2 const_ref_error.cpp -o out/const_ref_error
const_ref_error.cpp:7:10: error: cannot assign to return value because function 'operator[]' returns a const value
    7 |     v[0] = 0.0;          // error: v is read-only here
      |     ~~~~ ^
/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/array:272:55: note: function 'operator[]' which returns const-qualified type 'const_reference' (aka 'const double &') declared here
...
1 error generated.
```

The message is wordier than chapter 2's, because `v[0]` on a `std::array` is itself a function call
(`operator[]`). Read only the essential part: through a `const` array, `[]` gives back a `const double&`,
and you cannot assign to that.

This gives three ways to receive a parameter, and the repo follows one rule for choosing:

| Parameter | Copies? | Function may change it? | Use for |
|---|---|---|---|
| `double x` | yes (8 bytes — free) | its own copy only | small values: numbers, `bool` |
| `const Vec3& v` | no | no | anything bigger than a few numbers, read-only |
| `Vec3& v` | no | **yes**, and the caller sees it | outputs, and things changed in place |

**What `const T&` buys:** no copy and no risk. **What it costs:** nothing at run time; it is the
compiler's promise check again. When you see `const SomeType&` in a signature, read it as "input".
When you see a non-`const` `SomeType&`, read it as "this function writes here".

---

## 3.4 `std::array` and `using`

A position is three `double`s. `std::array<double, 3>` (from the header `<array>`) is a fixed-size group
of exactly three `double`s, stored side by side:

| | `std::array<double, 3>` |
|---|---|
| create | `std::array<double, 3> p = {1.0, 2.0, 3.0};` |
| read / write one element | `p[0]`, `p[0] = 5.0;` |
| length | always 3, fixed when compiling; cannot grow |
| element type | always `double`, fixed |
| `q = p` | copies all three (§3.0) |
| memory | 24 bytes, just the three numbers |

The `<double, 3>` part fills in two blanks in the general `std::array` recipe: the element type and the
count. That pattern — a library type with blanks in angle brackets — is a **template**; chapter 9 writes
one. For now, read `std::array<double, 3>` as one type name. `pointer.cpp` (§3.6) prints
`sizeof(v) = 24` for one: 3 × 8 bytes, nothing else.

Writing `std::array<double, 3>` everywhere is long, so mini_lj gives it a shorter name:

```cpp
using Vec3 = std::array<double, 3>;   // a new name for "three doubles"
```

`using NewName = ExistingType;` creates a **type alias**: a second name for the same type, not a new type.
A `Vec3` and a `std::array<double, 3>` can be assigned to each other freely. The repo does not use the alias; it writes
`std::array<double, 3>` in full everywhere, so expect the long form in §3.8.

Two more pieces of `std::array` syntax mini_lj uses:

- **Return a group with braces.** `return {b[0] - a[0], b[1] - a[1], b[2] - a[2]};` builds a `Vec3` from
  three values, because the function's return type says it is a `Vec3`.
- **A counting loop.** `for (int k = 0; k < 3; ++k) { … }` runs the body with `k = 0, 1, 2`. It has three
  parts: start (`int k = 0`), keep going while (`k < 3`), after each
  pass (`++k`, which adds 1 to `k`). Chapter 4 uses it heavily.

---

## 3.5 The mini_lj step: positions, a box, and functions that use references

Chapter 2 had `double r = 1.2;`. Now two atoms have positions, `a` at x = 0.2 and `b` at x = 4.0, in a
periodic box of length 5. "Periodic" means the box repeats in every direction, so `b` also has copies at
x = −1.0, 9.0, and so on. The **minimum image** rule says: measure to the nearest copy. Here that is
x = −1.0, at distance 1.2 — the same 1.2 as chapter 2, now worked out instead of typed in.

`examples/03/mini_lj.cpp`, the new parts:

```cpp
 7  using Vec3 = std::array<double, 3>;   // a new name for "three doubles"
 8
 9  // d is changed in place: the caller sees the result.
10  void applyMinimumImage(Vec3& d, double box) {
11      for (int k = 0; k < 3; ++k) {
12          d[k] -= box * std::round(d[k] / box);
13      }
14  }
15
16  // a and b are read, never copied, never changed. The result is returned by value.
17  Vec3 displacement(const Vec3& a, const Vec3& b) {
18      return {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
19  }
20
21  // Two outputs, so two reference parameters instead of a return value.
22  void pairForce(double epsilon, double sigma, const Vec3& d, Vec3& f1, Vec3& f2) {
23      const double r2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
24      const double sr2 = sigma * sigma / r2;
25      const double sr6 = sr2 * sr2 * sr2;
26      const double sr12 = sr6 * sr6;
27      const double fOverR = 24.0 * epsilon * (2.0 * sr12 - sr6) / r2;
28      for (int k = 0; k < 3; ++k) {
29          f1[k] -= fOverR * d[k];
30          f2[k] += fOverR * d[k];
31      }
32  }
```

Each function uses a different parameter style, on purpose:

| Function | Parameter | Style | Why |
|---|---|---|---|
| `applyMinimumImage` | `Vec3& d` | reference | its whole job is to change the caller's `d` in place |
| | `double box` | value | a single number — copying it is free |
| `displacement` | `const Vec3& a, const Vec3& b` | const reference | only read |
| | return type `Vec3` | **return by value** | the result is a new `Vec3`; the caller receives it |
| `pairForce` | `double epsilon, double sigma` | value | single numbers |
| | `const Vec3& d` | const reference | only read |
| | `Vec3& f1, Vec3& f2` | reference: **out-parameters** | two results; the function adds into the caller's arrays |

An **out-parameter** is a parameter the function writes its result into, instead of returning it. C++ can also
return several values at once (chapter 5's `std::pair`), but the repo's force code uses out-parameters, because forces are **accumulated**: atom i gets a contribution from every pair
it is in, so the function must *add to* an existing total (`-=`, `+=`), not hand back a fresh one.
Why one line has `-=` and the other `+=` — one spring, two ends — is explained in
[learn-parallel lesson 00](../learn-parallel/00_starting_point.md#why-one-minus-and-one-plus).

Line 12, the minimum image, on our numbers: the raw `dx` is 4.0 − 0.2 = 3.8. `3.8 / 5 = 0.76`,
`std::round(0.76)` is 1, so `dx -= 5 * 1` gives −1.2: the nearest copy of `b` is 1.2 to the *left* of `a`.

Lines 23–27 avoid `std::pow` and the square root. They work with r² and compute F/r directly
(`fOverR`); multiplying it by `d[k]` (which has length r) gives F times the unit direction. Same force,
fewer operations. The `const` on each line is chapter 2's: these values never change after they are set.

And `main`, which calls them:

```cpp
34  int main() {
35      const double box = 5.0;
36      Vec3 a = {0.2, 0.0, 0.0};
37      Vec3 b = {4.0, 0.0, 0.0};
38
39      Vec3 d = displacement(a, b);
40      std::cout << "raw dx      = " << d[0] << std::endl;
41      applyMinimumImage(d, box);
42      std::cout << "imaged dx   = " << d[0] << std::endl;
43
44      Vec3 fa = {0.0, 0.0, 0.0};
45      Vec3 fb = {0.0, 0.0, 0.0};
46      pairForce(1.0, 1.0, d, fa, fb);
47      std::cout << "force on a  = " << fa[0] << std::endl;
48      std::cout << "force on b  = " << fb[0] << std::endl;
49      return 0;
50  }
```

Line 41 is the one to stare at: `applyMinimumImage(d, box);` has no `=`, yet `d` is different afterwards.
You know it from the declaration on line 10 (`Vec3& d`), not from the call. Lines 44–45 start the forces at zero, because `pairForce` adds
into them.

```bash
c++ -std=c++17 -Wall -O2 mini_lj.cpp -o out/mini_lj
./out/mini_lj
```
```
raw dx      = 3.8
imaged dx   = -1.2
force on a  = -2.21169
force on b  = 2.21169
```

The size of the force, 2.21169, is chapter 2's `F(1.2) = -2.21169`. Now it has a direction: `a` is pulled
toward −x (toward `b`'s nearest copy, on its left) and `b` toward +x. Equal and opposite.

---

## 3.6 Pointers: a variable that holds an address

Every box has an **address**: the number of its first byte in memory. A **pointer** is a variable whose
value is an address. Where a reference is a second name that is fixed forever, a pointer is a separate
small box (8 bytes on this machine) holding "where to find" something — and that can be changed, or set to
"nowhere". `examples/03/pointer.cpp`:

```cpp
 6      double a = 1.0;
 7      double b = 2.0;
 8
 9      double* p = &a;                 // p holds the address of a
10      std::cout << "p points at a, *p = " << *p << std::endl;
11      *p = 10.0;                      // write through the pointer
12      std::cout << "after *p = 10:   a = " << a << std::endl;
13
14      p = &b;                         // unlike a reference, a pointer can be re-pointed
15      std::cout << "p re-pointed,  *p = " << *p << std::endl;
16
17      p = nullptr;                    // points at nothing
18      if (p == nullptr) std::cout << "p is nullptr now" << std::endl;
19
20      std::array<double, 3> v = {4.0, 5.0, 6.0};
21      const double* first = v.data(); // address of v[0]
22      std::cout << "first[0] = " << first[0] << ", first[2] = " << first[2] << std::endl;
23
24      std::cout << "sizeof(double*) = " << sizeof(p) << ", sizeof(v) = " << sizeof(v) << std::endl;
25      std::cout << "&v[0] = " << &v[0] << ", &v[1] = " << &v[1] << std::endl;
```
```
$ c++ -std=c++17 -Wall -O2 pointer.cpp -o out/pointer && ./out/pointer
p points at a, *p = 1
after *p = 10:   a = 10
p re-pointed,  *p = 2
p is nullptr now
first[0] = 4, first[2] = 6
sizeof(double*) = 8, sizeof(v) = 24
&v[0] = 0x16d6c6b20, &v[1] = 0x16d6c6b28
```

| Line | Syntax | Meaning |
|---|---|---|
| 9 | `double* p` | `p` is a pointer to a `double` |
| 9 | `&a` | **address-of**: the address of `a` |
| 10 | `*p` | **dereference**: the `double` that `p` points at |
| 11 | `*p = 10.0` | write into the box `p` points at — `a` becomes 10 |
| 14 | `p = &b` | change `p` itself: now it holds `b`'s address. (A reference could never do this.) |
| 17 | `nullptr` | the "points at nothing" value. Following it (`*p`) is undefined behaviour, so check first |
| 21 | `v.data()` | the address of the array's first element |
| 21 | `const double*` | a pointer through which the `double`s can only be read |
| 22 | `first[2]` | the `double` two places after the one `first` points at |
| 25 | `&v[0]`, `&v[1]` | addresses printed in hexadecimal; they differ by **8** — one `double` — because the elements sit side by side |

The addresses themselves change on every run (the operating system places the program at a random spot
for security; two more runs gave `0x16d1deb20` and `0x16f8beb20`). The gap of 8 never changes.

Line 22 is the one the parallel kernels rely on. A pointer to the first of several `double`s stored side
by side can be indexed like an array: `first[i]` means "start at `first`, step `i` doubles along, read
that one". There is **no length check**: `first[7]` would compile and read whatever memory is there.

### `&` and `*` each mean two things

The symbols are reused, and which meaning you get depends on whether they sit in a **type** or in an
**expression** (a calculation):

| Symbol | In a type (declaring something) | In an expression (using something) |
|---|---|---|
| `&` | `double& c` — `c` is a reference | `&a` — the address of `a` |
| `*` | `double* p` — `p` is a pointer | `*p` — the thing `p` points at (and `a * b` is still multiply) |

> **Unlike Python:** Python has no pointer syntax, but every Python name is, underneath, a pointer to an
> object (CPython's `PyObject*`). Python hides the address completely and never lets you point at "the 3rd
> float inside this list". That is why it needs no `*` or `&`, and why sharing is its default: assigning a
> name copies a hidden pointer.

### References or pointers?

| | copy (`T`) | reference (`T&`) | pointer (`T*`) |
|---|---|---|---|
| a new independent box? | yes | no | a small box holding an address |
| can be changed to refer elsewhere? | — | never | yes |
| can be "nothing"? | — | no | yes, `nullptr` |
| syntax to use it | `x` | `x` (looks like the original) | `*p`, `p[i]` |
| used in this repo for | small values | function parameters: inputs (`const&`) and outputs (`&`) | `argv`; the flat arrays the parallel kernels read (§3.9) |

Rule of thumb, and the repo follows it: **use a reference when you can, a pointer when you must** — when
"nothing" is a valid answer, when you need to re-point, or when you are walking through a block of numbers
by index.

---

## 3.7 Which part is `const`? (skip if you only read code)

With pointers there are two things that could be read-only: the `double`s pointed at, or the pointer
itself. The rule is that `const` applies to what is immediately to its **left**, or, if nothing is on its
left, to what is immediately to its right. Read the declaration from right to left:

| Declaration | Read right-to-left | Can change `*p`? | Can re-point `p`? |
|---|---|---|---|
| `double* p` | p is a pointer to double | yes | yes |
| `const double* p` | p is a pointer to double that is const | **no** | yes |
| `double* const p` | p is a const pointer to double | yes | **no** |

`const double* first` in `pointer.cpp` line 21, and every input pointer in the repo's kernels, is the
second row: the data is read-only, and the pointer itself can still be moved. You will rarely meet the
third row.

---

## 3.8 In the real code

### `char* argv[]` and `const std::exception&` — chapter 1's two shapes

```cpp
// src/tools/run_simulation.cpp
10  int main(int argc, char* argv[]) {
...
16      const std::string configPath = argv[1];
...
73      } catch (const std::exception& error) {
```

**Line 10, `char* argv[]`.** A `char` is a single character (1 byte). A piece of text in C — older than
`std::string` — is a run of `char`s side by side, ending in a hidden zero byte (the "end marker" from
chapter 2's `const char[6]`). It is passed around as a `char*`: a pointer to its first letter. `argv` is
a list of those pointers, one per word on the command line; the `[]` says "a list of". So `argv[1]` is a
`char*` pointing at the first letter of the first argument. `examples/03/argv_pointer.cpp` shows it:

```cpp
char* word = argv[1];              // a pointer to the first character
std::cout << "word     = " << word << std::endl;      // << follows it until the end marker
std::cout << "word[0]  = " << word[0] << std::endl;   // the first character itself
std::cout << "*word    = " << *word << std::endl;     // same thing, written with *

std::string copy = argv[1];        // run_simulation.cpp:16 does this: copy the letters into a string
copy[0] = 'X';
std::cout << "copy     = " << copy << ", argv[1] = " << argv[1] << std::endl;
```
```
$ c++ -std=c++17 -Wall -O2 argv_pointer.cpp -o out/argv_pointer && ./out/argv_pointer config.yaml
word     = config.yaml
word[0]  = c
*word    = c
copy     = Xonfig.yaml, argv[1] = config.yaml
```

`word[0]` and `*word` are the same character (§3.6: `p[0]` is `*p`). And **line 16 of `run_simulation.cpp`
is a copy**: `const std::string configPath = argv[1];` copies the letters into a new `std::string` that
owns them. Changing the copy did not change `argv[1]`. After line 16 the program never touches the raw
pointer again.

**Line 73, `const std::exception& error`.** `catch` receives the thrown error like a function parameter
(§3.2). `const …&` means: do not copy it, do not change it, just read it — the "input" pattern from §3.3.
There is a second reason it must be a reference, which chapter 7 explains: the error thrown is usually a
more specific kind (a subclass) of `std::exception`, and catching by reference keeps it intact, so
`error.what()` gives the specific message.

### The force function's signature

`src/engine/potentials/lj_potential.h:101-104` declares the repo's version of mini_lj's `pairForce`:

```cpp
101      void calculatePairForce(double epsilon, double sigma, double distance,
102                             const std::array<double, 3>& direction,
103                             std::array<double, 3>& force1,
104                             std::array<double, 3>& force2);
```

Read it with §3.3's table: three numbers by value; `direction` is an input (`const&`); `force1` and
`force2` are outputs (`&`, no `const`). Its body in `lj_potential.cpp:391-392` is mini_lj's lines 29–30:
`force1[i] -= forceMagnitude * direction[i];` and `force2[i] += …`. The one difference is the split of the
maths: the repo passes a unit `direction` and the full force size; mini_lj passes `d` and F/r. The product
is the same.

And the call, `src/engine/potentials/lj_potential.cpp:337-338`:

```cpp
337              std::array<double, 3> direction = {dx/distance, dy/distance, dz/distance};
338              calculatePairForce(epsilon, sigma, distance, direction, forces[i], forces[j]);
```

`forces[i]` and `forces[j]` are elements of a list of forces (a `std::vector`, chapter 4). Because the
parameters are references, `calculatePairForce` adds directly into atom i's and atom j's entries in that
list. No copy goes in, nothing is returned, and the totals build up pair by pair.

### The interface every potential implements

`src/engine/force_calculator.h:44-48`, from the `Potential` base class (chapter 7):

```cpp
44      virtual double calculateEnergy(const Geometry& geometry) = 0;
...
47      virtual void calculateForces(const Geometry& geometry,
48                                  std::vector<std::array<double, 3>>& forces) = 0;
```

Ignore `virtual` and `= 0` until chapter 7. The parameters tell the whole data-flow story: `const
Geometry&` — the atoms are read, never copied, never changed; `std::vector<…>& forces` — the caller's force
list is filled in. Every potential in the repo has this shape.

### A `const` reference to avoid a copy

`src/engine/potentials/lj_potential.cpp:280-282`, at the top of the reference energy loop:

```cpp
280      const auto& atoms = geometry.getAtoms();
281      const auto& positions = geometry.getPositions();
282      const auto& boxDim = geometry.getBoxDimensions();
```

`const auto&` is chapter 2's `auto` plus this chapter's `const &`: "a read-only second name for whatever
this returns, whatever its type is". Without the `&`, line 281 would copy every position in the system
into a new local list before the loop even started.

---

## 3.9 Raw pointers in the parallel kernels (deep dive later)

The parallel code in `src/engine/potentials/lj_kernels.h` deliberately uses pointers instead of
references and containers. `lj_kernels.h:27-38`:

```cpp
27      const double* x = nullptr;
28      const double* y = nullptr;
29      const double* z = nullptr;
...
33      const int* type = nullptr;
...
36      double* fx = nullptr;
37      double* fy = nullptr;
38      double* fz = nullptr;
```

Every concept here is from §3.6–3.7. `const double* x` is a read-only pointer to the first of n x
coordinates, used as `x[j]` inside the kernel. `double* fx` is a pointer through which forces are
written. Each starts as `nullptr`, "not set yet". They are filled in by `LJPotential::buildView`,
`src/engine/potentials/lj_potential.cpp:136-137`:

```cpp
136      view.x = sx.data();  view.y = sy.data();  view.z = sz.data();
137      view.fx = sfx.data(); view.fy = sfy.data(); view.fz = sfz.data();
```

— `.data()`, exactly as in `pointer.cpp` line 21, but on `std::vector`s (chapter 4). Why the kernels want
three separate lists of x, y and z instead of a list of `Vec3`s is the AoS/SoA question, taught in
[learn-parallel lesson 02](../learn-parallel/02_serial_refactor.md) (glossary:
[Memory and layout](../learn-parallel/GLOSSARY.md#memory-and-layout)). Why it is safe for these pointers
to point into storage that someone else owns — and when it stops being safe — is chapter 8.

---

## Next

mini_lj now has two atoms with real positions — `a` and `b`, each a named `Vec3` — and the
functions take them by reference. That does not scale. A real system has hundreds or thousands of atoms,
and they cannot be thousands of named variables; the single pair (a, b) has to become every pair (i, j),
each with ε and σ looked up from the atoms' types. Chapter 4 brings in containers that hold N of
something — `std::vector` and `std::map` — and a loop over every pair.
