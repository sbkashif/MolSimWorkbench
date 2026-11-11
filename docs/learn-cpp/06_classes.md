# Chapter 6 — Classes: data and the functions that use it

Plain version: [06p_classes_plain.md](06p_classes_plain.md).
Code: [`examples/06/`](examples/06/) (mini_lj step 6, plus small toy programs). Every output below was
produced by compiling and running them with Apple clang 17 on an M1 Mac.

**After this chapter you can say:**

- what a C++ `class` is made of: data members, methods, a constructor, and the hidden `this` pointer;
- what `public` and `private` do, and why the compiler, not a naming convention, enforces them;
- what a `const` method is, and why `Geometry`'s getters must be `const` for
  `calculateEnergy(const Geometry&)` to compile;
- why getters return `const T&`, and what `this->cutoff = cutoff` is protecting against;
- where a method's body may live: inside the class in the header, or as `Class::method` in the `.cpp`;
- how to read `src/builder/geometry.h` and `src/engine/potentials/lj_potential.h` top to bottom, apart
  from the inheritance parts that are chapter 7.

Time: about 30 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/06
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## Where we are

At the end of chapter 5, mini_lj's maths lived in named functions in their own files. But `main` still
juggled five loose variables — `positions`, `types`, `box`, `params` and `cutoff` — and passed them
separately into every call. The box even came back through an out-parameter, `double& box`, because
`makeCubicLattice` had already used its return value for the positions. Nothing tied the box to the
atoms it belongs to, and nothing stopped a caller from passing the wrong map.

This chapter bundles each group of data together with the functions that use it. The atoms and the box
become one `Geometry` object. The parameters, the cutoff and the three LJ functions become one
`LJPotential` object. Both have the same public shape as the real classes in the repo.

---

## 6.0 Anatomy of a C++ class

Here is `Geometry`, the bundle of atoms and box, from `examples/06/geometry.h:11-26`:

```cpp
11  class Geometry {
12  public:
13      void addAtom(const std::string& atomType, const Vec3& coordinates);
14      std::size_t getNumberOfAtoms() const { return atomTypes.size(); }
15
16      const std::vector<std::string>& getAtoms() const { return atomTypes; }
17      const std::vector<Vec3>& getPositions() const { return atomCoordinates; }
18
19      void setBoxDimensions(const Vec3& dimensions) { boxDimensions = dimensions; }
20      const Vec3& getBoxDimensions() const { return boxDimensions; }
21
22  private:
23      std::vector<std::string> atomTypes;
24      std::vector<Vec3> atomCoordinates;
25      Vec3 boxDimensions = {0.0, 0.0, 0.0};
26  };
```

A **class** is a type you define, which bundles data with the functions that work on it. A value of that
type is an **object** (or **instance**). The pieces have names:

| Term | Meaning | In `Geometry` |
|---|---|---|
| **data member** (member variable) | a variable that every object carries its own copy of | `atomTypes`, `atomCoordinates`, `boxDimensions` |
| **method** (member function) | a function that belongs to the class and runs on one object | `addAtom`, `getNumberOfAtoms`, … |
| **access specifier** | `public:` / `private:` — who may use what follows | lines 12 and 22 |
| the closing `};` | a class definition ends with a semicolon | line 26 |

**Every data member is declared in the class body, with its type.** A C++ object has exactly the members
its class lists — the compiler knows each object's size and layout before the program runs (chapter 2's
"a variable is its storage", applied to a whole object).

> **Unlike Python:** there, attributes are created by assigning to `self.x` anywhere, and new ones can be
> added to an object later. A C++ object can never grow a member its class did not declare.

Forget the `;` after `}` and the compiler reports an error on the *next* line of code, which is
confusing the first time you see it. Functions do not take that semicolon; classes and structs do.

---

## 6.1 Every object has its own members

```cpp
Geometry a;
Geometry b;
a.addAtom("LJ", {0.0, 0.0, 0.0});
// a.getNumberOfAtoms() is 1, b.getNumberOfAtoms() is 0
```

`Geometry a;` creates an object. There is no `Geometry()` call with parentheses and no `new` — this is
the same as `double x;` creating a `double` (chapter 2). The object lives in the variable `a` itself.
`a.addAtom(…)` calls a method on `a`, with `.`.

