# Chapter 7 — Inheritance and virtual functions

Plain version: [07p_inheritance_and_virtual_plain.md](07p_inheritance_and_virtual_plain.md).
Code: [`examples/07/`](examples/07/) (mini_lj step 7, plus small toy programs). Every output below was
produced by compiling and running them with Apple clang 17 on an M1 Mac.

**After this chapter you can say:**

- why C++ needs a declared common base class before one loop can handle several kinds of potential;
- what `virtual`, `= 0`, "pure virtual" and "abstract class" mean, and what the compiler does when you
  try to create an abstract object;
- how a call through a `Potential*` ends up running `LJPotential`'s code, and why it would not without
  `virtual`;
- what `override` catches, and why storing objects *by value* in a base-class container slices them;
- what a constructor initializer list `: k(k)` is, and what `explicit` prevents;
- how `ForceCalculator` sums any number of potentials in one loop — the repo's seam for new potentials.

Time: about 35 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/07
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## Where we are

At the end of chapter 6, `main` built a `Geometry` and an `LJPotential` and asked for
`lj.calculateEnergy(geometry)`. Now we want a second energy term: a spring that pulls every atom towards
the centre of the box. With only what chapter 6 taught, that is a second class with its own
`calculateEnergy`, plus a second variable and a second call in `main`. Every new kind of potential
would mean editing `main` again, and later the repo's `ForceCalculator` too.

What we want is one loop over "any potential". This chapter builds it.

---

## 7.0 The goal: one loop over any potential

We want to write `total += p.calculateEnergy(geometry)` once, in a loop where `p` is sometimes an
`LJPotential` and sometimes a `HarmonicTether`. Two things stand in the way. First, chapter 4's rule: a
`std::vector` holds elements of **one** type, so a vector of `LJPotential` cannot hold a
`HarmonicTether`. Second, the compiler checks every call before the
program runs, so it must know, from the *type* of `p`, that `p` has a `calculateEnergy` method.

So C++ needs a **common type** that both classes can be treated as, and a way for a call on that common
type to run each class's own code. Those are inheritance and `virtual`.

> **Unlike Python:** `for p in [lj, tether]: p.calculate_energy(geometry)` just works there, because a
> list holds any objects and each call looks the method up on the actual object while running (**duck
> typing**: "if it has the method, it will do"). C++ has to know, before the program runs, that every
> element has that method, so the shared type must be declared.

---

## 7.1 The job description: an abstract base class

Here is the common type. `examples/07/potential.h:9-16`:

```cpp
 9  class Potential {
10  public:
11      virtual ~Potential() = default;
12
13      virtual double calculateEnergy(const Geometry& geometry) = 0;
14      virtual void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) = 0;
15      virtual std::string getName() const = 0;
16  };
```

It is word for word the `Potential` class in the repo, `src/engine/force_calculator.h:35-48`, with
`Vec3` for `std::array<double, 3>`.

| Piece | Meaning |
|---|---|
| `virtual` | "a derived class may supply its own version of this method, and a call through a `Potential` pointer or reference will run that version" (§7.3) |
| `= 0` | "**no body here**. Every class that derives from me must supply one." A method with `= 0` is a **pure virtual** function |
| a class with at least one pure virtual | an **abstract class**: you cannot create an object of it. It only describes what its derived classes must be able to do |
| `virtual ~Potential() = default;` | a **virtual destructor**. §7.9 says why; `= default` asks the compiler to write the obvious body |

`Potential` holds no data and does no work. It is a contract: "anything that is a `Potential` can
compute an energy, compute forces, and tell you its name."

Trying to create an abstract object is a compile error. `examples/07/abstract.cpp` tries to create a `Potential`,
and an `Unfinished` class that implements only two of the three methods:

```cpp
class Unfinished : public Potential {
public:
    double calculateEnergy(const Geometry&) override { return 0.0; }
    std::string getName() const override { return "unfinished"; }
    // calculateForces is missing
};

int main() {
    Potential p;      // error: Potential is abstract
    Unfinished u;     // error: Unfinished is still abstract
}
```
```
abstract.cpp:12:15: error: variable type 'Potential' is an abstract class
   12 |     Potential p;      // error: Potential is abstract
      |               ^
./potential.h:13:20: note: unimplemented pure virtual method 'calculateEnergy' in 'Potential'
   13 |     virtual double calculateEnergy(const Geometry& geometry) = 0;
      |                    ^
./potential.h:14:18: note: unimplemented pure virtual method 'calculateForces' in 'Potential'
   14 |     virtual void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) = 0;
      |                  ^
./potential.h:15:25: note: unimplemented pure virtual method 'getName' in 'Potential'
   15 |     virtual std::string getName() const = 0;
      |                         ^
abstract.cpp:13:16: error: variable type 'Unfinished' is an abstract class
   13 |     Unfinished u;     // error: Unfinished is still abstract
      |                ^
./potential.h:14:18: note: unimplemented pure virtual method 'calculateForces' in 'Unfinished'
   14 |     virtual void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) = 0;
      |                  ^
2 errors generated.
```

The notes list exactly which promises are still unkept. A class becomes usable — **concrete** — only
when every pure virtual has a body.

> **Unlike Python:** `abc.ABC` with `@abstractmethod` expresses the same contract, but `Potential()` only
> raises `TypeError` when that line runs. In C++ the program never gets built.

---

## 7.2 Deriving: `class LJPotential : public Potential`

The chapter 6 `LJPotential` signs up to the contract with a few changes to its header
(`examples/07/lj_potential.h:8-17`):

```cpp
 8  #include "potential.h"                                          // was: #include "geometry.h"
    ...
11  class LJPotential : public Potential {                          // was: class LJPotential {
12  public:
13      LJPotential();
14
15      double calculateEnergy(const Geometry& geometry) override;    // + override
16      void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) override;
17      std::string getName() const override { return "Lennard-Jones"; }
```

The `.cpp` file did not change at all: `diff examples/06/lj_potential.cpp examples/07/lj_potential.cpp`
prints nothing.

| Piece | Meaning |
|---|---|
| `class LJPotential : public Potential` | `LJPotential` **derives from** (inherits from) `Potential`. `Potential` is the **base class**, `LJPotential` the **derived class** |
| `public` (in `: public Potential`) | everyone may treat an `LJPotential` as a `Potential`. The repo only uses `public` inheritance |
| `override` | "this method replaces a virtual method of the base". Optional, but checked (§7.6) |
| `addAtomParameters`, `setCutoff` | still here, with no `override`: a derived class may add methods the base never promised |

Read `:` in a class header as "**is a**": an `LJPotential` *is a* `Potential`. Everything `Potential`
promised, `LJPotential` provides, so any code written for a `Potential` can be handed an `LJPotential`.

### Single inheritance only

C++ allows several base classes, but every class in this repo has **at most one**. So the lookup
rule you need is simple: look in the derived class, and if the name is not there, look in its one base.

---

## 7.3 Calling through a base pointer

Now the loop. `examples/07/main.cpp:14-29`:

```cpp
14      LJPotential lj;
15      lj.addAtomParameters("LJ", 1.0, 1.0);
16      lj.setCutoff(2.5);
17      HarmonicTether tether(0.1);
18
19      // One list, two different classes. The loop below only knows "Potential".
20      std::vector<Potential*> potentials = {&lj, &tether};
21
22      std::cout.precision(12);
23      double total = 0.0;
24      for (Potential* p : potentials) {
25          const double e = p->calculateEnergy(geometry);   // which function runs is decided at run time
26          std::cout << p->getName() << ": " << e << std::endl;
27          total += e;
28      }
```

| Line | What happens |
|---|---|
| 20 | `std::vector<Potential*>`: a vector of **pointers** to `Potential` (chapter 3). `&lj` is the address of `lj`, an `LJPotential*`. Because an `LJPotential` *is a* `Potential`, the compiler converts it to a `Potential*` automatically. Same for `&tether` |
| 24 | a range-for loop (chapter 4); each `p` is one of the two pointers |
| 25 | `p->calculateEnergy(geometry)`: call the method on the object `p` points at. `p->f()` means `(*p).f()` |
| 25 | `p`'s type is `Potential*`, but the first time round it runs **`LJPotential::calculateEnergy`**, and the second time **`HarmonicTether::calculateEnergy`** |

