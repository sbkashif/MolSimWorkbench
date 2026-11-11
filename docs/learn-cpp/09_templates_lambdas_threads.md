# Chapter 9 — Templates, lambdas, and a first thread

Plain version: [09p_templates_lambdas_threads_plain.md](09p_templates_lambdas_threads_plain.md).
Code: [`examples/09/`](examples/09/). Every output below was produced by compiling and running it with
Apple clang 17 on this machine: an M1 with 4 performance cores and 4 efficiency cores. Your timings will
differ; your energies should not.

**After this chapter you can say:**

- what `template <typename F>` means, and why a function that takes "any piece of code" needs one;
- how to write a **lambda**, and what its capture list `[&lj, i0, t]` grabs, as a copy or as the original;
- how `timeMs` measures a piece of code (warm-up, then the fastest of several runs), and where the
  methodology is taught properly;
- how to start a `std::thread`, why it must be joined, and why each thread writes only its own `partial[t]`;
- why two threads gave **1.38×** and not 2× on the energy loop: the pair counts of a triangular loop;
- why splitting the *forces* the same way would be wrong, and which learn-parallel lessons deal with it.

Time: about 35 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/09
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## Where we are

At the end of chapter 8, `mini_lj` built its potentials in a function and owned them through
`shared_ptr`, exactly like the repo; every object lived as long as it was needed and no longer. What we
never did is ask how long the energy loop takes. It runs on one core, and this M1 has eight. This chapter
times the loop, then splits it across threads, and for both jobs we need to hand *a piece of code* to
another piece of code.

---

## 9.0 The shape of the problem

To time something, we want to write the timer once and pass in "the thing to time". To run work on a
thread, we pass the thread "the work to run". Both mean passing code as a value.

C++ needs two pieces for that: a **lambda** to write the code on the spot (§9.2), and a **template** so
the timer can accept it (§9.1). We take templates first because the lambda's type turns out to need one.

> **Unlike Python:** any function or `lambda` is already a value you can pass around, and nobody writes
> down its type. In C++ the type of every parameter must be known when the program is compiled, including
> the type of a piece of code, and that is the job templates do here.

---

## 9.1 Templates: a recipe with a blank

A **function template** is a function written with a placeholder type. `examples/09/template_blank.cpp`:

```cpp
template <typename T>
T twice(T x) {
    return x + x;
}

int main() {
    std::cout << twice(21) << std::endl;                  // T = int
    std::cout << twice(1.25) << std::endl;                // T = double
    std::cout << twice(std::string("ab")) << std::endl;   // T = std::string
```
```bash
c++ -std=c++17 -Wall -O2 template_blank.cpp -o out/template_blank
./out/template_blank
```
```
42
2.5
abab
```

`template <typename T>` says: "`T` is a blank to be filled in with a type." At each call, the compiler
works out `T` from the argument and writes a real function for that type: one `twice` for `int`, one for
`double`, one for `std::string`. This is called **instantiation**. Each instantiation is compiled and
checked like any hand-written function.

So the template works for any type that supports `+`, and whether a type does is checked *when the
program is compiled*, at the call that instantiates it. The same file has a fourth call behind a switch
(`#ifdef`, ch 1) so you can see the failure:

```bash
c++ -std=c++17 -Wall -O2 -DSHOW_ERROR template_blank.cpp -o out/tb_err
```
```
template_blank.cpp:8:14: error: invalid operands to binary expression ('std::vector<int>' and 'std::vector<int>')
    8 |     return x + x;
      |            ~ ^ ~
template_blank.cpp:17:5: note: in instantiation of function template specialization 'twice<std::vector<int>>' requested here
   17 |     twice(v);                                             // T = std::vector<int>: no '+' for that
      |     ^
```

(Followed by a long list of `+` operators the compiler tried and rejected. Read the first error and the
"in instantiation of" note; skip the rest.) No program was produced.

