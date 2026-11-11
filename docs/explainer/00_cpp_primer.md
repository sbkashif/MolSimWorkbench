# 00 — C++ Primer for Reading This Codebase

> Read this if you know the physics but not the C++.
>
> This primer walks **one file** — `src/engine/potentials/lj_potential.h`, all 48 lines of it — from the first line to the last, explaining every keyword the moment it first appears. That one file happens to contain almost every C++ construct this repo uses. Part 2 does the same for its implementation file. By the end you should be able to open any other file here and recognise everything in it.
>
> Open `src/engine/potentials/lj_potential.h` beside this document and follow along.

> **Note.** This primer walks the 48-line `lj_potential.h` as it was before the parallel LJ work, and
> quotes it in full below, so it reads on its own. Today's file is longer (about 105 lines): the
> additions are the backend switches, tail correction and kernel tables described in
> [`docs/implementation/parallel_lj.md`](../implementation/parallel_lj.md). Line numbers below refer to
> the quoted listing, not to the current file.

---

## The file, in full

```cpp
 1  #ifndef LJ_POTENTIAL_H
 2  #define LJ_POTENTIAL_H
 3
 4  #include <string>
 5  #include <map>
 6  #include <vector>
 7  #include <memory>
 8  #include <array>
 9  #include "../force_calculator.h"
10  #include "../../builder/geometry.h"
11
12  // Class for Lennard-Jones potential
13  class LJPotential : public Potential {
14  public:
15      LJPotential();
16      ~LJPotential() override;
17
18      // Potential interface implementation
19      double calculateEnergy(const Geometry& geometry) override;
20      void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) override;
21      std::string getName() const override { return "Lennard-Jones"; }
22
23      // LJ-specific methods
24      void addAtomParameters(const std::string& atomType, double epsilon, double sigma);
25      void setCutoff(double cutoff) { this->cutoff = cutoff; }
26      void setMixingRule(const std::string& rule) { this->mixingRule = rule; }
27
28  private:
29      // Atom parameters
30      struct AtomParameters {
31          double epsilon;
32          double sigma;
33      };
34
35      std::map<std::string, AtomParameters> atomParameters;
36      double cutoff = 10.0;  // Default cutoff
37      std::string mixingRule = "geometric";  // Default mixing rule
38
39      // Helper methods
40      std::pair<double, double> getMixedParameters(const std::string& type1, const std::string& type2);
41      double calculatePairEnergy(double epsilon, double sigma, double distance);
42      void calculatePairForce(double epsilon, double sigma, double distance,
43                             const std::array<double, 3>& direction,
44                             std::array<double, 3>& force1,
45                             std::array<double, 3>& force2);
46  };
47
48  #endif // LJ_POTENTIAL_H
```

Notice what is **not** here: no code. Not one line of arithmetic, no loop, no LJ formula. This file is a **declaration** — a list of promises about what `LJPotential` can do. The bodies live in `lj_potential.cpp` (Part 2), and Appendix A explains why they're separated.

---

# Part 1 — Reading `lj_potential.h`

## Lines 1–2 and 48 — `#`, `#ifndef`, `#define`, `#endif`

```cpp
 1  #ifndef LJ_POTENTIAL_H
 2  #define LJ_POTENTIAL_H
    ...
48  #endif // LJ_POTENTIAL_H
```

### `#` — this line is not C++

Any line starting with `#` is handled by a separate program called the **preprocessor**, which runs *before* the compiler and does nothing but shuffle text around. It doesn't know what a class is. It cuts, pastes, and deletes, then hands the compiler one giant flattened file called a **translation unit**.

You can watch it happen. `-E` means "preprocess only, then stop":

```bash
$ wc -l src/engine/potentials/lj_potential.cpp
     175

$ g++ -std=c++17 -E -Isrc src/engine/potentials/lj_potential.cpp | wc -l
   74955
```

175 lines become **74,955** before the compiler sees a single token. That growth is entirely `#include` pasting other files in.

### The three directives

| Directive | Reads as |
|-----------|----------|
| `#define LJ_POTENTIAL_H` | "Remember the name `LJ_POTENTIAL_H` as defined." It's a flag in the preprocessor's notebook — a **macro**. It has no value and never appears in your compiled program. |
| `#ifndef LJ_POTENTIAL_H` | "**if not defined** — if that name is *not* in the notebook, keep the text below. Otherwise delete everything down to the matching `#endif`." |
| `#endif` | Closes the `#ifndef` block. The `// LJ_POTENTIAL_H` after it is just a comment reminding you which one it closes. |

Together they form an **include guard**, and the sequence is:

1. **First** time this file is pasted somewhere: `LJ_POTENTIAL_H` isn't defined → `#ifndef` passes → line 2 immediately defines it → lines 3–47 get pasted.
2. **Any later** time the same file is pasted into the same translation unit: the name is now defined → `#ifndef` fails → everything down to `#endif` is deleted. The file contributes nothing the second time.

### Why you need it

Not paranoia — this repo triggers it constantly. Line 9 pastes in `force_calculator.h`, line 10 pastes in `geometry.h`, and `force_calculator.h` *also* pastes in `geometry.h`. So `geometry.h` arrives twice. Without a guard, `class Geometry { ... };` would be declared twice, and C++'s **One Definition Rule** forbids that. Here's the real error, reproduced with a two-line test file:

```
./thing.h:1:7: error: redefinition of 'Thing'
    1 | class Thing {
      |       ^
main.cpp:1:10: note: './thing.h' included multiple times, additional include site here
```

With the guard in place, it works — the class survives exactly once:

```bash
$ g++ -std=c++17 -E -Isrc src/engine/potentials/lj_potential.cpp | grep -c "class Geometry"
1
```

### Conventions

The macro name is arbitrary but must be unique across the whole program, hence the filename-in-shouty-snake-case convention (`GEOMETRY_H`, `MOLECULAR_DYNAMICS_H`). Two different headers picking the same name would silently delete one of them — a genuinely miserable bug. Most compilers accept a one-line replacement, `#pragma once`, with identical effect and no name to collide; it isn't in the official standard, though it's universally supported. This repo uses classic guards throughout, so match that style if you add a header.