That last row is the whole point. The type of `p` says only "some `Potential`". Because
`calculateEnergy` is `virtual`, the program looks, **while running**, at what kind of object `p`
actually points at, and calls that class's version. This is **dynamic dispatch** (also called
**polymorphism**: one call, many forms).

### Without `virtual`

Only `virtual` methods behave this way. A non-virtual method is
chosen by the compiler from the **pointer's type**, before the program runs. `examples/07/no_virtual.cpp`
has two pairs of classes, identical except for `virtual`:

```cpp
struct PlainBase    { std::string name() const { return "PlainBase"; } };
struct PlainDerived : PlainBase { std::string name() const { return "PlainDerived"; } };

struct VirtualBase  { virtual ~VirtualBase() = default;
                      virtual std::string name() const { return "VirtualBase"; } };
struct VirtualDerived : VirtualBase { std::string name() const override { return "VirtualDerived"; } };

PlainBase* p = &pd;      // pd is a PlainDerived
VirtualBase* v = &vd;    // vd is a VirtualDerived
```
```
plain:   pd.name() = PlainDerived,  p->name() = PlainBase
virtual: vd.name() = VirtualDerived, v->name() = VirtualDerived
```

Through the base pointer, the non-virtual call ran the **base** version, even though the object is a
`PlainDerived`. No error, no warning. That is why `Potential` marks every method `virtual`.

(These toy classes use `struct` so everything is public without writing `public:`. Inheriting from a
struct is public by default, too.)

| | `virtual` method | non-virtual method |
|---|---|---|
| which method runs for `p->f()` | the actual object's, chosen at run time | the one for the pointer's or variable's declared type, fixed at compile time |
| cost | one extra memory read per call (§7.11) | none |

> **Unlike Python:** there, every method is effectively virtual: `obj.f()` is always looked up on the
> actual object at run time (a dictionary search through the object, its class and the MRO, on every
> call). In C++ a method without `virtual` silently runs the base version through a base pointer.

---

## 7.4 Through a `Potential*`, only `Potential`'s methods

The compiler only knows that `p` points at *some* `Potential`. So only the methods `Potential` declares
can be called through it. `examples/07/base_view.cpp`:

```cpp
LJPotential lj;
lj.setCutoff(2.5);          // fine: lj is an LJPotential
Potential* p = &lj;
p->setCutoff(3.0);          // refused: Potential has no setCutoff
```
```
base_view.cpp:8:8: error: no member named 'setCutoff' in 'Potential'
    8 |     p->setCutoff(3.0);          // refused: Potential has no setCutoff
      |     ~  ^
1 error generated.
```

That is why `main` sets up `lj` through its own concrete type first (lines 14-16), and only then puts its
address in the `Potential*` list. The repo's YAML parser does the same: it creates an `LJPotential`,
calls `setCutoff` and `addAtomParameters` on it, and only then returns it as a `Potential`
(`src/utils/yaml_parser.cpp:301-321`).

---

## 7.5 C++ does not duck-type

What if a class has all three methods, with the right signatures, but does not derive from `Potential`?
`examples/07/duck.cpp`:

```cpp
class Duck {   // same three methods as Potential, but does not inherit from it
public:
    double calculateEnergy(const Geometry&) { return 0.0; }
    void calculateForces(const Geometry&, std::vector<Vec3>&) {}
    std::string getName() const { return "duck"; }
};

Duck duck;
Potential* p = &duck;
```
```
duck.cpp:14:16: error: cannot initialize a variable of type 'Potential *' with an rvalue of type 'Duck *'
   14 |     Potential* p = &duck;
      |                ^   ~~~~~
1 error generated.
```

Having the right methods is not enough. A class is a `Potential` only if it **says so**, with
`: public Potential`. (An *rvalue* is a temporary value, here the address `&duck`; the word is not
important for this error.)

The repo has a real example. `src/engine/potentials/machine_learning_potential.h:6` declares
`class MachineLearningPotential {` — with no `: public Potential`. It has a `calculateForces`, but with
different parameters, and it is not a `Potential`, so it could never be put into a `ForceCalculator`.
It is also not listed in `CMakeLists.txt` (the `EngineLib` sources at lines 82-88 are
`force_calculator.cpp`, `molecular_dynamics.cpp`, `lj_potential.cpp`, `lj_kernels.cpp` and
`dft_potential.cpp`), so it is never compiled at all. It is a placeholder.