You have been *filling in* templates since chapter 3: `std::array<double, 3>`, `std::vector<Vec3>`,
`std::map<std::string, Params>`, `std::shared_ptr<Potential>`, `std::make_shared<LJPotential>()`. The
angle brackets fill the blanks. `template <typename T>` is the other side: *declaring* a blank.

| | C++ template |
|---|---|
| one function, many types | yes: one instantiation per type |
| when a wrong type is caught | when the program is compiled |
| cost per call | none: each instantiation is ordinary compiled code |
| cost elsewhere | compile time, and long error messages |

> **Unlike Python:** duck typing gives the same "works for anything with `+`", but Python looks `+` up at
> run time on every call, and a wrong type fails only when that line runs. A template settles both before
> the program exists.

---

## 9.2 Lambdas: a function written where you need it

A **lambda** is an unnamed function written inside an expression:

```cpp
[capture list](parameters) { body }
```

- `(parameters)` and `{ body }` are what you expect. The body may be many statements.
- `[capture list]` names the variables *from the surrounding code* that the body may use.

The capture list is a decision you make for every variable the body uses. Each one is captured one of
two ways:

| Written | Name | The lambda gets |
|---|---|---|
| `[t]` | capture **by value** | its own copy of `t`, taken when the lambda is created |
| `[&t]` | capture **by reference** | the variable `t` itself (a reference, ch 3) |
| `[=]` | everything by value | copies of whatever the body uses |
| `[&]` | everything by reference | the originals of whatever the body uses |

`examples/09/capture.cpp`:

```cpp
    int t = 0;
    auto byValue = [t]() { return t; };       // copies t now: the copy holds 0
    auto byReference = [&t]() { return t; };  // refers to t itself
    t = 5;
    std::cout << "byValue() = " << byValue() << ", byReference() = " << byReference() << std::endl;

    int calls = 0;
    auto count = [&calls]() { ++calls; };     // by reference, so it can change the caller's variable
    count();
    count();
    std::cout << "calls = " << calls << std::endl;
```
```
byValue() = 0, byReference() = 5
calls = 2
```

`byValue` took a photocopy of `t` while `t` was 0; changing `t` later does not touch the copy.
`byReference` reads `t` itself, so it sees 5. `count` must capture `calls` by reference to change it.
§9.5 shows what happens when threads get this choice wrong.

> **Unlike Python:** closures there always look the variable up *when they are called* ("late binding"),
> which is capture by reference with no way to say otherwise in the syntax:
> `[lambda: i for i in range(3)]` all return 2 (`examples/09/capture.py` prints `[2, 2, 2]`). Only the
> `i=i` default-argument trick takes the value at creation. In C++ you write that choice in the brackets,
> every time.

### Every lambda has its own type

Each lambda expression creates an object of a brand-new class that the compiler names privately. You
cannot write that type's name. That is why `auto` holds lambdas above, and why a function that *takes* a
lambda needs a template parameter: there is no type name to put in the parameter list. Which is exactly
what `timeMs` does.

The [GLOSSARY](../learn-parallel/GLOSSARY.md) has short reminders for **Lambda**, **`template <typename F>`**
and **Lambda capture list**; this section is the full lesson behind them.

---

## 9.3 Timing a piece of code: `timeMs`

`examples/09/timing.h`, the whole file that matters:

```cpp
10  template <typename F>
11  double timeMs(F&& f, int reps) {
12      f();
13      double best = std::numeric_limits<double>::max();
14      for (int r = 0; r < reps; ++r) {
15          const auto start = std::chrono::steady_clock::now();
16          f();
17          const auto stop = std::chrono::steady_clock::now();
18          best = std::min(best, std::chrono::duration<double, std::milli>(stop - start).count());
19      }
20      return best;
21  }
```