One more thing: the notebook is **wiped clean for every `.cpp` file compiled**. `geometry.h` is genuinely pasted into `lj_potential.cpp`, `force_calculator.cpp`, `molecular_dynamics.cpp` and the rest — once each. That's fine and expected.

---

## Lines 4–10 — `#include`, and what `<` means

```cpp
 4  #include <string>
 5  #include <map>
 6  #include <vector>
 7  #include <memory>
 8  #include <array>
 9  #include "../force_calculator.h"
10  #include "../../builder/geometry.h"
```

### `#include` is literal copy-paste

`#include "geometry.h"` means: *find that file and paste its entire contents right here, replacing this line.* That's the whole mechanism. No import system, no module registry, no namespacing — just text substitution.

### `<...>` versus `"..."`

The two bracket styles control **where the preprocessor looks**:

| Form | Search order | Use for |
|------|-------------|---------|
| `#include <vector>` | System/compiler include directories only | Standard library, installed third-party libraries |
| `#include "geometry.h"` | The current file's own directory **first**, then the system directories | Files in your own project |

So the `<` and `>` on lines 4–8 are **not** less-than and greater-than. They are pure punctuation meaning "system header". Nothing is being compared. (`<` has two other jobs in C++ — see line 35 below.)

The quoted paths on lines 9–10 are relative to *this* file, which sits in `src/engine/potentials/`:
- `"../force_calculator.h"` → up one → `src/engine/force_calculator.h`
- `"../../builder/geometry.h"` → up two → `src/builder/geometry.h`

### Yes, `<string>` and `<map>` are real files elsewhere on your disk

They're ordinary text files in your compiler's install directory. On this machine:

```bash
$ wc -l /opt/homebrew/opt/llvm/include/c++/v1/{string,map,vector,memory,array}
    4352 .../string
    2181 .../map
    3031 .../vector
     998 .../memory
     515 .../array
```

Two surprises there:

**They have no `.h` extension.** The file is literally named `array`, no extension at all — a convention distinguishing C++ standard headers from C ones (`<cmath>` is the C++ spelling of C's `<math.h>`). The preprocessor doesn't care; it opens whatever filename you name.

**They use the exact same guard pattern as line 1.** Head of the real `<array>`:

```cpp
#ifndef _LIBCPP_ARRAY
#define _LIBCPP_ARRAY
```

The leading underscore-plus-capital is a name form reserved for the standard library, so its guards can never collide with yours.

### What each include on this page is actually for

An include earns its place only if something in the file needs it:

| Line | Gives you | The line here that needs it |
|------|-----------|------------------------------|
| `<string>` | `std::string` — growable text that manages its own memory | line 21, 24, 37 |
| `<map>` | `std::map<K,V>` — sorted key→value dictionary, `O(log n)` lookup | line 35 |
| `<vector>` | `std::vector<T>` — growable array that owns its memory | line 20 |
| `<memory>` | The smart pointers (`shared_ptr`, `unique_ptr`) | nothing here, actually — headers often over-include; harmless |
| `<array>` | `std::array<T,N>` — fixed-size array, size baked into the type | lines 20, 43–45 |
| `"../force_calculator.h"` | The `Potential` base class | line 13 — **required**, you cannot inherit from a name you've only heard of |
| `"../../builder/geometry.h"` | The `Geometry` class | lines 19, 20 |

That last distinction matters. To **inherit** from a class, or to hold one **by value** as a member, the compiler needs its full size and layout — you need the real header. To merely take one by **reference or pointer**, a bare `class Geometry;` would do, because every pointer is the same size whatever it points at. That shortcut is called a **forward declaration**, and `force_calculator.h` uses it (`class Potential;`) to break the circular dependency between those two files. You'll also see it in `geometry.h` (`class YAMLParser;`) and `molecular_dynamics.h` (`class TrajectoryWriter;`).

---

## Line 12 — comments

```cpp
12  // Class for Lennard-Jones potential
```

`//` comments to end of line; `/* ... */` spans lines. Stripped out before compilation, exactly like the preprocessor directives.

---

## Line 13 — `class`, `:`, `public`, and inheritance

```cpp
13  class LJPotential : public Potential {
```

Three separate ideas in one line.

### `class LJPotential`

A **class** is a blueprint bundling data (**member variables**) with the functions that operate on it (**member functions**, a.k.a. **methods**). Every `LJPotential` **object** created from this blueprint gets its own copy of the member variables on lines 35–37 — its own `cutoff`, its own parameter map.

### `: public Potential` — the inheritance clause

Read `:` here as "**is a**". `LJPotential` **inherits** from `Potential`:

- **`Potential`** is the **base class** (also called parent class, or superclass).
- **`LJPotential`** is the **derived class** (also called child class, or subclass).

The derived class automatically gets everything the base declared, and must fulfil any promises the base left unfulfilled. `public` inheritance means the "is-a" relationship is visible to everyone: anywhere the code wants a `Potential`, an `LJPotential` is accepted.

`DFTPotential` does the same thing. The two of them are **siblings** — each derives from `Potential`, neither knows the other exists. (`MachineLearningPotential` looks like a third sibling but is not: it does not derive from `Potential` and is not built — see the "unbuilt" note near the end of this primer.)

### Go look at the base class

Line 9 pasted it in. From `src/engine/force_calculator.h`:

```cpp
class Potential {
public:
    virtual ~Potential() = default;

    virtual double calculateEnergy(const Geometry& geometry) = 0;
    virtual void calculateForces(const Geometry& geometry,
                                 std::vector<std::array<double, 3>>& forces) = 0;
    virtual std::string getName() const = 0;
};
```

Decoding it:

- **`virtual`** — "a derived class is allowed to replace this method, and when someone calls it through a base-class handle, the **derived** version runs." Without `virtual` the base version would run instead: the bug that makes polymorphism silently not work.
- **`= 0`** — a **pure virtual function**. No body; every derived class *must* provide one. A class with any pure virtual is an **abstract class**: you cannot create a `Potential` object at all. It's a contract and nothing else.
- **`virtual ~Potential() = default;`** — a **virtual destructor**, mandatory on any base class you might delete through a base-class pointer; without it the derived part never gets cleaned up. `= default` means "compiler, write the obvious one for me."

So line 13 signs `LJPotential` up to three obligations, which lines 19–21 discharge.

### `{` and line 46's `};`

The class body runs from the `{` here to the `}` on line 46. **That semicolon after the closing brace is required** — a leftover from C, and forgetting it produces a spectacularly confusing error pointing at the *next* line. Functions don't take one; classes and structs do.

---

## Lines 14 and 28 — `public:` and `private:`

```cpp
14  public:
    ...
28  private:
```

These are **access specifiers**, and they apply to everything below them until the next one:

- **`public:`** — anyone can use this. Lines 15–26 are the class's outward-facing interface.
- **`private:`** — only code inside this class may touch it. Lines 29–45 are internal.

So outside code can call `lj.setCutoff(12.0)`, but cannot write `lj.cutoff = 12.0` — the member on line 36 is private. This is **encapsulation**: the class controls its own data, and you can change how `cutoff` is stored later without breaking every caller.

There's a third, `protected:` (visible to derived classes but not the outside world), not used in this file.

---

## Lines 15–16 — constructor and destructor

```cpp
15      LJPotential();
16      ~LJPotential() override;
```

Two special member functions. Neither is ever called explicitly — the compiler inserts the calls for you.

### `LJPotential()` — the constructor

Same name as the class, **no return type**, not even `void`. That's how you recognise one. It runs automatically when an object is created, and its job is to put the object into a valid initial state. The empty `()` means no arguments, so `LJPotential lj;` calls it.

A class can have several constructors taking different arguments (**overloading**). This one has just the no-argument form, because the real configuration happens afterwards through `addAtomParameters` and `setCutoff`.

### `~LJPotential()` — the destructor

Same name with a **`~`** (tilde) prefix. That's the entire syntax rule: `~ClassName()`, no arguments, no return type, and there can only ever be one per class.

The **destructor** runs automatically when the object dies. "Dies" means one of:

```cpp
{
    LJPotential lj;             // constructor runs here
    ...
}                               // destructor runs here — end of scope

auto p = std::make_shared<LJPotential>();
...                             // destructor runs when the last shared_ptr goes away

std::vector<LJPotential> v;     // each element's destructor runs when v is destroyed
```

Its job is to release whatever the object was holding — file handles, memory, locks, network connections.

### What this one actually does: nothing

Open `lj_potential.cpp:9`:

```cpp
LJPotential::~LJPotential() {
    // Destructor
}
```

An empty body. That is not a bug or an oversight — it's the normal case, and understanding *why* explains a lot about modern C++.

Look at what `LJPotential` owns (header lines 35–37): a `std::map`, a `double`, a `std::string`. Every one of those **already cleans up after itself** — `std::map` has its own destructor that frees its nodes, `std::string` frees its character buffer. When `~LJPotential()` finishes, the compiler automatically destroys each member in turn. There is nothing left for you to do.

This is the principle called **RAII** (Resource Acquisition Is Initialization): a resource is owned by an object, acquired in its constructor and released in its destructor, so it can't leak no matter how the scope is exited — including by an exception. It's why C++ has no `finally` block and why you almost never see `delete` in well-written modern code. `std::shared_ptr` is RAII applied to memory; `std::vector` is RAII applied to arrays.

The **Rule of Zero** follows from it: if your class members all manage themselves, **don't write a destructor at all** — let the compiler generate one. Most classes in this repo could follow that. `LJPotential` writes an empty one anyway, which is harmless, and there's one legitimate reason to do it deliberately (below).

You'd only need a real body if the class held something raw — a `FILE*` from `fopen`, memory from `new`, a socket. Nothing here does.

### The destruction order

When an `LJPotential` is destroyed, three things happen in this order:

1. `~LJPotential()`'s body runs (empty here);
2. its members are destroyed — `mixingRule`, `cutoff`, `atomParameters` — in **reverse** declaration order;
3. **then** the base class destructor, `~Potential()`, runs.

Derived first, base last — the exact reverse of construction, which builds the base first. That symmetry is what makes the next part matter.

### Why the base class marks its destructor `virtual`

Go back to `force_calculator.h`:

```cpp
class Potential {
public:
    virtual ~Potential() = default;
    ...
};
```

That `virtual` is not decoration. Here's what it prevents, as a runnable demo — two base classes identical except for the keyword, each deleted through a **base-class pointer**:

```cpp
struct BaseNoVirtual        { ~BaseNoVirtual() { std::cout << "  ~BaseNoVirtual\n"; } };
struct DerivedA : BaseNoVirtual { ~DerivedA()  { std::cout << "  ~DerivedA\n"; } };

struct BaseVirtual  { virtual ~BaseVirtual() { std::cout << "  ~BaseVirtual\n"; } };
struct DerivedB : BaseVirtual { ~DerivedB() override { std::cout << "  ~DerivedB\n"; } };

int main() {
    { BaseNoVirtual* p = new DerivedA(); delete p; }
    { BaseVirtual*   p = new DerivedB(); delete p; }
}
```

```
delete through base pointer, NON-virtual destructor:
  ~BaseNoVirtual
delete through base pointer, VIRTUAL destructor:
  ~DerivedB
  ~BaseVirtual
```

In the first case **`~DerivedA` never runs.** The pointer's static type is `BaseNoVirtual*`, so without `virtual` the compiler resolves `delete p` at compile time to the base destructor only. Everything the derived class was holding leaks, silently, with no error and no warning. (The standard actually calls this undefined behaviour.)

In the second case the call is dispatched at runtime to `~DerivedB`, which then chains to `~BaseVirtual` — the derived-then-base order from above.

Now connect it to this repo. `ForceCalculator` stores `std::vector<std::shared_ptr<Potential>>` — base-class handles holding derived objects. When that vector is cleared, each `shared_ptr` deletes a `Potential*` that is really an `LJPotential*`. **Without `virtual ~Potential()`, `~LJPotential()` would never run**, and the `std::map` of atom parameters would leak on every simulation teardown.

The rule: **any class with a `virtual` function needs a `virtual` destructor.** If a class is meant to be inherited from and deleted polymorphically, mark it. `= default` there just means "compiler, generate the obvious body" — you get the `virtual` without writing a body.

### `override` on line 16

Same meaning as on lines 19–21 (next section): *I'm replacing a base-class virtual.* Since `~Potential()` is virtual, `~LJPotential()` is automatically virtual too — inherited virtualness is not optional. Writing `override` doesn't change the behaviour; it makes the compiler confirm that a base-class virtual destructor really exists.

**And that's the one good reason to declare this destructor at all.** By the Rule of Zero it could have been omitted entirely. Writing `~LJPotential() override;` documents at a glance that this class sits in a polymorphic hierarchy and is safe to delete through a base pointer.

---

## Lines 19–21 — the interface methods

```cpp
19      double calculateEnergy(const Geometry& geometry) override;
20      void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) override;
21      std::string getName() const override { return "Lennard-Jones"; }
```

These three discharge the three pure-virtual obligations from the base class. Five new concepts.

### `void`

Line 20's return type. `void` means **returns nothing**. Compare line 19, which returns a `double`. If a function returns nothing, it must be doing its work by side effect — and here that side effect is the second parameter.

### `&` — a reference

This is the single biggest adjustment coming from Python. There are three ways to hand an object to a function:

```cpp
void f(Geometry g);          // BY VALUE — copies the whole object. Changes don't escape.
void f(Geometry& g);         // BY REFERENCE — an alias for the caller's object. Changes DO escape.
void f(const Geometry& g);   // BY CONST REFERENCE — an alias, but read-only. No copy, no changes.
```

**A reference is just another name for an existing object.** Not a copy, not a pointer — an alias. Once bound it can never be re-pointed at something else.

`const Geometry& geometry` is the workhorse idiom of this entire repo, and it means: *I'm giving you the real object rather than a copy (fast — a `Geometry` may hold a thousand atoms), and I promise not to modify it.* When you see it in a signature, read it as **"input"**.

### The out-parameter convention

Line 20 has two parameters with two different roles:

```cpp
void calculateForces(const Geometry& geometry,                        // input: const ref
                     std::vector<std::array<double, 3>>& forces)      // OUTPUT: non-const ref
     ^^^^ returns nothing
```

The second parameter is a reference **without** `const`, so the function writes into the caller's vector rather than returning a new one. That's why the return type is `void`. The caller creates the vector, passes it in, and reads it after the call:

```cpp
std::vector<std::array<double, 3>> forces;
potential.calculateForces(geometry, forces);   // forces is filled in here
```

**Watch for the missing `const`** — that's your signal a parameter is being written to. This pattern exists to avoid allocating and copying a fresh vector on every one of a million MD steps.

### `const` in its two positions

Line 21 has `const` in a place line 19 doesn't:

```cpp
19      double calculateEnergy(const Geometry& geometry) override;
                              ^^^^^ (1) on a parameter
21      std::string getName() const override { ... }
                            ^^^^^ (2) after the parameter list
```

1. **On a parameter type** — "the object cannot be modified through this handle."
2. **After the parameter list** — "**this method** does not modify the object it is called on." A `const` method can be called on a `const` object; a non-`const` one cannot.

Position 2 is why every getter in `geometry.h` is marked `const`. Without it, `calculateEnergy(const Geometry& geometry)` on line 19 couldn't call `geometry.getAtoms()` — you'd be calling a possibly-modifying method on an object you promised not to modify, and the compiler would reject it. Const-correctness is contagious in exactly this way.

### `override`

Says: *I intend to be replacing a base-class virtual function.* It's optional, but the compiler errors if there's no matching virtual in the base — so if you typo the name, change an argument type, or forget the `const` on line 21, you find out immediately instead of silently creating an unrelated new method that never gets called. Free bug protection.

When reading, treat `override` as a reliable signal: **this method is fulfilling a base-class contract**, so go look at the base class to understand what it's promising.

### A body in the header

Line 21 ends with `{ return "Lennard-Jones"; }` instead of `;` — the method is defined right here. That's allowed for short methods; defining a method inside the class body makes it implicitly `inline`, which lets it be pasted into many files without the linker complaining (Appendix A.4). Reserved by convention for one-liners: getters, setters, trivial forwarding.

---

## Lines 24–26 — the LJ-specific methods, and `this->`

```cpp
24      void addAtomParameters(const std::string& atomType, double epsilon, double sigma);
25      void setCutoff(double cutoff) { this->cutoff = cutoff; }
26      void setMixingRule(const std::string& rule) { this->mixingRule = rule; }
```

These exist **only** on `LJPotential`, not on `Potential`. No `override`, because they're not fulfilling any base-class promise — a derived class is free to add methods of its own.

That has a consequence worth internalising: if you're holding a `Potential` handle, you can call `calculateEnergy` but **not** `setCutoff`, because the base class never promised it. You must know you have an `LJPotential` specifically. That's why setup code creates a concrete `LJPotential`, configures it via these methods, and only then hands it off as a `Potential`.

### `this->`

`this` is a pointer to "the object I am currently inside". `->` gets at a member through a pointer (more on `->` in Part 2). So `this->cutoff` means "the `cutoff` member variable of this object."

Why is it needed here? Because the parameter is *also* called `cutoff` and shadows the member:

```cpp
void setCutoff(double cutoff) { this->cutoff = cutoff; }
//                              ^^^^^^^^^^^^   ^^^^^^ the parameter
//                              the member (line 36)
```

Without `this->` you'd write `cutoff = cutoff`, assigning the parameter to itself — a no-op the compiler would happily accept. Most methods in this repo don't need `this->`; members are reachable by bare name.

### `double`

The numeric type used for essentially every physical quantity here: 64-bit floating point, ~15–17 significant digits. C++ also has `float` (32-bit, ~7 digits), which you won't see in this codebase — MD energy conservation needs the precision.

---

## Lines 30–33 — `struct`

```cpp
30      struct AtomParameters {
31          double epsilon;
32          double sigma;
33      };
```

A **struct** is a class whose members default to `public` instead of `private`. That is the *only* language-level difference in C++. By convention — followed here and nearly everywhere — `struct` is used for a dumb bag of data with no behaviour, `class` for something with methods and invariants to protect.

`AtomParameters` groups the two LJ parameters so they travel together: one lookup returns both ε and σ. It's declared *inside* the class and after `private:`, so it's an implementation detail — its full name is `LJPotential::AtomParameters` and outside code can't use it.

Note the `;` on line 33, same rule as line 46.

---

## Lines 35–37 — member variables, templates, and defaults

```cpp
35      std::map<std::string, AtomParameters> atomParameters;
36      double cutoff = 10.0;  // Default cutoff
37      std::string mixingRule = "geometric";  // Default mixing rule
```

These are the **member variables** (also: data members, fields) — the actual state. Every `LJPotential` object carries its own set.

### `std::` — the namespace

`std` is the **namespace** of the C++ standard library, and `::` is the **scope resolution operator**, which you can read as "belonging to". `std::map` means "the `map` that belongs to `std`". It's a surname, not an operation — there to stop the library's `map`, `array`, `string` from colliding with anyone else's.

### `<...>` again — templates

Here `<` has a completely different job than on line 4:

```cpp
#include <map>                                 // line 5: punctuation, "system header"
std::map<std::string, AtomParameters> params;  // line 35: template arguments
for (size_t i = 0; i < atoms.size(); ++i)      // in the .cpp: actual less-than
```

Tell them apart by what comes immediately before the `<`:
- right after `#include` → header punctuation;
- right after a **type name** (`map`, `vector`, `array`, `shared_ptr`) → **template arguments**;
- between two **values** → comparison.

A **template** is a recipe with blanks. `std::map` on its own isn't a type — it's "a dictionary from *something* to *something*". Filling the blanks makes a real type: `std::map<std::string, AtomParameters>` is a dictionary **from** atom-type strings **to** `AtomParameters` structs. Look up `"O"`, get back its ε and σ.

Templates nest, which is where line 20's thicket comes from:

```cpp
std::vector<std::array<double, 3>> forces
```

Read outside-in: **a growable list, of fixed 3-element arrays of double** — i.e. one xyz force vector per atom. `std::array<double, 3>` is the repo's universal representation of a 3-vector; note that its size `3` is part of its *type*, not runtime data.

### `= 10.0` — default member initializers

Lines 36–37 give starting values right in the declaration. If the constructor doesn't set them, these apply — so a freshly built `LJPotential` has a 10 Å cutoff and geometric mixing without anyone specifying either. Line 35 has no `= ...` because a `std::map` starts empty on its own.

---

## Lines 40–45 — private helpers, and `std::pair`

```cpp
40      std::pair<double, double> getMixedParameters(const std::string& type1, const std::string& type2);
41      double calculatePairEnergy(double epsilon, double sigma, double distance);
42      void calculatePairForce(double epsilon, double sigma, double distance,
43                             const std::array<double, 3>& direction,
44                             std::array<double, 3>& force1,
45                             std::array<double, 3>& force2);
```

Private methods: internal machinery, callable only from inside `LJPotential`. This is where the actual physics lives — the mixing rules and the LJ formula itself.

**`std::pair<double, double>`** (line 40) is another template: two values glued into one. It's how a function returns two things at once, since C++ functions return a single value. Here: `(epsilon, sigma)` for a given pair of atom types. Part 2 shows how the caller unpacks it.

**Lines 42–45** are the out-parameter pattern again, at full stretch: three inputs by value (`double`), one input by const reference (`direction`), and **two outputs** (`force1`, `force2` — non-const references). Newton's third law made literal: compute the pair force once, write it into atom *i*'s slot and the negative into atom *j*'s.

That's the whole file. Every construct in it now has a name.

---

# Part 2 — Reading `lj_potential.cpp`

Same class, the implementation side. This is where the promises get bodies.

## The top of the file

```cpp
1  #include "lj_potential.h"
2  #include <cmath>
3  #include <iostream>
```

Line 1 pastes in the header you just read — that's how the compiler knows what `LJPotential` is. Every `.cpp` in this repo includes its own header first, and that's the convention to follow: it guarantees the header compiles standalone.

`<cmath>` brings in `std::sqrt`, `std::round`, `std::pow`.

## `::` again — defining a method

```cpp
double LJPotential::calculateEnergy(const Geometry& geometry) {
//     ^^^^^^^^^^^^^ "this is LJPotential's calculateEnergy, promised on line 19 of the header"
```

Same `::` as in `std::map`, same meaning — "belonging to". Without the `LJPotential::` prefix this would be an unrelated free function that happens to share a name.

Two things from the header do **not** get repeated here: `virtual`/`override` (declared once, in the header) and default argument values. Repeat them and the compiler objects.

## The worked example, line by line

`LJPotential::calculateEnergy`, `src/engine/potentials/lj_potential.cpp:17`. Everything you need is now in hand:

```cpp
double LJPotential::calculateEnergy(const Geometry& geometry) {
    double totalEnergy = 0.0;
```
> Returns a `double`. Takes a read-only, no-copy handle on the caller's geometry. A local accumulator starts at zero — locals are **not** zero-initialised automatically in C++, so that `= 0.0` is doing real work.

```cpp
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& boxDim = geometry.getBoxDimensions();
```
> **`auto`** means "compiler, work out the type yourself." `const auto& atoms` is exactly `const std::vector<std::string>& atoms` — the `&` matters enormously: without it you'd **copy the entire atom list** on every energy evaluation. `const auto&` is the idiom for "borrow, don't copy."
>
> **`.`** (not `->`) because `geometry` is a reference, not a pointer. And these three getters are callable at all only because they're marked `const` in `geometry.h` — the const-correctness chain from line 19 paying off.

```cpp
    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
```
> The classic C-style loop: initialise, test, advance. **`size_t`** is the unsigned integer type used for sizes and indices — `.size()` returns one, so the counter matches. **`++i`** increments by one.
>
> The physics is in `j = i + 1`: each pair is visited exactly once, with no self-interaction and no double counting. The O(N²) all-pairs sum.

```cpp
            double dx = positions[j][0] - positions[i][0];
```
> `positions[j]` is the `std::array<double,3>` for atom *j*; `[0]` is its x. Chained `[]` because the type is a vector **of** arrays. `[]` does no bounds checking — fast, and your responsibility. (`geometry.cpp` uses `.at(index)` in its getters, which *does* check and throws `std::out_of_range`.)

```cpp
            if (boxDim[0] > 0.0) {
                dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
            }
```
> Minimum image convention, guarded by "is a box even set." `std::round` from `<cmath>`.

```cpp
            if (distance > cutoff || distance < 1e-10) continue;
```
> **`cutoff` appears with no declaration in sight** — because it's a member variable (header line 36). Inside a method, members are in scope by bare name. This is the single most common "where did that variable come from?" moment when reading C++: if you can't find a local, look at the class's private section.
>
> `||` is logical OR; `continue` skips to the next `j`.

```cpp
            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
```
> A **structured binding**: `getMixedParameters` returns the `std::pair<double,double>` from header line 40, and this unpacks it into two named variables in one line. Same idea as Python's `a, b = f()`.
>
> Called with no object prefix because we're already inside an `LJPotential` — it's the private helper from line 40, implicitly `this->getMixedParameters(...)`.

```cpp
            totalEnergy += calculatePairEnergy(epsilon, sigma, distance);
        }
    }
    return totalEnergy;
}
```
> The other private helper (line 41) holds the actual 4ε[(σ/r)¹² − (σ/r)⁶]. Accumulate, and return.

## Pointers and polymorphism — `force_calculator.cpp`

One construct the LJ files don't show. From `src/engine/force_calculator.cpp`:

```cpp
double ForceCalculator::calculateEnergy(const Geometry& geometry) {
    double totalEnergy = 0.0;
    for (const auto& potential : potentials) {
        totalEnergy += potential->calculateEnergy(geometry);
    }
    return totalEnergy;
}
```

### The range-for loop

```cpp
for (const auto& potential : potentials)     // read-only pass over each element
for (auto& force : forces)                   // '&' without const → modify in place
for (auto potential : potentials)            // copies each element — usually a mistake
```

Python's `for x in xs`, with an explicit choice about copying.

### `*` — a pointer

`potentials` is declared `std::vector<std::shared_ptr<Potential>>`. A **pointer** is a variable holding a memory address. Unlike a reference it can be reassigned, can be null, and needs explicit dereferencing. The access syntax is how you tell them apart while reading:

```cpp
geometry.getAtoms()      // '.'  — a value or a reference
potential->getName()     // '->' — a pointer; shorthand for (*potential).getName()
```

### Smart pointers

`std::shared_ptr<Potential>` is a **smart pointer**: it owns the object and deletes it automatically when the last `shared_ptr` to it goes away. No manual `delete`, no leaks. Two flavours appear in this repo:

- `std::unique_ptr<T>` — exactly one owner, cheapest, cannot be copied.
- `std::shared_ptr<T>` — reference-counted, many owners. Used for potentials because both your setup code and the `ForceCalculator` hold them.

You create one with `std::make_shared`:

```cpp
auto ljPotential = std::make_shared<LJPotential>();
ljPotential->setCutoff(12.0);          // LJ-specific method — needs the concrete type
forceCalc->addPotential(ljPotential);  // accepted as a shared_ptr<Potential>
```

### The payoff: polymorphism

That loop has no idea whether any given element is an LJ, DFT, or ML potential. `potential->calculateEnergy(geometry)` dispatches **at runtime** to whichever derived class is actually there — the `virtual` keyword from the base class doing its work. That is **polymorphism**, and it only functions through a pointer or reference to the base, never through a base-class *value*.

It's also the reason the engine is pluggable: `ForceCalculator` sums energies and forces over an arbitrary list of potentials without a single `if` testing which kind each one is. Add a fourth potential class tomorrow and this loop needs no change.

---

# Appendix A — Why `.h` and `.cpp` at all

If `#include` just pastes text, why not one file?

Because **only headers get pasted — `.cpp` files never are.** Nothing in this repo includes a `.cpp`, and that asymmetry is the entire point.

## A.1 Where `geometry.cpp` is when `lj_potential.cpp` compiles

Nowhere near it. Line 10 of the header pastes in `geometry.h` — the promises — and nothing else:

```bash
$ g++ -std=c++17 -E -Isrc src/engine/potentials/lj_potential.cpp > expanded.txt

$ grep -n "void addAtom" expanded.txt          # the DECLARATION, from geometry.h
69056:    void addAtom(const std::string& atomType, const std::array<double,3>& coordinates);

$ grep -c "Geometry::addAtom" expanded.txt     # the BODY, from geometry.cpp
0
```

Declaration present; body appears **zero times**. `lj_potential.cpp` is compiled *knowing `addAtom` exists and what arguments it takes*, with no idea what it does.

## A.2 Each `.cpp` is its own lane

```
  geometry.cpp              lj_potential.cpp          force_calculator.cpp
       │                          │                          │
  [PREPROCESS]              [PREPROCESS]              [PREPROCESS]
  pastes geometry.h         pastes lj_potential.h,    pastes force_calculator.h,
                            geometry.h, <map>, …      geometry.h, <vector>, …
       ▼                          ▼                          ▼
  [COMPILE] — independently, in parallel; none knows the others exist
       ▼                          ▼                          ▼
  geometry.cpp.o          lj_potential.cpp.o       force_calculator.cpp.o
       └──────────────────────────┼──────────────────────────┘
                                  ▼
                            [LINK]  ← the only place they meet
                                  ▼
                    run_simulation  /  _molsim.so
```

Headers flow *down into* many lanes; sources stay in their own lane. Your build directory is literally this picture — `find build -name '*.o'` shows one `.o` per `.cpp`, no exceptions.

## A.3 What a `.o` looks like from outside

`nm` lists the names in an object file. Two letters matter: **`T`** = defined here, **`U`** = **U**ndefined, a hole for someone else to fill.

`force_calculator.cpp` calls `geometry.getNumberOfAtoms()`:

```bash
$ nm build/CMakeFiles/EngineLib.dir/src/engine/force_calculator.cpp.o | c++filt
000000000000014c T ForceCalculator::calculateEnergy(Geometry const&)
                 U Geometry::getNumberOfAtoms() const        ← a hole

$ nm build/CMakeFiles/BuilderLib.dir/src/builder/geometry.cpp.o | c++filt
00000000000001e4 T Geometry::getNumberOfAtoms() const        ← the body
```

The compiler emitted a call instruction with a **blank where the address goes**, plus a note saying "someone fill this in." It had `geometry.h`, so it knew the call was legal; it had no body, so it couldn't generate one.

**Linking is just matching every `U` to exactly one `T`** and patching in real addresses. Which explains the two errors you'll actually hit:

| Error | Meaning |
|-------|---------|
| `undefined symbol: Geometry::getNumberOfAtoms()` | a `U` with **no** matching `T` — body never written, or the `.cpp` isn't in `CMakeLists.txt` |
| `duplicate symbol: Thing2::square(double)` | a `U` with **two** matching `T`s — a function body was put in a header and two `.cpp` files pasted it |

That second one is the hard constraint: a declaration may repeat freely, but a **body may exist exactly once in the program** (the One Definition Rule). Fourteen files in this repo include `geometry.h`. If `addAtom`'s body lived there, all fourteen would carry a copy and the link would fail. Putting it in `geometry.cpp` — compiled once — is what makes fourteen includes legal.

The other payoff is build time: edit a `.cpp` and **one** file recompiles; edit a `.h` and everything that pastes it does. Hence "keep headers small, put the bulk in the `.cpp`."

## A.4 The exceptions — where code *does* live in a header

1. **One-line methods inside the class body** (header lines 21, 25, 26). Defining a method there makes it implicitly `inline`: "expect many identical copies, keep one, don't complain."
2. **Templates.** `std::vector<double>` and `std::vector<std::string>` compile to different machine code, so nothing can be generated until the compiler sees which type you used — the implementation *must* be visible in the header. That's why `<vector>` is 3,031 lines with no `.cpp` anywhere, and why header-only libraries like `nlohmann_json` exist.
3. **Header-only by choice**, for small libraries where one `#include` beats the compile-time cost.

## A.5 Which `.cpp` files exist at all — `CMakeLists.txt`

Nothing in the source names a `.cpp`. That list lives in exactly one place:

```cmake
add_library(BuilderLib STATIC
    src/builder/geometry.cpp              ← there it is
    src/builder/molecule_builder.cpp
    src/builder/lj_particle_box_builder.cpp
    src/utils/yaml_parser.cpp
    src/utils/trajectory_writer.cpp
)
add_library(EngineLib STATIC
    src/engine/force_calculator.cpp
    src/engine/molecular_dynamics.cpp
    src/engine/potentials/lj_potential.cpp
    src/engine/potentials/dft_potential.cpp
)
target_link_libraries(EngineLib PUBLIC BuilderLib)

add_executable(run_simulation src/tools/run_simulation.cpp)
pybind11_add_module(_molsim   src/bindings/molsim_bindings.cpp)
target_link_libraries(run_simulation PRIVATE BuilderLib EngineLib)
target_link_libraries(_molsim        PRIVATE BuilderLib EngineLib)
```

- `add_library` / `add_executable` — "compile exactly these `.cpp` files into this target."
- `target_link_libraries(EngineLib PUBLIC BuilderLib)` — "when EngineLib's objects have holes, look in BuilderLib." That one line is what closes the `U` in A.3.
- `target_include_directories(... /src)` — what puts `-Isrc` on the real compile command.

Note the direction: **`BuilderLib` knows nothing about `EngineLib`.** Engine → builder, one way only. A potential may use a `Geometry`; a `Geometry` never uses a potential. That's the layer stack from [01_architecture.md](01_architecture.md), enforced right here.

**A `.cpp` not listed here does not exist as far as the program is concerned.** Two files in this repo are in that position:

```bash
$ find src -name '*.cpp' | wc -l                                    # 13 on disk
$ grep -o 'src/[a-z_/]*\.cpp' CMakeLists.txt | sort -u | wc -l      # 11 actually built
```

The unbuilt two are `src/engine/potentials/machine_learning_potential.cpp` and `src/utils/file_reader.cpp`. No `.o` is ever produced for them, so a call into `MachineLearningPotential` would fail at **link** time with `undefined symbol`, not at compile time. Worth knowing before you go looking for the ML potential and wonder why it never runs. (`src/engine/integrators/` and `src/engine/minimizers/` are dead for a related reason — headers with no `.cpp` at all, and nothing includes them.)

---

# Appendix B — Decoder table

| You see | It means |
|---------|----------|
| `#include <vector>` | paste in a system/library header |
| `#include "geometry.h"` | paste in a project file, path relative to this one |
| `#ifndef X` / `#define X` / `#endif` | include guard — paste this file at most once per `.cpp` |
| `class Foo;` | forward declaration — the name exists, details later |
| `class B : public A` | B is a derived/child class of base/parent A |
| `virtual f()` | derived classes may replace it; dispatch happens at runtime |
| `virtual f() = 0` | pure virtual — derived classes **must** implement it; the class is abstract |
| `override` | this replaces a base-class virtual (compiler-checked) |
| `virtual ~Foo()` | virtual destructor — required on any base class |
| `= default` | compiler generates the obvious implementation |
| `public:` / `private:` | everything below is callable from outside / only from inside |
| `Foo::bar` | `bar` belongs to `Foo` (a class or a namespace) |
| `std::` | from the C++ standard library |
| `const Foo& x` | read-only borrow — an **input** parameter |
| `Foo& x` | writable borrow — an **output** parameter |
| `Foo x` | a copy |
| `method() const` | this method does not modify its object |
| `obj.method()` | call through a value or reference |
| `ptr->method()` | call through a (smart) pointer |
| `std::shared_ptr<T>` | auto-freeing shared-ownership pointer |
| `std::make_shared<T>()` | create an object and a `shared_ptr` to it |
| `auto` | infer the type; `const auto&` = borrow, don't copy |
| `Foo<Bar>` | template — a recipe with the blank filled by `Bar` |
| `size_t` | unsigned integer for sizes and indices |
| `auto [a, b] = f()` | structured binding — unpack a pair/tuple |
| `struct` | a class whose members default to public; used for plain data |
| `enum class E { A, B }` | named set of options, used as `E::A` |
| `};` after a class | the semicolon is required |

---

# Appendix C — Where to learn more

Two references worth knowing the difference between:

- **[cppreference.com](https://en.cppreference.com/)** — the precise, authoritative spec, written for people who already know the concept and need the exact rules. Terse. Use it to answer "what exactly does this do?"
- **[learncpp.com](https://www.learncpp.com/)** — a free, well-sequenced tutorial written for people meeting the concept for the first time. Verbose, with examples. Use it to answer "why does this exist?"

Start at learncpp for a concept you've never met, then cppreference once you need the details.

## By concept, in the order this primer introduces them

| Concept | cppreference | learncpp chapter |
|---------|-------------|------------------|
| Preprocessor, `#include`, `#define` | [Preprocessor](https://en.cppreference.com/w/cpp/preprocessor) | "Introduction to the preprocessor" |
| Header files, include guards | [Modules and headers](https://en.cppreference.com/w/cpp/header) | "Header files", "Header guards" |
| Classes, members, access specifiers | [Classes](https://en.cppreference.com/w/cpp/language/classes) | "Introduction to classes" / "Public and private members" |
| Constructors | [Constructors](https://en.cppreference.com/w/cpp/language/constructor) | "Constructors" |
| **Destructors, `~`** | [Destructors](https://en.cppreference.com/w/cpp/language/destructor) | "Destructors" |
| **Virtual destructors** | [`virtual` specifier](https://en.cppreference.com/w/cpp/language/virtual) | "Virtual destructors, virtual assignment, and overriding virtualization" |
| RAII / Rule of Zero | [RAII](https://en.cppreference.com/w/cpp/language/raii) · [Rule of three/five/zero](https://en.cppreference.com/w/cpp/language/rule_of_three) | "Destructors" (RAII discussion) |
| Inheritance, base/derived classes | [Derived classes](https://en.cppreference.com/w/cpp/language/derived_class) | "Introduction to inheritance" |
| `virtual`, `override`, polymorphism | [`virtual`](https://en.cppreference.com/w/cpp/language/virtual) · [`override`](https://en.cppreference.com/w/cpp/language/override) | "Virtual functions" |
| Abstract classes, pure virtual `= 0` | [Abstract class](https://en.cppreference.com/w/cpp/language/abstract_class) | "Pure virtual functions, abstract base classes, and interface classes" |
| References (`&`) | [Reference declaration](https://en.cppreference.com/w/cpp/language/reference) | "Lvalue references" / "Pass by reference" |
| Pointers (`*`) | [Pointer declaration](https://en.cppreference.com/w/cpp/language/pointer) | "Introduction to pointers" |
| `const` correctness | [const](https://en.cppreference.com/w/cpp/language/cv) | "Const class objects and const member functions" |
| `std::shared_ptr` / `unique_ptr` | [`shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr) · [`unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr) | "std::unique_ptr" / "std::shared_ptr" |
| `std::vector`, `std::array`, `std::map` | [Containers](https://en.cppreference.com/w/cpp/container) | "Introduction to std::vector" |
| Templates | [Templates](https://en.cppreference.com/w/cpp/language/templates) | "Introduction to function templates" |
| `auto`, structured bindings | [`auto`](https://en.cppreference.com/w/cpp/language/auto) · [Structured binding](https://en.cppreference.com/w/cpp/language/structured_binding) | "Type deduction for objects" |
| Compilation and linking | — | "Introduction to the compiler, linker, and libraries" |

## Books, if you want one

- **Bjarne Stroustrup, *A Tour of C++* (3rd ed.)** — ~250 pages by the language's designer, aimed at programmers who already know how to program. The fastest route to reading C++ fluently, and the best match for what you're doing here.
- **Scott Meyers, *Effective Modern C++*** — 42 short items on `auto`, smart pointers, move semantics, `override`. Read *after* you're comfortable reading the code; it's about writing it well.

## Quick answers, in-repo

Most of what you'll want while reading is already answered here:

- **Appendix B** — the decoder table: any symbol → one-line meaning.
- **Appendix A** — why `.h`/`.cpp`, what the linker does, which files are actually built.
- **Part 1** — every construct in `lj_potential.h`, by line number.

## A note on C++ versions

This project is built with **C++17** (`set(CMAKE_CXX_STANDARD 17)` in `CMakeLists.txt`). Both sites label which standard a feature arrived in — structured bindings (`auto [a, b] = ...`) are C++17, for instance. Anything marked C++20 or C++23 won't compile here without changing that line.

---

## Where to go next

[01_architecture.md](01_architecture.md) for the layer map, then [02_lj_implementation.md](02_lj_implementation.md) with `src/engine/potentials/lj_potential.cpp` open beside it — you have now read the header for that exact file, so the rest of it should follow.