---

## 7.6 `override` catches near misses

To replace a base method, the derived method must match it **exactly**: same name, same parameter
types, same `const`. If it differs in any way, it is a *new, unrelated* method that happens to share a
name. `examples/07/override_typo.cpp` forgets one `const`, and does not write `override`:

```cpp
struct Base {
    virtual ~Base() = default;
    virtual std::string getName() const { return "unnamed"; }
};

struct Tether : Base {
    std::string getName() { return "Tether"; }               // forgot `const`
};

Tether t;
Base* b = &t;
```
```
override_typo.cpp:11:17: warning: 'Tether::getName' hides overloaded virtual function [-Woverloaded-virtual]
   11 |     std::string getName() { return "Tether"; }               // forgot `const`
      |                 ^
override_typo.cpp:7:25: note: hidden overloaded virtual function 'Base::getName' declared here: different qualifiers ('const' vs unqualified)
    7 |     virtual std::string getName() const { return "unnamed"; }
      |                         ^
1 warning generated.
t.getName()  = Tether
b->getName() = unnamed
```

It compiled, and through the base pointer the **base** version ran. Clang's `-Wall` happened to warn
here; compiled without `-Wall`, the same program builds with no message at all and prints the same two
lines. `examples/07/override_caught.cpp` is the same mistake with `override` written:

```cpp
struct Tether : Base {
    std::string getName() override { return "Tether"; }      // forgot `const`, said `override`
};
```
```
override_caught.cpp:10:27: error: non-virtual member function marked 'override' hides virtual member function
   10 |     std::string getName() override { return "Tether"; }      // forgot `const`, said `override`
      |                           ^
override_caught.cpp:6:25: note: hidden overloaded virtual function 'Base::getName' declared here: different qualifiers ('const' vs unqualified)
    6 |     virtual std::string getName() const { return "unnamed"; }
      |                         ^
1 error generated.
```

Now it is an error, and the note names the exact difference: `'const' vs unqualified`. Every
derived-class method in the repo that implements a `Potential` method is marked `override`. When you
read one, it is a reliable sign: "look in the base class for the contract this fulfils".

---

## 7.7 Why a pointer, not a value: slicing

`std::vector<Potential*>` holds pointers. Why not objects?

For `Potential` itself, the compiler would refuse: it is abstract (§7.1), so there can be no `Potential`
values. But even with a concrete base class, storing derived objects by value goes wrong.
`examples/07/slicing.cpp`, with a base that is not abstract:

```cpp
struct Base    { virtual ~Base() = default; virtual std::string name() const { return "Base"; } };
struct Derived : Base { std::string name() const override { return "Derived"; }  double extra = 42.0; };

Derived d;
Base copy = d;            // a new Base object, copied from d's Base part
Base& ref = d;            // a second name for d itself

std::vector<Base> values = {d};      // each element is a Base: sliced
std::vector<Base*> pointers = {&d};  // each element points at the real object
```
```
copy.name() = Base
ref.name()  = Derived
values[0].name()    = Base
pointers[0]->name() = Derived
```

`Base copy = d;` creates a brand-new object of type `Base`, the size of a `Base`, and copies into it
only the part of `d` that a `Base` has room for. `extra` is cut off, and so is "being a `Derived`". This
is **slicing**. A `Base` value is always exactly a `Base`, so `copy.name()` runs `Base::name`.

A reference (`Base&`) or a pointer (`Base*`) is not a new object; it refers to `d` itself, so dynamic
dispatch finds `Derived::name`.

`std::vector<Base>` holds **copies**, each exactly a `Base`. Only `std::vector<Base*>` (or chapter 8's
smart pointers) keeps each object's real class.

Rule: **virtual dispatch only works through a pointer or a reference.**

> **Unlike Python:** a Python list always holds references, so `[lj, tether]` keeps each object's class
> and slicing cannot happen. In C++, putting a derived object into a base-typed *value* copies and cuts it.

---

## 7.8 A second potential: `HarmonicTether`

`examples/07/tether.h:8-18`:

```cpp
 8  class HarmonicTether : public Potential {
 9  public:
10      explicit HarmonicTether(double k) : k(k) {}
11
12      double calculateEnergy(const Geometry& geometry) override;
13      void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces) override;
14      std::string getName() const override { return "Harmonic tether"; }
15
16  private:
17      double k;
18  };
```

The physics is a spring on every atom towards the box centre (the origin, since the lattice is centred
on 0): energy ½k|r|², force −k r. It is not in the repo. It exists so mini_lj has two different
potentials to stack. Its `calculateEnergy` (`tether.cpp:3-8`) is a single loop over atoms.

Line 10 has two new pieces of syntax.

### The constructor initializer list: `: k(k)`

The part between `)` and `{` is the **member initializer list**. It says how to *initialize* each member,
before the constructor body runs:

```cpp
explicit HarmonicTether(double k) : k(k) {}
//                                  ^ ^
//                                  | the parameter k
//                                  the member k
```

Inside the parentheses, `k` is the parameter, as you would expect. Outside, in front of the
parentheses, only a member can stand, so the first `k` is the member. That is why `k(k)` works without
`this->`. The body `{}` is empty: there is nothing left to do.

| | Initializer list `: k(k) {}` | Assignment in the body `{ this->k = k; }` |
|---|---|---|
| when the member gets its value | as it is created | created first (a `double` with garbage), then overwritten |
| works for `const` members and reference members | yes | no — those cannot be assigned after creation |
| works for a base class | yes: `: Potential()` (§7.12 shows a real one) | no |

For a single `double` the difference is invisible, but the initializer list is the C++ habit, and the
repo's constructors use it (§7.12).

### `explicit`

A constructor that takes one argument also acts as a **conversion**: C++ will use it silently to turn a
`double` into a `HarmonicTether` when a function wants one. `explicit` switches that off.
`examples/07/explicit_ctor.cpp`:

```cpp
struct LooseTether  {          LooseTether(double k) : k(k) {}  double k; };
struct StrictTether { explicit StrictTether(double k) : k(k) {} double k; };

void report(const LooseTether& t)  { std::cout << "LooseTether with k = " << t.k << std::endl; }
void report(const StrictTether& t) { std::cout << "StrictTether with k = " << t.k << std::endl; }

report(0.1);                  // compiles: 0.1 silently becomes a LooseTether
report(StrictTether(0.1));    // fine: we said which type we meant
```
```
LooseTether with k = 0.1
StrictTether with k = 0.1
```

`report(0.1)` was given a plain number and quietly built a `LooseTether` from it. With
`-DSHOW_ERROR`, the file also tries `StrictTether t = 0.1;`:

```
explicit_ctor.cpp:20:18: error: no viable conversion from 'double' to 'StrictTether'
   20 |     StrictTether t = 0.1;         // refused: no silent conversion
      |                  ^   ~~~
...
explicit_ctor.cpp:9:14: note: explicit constructor is not a candidate
    9 |     explicit StrictTether(double k) : k(k) {}
      |              ^
1 error generated.
```

(Two notes about copy and move constructors are cut, marked `...`.) A spring constant is not a
potential, so `HarmonicTether`'s constructor is `explicit`. Mark one-argument constructors `explicit`
unless you want the conversion. The repo is not consistent about it: `DFTPotential(const std::string& parametersFile)`
(`dft_potential.h:12`) is not marked `explicit`.

---

## 7.9 The virtual destructor (brief)

`potential.h:11`, `virtual ~Potential() = default;`. A **destructor** is the method that runs when an
object is destroyed; it is written `~ClassName()`. Chapter 8 is about when that happens. The reason it
must be `virtual` here is the same as for `calculateEnergy`: if code ever destroys a derived object
**through a base pointer**, a non-virtual destructor would run only `~Potential` and skip the derived
part. The repo does exactly that: `ForceCalculator` holds its potentials as base-class smart pointers
(chapter 8), and they destroy the objects through `Potential*`.

The explainer primer has a runnable demonstration with output; read "Why the base class marks its
destructor `virtual`" in [`docs/explainer/00_cpp_primer.md`](../explainer/00_cpp_primer.md) rather
than repeating it here. The rule: **any class with a virtual method gets a virtual destructor.**

---

## 7.10 The mini_lj step