### Default member initializers

Line 25 gives `boxDimensions` a starting value right where it is declared:

```cpp
25      Vec3 boxDimensions = {0.0, 0.0, 0.0};
```

This is a **default member initializer**. Every new `Geometry` object starts with its *own*
`boxDimensions` set to zero. There is **one copy per object**: changing `a`'s `boxDimensions` never
touches `b`'s, and a vector member given a starting value is a separate vector in every object.

> **Unlike Python:** `x = 0.0` in a Python class body is a *class* attribute — one value, shared by all
> instances — which is where the mutable-default bug (`x = []` shared by every instance) comes from. The
> same-looking line in a C++ class body is per-object, like an attribute set in `__init__`.

C++ does have members shared by all objects of a class — they are marked `static` — but this repo's
classes do not use them, so this course does not need them.

Members without an initializer, like `atomTypes` on line 23, start in whatever state their type gives
them. A `std::vector` starts empty. A plain `double` with no initializer starts as **garbage** —
whatever bits were in that memory. That is why the repo writes `double cutoff = 10.0;` and not
`double cutoff;`.

---

## 6.2 `public` and `private`

Everything after `public:` can be used by any code. Everything after `private:` can be used only by the
class's own methods. `examples/06/private_access.cpp`:

```cpp
class Cutoff {
public:
    void set(double value) { cutoff = value; }   // inside the class: allowed
    double get() const { return cutoff; }
private:
    double cutoff = 10.0;
};

int main() {
    Cutoff c;
    c.set(2.5);
    std::cout << c.get() << std::endl;
    c.cutoff = -1.0;                             // outside the class: refused
}
```
```
private_access.cpp:17:7: error: 'cutoff' is a private member of 'Cutoff'
   17 |     c.cutoff = -1.0;                             // outside the class: refused
      |       ^
private_access.cpp:10:12: note: declared private here
   10 |     double cutoff = 10.0;
      |            ^
1 error generated.
```

The rule is part of the type: code that breaks it does not compile.

> **Unlike Python:** `_cutoff` is only a convention there ("please don't touch"); `obj._cutoff = -1.0`
> still runs. C++ `private` is checked by the compiler, so no outside code can bypass it.

**What it buys:** the class decides how its data may change. `Geometry` has no method that adds a
position without a type, so `atomTypes` and `atomCoordinates` always have the same length. That kind of
always-true rule is called an **invariant**, and `private` is how a class keeps one. §6.10 shows the
repo using this to keep a cache correct.

**What it costs:** you write small `get…`/`set…` methods instead of reaching into the data.

### `struct` versus `class`

Chapters 4 and 5 used `struct AtomParameters { double epsilon; double sigma; };` and reached in with
`p.epsilon`. A `struct` is a class whose members are `public` by default. A `class` starts `private` by
default. That is the only difference in the language. By convention, `struct` is used for a plain
bundle of data with no rules to protect, and `class` for something with methods and invariants.

---

## 6.3 Methods, and where `self` went

A method call `geometry.addAtom("LJ", p)` runs `addAtom` *on* `geometry`. The method is declared
without any parameter for the object:

| Step | Code |
|---|---|
| define | `void addAtom(const std::string& atomType, const Vec3& coordinates);` |
| use a member inside | `atomTypes.push_back(atomType);` |
| call | `geometry.addAtom("LJ", p);` |
| the object itself, inside | `this` (a hidden pointer the compiler provides) |

Inside a method, members are reachable by their bare name: `atomTypes` means "this object's
`atomTypes`". This is the most common "where did this variable come from?" moment when reading C++. If
a name in a method is not a local or a parameter, look in the class's member list.

The object is still there, as a hidden parameter called **`this`**. It is a **pointer** (chapter 3) to
the object the method was called on. So `atomTypes` inside a method is short for `this->atomTypes`.
The arrow `->` reaches a member through a pointer: `p->x` means `(*p).x`.

> **Unlike Python:** there, the object is an explicit first parameter you name (`self`), and every
> attribute access is spelled `self.x`. In C++ the object parameter is hidden, and a bare member name
> inside a method silently means `this->name`.

### When you must write `this->`