| Line | C++ | What it does |
|---|---|---|
| 10 | `template <typename F>` | `F` is a blank: the type of whatever lambda is passed |
| 11 | `F&& f` | take `f` without copying it (§9.3.1) |
| 12 | `f();` | **warm-up**: run once, untimed |
| 13 | `std::numeric_limits<double>::max()` | the largest `double`; any real time beats it |
| 15, 17 | `std::chrono::steady_clock::now()` | read a clock that only ever moves forward |
| 18 | `duration<double, std::milli>(stop - start).count()` | the difference, as a plain `double` in milliseconds |
| 18 | `std::min(best, …)` | keep the fastest run |

Why warm up, and why the minimum instead of the average, is the subject of
[learn-parallel 01, "Measure first"](../learn-parallel/01_measure_first.md). It explains the C++ of this
exact helper line by line and then measures why each part is there. The short version: the first call
pays one-off costs, and interference can only ever make a run slower, so the fastest run is the best
estimate. Read lesson 01 for the evidence; this chapter only uses the tool.

Header-only: `timing.h` has no `timing.cpp`. A template is a recipe, not a finished function, so the
compiler needs its full text in every file that calls it, to instantiate it there. That is why templates
live in headers.

### 9.3.1 What `F&&` means (deep dive later)

In a template, `F&& f` is a **forwarding reference**: it can bind to anything without copying it. If you
pass a named lambda (`timeMs(runThreads, 5)`), `f` becomes a reference to `runThreads`. If you pass a
lambda written right there in the call (`timeMs([&]() { … }, 5)`), `f` refers to that temporary object,
which lives until the end of the call. Either way nothing is copied. `F&&` only behaves this way when `F`
is a template parameter being deduced. Outside a template, `T&&` means something narrower (a reference to
a temporary). For reading this repo, "`F&&` = hand me the thing itself" is enough.

---

## 9.4 A first thread

A **thread** is an independent line of execution inside your program. Each thread has its own stack
(ch 8) but shares the heap and every object with the other threads. The operating system runs threads on
different cores at the same time.

```cpp
#include <thread>

std::thread worker([]() { /* work */ });   // starts running immediately
worker.join();                             // wait here until it has finished
```

| C++ `std::thread` | What it does |
|---|---|
| `std::thread t(f);` | constructing the thread starts it running `f`; there is no separate start call |
| `t.join()` | wait until `t` has finished |
| a `std::thread` destroyed without `join()` | stops the whole program (`std::terminate`) |
| two threads on two cores | really compute at the same time; nothing makes them take turns |

> **Unlike Python:** in CPython only one thread runs Python code at a time (the **GIL**, global
> interpreter lock), so threading a CPU-bound loop does not speed it up, and forgetting `join()` is
> harmless. C++ has no such lock: threads truly run in parallel, which is where both the speedup (§9.6)
> and the data race (§9.8) come from.

`join()` is also a lifetime guarantee, which ties back to chapter 8. A thread that uses `lj` and
`geometry` by reference must finish before `lj` and `geometry` are destroyed. Joining before the end of
the scope that owns them makes sure of it.

Threads go in a `std::vector<std::thread>`. A `std::thread` cannot be copied (two objects would claim the
same running thread), only moved, just like `unique_ptr` in chapter 8. So instead of building a thread and
copying it in with `push_back`, we use `pool.emplace_back(lambda)`. `emplace_back` builds the new element
*inside* the vector, passing its arguments straight to the element's constructor. Here it constructs
`std::thread(lambda)` in place, which starts the thread.

---

## 9.5 The capture list decides correctness

Here is the threaded part of `mini_lj` (`examples/09/main.cpp:26-41`):

```cpp
26      std::vector<double> partial(numThreads, 0.0);
27      auto runThreads = [&]() {
28          std::vector<std::thread> pool;
29          for (int t = 0; t < numThreads; ++t) {
30              const std::size_t i0 = n * t / numThreads;
31              const std::size_t i1 = n * (t + 1) / numThreads;
32              // i0, i1, t are copied into the lambda; partial and lj are shared.
33              pool.emplace_back([&lj, &geometry, &partial, i0, i1, t]() {
34                  partial[t] = lj.calculateEnergyRange(geometry, i0, i1);
35              });
36          }
37          for (std::thread& th : pool) th.join();
38      };
39      const double threadedMs = timeMs(runThreads, 5);
40      double threadedEnergy = 0.0;
41      for (double p : partial) threadedEnergy += p;
```