New files: `potential.h` (the base) and `tether.{h,cpp}` (the second potential). Changed:
`lj_potential.h` (five lines, §7.2) and `main.cpp` (§7.3). Unchanged: `geometry.*`, `lattice.*`,
`lj_potential.cpp`.

```bash
c++ -std=c++17 -Wall -O2 main.cpp geometry.cpp lattice.cpp lj_potential.cpp tether.cpp -o out/mini_lj
./out/mini_lj
```
```
[LJPotential constructed, cutoff = 10]
Lennard-Jones: -611.491665613
Harmonic tether: 43.428848333
total: -568.06281728
```

The LJ energy is the same as in chapters 4, 5 and 6. The loop in `main` never names `LJPotential` or
`HarmonicTether`. A third potential would be one more class and one more `&x` in the list; the loop
would not change.

---

## 7.11 How dispatch works: the vtable (deep dive later)

How does `p->calculateEnergy(geometry)` find the right function while the program runs? Almost every
compiler does it the same way.

- For each class with virtual methods, the compiler builds one table of function addresses, the
  **vtable**, with one slot per virtual method. In `LJPotential`'s table, the `calculateEnergy` slot
  holds the address of `LJPotential::calculateEnergy`; in `HarmonicTether`'s table, the same slot holds
  `HarmonicTether::calculateEnergy`.
- Every **object** of such a class carries one hidden pointer to its class's table, the **vptr**, set
  by the constructor.
- A virtual call compiles to: read the object's vptr, read the slot for `calculateEnergy`, call that
  address.

You can see the hidden pointer in the object's size. `examples/07/vtable_size.cpp`:

```cpp
struct Plain       { double k; double energy() const { return k; } };
struct WithVirtual { virtual ~WithVirtual() = default; virtual double energy() const { return k; } double k; };
```
```
sizeof(double)      = 8
sizeof(Plain)       = 8
sizeof(WithVirtual) = 16
```

`WithVirtual` holds the same one `double`, plus an 8-byte vptr.

**What it costs.** Per object, 8 bytes. Per call, a couple of extra memory reads, and — more
important for speed — the compiler cannot paste the function body into the caller (**inlining**),
because it does not know which body will run. For one call per potential per MD step, this is nothing.
Inside a loop over 8 million pairs, it would matter. That is why the repo calls a virtual method
**once** per potential (`calculateEnergy`) and keeps the pair loop inside it free of virtual calls —
the same shape as mini_lj. A vtable lookup is a fixed array index, not a search by name.

---

## 7.12 A second hierarchy in the repo: integrators (skip if you only care about potentials)

`Potential` is the hierarchy you need for the parallel course. The repo has one more,
`MDIntegrator`, and it shows three things `Potential` does not. `src/engine/integrators/md_integrator.h`:

```cpp
14  class MDIntegrator {
15  public:
16      MDIntegrator(const MDParameters& params = MDParameters())
17          : params(params), stepCount(0), time(0.0), energy(0.0),
18            geometry(nullptr), forceCalculator(nullptr) {}
...
124      virtual void step() = 0;
...
127      virtual void run() {
...
135          for (int i = 0; i < params.maxSteps; ++i) {
136              // Take a step
137              step();
...
184  protected:
185      MDParameters params;
186      int stepCount;
```

and `src/engine/integrators/velocity_verlet_integrator.h:9-15`:

```cpp
 9  class VelocityVerletIntegrator : public MDIntegrator {
10  public:
11      VelocityVerletIntegrator(const MDParameters& params = MDParameters())
12          : MDIntegrator(params) {}
13
14      // Implement the Velocity Verlet integration step
15      void step() override {
```

| Line | What it shows |
|---|---|
| `md_integrator.h:16-18` | an initializer list with six members, and a default argument (chapter 5 §5.8) |
| `md_integrator.h:127` | a virtual method **with** a body: derived classes *may* replace `run`, but need not |
| `md_integrator.h:137` | the base class's `run` calls the pure virtual `step()`. The loop is written once in the base; each derived class supplies only the step. (This shape is called the **template method pattern**; it has nothing to do with C++ templates) |
| `md_integrator.h:184` | **`protected:`** — like `private`, but derived classes may use these members too. `VelocityVerletIntegrator::step` reads `geometry`, `forces`, `params` directly |
| `velocity_verlet_integrator.h:12` | the derived constructor's initializer list calls the **base class constructor**, `: MDIntegrator(params)`. The base part of an object is built first |