You need `this->` when a parameter has the same name as a member. `LJPotential::setCutoff` does exactly
that (`examples/06/lj_potential.h:20`):

```cpp
20      void setCutoff(double cutoff) { this->cutoff = cutoff; }
```

Inside the body, the bare name `cutoff` means the **parameter**, because the innermost name wins. (This
is **shadowing**.) `this->cutoff` names the member.
`examples/06/self_assign.cpp` shows what happens without it:

```cpp
void setCutoffWrong(double cutoff) { cutoff = cutoff; }        // parameter = parameter
void setCutoffRight(double cutoff) { this->cutoff = cutoff; }  // member = parameter
```
```
self_assign.cpp:6:49: warning: explicitly assigning value of variable of type 'double' to itself; did you mean to assign to member 'cutoff'? [-Wself-assign]
    6 |     void setCutoffWrong(double cutoff) { cutoff = cutoff; }        // parameter = parameter
      |                                          ~~~~~~ ^ ~~~~~~
      |                                          this->
1 warning generated.
a: 10  b: 2.5
```

The wrong version compiles (with a warning, because of `-Wall`) and does nothing: `a` keeps its default
cutoff of 10.

---

## 6.4 `const` methods

`getNumberOfAtoms` has a `const` after its parameter list (`geometry.h:14`):

```cpp
14      std::size_t getNumberOfAtoms() const { return atomTypes.size(); }
```

That `const` is a promise: **this method does not change the object it is called on.** Inside it, every
member is read-only, and the compiler refuses any assignment to one.

Why the promise matters: chapter 5 taught you to pass inputs as `const T&`. `calculateEnergy` takes
`const Geometry& geometry`. Through that reference, only `const` methods may be called — the compiler
cannot check the body of a method from the call site, so it relies on the promise.
`examples/06/const_method.cpp` leaves the promise out:

```cpp
class Atoms {
public:
    void add(double x) { xs.push_back(x); }
    std::size_t count() { return xs.size(); }    // forgot `const`
private:
    std::vector<double> xs;
};

std::size_t countAtoms(const Atoms& atoms) {
    return atoms.count();
}
```
```
const_method.cpp:15:12: error: 'this' argument to member function 'count' has type 'const Atoms', but function is not marked const
   15 |     return atoms.count();
      |            ^~~~~
const_method.cpp:8:17: note: 'count' declared here
    8 |     std::size_t count() { return xs.size(); }    // forgot `const`
      |                 ^
1 error generated.
```

`count` only reads, but it never *said* so, and the compiler will not guess.

Now go back to the error from chapter 5 §5.7, for `epsilons["LJ"]` on a `const` map:
*"'this' argument has type 'const std::map…', but method is not marked const"*. It is the same error.
`operator[]` is a method of `std::map` that may insert, so it is not `const`, so it cannot be called on
a `const` map. `.at()` and `.find()` have `const` versions, so they can.

| Where `const` is written | What it promises | Example |
|---|---|---|
| on a local | this variable never changes | `const double sr6 = …;` |
| on a reference parameter | the function will not change the caller's object | `const Geometry& geometry` |
| **after a method's `()`** | this method will not change `*this` | `getNumberOfAtoms() const` |
| on a return type | the caller cannot change what it was handed | `const std::vector<Vec3>& getPositions() const` |

---

## 6.5 Getters that return `const T&`

`getPositions` returns `const std::vector<Vec3>&` (`geometry.h:17`), a **reference** to the member, not
a copy:

```cpp
17      const std::vector<Vec3>& getPositions() const { return atomCoordinates; }
```

The `&` means no copy is made: the caller gets a second name for the object's own vector. The `const`
means the caller can read it but not change it. So `geometry.getPositions().push_back(…)` does not
compile, and the invariant from §6.2 is safe.

At the call site, how you *receive* it matters. `examples/06/lj_potential.cpp:15-17`:

```cpp
15      const auto& atoms = geometry.getAtoms();
16      const auto& positions = geometry.getPositions();
17      const auto& box = geometry.getBoxDimensions();
```

| Written as | What you get |
|---|---|
| `const auto& positions = geometry.getPositions();` | a read-only second name for the vector inside `geometry`. No copy |
| `auto positions = geometry.getPositions();` | `auto` drops the `&`: a **full copy** of every position |