Two lambdas, nested. `runThreads` (line 27) is the whole job: start the threads, wait for them. It
captures `[&]` because it runs right here, inside `main`, while everything it uses is alive. It is what we
hand to `timeMs`, so the timing includes starting and joining the threads.

The inner lambda (line 33) is the work of one thread. Its capture list is chosen variable by variable:

| Captured | How | Why |
|---|---|---|
| `&lj` | reference | big object; one copy is shared by all threads; they only read it |
| `&geometry` | reference | 4096 positions; copying them per thread would be pure waste; read only |
| `&partial` | reference | the threads must write into `main`'s vector, not into private copies of it |
| `i0`, `i1` | **value** | they are local to one loop iteration (line 30–31) and **cease to exist** at line 36's `}`; the thread may not even have started by then |
| `t` | **value** | the loop changes it on the next iteration; this thread must keep *its* `t` |

Line 34 is the other half of correctness: each thread writes **only `partial[t]`**, its own element. No
two threads ever write the same `double`. After the `join`s, line 41 adds the parts in a fixed order
(0, 1, …), so the total comes out the same every run. Adding in finishing order could change the last
digits, because floating-point addition is not associative
([GLOSSARY](../learn-parallel/GLOSSARY.md), "Floating-point is not associative").

### The same loop with `[&]`

`examples/09/capture_bug.cpp` is that loop with one change: the inner lambda captures `[&]`, so `i0`,
`i1` and `t` are captured by reference. It is broken on purpose:

```cpp
24          for (int t = 0; t < numThreads; ++t) {
25              const std::size_t i0 = n * t / numThreads;
26              const std::size_t i1 = n * (t + 1) / numThreads;
27              pool.emplace_back([&]() {                       // BUG: i0, i1 and t by reference
28                  partial[t] = lj.calculateEnergyRange(geometry, i0, i1);
29              });
30          }   // i0, i1 (and, after the loop, t) stop existing; the threads may still be reading them
```
```bash
c++ -std=c++17 -Wall -O2 capture_bug.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/capture_bug
./out/capture_bug; echo "exit=$?"
```

It compiles without a warning. Two launches, one after the other:

```
correct E = -20105.493949
run 0:   E = 0
run 1:   E = 0
run 2:   E = 0
exit=0
```
```
correct E = -20105.493949
run 0:   E = 0
run 1:   E = 0
libc++abi: terminating
exit=134
```

The energy comes out 0. In six launches, three aborted partway like the second one. Each thread reads `i0`,
`i1` and `t` *through references to variables whose lifetime has ended*: chapter 8's dangling problem,
now with threads. AddressSanitizer (ch 8) names the variable:

```bash
c++ -std=c++17 -g -O1 -fsanitize=address capture_bug.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/capture_bug_asan
./out/capture_bug_asan
```
```
correct E = -20105.493949
=================================================================
==70456==ERROR: AddressSanitizer: stack-use-after-scope on address 0x00016f776980 at pc 0x00010068af0c bp 0x00016f916ed0 sp 0x00016f916ec8
READ of size 8 at 0x00016f776980 thread T3
...
    [624, 628) 't' (line 24)
    [640, 648) 'i0' (line 25) <== Memory access at offset 640 is inside this variable
    [672, 680) 'i1' (line 26)
```

(Shortened.) Thread T3 read `i0` from line 25 after its scope ended. The fix is the capture list on line
33 of `main.cpp`: copy what belongs to one iteration, share what outlives the threads.

---

## 9.6 The mini_lj step: time it, then split it

