# Glossary

Plain-English definitions of every specialist term used in this course, with
why it matters here. Skim it once now; come back when a lesson uses a word you
do not recognise.

---

## The two ways to write the loop

**Scatter**
One calculation, written to two places. The original loop computes the push
between atoms 10 and 999 once, then updates *both* atoms. Efficient, because
you never compute the same push twice.

The problem: when several workers run at once, two of them can try to update
the same atom at the same moment, and one update gets lost. See **data race**.

**Gather**
Each worker owns certain atoms and computes everything about them itself.
Nothing is ever written by more than one worker, so nothing can collide.

The cost: every pair gets computed twice — once from each atom's point of view.
Twice the arithmetic, but no collisions are *possible*, rather than merely
unlikely.

**Triangular loop**
The `for j = i+1` shape: it only looks at each pair once, so it visits the
"upper triangle" of the grid of all pairs. This is the scatter version.

---

## Things that go wrong in parallel

**Thread**
One worker. Eight threads means eight pieces of your program running at the
same time on different cores.

**Data race**
Two workers reading and writing the same value at the same time, so one of the
updates is silently lost. The program does not crash — it just quietly returns
a slightly wrong answer. In this course, 2.1% of atoms came out wrong while the
total energy still looked perfectly reasonable. Lesson 5.

**Atomic**
A "nobody interrupt me" instruction. Makes read-modify-write happen as a single
indivisible step, so no update can be lost. Correct, but workers end up queuing
for the same values.

**Deterministic / reproducible**
Running the same program twice gives *exactly* the same answer, down to the last
digit. Not automatic in parallel code, and not the same thing as correct — code
can be race-free and still give slightly different last digits each run,
because the order in which partial results get added up changes. See
**floating-point is not associative**.

**Barrier**
A point where every worker waits until all the others arrive. Everyone runs at
the speed of the slowest worker. This is why one slow core can hold up seven
fast ones.

**Reduction**
Combining per-worker partial results into one total — usually adding up eight
partial energies into one number. Easy on a CPU, and the genuinely hard part of
the equivalent GPU code.

**False sharing**
Two workers writing to different values that happen to sit next to each other in
memory. The hardware moves memory in 64-byte chunks, so it treats the two as
"shared" and does a lot of pointless bookkeeping. Fixed by having each worker
add things up in a local variable and write out once at the end.

---

## Memory and layout

**Cache line**
The chunk size memory moves in — 64 bytes, or 8 doubles. Asking for one number
always fetches its whole 64-byte neighbourhood. This is why *where* data sits
matters as much as how much of it there is.

**Array of Structures (AoS)** — one folder per atom
Each atom's x, y and z are kept together:

```
[x0 y0 z0][x1 y1 z1][x2 y2 z2]
```

Like a filing cabinet with one folder per atom, each holding three sheets. This
is what the repo started with (`std::vector<std::array<double,3>>`).

**Structure of Arrays (SoA)** — one cabinet per coordinate
Three separate lists instead:

```
x: [x0 x1 x2]     y: [y0 y1 y2]     z: [z0 z1 z2]
```

Like three filing cabinets — everyone's x sheets in one, everyone's y in
another.

**Which is better depends on the question you ask.** "Everything about atom 7"
is one folder (AoS wins). "The x of atoms 0–31" is one cabinet (SoA wins).

Our loop needs atom j's x, y *and* z to measure a distance — the folder
question — so AoS wins, and SoA measured **13% slower**. A graphics card asks the
other question: 32 workers each want a different atom's x at the same moment, so
SoA wins there. The repo pays the 13% to keep that door open. Lesson 2.

**Red-black tree**
The structure behind `std::map`. To look something up, you start at the top and
compare your key against one entry at a time, following a chain of pointers down
through separately-stored nodes until you find it. Each hop can be a slow trip
to memory. Contrast with an array index, which goes straight there.

**Page fault**
The first time you touch newly allocated memory, the operating system has to
actually hand it over, which is slow. This is one reason the first run of
anything is unrepresentative, and why benchmarks do an untimed warm-up run.

---

## Measuring

**Speedup**
Old time ÷ new time. Meaningless without saying what "old" is — see **baseline**.

**Baseline**
The thing you divide by. The honest choice is *the fastest correct
single-worker version you know how to write*. Divide by the original unimproved
code instead and you quietly take credit for serial improvements: 9.24× instead
of 4.03× in this repo. Lesson 7.

**Parallel efficiency**
Speedup ÷ number of workers. Eight workers giving 4× is 50% efficient. Tells you
how much of each extra core you are actually using.

**Amdahl's law**
If some fraction of your program can't be split up, that fraction sets a hard
ceiling on speedup no matter how many cores you add. If 2% is stuck being
serial, you can never go faster than 50×.