With 4096 atoms, the second form copies 98 KB on every energy call. `const auto&` is the idiom for
"borrow, don't copy", and you will see it at the top of almost every method in the repo.

What it costs: the reference is only valid while the `Geometry` it points into is alive. Here
`geometry` outlives the method call, so this is safe. Chapter 8 is about the cases where it is not.

---

## 6.6 Constructors

A **constructor** is a method that runs automatically when an object is created. Its job is to put the
object into a valid starting state. It has the class's name and **no return type**, not even `void`.
`examples/06/lj_potential.h:13` declares one, and `lj_potential.cpp:6-8` defines it:

```cpp
LJPotential::LJPotential() {
    std::cout << "[LJPotential constructed, cutoff = " << cutoff << "]" << std::endl;
}
```

| | Constructor |
|---|---|
| name | the class name: `LJPotential()` |
| when it runs | at `LJPotential lj;` |
| return value | no return type at all |
| members set before it runs | every member already exists; default member initializers have already been applied |

The last row is visible in mini_lj's output. The constructor prints `cutoff = 10`: the default member
initializer `double cutoff = 10.0;` ran *before* the constructor body. `main` calls `setCutoff(2.5)`
only afterwards.

> **Unlike Python:** `__init__` *creates* the attributes by assigning them. A C++ constructor body runs
> on an object whose members all exist already, so it can only change them.

`Geometry` has no constructor written at all. When a class declares none, the compiler writes an empty
one for you, which applies the default member initializers and nothing else. The repo's `Geometry`
declares `Geometry();` and defines it as `Geometry::Geometry() {}` (`src/builder/geometry.cpp:7`),
which does the same thing.

A constructor can also take parameters. Chapter 7's `HarmonicTether(double k)` is
the first one that does, and it brings a new piece of syntax with it.

---

## 6.7 Where a method's body lives

`Geometry` shows both places a method body can go.

**Inside the class, in the header.** `geometry.h:14-20` — the getters and `setBoxDimensions` — have
their bodies right in the class. A method defined inside the class body is automatically `inline`, so
it can sit in a header included by many `.cpp` files without the "duplicate symbol" error from chapter 5
§5.3. The repo uses this for one-line methods.

**In the `.cpp`, with the class name in front.** `addAtom` is only declared in the class
(`geometry.h:13`) and defined in `examples/06/geometry.cpp:3-6`:

```cpp
3  void Geometry::addAtom(const std::string& atomType, const Vec3& coordinates) {
4      atomTypes.push_back(atomType);
5      atomCoordinates.push_back(coordinates);
6  }
```

`Geometry::addAtom` reads "the `addAtom` that belongs to `Geometry`". The `::` is the **scope
resolution operator**, the same `::` as in `std::cout` (chapter 1: "the `cout` from `std`").
Without the `Geometry::` prefix, this would define an unrelated free function named `addAtom`, and its
body could not see `atomTypes`.

This is chapter 5's declaration/definition split, applied to methods. The class body in the header is
the menu; the `.cpp` holds the longer recipes. The same rule about default arguments applies: they are
written once, in the class body, not again on `Geometry::…`.

---

## 6.8 A private nested struct

`LJPotential` needs per-type ε and σ. Chapter 5 kept `struct AtomParameters` in `lj_math.h`, where
any file could use it. Now it moves **inside** the class, under `private:`
(`examples/06/lj_potential.h:22-29`):

```cpp
22  private:
23      struct AtomParameters {
24          double epsilon;
25          double sigma;
26      };
27
28      std::map<std::string, AtomParameters> atomParameters;
29      double cutoff = 10.0;
```

Its full name is now `LJPotential::AtomParameters`, and only `LJPotential`'s own methods can use it.
Code outside the class adds parameters through the public `addAtomParameters("LJ", 1.0, 1.0)` and never
sees how they are stored. If the class later stores them differently — the repo does, in flat tables
for the parallel kernels — no caller has to change.

The three helper functions from chapter 5 move into the private section too, lines 31-34. They are
internal machinery: `main` has no reason to call `calculatePairForce` directly any more.

---

## 6.9 The mini_lj step