What changed from chapter 8, in [`examples/09/`](examples/09/):

- **Dropped** `ForceCalculator` and the tether, and the printing constructor and destructor. We time one
  thing: the LJ energy loop. `lj` is back to being a plain stack object in `main` (ch 8: `main` owns it,
  the threads borrow it).
- **Added** `timing.h` (§9.3).
- **Added** `LJPotential::calculateEnergyRange(geometry, i0, i1)`: the energy of the pairs `(i, j > i)`
  for `i` in `[i0, i1)`. `calculateEnergy` is now the range `[0, N)` (`lj_potential.cpp:11-36`):

```cpp
11  double LJPotential::calculateEnergy(const Geometry& geometry) {
12      return calculateEnergyRange(geometry, 0, geometry.getNumberOfAtoms());
13  }
14
15  double LJPotential::calculateEnergyRange(const Geometry& geometry, std::size_t i0, std::size_t i1) {
    ...
21      for (std::size_t i = i0; i < i1; ++i) {
22          for (std::size_t j = i + 1; j < atoms.size(); ++j) {
```

  Only line 21 differs from chapter 8's loop: `i` runs over a slice instead of all atoms. The inner loop
  is unchanged, still `j > i`.
- **`main`** times the serial call with a lambda (`main.cpp:22-23`), then the threaded version (§9.5),
  then prints how many pairs each slice owns (`main.cpp:51-56`):

```cpp
22      double serialEnergy = 0.0;
23      const double serialMs = timeMs([&]() { serialEnergy = lj.calculateEnergy(geometry); }, 5);
```

The default is N = 4096 atoms (the size learn-parallel uses) and 2 threads; both can be given on the
command line.

```bash
c++ -std=c++17 -Wall -O2 main.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/mini_lj
./out/mini_lj
```
```
N = 4096, threads = 2
serial   E = -20105.493949   24.101416 ms
threaded E = -20105.493949   17.411542 ms
speedup  = 1.38x
thread 0: atoms [0, 2048)  pairs = 6290432
thread 1: atoms [2048, 4096)  pairs = 2096128
```

Same energy as chapter 4's N = 4096 run, both ways. Repeated runs gave speedups from 1.33× to 1.39×.
With 4 threads:

```bash
./out/mini_lj 4096 4
```
```
N = 4096, threads = 4
serial   E = -20105.493949   24.089375 ms
threaded E = -20105.493949   10.345666 ms
speedup  = 2.33x
thread 0: atoms [0, 1024)  pairs = 3669504
thread 1: atoms [1024, 2048)  pairs = 2620928
thread 2: atoms [2048, 3072)  pairs = 1572352
thread 3: atoms [3072, 4096)  pairs = 523776
```

And with the repo's default system size:

```bash
./out/mini_lj 125 2
```
```
N = 125, threads = 2
serial   E = -611.491665613   0.15225 ms
threaded E = -611.491665613   0.143917 ms
speedup  = 1.06x
thread 0: atoms [0, 62)  pairs = 5797
thread 1: atoms [62, 125)  pairs = 1953
```

At N = 125 the whole job takes about 0.15 ms, and starting and joining two threads costs a noticeable
part of that. Learn-parallel 01 shows that at this size threads can make things *slower*, and why every
exercise there uses N ≥ 4000.

---

## 9.7 Why 1.38× and not 2×

Two threads, equal numbers of atoms each, and still nowhere near twice as fast. The last lines of the
output say why. The loop is **triangular**: row `i` pairs atom `i` with every `j > i`, so row `i` has
`N − 1 − i` partners. Row 0 has 4095. Row 4095 has none.

```
row i:    0     1     2   ...  2047 | 2048  ...  4094  4095
pairs: 4095  4094  4093   ...  2048 | 2047  ...     1     0
       └──── thread 0: 6,290,432 ───┘└──── thread 1: 2,096,128 ──┘
```