**Strong scaling**
Same problem, more cores. "Can I get my answer sooner?" Gets harder as you add
cores, because there is less work each.

**Weak scaling**
Bigger problem *and* more cores, in proportion. "Can I do a bigger job in the
same time?" Usually looks much better, and answers a different question. Saying
which one you measured is part of being honest.

**Thermal throttling**
Your laptop gets hot and slows itself down. Makes repeated benchmark runs get
worse over time, which is why this course takes the *fastest* of several runs
rather than the average — interference only ever makes things slower.

**Floating-point is not associative**
`(a + b) + c` and `a + (b + c)` can differ in the last digit or two, because
each step rounds. So adding the same numbers in a different order gives a
slightly different total. This is why parallel code that adds partial results
in whatever order workers finish is not reproducible run to run.

**ULP**
"Unit in the last place" — the gap between one representable decimal number and
the next. A way of saying how big a rounding difference is.

---

## OpenMP

**OpenMP**
A way to parallelize a loop by adding one comment-like line above it, instead of
managing workers yourself. The compiler generates the worker management.

**Pragma**
The `#pragma omp parallel for` line itself — an instruction to the compiler.

**Schedule**
How iterations are handed out to workers.
- `static` — split into equal blocks up front. No overhead, but assumes every
  worker is equally fast.
- `static, 16` — deal out small blocks in rotation, decided up front. Balanced
  *and* repeatable.
- `dynamic` — workers grab more work when they finish. Handles uneven work, but
  the assignment changes every run, so results stop being reproducible.
- `guided` — like dynamic, but hands out big blocks first and smaller ones later.

**shared / private**
`shared` — one copy that all workers see. `private` — each worker gets its own.
Anything declared inside the loop is automatically private, which is why the
kernel's running totals are safe.

**Performance cores / efficiency cores (P-cores / E-cores)**
Your Mac has 4 fast cores and 4 slow, power-saving ones. The computer reports
"8 cores" and does not mention that half of them are slower. With `static`
scheduling this made a 5-worker run *slower* than a 4-worker run. Lesson 7.

---

## Physics in this repo

**Cutoff**
Distance beyond which the code stops calculating interactions, because they are
tiny. Here 2.5σ. Makes the problem tractable — and means 98.6% of the pairs the
loop looks at get thrown away.

**Periodic boundary conditions (PBC)**
The box wraps around: leave the right side, come back on the left. Simulates
bulk material with a small number of atoms.

**Minimum image convention**
Given wrapping, use the *nearest* copy of the other atom. The three lines doing
this turned out to be the real bottleneck, because they run for every pair
before the cutoff test throws most of them away. Lesson 2.

**Reduced units**
ε and σ both set to 1, so numbers stay near 1 and are easy to sanity-check.

**Dispersion tail correction**
An estimate of everything you ignored beyond the cutoff, worked out with a
formula instead of by looking at atoms. Worth 8.73% of the energy here.
Contributes exactly zero force, so it never enters the main loop. Lesson 8.

**Neighbour list / cell list**
Keeping track of which atoms are actually near each other, so you only check
those. Would remove ~98.6% of the work — more than parallelism gains. This repo
deliberately does not have one (see `CLAUDE.md`); lesson 9 explains why the
all-pairs version is still the right thing to learn on.

**PME (Particle Mesh Ewald)**
How real MD codes handle long-range electrical forces. Not needed here — these
atoms have no charge.

---

## Complexity

**O(N²)**
Double the atoms, quadruple the time. The pair loop, because every atom is
checked against every other.

**O(N)**
Double the atoms, double the time.

**O(1)**
Same cost regardless of size. The tail correction. Never worth parallelizing —
starting up workers costs more than the work itself.

---

## Graphics cards (lesson 9 only — nothing to install)

**Kernel**
A function that runs on the graphics card, one copy per worker.

**Thread / block / grid**
A graphics card runs thousands of tiny workers. They are organised into blocks,
and blocks into a grid.

**Warp**
32 workers that move in lockstep, executing the same instruction together.

**Coalescing**
When those 32 workers ask for 32 values sitting next to each other in memory,
the hardware fetches them in one go instead of 32. This is the reason for SoA,
and why it is worth accepting a 13% loss on the CPU.

**Shared memory**
Small, fast scratch space that all workers in one block can see. Used to avoid
fetching the same data over and over.

**Resident**
Keeping data on the graphics card between steps instead of copying it back and
forth every time. Copying is roughly 100× slower than the card's own memory, so
this is usually the difference between a fast port and a pointless one.

**POD ("plain old data")**
A structure containing only simple things — numbers and addresses, no
`std::string`, no `std::map`, nothing that manages its own memory. A graphics
card can only accept this kind of data, which is why `LJKernelView` looks the
way it does.