Two classes replace the loose variables. The lattice now returns a `Geometry` — box included — so the
`double& box` out-parameter from chapter 5 is gone (`examples/06/lattice.cpp:13-23`, shortened):

```cpp
Geometry geometry;
geometry.setBoxDimensions({box, box, box});
for (...)
    geometry.addAtom("LJ", {x, y, z});
return geometry;
```

The pair loop moved into `LJPotential::calculateEnergy` and `calculateForces`. Inside those methods,
`cutoff` and `atomParameters` are members, so `getMixedParameters` no longer needs the map passed in
(`lj_potential.cpp:30` and `:60-64`):

```cpp
30              auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
...
60  std::pair<double, double> LJPotential::getMixedParameters(const std::string& type1, const std::string& type2) {
61      const AtomParameters& p1 = atomParameters[type1];
62      const AtomParameters& p2 = atomParameters[type2];
63      return {std::sqrt(p1.epsilon * p2.epsilon), std::sqrt(p1.sigma * p2.sigma)};
64  }
```

Lines 61-62 use `[]` again. That is allowed here, because `getMixedParameters` is not a `const` method,
so `atomParameters` is not read-only inside it. It also means a missing type would silently insert a
zero entry, as in chapter 5 §5.7. The repo guards against that with `.find()` first.

`main.cpp` shrank to this (lines 12-23):

```cpp
12      const Geometry geometry = makeCubicLattice(numAtoms, 0.8, 42);
13
14      LJPotential lj;
15      lj.addAtomParameters("LJ", 1.0, 1.0);
16      lj.setCutoff(2.5);
17
18      std::vector<Vec3> forces;
19      lj.calculateForces(geometry, forces);
20
21      std::cout.precision(12);
22      std::cout << lj.getName() << ", N = " << geometry.getNumberOfAtoms() << std::endl;
23      std::cout << "E = " << lj.calculateEnergy(geometry) << std::endl;
```

`geometry` is `const`. Every method `main` calls on it — `getNumberOfAtoms`, and inside `LJPotential`,
`getAtoms`, `getPositions`, `getBoxDimensions` — is a `const` method, so this compiles. `lj` is not
`const`, because `addAtomParameters` and `setCutoff` change it.

```bash
c++ -std=c++17 -Wall -O2 main.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/mini_lj
./out/mini_lj
```
```
[LJPotential constructed, cutoff = 10]
Lennard-Jones, N = 125
E = -611.491665613
force on atom 0 = (-4.10635870563, 9.86021608902, -13.8938854543)
```

Same energy, same force on atom 0 as chapter 5. The first line is the constructor running at
`LJPotential lj;`, before `setCutoff(2.5)`.

---

## 6.10 In the real code

### `src/builder/geometry.h`

Open it. Lines 12-33 are the class, and every line is now readable:

```cpp
12  class Geometry {
13  public:
14      Geometry();
15
16      // Atom methods
17      void addAtom(const std::string& atomType, const std::array<double, 3>& coordinates);
18      size_t getNumberOfAtoms() const;
19      const std::vector<std::string>& getAtoms() const { return atomTypes; }
20      const std::vector<std::array<double, 3>>& getPositions() const { return atomCoordinates; }
21      const std::string& getAtomType(size_t index) const;
22      const std::array<double, 3>& getCoordinates(size_t index) const;
23      void setCoordinates(size_t index, const std::array<double, 3>& newCoordinates);
24
25      // Box methods
26      void setBoxDimensions(const std::array<double, 3>& dimensions);
27      const std::array<double, 3>& getBoxDimensions() const { return boxDimensions; }
28
29  private:
30      std::vector<std::string> atomTypes;
31      std::vector<std::array<double, 3>> atomCoordinates;
32      std::array<double, 3> boxDimensions = {0.0, 0.0, 0.0};
33  };
```

| Line | What you now recognise |
|---|---|
| 14 | a constructor, declared; defined empty at `geometry.cpp:7` |
| 17 | declared here, defined as `Geometry::addAtom` in `geometry.cpp:9-12` — the same two `push_back`s as mini_lj |
| 18 | a `const` method, but this time the body is in the `.cpp` (`geometry.cpp:14-16`) |
| 19-20, 27 | `const` getters returning `const &`, bodies inline |
| 21-22 | getters for one atom; their bodies use `.at(index)` (`geometry.cpp:18-24`), which throws if the index is out of range |
| 23 | `setCoordinates` — the one way to move an atom. Not `const`, because it changes the object |
| 29-32 | the private data. Same three members as mini_lj; `Vec3` is spelled out as `std::array<double, 3>` |