Equal slices of `i` are very unequal slices of *work*. Thread 0 owns 75% of the 8,386,560 pairs. Thread 1
finishes early and waits at `join`. The threaded run cannot end before its biggest slice does.
`examples/09/slices.cpp` times each slice alone, on one thread:

```bash
c++ -std=c++17 -Wall -O2 slices.cpp geometry.cpp lattice.cpp lj_potential.cpp -o out/slices
./out/slices
```
```
all rows [0, 4096):     24.0917 ms
thread 0 [0, 2048):     17.13 ms
thread 1 [2048, 4096):  6.93983 ms
```

The threaded run took 17.41 ms. Thread 0's slice alone takes 17.13 ms. The two threads together are only
as fast as the slowest one. Counting pairs predicts 8,386,560 / 6,290,432 = 1.33×. The measured 1.38× is a
little better, because the short rows near the end cost slightly more per pair (6.94 ms for 2.10 M pairs,
against 17.13 ms for 6.29 M), so thread 0's share of the *time* is a bit under 75%. With 4 threads the
same thing happens: thread 0 has 3,669,504 pairs, seven times thread 3's 523,776, and 2.33× instead of 4×.

This has a name: **load imbalance**. The work is split evenly by count of items, but the items do not
cost the same. We stop here and do not fix it. Learn-parallel takes it up:
[03](../learn-parallel/03_scatter_vs_gather.md) explains why the repo's threaded kernels use a
different loop shape altogether, and [06](../learn-parallel/06_openmp.md) shows how the way iterations
are handed out to threads (its `schedule` section) changes the picture, including on this machine's
mix of fast and slow cores.

---

## 9.8 Energy only: why we did not split the forces

We threaded `calculateEnergy`, not `calculateForces`. Look at what the force loop writes
(`examples/09/lj_potential.cpp:44-56`):

```cpp
44      for (std::size_t i = 0; i < atoms.size(); ++i) {
45          for (std::size_t j = i + 1; j < atoms.size(); ++j) {
    ...
56              calculatePairForce(epsilon, sigma, r, direction, forces[i], forces[j]);
```

