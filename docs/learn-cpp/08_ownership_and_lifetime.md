# Chapter 8 — Ownership and lifetime: who throws the object away

Plain version: [08p_ownership_and_lifetime_plain.md](08p_ownership_and_lifetime_plain.md).
Code: [`examples/08/`](examples/08/). Every output below was produced by compiling and running it with
Apple clang 17 on this machine (an M1).

**After this chapter you can say:**

- when a C++ object is destroyed (at the closing brace of the block that made it), and what a
  **destructor** is;
- the difference between the **stack** and the **heap**, and which one `std::make_shared` uses;
- why `return &local;` is a bug, what a **dangling pointer** is, and why chapter 7's `{&lj, &tether}`
  breaks the moment it moves into a function;
- what `std::unique_ptr` and `std::shared_ptr` own, what `use_count()` counts, and why the repo never
  writes `new` or `delete`;
- why a raw pointer is fine when something else owns the memory, and why `LJKernelView`'s pointers
  would go stale if a vector resized after they were taken;
- how `run_simulation.cpp:43-46` builds a `ForceCalculator` that outlives every line that built it.

Time: about 35 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/08
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## Where we are

At the end of chapter 7, `main` held `std::vector<Potential*> potentials = {&lj, &tether};` and summed
both energies through one loop. That works because `lj` and `tether` are variables of `main`, and `main`
outlives the loop that uses them. But the real program does not build its potentials in `main`. The YAML
parser builds them inside a function and hands them back. So: if a function builds an `LJPotential` and
returns its address, what does that address point at once the function has returned?

---

## 8.0 Lifetime: who decides when an object dies

Chapter 2 said a C++ variable *is* its storage: `double cutoff = 2.5;` is 8 bytes with a type painted on.
Storage has a **lifetime**: the stretch of the run during which it exists. For an ordinary variable, the
lifetime is fixed by *where you wrote it*, not by who still refers to it. Nothing counts references unless
you ask for counting.

| Question | C++ answer |
|---|---|
| who decides when an object dies | the place the object was created, unless you choose otherwise |
| `b = a` | a **copy** (ch 3): two independent objects with separate lifetimes |
| can something still point at a dead object? | yes, and it is a serious bug (§8.3) |
| cost of tracking lifetimes | zero, unless you opt in to counting (§8.6) |

> **Unlike Python:** you never decide when a Python object dies. Every object carries a count of the names
> and containers that refer to it, `b = a` adds one to that count, and the object goes when the count
> reaches zero (a separate collector handles cycles). So pointing at an object keeps it alive, and a
> pointer to a dead object cannot exist. In C++ it can.

This chapter is the "unless you choose otherwise" part.

---

## 8.1 The closing brace destroys: destructors

A **scope** is the region between a `{` and its matching `}`: a function body, a loop body, an `if`
branch, or a bare block. An object created inside a scope is destroyed when the run reaches that
scope's closing brace.

Just before an object is destroyed, C++ calls its **destructor** (chapter 7 §7.9 met the word; this is when it runs). That is a member function named
`~` plus the class name, with no parameters and no return type. It runs at a *known* moment, every time.

`examples/08/scope.cpp`:

```cpp
class Noisy {
public:
    explicit Noisy(const std::string& name) : name(name) {
        std::cout << "  construct " << name << std::endl;
    }
    ~Noisy() { std::cout << "  destroy   " << name << std::endl; }   // the destructor

private:
    std::string name;
};

int main() {
    std::cout << "enter main" << std::endl;
    Noisy a("a");
    {
        std::cout << "enter inner block" << std::endl;
        Noisy b("b");
        Noisy c("c");
        std::cout << "leave inner block" << std::endl;
    }   // c, then b, are destroyed here: reverse order of creation
    std::cout << "leave main" << std::endl;
    return 0;
}   // a is destroyed here
```
```bash
c++ -std=c++17 -Wall -O2 scope.cpp -o out/scope
./out/scope
```
```
enter main
  construct a
enter inner block
  construct b
  construct c
leave inner block
  destroy   c
  destroy   b
leave main
  destroy   a
```

Three rules to read off that output:

1. `b` and `c` die at the inner `}`, before `leave main` is printed. Nobody wrote "delete b".
2. Objects in one scope die in **reverse** order of creation: `c` before `b`. (Something made later may
   depend on something made earlier, so the later one goes first.)