`size_t` is `std::size_t` without the `std::`; both spellings work.

### `src/engine/potentials/lj_potential.h`

This header is 105 lines now, because the parallel work (`docs/learn-parallel/`) added backends and
scratch buffers. The parts that are this chapter's — skip the `: public Potential`, `override` and
`~LJPotential` for now; they are chapter 7 and 8:

```cpp
16      LJPotential();
...
26      void addAtomParameters(const std::string& atomType, double epsilon, double sigma);
27      void setCutoff(double cutoff) { this->cutoff = cutoff; tablesDirty = true; }
28      void setMixingRule(const std::string& rule) { this->mixingRule = rule; tablesDirty = true; }
...
63  private:
64      // Atom parameters
65      struct AtomParameters {
66          double epsilon;
67          double sigma;
68      };
69
70      std::map<std::string, AtomParameters> atomParameters;
71      double cutoff = 10.0;  // Default cutoff
72      std::string mixingRule = "geometric";  // Default mixing rule
...
80      bool tablesDirty = true;
...
97      std::pair<double, double> getMixedParameters(const std::string& type1, const std::string& type2);
98      double calculatePairEnergy(double epsilon, double sigma, double distance);
99      void calculatePairForce(double epsilon, double sigma, double distance,
```

Lines 63-72 and 97-99 are mini_lj's private section, member for member. Line 72 adds a second
default member initializer, `mixingRule`, which the repo's `getMixedParameters` reads to choose
geometric or arithmetic mixing (`lj_potential.cpp:346-351`).

Lines 27-28 show **what `private` buys**. `setCutoff` does not only store the cutoff. It also sets
`tablesDirty = true`. The fast kernels precompute tables from the parameters and the cutoff, and
`tablesDirty` (line 80) records that those tables are out of date. `addAtomParameters` does the same
(`lj_potential.cpp:42-45`), and `buildView` rebuilds the tables when the flag is set
(`lj_potential.cpp:92`: `if (tablesDirty) rebuildTables();`). Because `cutoff` is private, *every*
change goes through a setter, so the tables can never silently go stale. If `cutoff` were public, one
`lj.cutoff = 3.0` anywhere would break that rule with no error.

### Why `calculateEnergy` is not `const` (deep dive later)

You might expect `calculateEnergy` to be a `const` method: computing an energy should not change the
potential. In the repo it is not, and that is honest. `LJPotential::calculateEnergy`
(`lj_potential.cpp:147-157`) calls `buildView`, which may rebuild the tables and **fills member
vectors** `sx`, `sy`, `sz`, … with the positions on every call (`lj_potential.cpp:99-106`). Those are
scratch buffers kept as members so a long MD run allocates them once (the comment at
`lj_potential.h:86-89` says so). Writing to members means the method is not `const`. The cost: you
cannot call `calculateEnergy` on a `const LJPotential`. That is also why the base class `Potential`,
which you will meet in chapter 7, does not declare it `const`.

### The primer, as a review

Now read Part 1 of [`docs/explainer/00_cpp_primer.md`](../explainer/00_cpp_primer.md). It walks
`lj_potential.h` keyword by keyword. Its sections on `public:`/`private:`, the constructor, `const` in
its two positions, a body in the header, `this->`, `struct`, default member initializers and the
private helpers should all read as review. Its line numbers come from an older, 48-line version of the
header, so match them by content, not by number. The sections on inheritance, `virtual`, `override` and
the destructor are chapters 7 and 8.

---

## Next

mini_lj now has two classes, and `main` reads as setup plus one call: `lj.calculateEnergy(geometry)`.
But suppose we want a second energy term — say a spring that pulls every atom towards the centre of the
box. It would be a second class with its own `calculateEnergy`, and `main` would need a second variable
and a second call; each new kind of potential means editing `main` again. Chapter 7 lets one loop handle
**any** potential, without knowing which kind it is holding.