These integrator classes are not used by `run_simulation` today — `MolecularDynamics` has its own
loop, `runSimulation` and `integrateVelocityVerlet` (`src/engine/molecular_dynamics.cpp:237`, `:440`) —
but they are a clean, small example of a base class doing real work.

---

## 7.13 In the real code

### The contract: `src/engine/force_calculator.h:34-48`

```cpp
34  // Base class for all interaction potentials
35  class Potential {
36  public:
37      virtual ~Potential() = default;
38
39      // Calculate energy for a given geometry
40      virtual double calculateEnergy(const Geometry& geometry) = 0;
41
42      // Calculate forces for a given geometry
43      virtual void calculateForces(const Geometry& geometry,
44                                  std::vector<std::array<double, 3>>& forces) = 0;
45
46      // Get the name of the potential
47      virtual std::string getName() const = 0;
48  };
```

Every line is now §7.1. Note that it lives in `force_calculator.h`, not its own header. Line 11 of the
same file, `class Potential;`, is a **forward declaration**: "a class called `Potential` exists". It is
enough for line 31 to declare a vector of pointers to one before the class is fully described, because
a pointer's size does not depend on what it points at.

### Two signers: `lj_potential.h:14-23` and `dft_potential.h:10-17`

```cpp
14  class LJPotential : public Potential {
15  public:
16      LJPotential();
17      ~LJPotential() override;
...
21      double calculateEnergy(const Geometry& geometry) override;
22      void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) override;
23      std::string getName() const override { return "Lennard-Jones"; }
```

```cpp
10  class DFTPotential : public Potential {
11  public:
12      DFTPotential(const std::string& parametersFile);
13      ~DFTPotential() override = default;
14
15      double calculateEnergy(const Geometry& geometry) override;
16      void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) override;
17      std::string getName() const override { return "DFT"; }
```

Same three methods, same `override`s. Line 17 of `lj_potential.h` and line 13 of `dft_potential.h`
declare destructors with `override`: they confirm that `Potential`'s destructor is virtual (chapter 8
covers destructors). `DFTPotential`'s constructor takes the parameter file, so a DFT potential cannot
exist without one; it is the not-`explicit` constructor from §7.8.

`DFTPotential::calculateEnergy` (`dft_potential.cpp:11-13`) just calls its own
`calculatePotentialEnergy`. That is a common move: a derived class adapts code it already had to the
base class's contract.

### The one loop: `src/engine/force_calculator.cpp:13-19`

```cpp
13  double ForceCalculator::calculateEnergy(const Geometry& geometry) {
14      double totalEnergy = 0.0;
15      for (const auto& potential : potentials) {
16          totalEnergy += potential->calculateEnergy(geometry);
17      }
18      return totalEnergy;
19  }
```

This is mini_lj's loop. `potentials` is the member declared at `force_calculator.h:31`,
`std::vector<std::shared_ptr<Potential>>` — a vector of chapter 8's smart pointers instead of raw
`Potential*`, but `->` and the dispatch work the same way. Line 16 is a virtual call: it runs
`LJPotential::calculateEnergy` or `DFTPotential::calculateEnergy`, whichever the element holds.
`calculateForces` (lines 21-45) does the same for forces, summing each potential's forces into one
vector.

This is what the project's `CLAUDE.md` means by "the seam where new potential types plug in". A **seam**
is a joint designed so one side can change without editing the other (see
[GLOSSARY: Seam](../learn-parallel/GLOSSARY.md#design-vocabulary)). A new potential is a new class
derived from `Potential`; `ForceCalculator` does not change by one character.

---

## Next

The list `{&lj, &tether}` holds the addresses of two objects that live in `main`'s own variables, and
they stay alive until `main` returns, so the pointers are safe. The repo does not work that way: a
function such as `YAMLParser::parsePotential` builds the potential inside itself and **returns** it to
the caller. A variable that lives inside a function is destroyed when the function returns, so a
pointer to it would point at nothing. Chapter 8 is about who owns an object and how long it lives —
and why `ForceCalculator` holds `std::shared_ptr<Potential>` rather than `Potential*`.