3. `a` dies after `return 0;`, at `main`'s own closing brace.

### RAII: the pattern built on rule 1

Many objects hold a **resource**: something that must be given back. Examples are a block of memory,
an open file, a lock. The C++ habit is: acquire the resource in the constructor, release it in the
destructor. Then the closing brace releases it, whatever path the code took to get there, including an
exception. The name for this habit is **RAII**, "Resource Acquisition Is Initialization". It is a clumsy
name for a simple rule: *the object's lifetime is the resource's lifetime*.

You have used RAII since chapter 4 without seeing it. A `std::vector<Vec3>` asks for a block of memory
for its elements, and its destructor gives the block back. The repo's trajectory writer closes its file
in its destructor (`src/utils/trajectory_writer.cpp:51-53`):

```cpp
51  TrajectoryWriter::~TrajectoryWriter() {
52      close();
53  }
```

Nothing has to be remembered: the destructor always runs at the closing brace.

> **Unlike Python:** releasing a resource at a known point needs a `with` block, and you must remember to
> write it; otherwise the object goes whenever the runtime gets to it. In C++ every block behaves like a
> `with` block, for every object in it.

---

## 8.2 Two places to keep things: the stack and the heap

Where does the storage for an object come from? There are two places.

**The stack.** Each function call gets a **stack frame**: a slab of memory for its local variables.
Calling a function pushes a frame; returning pops it, and everything in it is destroyed. Making room
on the stack is nearly free. The catch: the frame vanishes when the function returns. Objects here have
**automatic** lifetime. Every variable you have declared inside a function so far lives here.

**The heap.** A large pool of memory that is not tied to any function call. You ask for a piece; it is
yours until someone gives it back. Asking costs more (the program must find a free piece and record
it), and *someone* has to give it back. (The name "heap" here has nothing to do with the heap data
structure.)

| | stack | heap |
|---|---|---|
| how you get it | declare a local variable: `LJPotential lj;` | ask: `new`, or better `std::make_unique` / `std::make_shared` |
| when it goes | at the closing brace, automatically | when the owner gives it back |
| cost to get | almost nothing | a real allocation |
| size | small: megabytes, not gigabytes | as big as memory |
| survives the function returning? | **no** | yes |

Most objects use both. In `mini_lj`, `const Geometry geometry = makeCubicLattice(...)` in `main` is a
stack object. But a `std::vector` inside it is only a small handle on the stack: a few pointers saying
where its elements are. The 4096 positions themselves sit in a heap block that the vector owns, and the
vector's destructor frees that block. That is how a vector can grow: the handle stays put while the
heap block is replaced by a bigger one.

---

## 8.3 The dangling pointer

A **dangling pointer** is a pointer that still holds an address after the object at that address has been
destroyed. Following it is **undefined behaviour**: the C++ standard places no requirement at all on what
happens. The program may print a plausible number, print garbage, or crash, and may do a different one
tomorrow.

The smallest way to make one is to return the address of a local variable. `examples/08/dangling.cpp`:

```cpp
double* makeCutoff() {
    double cutoff = 2.5;   // lives in makeCutoff's stack frame
    return &cutoff;        // the address of something destroyed at the next '}'
}

void otherWork() {
    volatile double scratch[4] = {111.0, 222.0, 333.0, 444.0};   // reuses the same stack space
    (void)scratch;
}

int main() {
    double* p = makeCutoff();
    otherWork();
    std::cout << "*p = " << *p << std::endl;   // undefined behaviour: anything may happen
    return 0;
}
```
```bash
c++ -std=c++17 -Wall -O2 dangling.cpp -o out/dangling
./out/dangling
```
```
dangling.cpp:6:13: warning: address of stack memory associated with local variable 'cutoff' returned [-Wreturn-stack-address]
    6 |     return &cutoff;        // the address of something destroyed at the next '}'
      |             ^~~~~~
1 warning generated.
*p = 0
```