Each pair adds to **both** `forces[i]` and `forces[j]` (Newton's third law: equal and opposite). Split `i` into slices
and the `j` side is not in anyone's slice: thread 0, working on `i = 10`, writes `forces[3000]`, which is
an atom in thread 1's slice, and thread 1 also writes `forces[3000]` for its own pairs. Two threads
writing the same memory at the same time, with nothing ordering them, is a **data race**. The energy loop
had no such problem only because each thread kept its own running total and wrote its own `partial[t]`.

What a data race does to the numbers, and the ways out of it, are the core of learn-parallel:
[03 (scatter vs gather)](../learn-parallel/03_scatter_vs_gather.md) and
[05 (the race)](../learn-parallel/05_the_race.md). This chapter only names it.

---

## 9.9 In the real code

**The `std::thread` backend** — `src/engine/potentials/lj_kernels.cpp:302-333`. Put it next to §9.5:

```cpp
302          case LJBackend::Threads: {
303              // Hand-rolled partitioning: exactly what OpenMP's schedule(static)
304              // does for you, written out so you can see it (lesson 4).
305              const std::size_t n = v.n;
306              const int T = numThreads;
307              std::vector<double> partial(static_cast<std::size_t>(T), 0.0);
308              std::vector<std::thread> pool;
309              pool.reserve(static_cast<std::size_t>(T));
310
311              for (int t = 0; t < T; ++t) {
312                  // Balanced split: the first (n % T) chunks get one extra atom,
313                  // so no thread is ever more than one atom behind another.
314                  const std::size_t chunk = n / static_cast<std::size_t>(T);
315                  const std::size_t rem = n % static_cast<std::size_t>(T);
316                  const std::size_t ut = static_cast<std::size_t>(t);
317                  const std::size_t i0 = ut * chunk + std::min(ut, rem);
318                  const std::size_t i1 = i0 + chunk + (ut < rem ? 1u : 0u);
319
320                  // Each thread writes partial[t] -- its own slot -- so the
321                  // energy reduction needs no synchronisation either.
322                  pool.emplace_back([&v, i0, i1, t, &partial]() {
323                      partial[static_cast<std::size_t>(t)] = computeRange(v, i0, i1);
324                  });
325              }
326              for (auto& th : pool) th.join();
327
328              // Summed in a fixed thread order, so the result is deterministic
329              // run to run. Floating-point addition is not associative; summing
330              // in completion order would make the energy jitter in the last bits.
331              for (double p : partial) doubleCounted += p;
332              break;
333          }
```

| `mini_lj` (`main.cpp`) | repo (`lj_kernels.cpp`) | Note |
|---|---|---|
| `std::vector<double> partial(numThreads, 0.0);` (26) | `partial(static_cast<std::size_t>(T), 0.0)` (307) | `static_cast` converts `int` to `std::size_t` explicitly |
| `std::vector<std::thread> pool;` (28) | same, plus `pool.reserve(T)` (308–309) | `reserve` sets the capacity up front, so the vector never has to move to a bigger block while threads are added (ch 8 §8.8) |
| `i0 = n * t / numThreads` (30–31) | `chunk`, `rem`, `std::min` (314–318) | a different formula for the slice edges; learn-parallel 04 explains it |
| `[&lj, &geometry, &partial, i0, i1, t]` (33) | `[&v, i0, i1, t, &partial]` (322) | the same choice: the shared data by reference, the per-thread numbers by value |
| `partial[t] = lj.calculateEnergyRange(...)` (34) | `partial[t] = computeRange(v, i0, i1)` (323) | each thread writes its own slot |
| `th.join()` (37) | `th.join()` (326) | |
| `for (double p : partial) …` (41) | same, with the comment about fixed order (328–331) | |

`v` is the `LJKernelView` from chapter 8 §8.8: plain pointers borrowed from `LJPotential`'s vectors, alive
for the whole call. The capture list shares the view itself by reference, so every thread follows the
same pointers.

One real difference: `computeRange`'s inner loop runs over *all* `j`, skipping only `j == i`
(`lj_kernels.cpp:58-59`), not `j > i`:

```cpp
58          for (std::size_t j = 0; j < n; ++j) {
59              if (j == i) continue;
```

Why the repo pays for that, and what it buys, is learn-parallel 03. For now, notice that §9.7's
triangle is gone from this loop.

**The timing helper** — `benchmarks/bench_lj.cpp:56-68` is `timing.h` with two small differences:

```cpp
56  template <typename F>
57  double timeMs(F&& f, int reps) {
58      f();  // warm-up: the first call pays page faults, cold caches and thread
59            // creation. Timing it would measure startup, not the kernel.
60      double best = 1e300;
61      for (int r = 0; r < reps; ++r) {
62          const auto t0 = Clock::now();
63          f();
64          const auto t1 = Clock::now();
65          best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
66      }
67      return best;
68  }
```

It starts from `1e300` instead of `numeric_limits<double>::max()`, and `Clock` is
`std::chrono::high_resolution_clock` (`bench_lj.cpp:26`) instead of `steady_clock`. And the lambdas that
use it (`bench_lj.cpp:107`, `:128`) are §9.2's `[&]` form:

```cpp
107      const double refMs = timeMs([&] { lj.calculateForcesReference(geometry, refForces); }, reps);
```

(`[&] { … }` with no `()` is allowed when the lambda takes no parameters.)

---

## Next

`mini_lj` has now used every C++ idea that the repo's LJ path is built from: classes, a
`Potential` interface, shared ownership, a timer and threads. But it is a teaching program in
`docs/learn-cpp/examples/`. How much of it *is* the repo? Chapter 10 lays `mini_lj` over the real source
file by file, builds and runs the real program, and checks that it computes the same energy.