The compiler spotted this one. It is a **warning**, not an error, so a program was still produced. That
program printed `0`, not `2.5`. Built at `-O0` instead, the same source printed `*p = 2.14436e-314`. Neither
number means anything. (`volatile` only stops the optimiser from deleting `otherWork`'s array.)

### Chapter 7's line, moved into a function

Now the question from "Where we are". `examples/08/dangling_potentials.cpp` takes chapter 7's two
potentials and builds them in a function, the way the YAML parser does:

```cpp
std::vector<Potential*> buildPotentials() {
    LJPotential lj;
    lj.addAtomParameters("LJ", 1.0, 1.0);
    lj.setCutoff(2.5);
    HarmonicTether tether(0.1);
    std::vector<Potential*> potentials = {&lj, &tether};
    return potentials;   // the vector of addresses is copied out...
}                        // ...and lj and tether are destroyed here

int main() {
    const Geometry geometry = makeCubicLattice(125, 0.8, 42);
    std::vector<Potential*> potentials = buildPotentials();
    std::cout << "[back in main, holding " << potentials.size() << " addresses]" << std::endl;
    for (Potential* p : potentials)
        std::cout << p->getName() << ": " << p->calculateEnergy(geometry) << std::endl;
    return 0;
}
```

This uses chapter 8's `LJPotential`, which now has a destructor that prints (§8.7).

```bash
c++ -std=c++17 -Wall -O2 dangling_potentials.cpp geometry.cpp lattice.cpp lj_potential.cpp tether.cpp -o out/dangling_potentials
./out/dangling_potentials; echo "exit=$?"
```
```
[LJPotential constructed, cutoff = 10]
[LJPotential destroyed]
[back in main, holding 2 addresses]
exit=139
```

No warning this time: the addresses travel inside a vector, and the compiler does not follow them.
`[LJPotential destroyed]` is printed *before* we get back to `main`. Then the first `p->getName()` reads a
dead object and the program is killed. Exit status 139 is 128 + 11, and signal 11 is a **segmentation
fault**: the operating system stopped the program for touching memory in a way it is not allowed to. (An
interactive shell usually also prints a message such as `segmentation fault`.)

So the answer to chapter 7's question is: the addresses point at counter space that has already been
wiped. To return potentials from a function, they must live somewhere that does not vanish when the
function returns. That means the heap, and something to own them there.

---

## 8.4 `new` and `delete`, and why the repo never writes them

The old way to put an object on the heap is `new`. It allocates the storage, runs the constructor, and
returns a pointer. `delete p` runs the destructor and gives the storage back. `examples/08/raw_new.cpp`:

```cpp
Noisy* makeOnHeap(const std::string& name) {
    return new Noisy(name);    // storage outlives this function: fine
}

int main() {
    Noisy* kept = makeOnHeap("kept");
    Noisy* forgotten = makeOnHeap("forgotten");
    delete kept;               // destructor runs, storage returned
    (void)forgotten;           // no delete: a leak. Its destructor never runs.
    std::cout << "end of main" << std::endl;
    return 0;
}
```
```
  construct kept
  construct forgotten
  destroy   kept
end of main
```

`forgotten` is never destroyed. That is a **leak**: memory that nobody will ever give back. Here the
operating system reclaims it when the program exits. In a loop that runs 5000 MD steps, a leak per step
adds up.

Raw `new`/`delete` asks you to get three things right, on every path through the code, forever:

| mistake | result |
|---|---|
| forget `delete` | leak; destructor never runs (the file is never closed, …) |
| `delete` twice | undefined behaviour, often a crash far away from the bug |
| use the pointer after `delete` | dangling pointer, as in §8.3 |
| an exception is thrown between `new` and `delete` | the `delete` line is skipped: leak |

The pointer `Noisy*` does not say whether its holder must `delete` it. That missing information is the
real problem. Modern C++ puts it in the type, with a **smart pointer**: a small object that holds a raw
pointer and whose destructor does the `delete` for you. It is RAII again: the smart pointer's lifetime
is the heap object's ownership.

This repo has no `new` or `delete` anywhere in `src/` outside comments:

```bash
grep -rnwE 'new|delete' src
```
```
src/utils/yaml_parser.cpp:338:            // Create a new LJ potential
src/engine/potentials/lj_kernels.h:57:// to any caller — only a new case inside ljComputeGather.
src/engine/minimizers/conjugate_gradient_minimizer.h:72:    // Override to calculate new conjugate directions after energy calculation
src/engine/minimizers/conjugate_gradient_minimizer.h:86:            // Calculate new energy and forces
src/engine/minimizers/energy_minimizer.h:64:            // Calculate new energy and forces
src/engine/integrators/velocity_verlet_integrator.cpp:9:    // First half: new positions from f(t), velocities half-kicked by f(t)
src/engine/integrators/velocity_verlet_integrator.cpp:28:    // f(t+dt) at the new positions
src/engine/molecular_dynamics.cpp:26:    // No need to delete forceCalculator as it's a shared_ptr
src/engine/molecular_dynamics.cpp:87:        // Create the new path
src/engine/molecular_dynamics.cpp:318:        // and potential energy at the new positions
src/engine/molecular_dynamics.cpp:438:        // Calculate new energy and forces
```

Every hit is a comment. Heap objects in the repo are made with `std::make_shared` or `std::make_unique`.

---

## 8.5 `std::unique_ptr`: exactly one owner

`std::unique_ptr<T>` (from `<memory>`) owns one heap object of type `T`. When the `unique_ptr` is
destroyed, it deletes the object. It cannot be copied, because two copies would both try to delete.
It can be **moved**: ownership passes to another `unique_ptr`, and the old one becomes empty.

`std::make_unique<T>(args...)` allocates a `T` on the heap, passes `args` to its constructor, and returns
the `unique_ptr` that owns it. First half of `examples/08/smart.cpp`:

```cpp
    {
        std::unique_ptr<Noisy> u = std::make_unique<Noisy>("u");
        // std::unique_ptr<Noisy> copy = u;          // does not compile: only one owner allowed
        std::unique_ptr<Noisy> moved = std::move(u); // hand ownership over
        std::cout << "  u is " << (u ? "full" : "empty") << " after the move" << std::endl;
    }   // 'moved' goes out of scope: destroy
```
```
-- unique_ptr
  construct u
  u is empty after the move
  destroy   u
```

One construct, one destroy, and the destroy happens at the closing brace because that is where `moved`,
the current owner, dies. `u ? … : …` works because a smart pointer tests as `true` when it holds
something.

In the repo, `MolecularDynamics` owns its trajectory writer this way (`src/engine/molecular_dynamics.h:123`
and `src/engine/molecular_dynamics.cpp:92`):

```cpp
123      std::unique_ptr<TrajectoryWriter> trajectoryWriter;
```
```cpp
92      trajectoryWriter = std::make_unique<TrajectoryWriter>(outputFilename, selectedFormat);
```

Nobody else needs the writer, so one owner is the right answer. When the `MolecularDynamics` object
dies, its `unique_ptr` member dies, which deletes the writer, whose destructor closes the file
(§8.1). One chain of closing braces, no `delete` written anywhere.

### What `std::move` does, and what "moved-from" means

`std::move(u)` does not move anything by itself. It marks `u` as "you may take this one's insides". The
`unique_ptr` that receives it takes the raw pointer and sets `u`'s to `nullptr`. That is why the output
says `u is empty`. A moved-from `unique_ptr` is safe to destroy or to assign to, but it no longer owns
anything. The same mechanism lets a `std::vector` be returned from a function without copying its
elements: the heap block is handed over, not duplicated. You will meet `std::move` again in chapter 9:
`std::thread`, like `unique_ptr`, can be moved but not copied.

---

## 8.6 `std::shared_ptr`: counted owners

Sometimes more than one part of the program needs the same object, and none of them is clearly the
last user. In `run_simulation`, both `main` and the `MolecularDynamics` object hold the `ForceCalculator`.
For that, `std::shared_ptr<T>` keeps a **use count**: how many `shared_ptr`s currently own the object.

- Copying a `shared_ptr` adds an owner: count + 1.
- Destroying (or `reset()`-ing) one removes an owner: count − 1.
- When the count reaches 0, the last one deletes the object.

`std::make_shared<T>(args...)` allocates the `T` and its count together on the heap and returns the
first `shared_ptr`, count 1. Second half of `examples/08/smart.cpp`:

```cpp
    std::shared_ptr<Noisy> first = std::make_shared<Noisy>("s");
    std::cout << "  use_count = " << first.use_count() << std::endl;
    {
        std::shared_ptr<Noisy> second = first;       // copying a shared_ptr adds an owner
        std::cout << "  use_count = " << first.use_count() << std::endl;
    }   // 'second' gone: count drops, object stays
    std::cout << "  use_count = " << first.use_count() << std::endl;
    first.reset();                                   // last owner lets go: destroy
    std::cout << "end of main" << std::endl;
```
```bash
c++ -std=c++17 -Wall -O2 smart.cpp -o out/smart
./out/smart
```
```
-- shared_ptr
  construct s
  use_count = 1
  use_count = 2
  use_count = 1
  destroy   s
end of main
```

Two owners, one lets go, the object stays; the last lets go, the object goes.
`std::shared_ptr<Noisy> second = first;` makes a second handle on the *same* object, not a copy of the
object.

> **Unlike Python:** there, every object is reference-counted, always, and `b = a` always means "a second
> name on the same object". A `shared_ptr` is that same counting, opted into for one object; copying it is
> the closest C++ gets to Python's `b = a`.

You reach the object through a smart pointer with `->`, exactly as through a raw pointer (ch 3):
`lj->setCutoff(2.5)` means "call `setCutoff` on the object `lj` points at".

| | `std::shared_ptr` | `std::unique_ptr` |
|---|---|---|
| what is counted | only objects you create with `make_shared` | nothing: one owner |
| `b = a` | copy adds an owner | does not compile; `std::move` hands over |
| how many owners now? | `a.use_count()` | always one |
| letting go | `a.reset()`, or `a` going out of scope | same |
| cost | a count update on each copy | nothing beyond a raw pointer |

### Cycles, and `std::weak_ptr` (deep dive later)

A **cycle** is A referring to B and B referring to A, so neither count ever reaches zero. `shared_ptr`
has no collector that looks for cycles. If two objects hold `shared_ptr`s to each other, both leak. The standard fix is `std::weak_ptr`: a pointer that watches a shared object without adding to its
count. The repo has no such cycles: ownership only ever flows downward (`main` → `ForceCalculator` →
potentials), which is the usual way to keep `shared_ptr` code simple.

---

## 8.7 The mini_lj step: build the potentials in a function

Chapter 7's `main` built `lj` and `tether` itself. Chapter 8 moves that into a function,
`buildForceCalculator()`, which is the shape the repo's YAML parser has. Three new things, all in
[`examples/08/`](examples/08/):

**1. A `ForceCalculator` that owns its potentials** (`force_calculator.h`, same shape as the repo's):

```cpp
 9  class ForceCalculator {
10  public:
11      void addPotential(std::shared_ptr<Potential> potential) { potentials.push_back(potential); }
12      double calculateEnergy(const Geometry& geometry);
13      void calculateForces(const Geometry& geometry, std::vector<Vec3>& forces);
14
15  private:
16      std::vector<std::shared_ptr<Potential>> potentials;
17  };
```

Line 16 replaces chapter 7's `std::vector<Potential*>`. The vector now holds *owners*, not bare addresses.
`calculateEnergy` (`force_calculator.cpp:3-7`) is chapter 7's loop, moved into the class:

```cpp
3  double ForceCalculator::calculateEnergy(const Geometry& geometry) {
4      double total = 0.0;
5      for (const auto& potential : potentials) total += potential->calculateEnergy(geometry);
6      return total;
7  }
```

`potential->calculateEnergy(...)` still dispatches through `virtual` (ch 7). The `->` goes through the
`shared_ptr` to the object, and the object's real class decides which function runs.

**2. A destructor that prints** (`lj_potential.h:14`, `lj_potential.cpp:10-12`), so we can see when the
`LJPotential` dies:

```cpp
14      ~LJPotential() override;
```
```cpp
10  LJPotential::~LJPotential() {
11      std::cout << "[LJPotential destroyed]" << std::endl;
12  }
```

`override` works here because chapter 7 gave `Potential` a `virtual` destructor (`potential.h:11`).

**3. The function that builds everything** (`main.cpp:13-23`):

```cpp
13  std::shared_ptr<ForceCalculator> buildForceCalculator() {
14      auto lj = std::make_shared<LJPotential>();
15      lj->addAtomParameters("LJ", 1.0, 1.0);
16      lj->setCutoff(2.5);
17
18      auto forceCalculator = std::make_shared<ForceCalculator>();
19      forceCalculator->addPotential(lj);
20      forceCalculator->addPotential(std::make_shared<HarmonicTether>(0.1));
21      std::cout << "[leaving buildForceCalculator; lj use_count = " << lj.use_count() << "]" << std::endl;
22      return forceCalculator;
23  }
```

| Line | What happens | `LJPotential`'s count |
|---|---|---|
| 14 | `make_shared` puts an `LJPotential` on the heap; the local `lj` owns it. `auto` saves writing `std::shared_ptr<LJPotential>` | 1 |
| 19 | `addPotential` takes a copy (a `shared_ptr<Potential>`, made from `lj`) and `push_back` stores it in the vector | 2 |
| 20 | a `HarmonicTether` goes on the heap, owned only by the vector | (tether: 1) |
| 21 | prints the count | **2** |
| 22–23 | returns `forceCalculator`; the local `lj` dies at the `}` | 1 |

The `LJPotential` survives the closing brace because the vector inside the `ForceCalculator` still owns
it, and the `ForceCalculator` itself is owned by the `shared_ptr` that `main` receives.

Line 19 also converts: `lj` is a `shared_ptr<LJPotential>`, but `addPotential` takes a
`shared_ptr<Potential>`. A `shared_ptr` to a derived class converts to a `shared_ptr` to its base, just
as `LJPotential*` converts to `Potential*` (ch 7). Both share the same count.

`main` (`main.cpp:25-39`) now has no potential variables at all:

```cpp
29      auto forceCalculator = buildForceCalculator();
30
31      std::vector<Vec3> forces;
32      forceCalculator->calculateForces(geometry, forces);
33
34      std::cout.precision(12);
35      std::cout << "total E = " << forceCalculator->calculateEnergy(geometry) << std::endl;
36      std::cout << "force on atom 0 = (" << forces[0][0] << ", " << forces[0][1] << ", " << forces[0][2] << ")" << std::endl;
37      std::cout << "[end of main]" << std::endl;
38      return 0;
39  }
```

Compile and run (list the files: the folder also holds the toy programs, which have their own `main`):

```bash
c++ -std=c++17 -Wall -O2 main.cpp force_calculator.cpp geometry.cpp lattice.cpp lj_potential.cpp tether.cpp -o out/mini_lj
./out/mini_lj
```
```
[LJPotential constructed, cutoff = 10]
[leaving buildForceCalculator; lj use_count = 2]
total E = -568.06281728
force on atom 0 = (-3.89410964909, 10.0790696534, -13.6814548653)
[end of main]
[LJPotential destroyed]
```

Read it top to bottom:

- `total E = -568.06281728` is chapter 7's total (LJ −611.491665613 plus tether 43.428848333). Same
  objects, same numbers. Only who owns them changed.
- The force on atom 0 differs from chapter 6's LJ-only `(-4.10635870563, 9.86021608902, -13.8938854543)`
  because the tether's pull is now added in by `ForceCalculator::calculateForces`.
- `[LJPotential destroyed]` comes **after** `[end of main]`. The last owner is `main`'s
  `forceCalculator`. It dies at `main`'s closing brace. Its destructor destroys its vector. The vector
  destroys its `shared_ptr`s. The `LJPotential`'s count hits 0, and its destructor runs. Nobody wrote a
  single line of that.
- The tether is destroyed at the same moment, silently; its class has no printing destructor.

What it buys: objects can be built anywhere and handed around freely, with no question of who must delete
them. What it costs: a heap allocation per object, and an extra count update on every copy of a
`shared_ptr`. For a handful of potentials built once per run, both are negligible. You would not use
`shared_ptr` for each of 4096 atoms.

---

## 8.8 Borrowing: raw pointers that do not own

`shared_ptr` everywhere would be wrong too. Most pointers in a program do not own anything; they
**borrow**: they point at memory that someone else owns and will free. A borrowed pointer is perfectly
safe under one rule:

> **A non-owning pointer must not be used after its owner has freed or moved the memory.**

The repo's fast LJ kernel is built on borrowed pointers. `LJKernelView` (`src/engine/potentials/lj_kernels.h:25-38`)
holds plain addresses:

```cpp
25  struct LJKernelView {
26      // Positions, length n each.
27      const double* x = nullptr;
28      const double* y = nullptr;
29      const double* z = nullptr;
    ...
36      double* fx = nullptr;
```

The view owns nothing. The numbers live in `std::vector<double>` members of `LJPotential`
(`lj_potential.h:91`):

```cpp
91      std::vector<double> sx, sy, sz, sfx, sfy, sfz;
```

and `buildView` points the view at them with `.data()`, which returns the address of a vector's first
element (`src/engine/potentials/lj_potential.cpp:99` and `:136-138`):

```cpp
 99      sx.resize(n); sy.resize(n); sz.resize(n);
    ...
136      view.x = sx.data();  view.y = sy.data();  view.z = sz.data();
137      view.fx = sfx.data(); view.fy = sfy.data(); view.fz = sfz.data();
138      view.type = stype.data();
```

### Why the order of those lines matters

A vector's elements sit in one heap block (§8.2). When the vector must grow past that block's size
(its **capacity**), it gets a new, bigger block, moves the elements across, and **frees the old block**.
Any pointer taken from `.data()` before that still holds the old address. `examples/08/stale_data.cpp`:

```cpp
    std::vector<double> sx(4, 1.0);
    const double* x = sx.data();   // non-owning: sx still owns the numbers
    std::cout << "capacity " << sx.capacity() << ", x == sx.data(): " << (x == sx.data()) << std::endl;

    sx.resize(1000);               // too big for the old block: new block, old one freed
    std::cout << "capacity " << sx.capacity() << ", x == sx.data(): " << (x == sx.data()) << std::endl;

    std::cout << "x[0] = " << x[0] << std::endl;   // reads freed memory: undefined behaviour
```
```bash
c++ -std=c++17 -Wall -O2 stale_data.cpp -o out/stale_data
./out/stale_data
```
```
capacity 4, x == sx.data(): 1
capacity 1000, x == sx.data(): 0
x[0] = 5.88185e-310
```

(`1` and `0` are how `std::cout` prints `true` and `false`.) The vector was filled with `1.0`, and `x[0]`
printed `5.88185e-310`. No warning, no crash: just a wrong number.

That is why `buildView` resizes first (line 99) and takes the addresses last (lines 136–141). The view is
also a local variable of the function that uses it (`lj_potential.cpp:172-174`):

```cpp
172      LJKernelView view;
173      buildView(geometry, view);
174      const double energy = ljComputeGather(view, backend, numThreads);
```

The view is born after the vectors were sized and dies at the end of `calculateForcesAndEnergy`. No resize can
happen while it is alive. The owner (`LJPotential`) outlives the borrower (`view`). That is the whole
safety argument, and it is why the view can be plain pointers, which is what a GPU needs (learn-parallel
09, and the POD entry in [GLOSSARY.md](../learn-parallel/GLOSSARY.md)).

### Catching it with AddressSanitizer (skip if you only read code)

Bugs like `stale_data.cpp` print a wrong number and carry on. The compiler can build a checked program
that stops on them instead. `-fsanitize=address` turns on **AddressSanitizer**, which watches every
memory access (the program runs a few times slower):

```bash
c++ -std=c++17 -g -fsanitize=address stale_data.cpp -o out/stale_asan
./out/stale_asan
```
```
capacity 4, x == sx.data(): 1
capacity 1000, x == sx.data(): 0
=================================================================
==70332==ERROR: AddressSanitizer: heap-use-after-free on address 0x603000001bd0 at pc 0x000100fe0b6c bp 0x00016ee1e950 sp 0x00016ee1e948
READ of size 8 at 0x603000001bd0 thread T0
    #0 0x000100fe0b68 in main stale_data.cpp:13
...
freed by thread T0 here:
...
    #8 0x000100fe0fe4 in std::__1::vector<double, std::__1::allocator<double>>::resize(unsigned long) vector.h:1355
    #9 0x000100fe0a8c in main stale_data.cpp:10
```

(Shortened; addresses differ on every run.) It names the bad read (line 13) *and* the line that freed the
memory (line 10, the `resize`). `-g` adds the file and line information. Chapter 9 uses the same tool on a
threading bug.

---

## 8.9 In the real code

**Building and handing over** — `src/tools/run_simulation.cpp:42-51`:

```cpp
42          // Load potentials
43          auto forceCalculator = std::make_shared<ForceCalculator>();
44          for (const auto& potentialFile : config.potentialConfigFiles) {
45              forceCalculator->addPotential(parser.parsePotential(potentialFile));
46          }
47
48          // Set up and run molecular dynamics
49          MolecularDynamics dynamics;
50          dynamics.setGeometry(geometry);
51          dynamics.setForceCalculator(forceCalculator);
```

This is `mini_lj`'s `buildForceCalculator`, spread over two places. Line 43 is `main.cpp:18`. Line 45 calls
`parsePotential`, which returns a `std::shared_ptr<Potential>` built inside the parser
(`src/utils/yaml_parser.cpp:331`, `:339`, `:359`):

```cpp
331  std::shared_ptr<Potential> YAMLParser::parseClassicalPotential(const nlohmann::json& json) {
     ...
339              auto ljPotential = std::make_shared<LJPotential>();
     ...
359              return ljPotential;
```

Line 339 is `main.cpp:14`, nearly character for character. Line 359 returns a `shared_ptr<LJPotential>`
from a function declared to return `shared_ptr<Potential>`: the derived-to-base conversion from §8.7.
The parser's local `ljPotential` dies at the function's `}`, and the object lives on in the returned
`shared_ptr`. This is the exact situation `dangling_potentials.cpp` got wrong, done right.

**Who owns what** — `src/engine/force_calculator.h:19` and `:35`:

```cpp
19      void addPotential(std::shared_ptr<Potential> potential);
    ...
35      std::vector<std::shared_ptr<Potential>> potentials;
```

and `src/engine/molecular_dynamics.cpp:208-210`:

```cpp
208  void MolecularDynamics::setForceCalculator(std::shared_ptr<ForceCalculator> forceCalc) {
209      this->forceCalculator = forceCalc;
210  }
```

Line 209 copies a `shared_ptr` into the member declared at `src/engine/molecular_dynamics.h:116`
(`std::shared_ptr<ForceCalculator> forceCalculator;`). After `run_simulation.cpp:51`, the
`ForceCalculator` has two owners: `main`'s local `forceCalculator` and `dynamics`' member. Neither has to
know whether the other is finished with it. The destructor at `src/engine/molecular_dynamics.cpp:25-27`
records exactly that:

```cpp
25  MolecularDynamics::~MolecularDynamics() {
26      // No need to delete forceCalculator as it's a shared_ptr
27  }
```

**The whole ownership tree of a run**, at the moment `dynamics.runSimulation()` starts:

```
main (stack)
├── geometry             Geometry              stack object; its vectors own heap blocks
├── forceCalculator      shared_ptr ─────┐
└── dynamics             MolecularDynamics (stack)
    ├── forceCalculator  shared_ptr ─────┴──► ForceCalculator (heap, use_count 2)
    │                                         └── potentials: vector<shared_ptr<Potential>>
    │                                             └──► LJPotential (heap, use_count 1)
    │                                                  └── sx, sy, sz, … vectors (borrowed by LJKernelView)
    ├── integrator       shared_ptr ────────► VelocityVerletIntegrator (heap)
    ├── thermostat       shared_ptr ────────► VelocityRescaleThermostat (heap)
    └── trajectoryWriter unique_ptr ────────► TrajectoryWriter (heap) ── closes its file in ~TrajectoryWriter
```

When `main`'s `try` block ends, `dynamics` and `forceCalculator` die in reverse order, and every heap object
in that tree is freed by a destructor. The repo never says `delete`.

### Why pybind11 wants `shared_ptr` (deep dive later)

`src/bindings/molsim_bindings.cpp:108` and `:110`:

```cpp
108     py::class_<Potential, std::shared_ptr<Potential>>(module, "Potential");
    ...
110     py::class_<LJPotential, Potential, std::shared_ptr<LJPotential>>(module, "LJPotential")
```

The second template argument tells pybind11 to hold these objects through `shared_ptr`. A Python script
can create an `LJPotential`, give it to a `ForceCalculator`, and then drop its own name for it. Python's
reference count and the C++ use count then each keep the object alive for their own side. Ownership is
shared across the language boundary. [`docs/explainer/05_python_interface.md`](../explainer/05_python_interface.md)
walks the bindings.

---

## Next

`mini_lj` now builds its potentials in a function and owns them the way the repo does. Every
object lives exactly as long as something needs it. What we have never asked is how *long* the energy
loop takes. And it runs on one core, while this M1 has eight. Chapter 9 times the loop and splits it
across threads, which needs two new tools: templates and lambdas.
