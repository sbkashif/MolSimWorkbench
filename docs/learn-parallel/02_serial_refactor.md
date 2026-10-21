# Lesson 2: The Serial Refactor — and Why It Is Also the Port

> **Where we are.** Lesson 1 gave you a harness you can trust: warm-up, minimum of several runs, correctness printed beside speed. Now use it on lesson 0's suspects, one at a time, still on a single core.

## Learning Objectives

- Find the real hot path by ablation instead of by intuition
- Explain why a cutoff loop has two populations of work with a 74:1 size ratio
- Recognise that division and square root are not "just arithmetic"
- Judge when a change that *costs* CPU time is still the right call

## A hypothesis, and its demolition

Reading the original loop, the obvious culprit is this:

```cpp
auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
```

Two `std::map<std::string, ...>` lookups plus a `sqrt`, to recompute a constant.
That has to be the problem. It is the kind of thing that gets flagged in code
review, and I was confident about it.

It is worth 10%.

Here is the ablation — one change at a time, serial and triangular throughout,
N=4096:

| Variant | ms | step gain |
|---|---:|---:|
| V0 original (`map`+`string`, `pow`, `sqrt`, AoS) | 32.07 | — |
| V1 + integer type ids, flat parameter table | 29.20 | **1.10×** |
| V1b + `std::pow(x,6)` → multiply chain | 29.04 | **1.01×** |
| V2 + drop `sqrt`, work in r² | 11.56 | **2.51×** |
| V3 + structure-of-arrays | 13.95 | **0.83×** |

Two of my four predictions were wrong:

- **The `std::map` lookup barely matters** (1.10×) — and the reason is worth
  more than the measurement. Look at where the call actually sits:

  ```cpp
  if (distance > cutoff || distance < 1e-10) continue;      // 98.6% leave here
  auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
  ```

  It is **after** the cutoff test. So it does not run 8.4 million times — it runs
  113,910 times, on the 1.36% of pairs that survive. I had assumed it was in the
  hot path; I had misread which side of the `continue` it was on.

  The arithmetic confirms it exactly. Removing it saved 2.87 ms across 113,910
  calls = **25 ns per pair**, which is a perfectly reasonable price for two tree
  lookups and a `sqrt`. Had it really run on all 8,386,560 pairs at 25 ns, it
  would have cost **211 ms** — dwarfing everything else, precisely as predicted.

  **The estimate of the cost was right. The estimate of the frequency was wrong
  by a factor of 74.** That is the more common way performance intuition fails:
  not mispricing an operation, but misjudging how often it runs.

### The single-type measurement is the map's best case (deep dive later)

  1.10× is not a general result about `std::map`. This repo's example has **one
  atom type** — a one-node tree and a two-character key. That is the friendliest
  input the data structure will ever see, and the engine is built for arbitrary
  mixtures.

  Holding σ fixed so the set of pairs inside the cutoff is identical at every
  type count, and varying only the number of types:

  | types | map + text | flat table | map penalty | ns per lookup |
  |---:|---:|---:|---:|---:|
  | 1 | 32.28 ms | 29.37 ms | 1.10× | 25.6 |
  | 2 | 32.82 | 29.33 | 1.12× | 30.6 |
  | 5 | 34.68 | 29.34 | 1.18× | 46.9 |
  | 10 | 36.88 | 29.41 | 1.25× | 65.6 |
  | 20 | 38.25 | 29.36 | 1.30× | 78.1 |
  | 50 | 41.45 | 29.41 | 1.41× | 105.6 |
  | 100 | 44.09 | 29.84 | **1.48×** | **125.2** |

  **The flat table does not move** — 29.37 to 29.84 ms across a hundredfold
  change in type count, a 1.6% drift. One integer index costs the same whatever
  `T` is. The map climbs by 5× per lookup: deeper tree, longer keys, more
  pointer chasing to scattered nodes.

  And the effect compounds with the other fixes. At 100 types the lookup adds
  14.25 ms of overhead. Once the r² rewrite has brought the rest of the kernel
  down to 11.6 ms, that overhead would be **more than half the total runtime** —
  against 9% here.

  So the honest statement is: the lookup is a minor cost *in this benchmark*, a
  major one in the mixtures the engine is designed for, and a hard blocker for
  the GPU in every case. Reproduce it yourself:

  ```bash
  ./build/benchmarks/bench_ablation 4096
  ```
- **`std::pow(x, 6)` is free** (1.01×). Clang recognises a constant integer
  exponent and emits the multiply chain itself. Hand-optimising it changed
  nothing. The compiler had already done it.
- **Structure-of-arrays made it *slower*** (0.83×). More on that below.

This is the lesson-1 discipline paying for itself immediately. The fix I was
sure about was worth 10%; the fix I had not thought about was worth 2.5×.

## The real structure: two populations of work

The cutoff test splits the inner loop into two completely different jobs:

```
N = 4096
pairs evaluated:        8,386,560
pairs inside cutoff:      113,910   (1.36%)
```

**Everything before `if (r2 > cutoff2) continue;` runs 8.4 million times.**
Everything after it runs 114 thousand times — 74× less often. An instruction
saved before the cutoff test is worth 74 instructions saved after it.

So: what is in the pre-cutoff path in the original code?

```cpp
if (boxDim[0] > 0.0) dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
if (boxDim[1] > 0.0) dy = dy - boxDim[1] * std::round(dy / boxDim[1]);
if (boxDim[2] > 0.0) dz = dz - boxDim[2] * std::round(dz / boxDim[2]);
double distance = std::sqrt(distanceSquared);
```

**Three floating-point divisions, three branches, and a square root — for every
one of the 8.4 million pairs, 98.6% of which are about to be discarded.**

Splitting the r² rewrite into its pre- and post-cutoff halves confirms it:

| Variant | ms | gain |
|---|---:|---:|
| original | 28.08 | — |
| **fix only the wrapping** (before the cutoff test) — divisions → multiplies, branches gone | 15.20 | **1.85×** |
| fix only the `sqrt` (after the cutoff test) — no square root, no normalisation | 22.13 | 1.27× |
| fix both | 11.46 | 2.45× |

*(Both halves are suspect **(C)** from lesson 0. Splitting it in two shows that
the part running before the cutoff test is worth more than the part running
after it — which is the whole point.)*

The minimum-image convention was the hot spot all along.

### First: nothing is being skipped

The obvious objection, and it is the right instinct: *you have to compute the
periodic distance to decide whether a pair is inside the cutoff. That work is
necessary. How can it be waste?*

It is necessary. And none of it is removed. **The same work is done with cheaper
instructions.**

**The wrapping.** You need the minimum-image displacement. You do not need a
division to get it:

```cpp
// before — a DIVISION, 8,386,560 times
dx -= bx * std::round(dx / bx);

// after — the reciprocal taken ONCE, outside the loop
const double invbx = 1.0 / bx;         // one division, ever
dx -= bx * std::round(dx * invbx);     // a MULTIPLY, 8,386,560 times
```

`dx / bx` and `dx * (1/bx)` are the same quantity. Every pair still gets wrapped,
`round` still runs, the displacement is identical. Only the instruction changed.

**The square root.** This one was never needed at all. You want to know "is this
pair closer than 2.5?" — and for non-negative numbers,

$$r < r_c \iff r^2 < r_c^2$$

Comparing squares gives exactly the same yes/no on every pair. Square `cutoff`
once, before the loop, and test `r2 > cutoff2`.

It goes further: the physics does not need `r` either. The energy can be written
`(σ²/r²)³` instead of `(σ/r)⁶`, and the force as a scalar times the *raw*
displacement vector — which removes the three divisions that were normalising it:

$$\vec{F} = \frac{F}{r}\cdot\vec{r}, \qquad \frac{F}{r} = \frac{24\varepsilon}{r^{2}}\left[2\left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^{6}\right]$$

So the square root disappears from the cutoff test *and* from the force. It was
pure overhead in both places.

**And the proof it is the same physics:** if corners had been cut, the numbers
would move. They do not.

```
reference          -20105.493949019
serial-triangular  -20105.493949019      max relative force difference: 1.5e-11
```

Nine matching decimal places. The 1.5e-11 residue is rounding from doing the
same arithmetic in a different order, not a missing term.

**No physics removed. No pair skipped. Identical answers. Cheaper instructions.**

### Why division is special (deep dive later)

On modern hardware a `double` multiply has ~4 cycles latency and is *fully
pipelined* — you can start a new one every cycle. Division has ~13–20 cycles
latency and is **not** pipelined; the divider is a single unit that blocks.
`sqrt` is similar. Ten multiplies can be cheaper than one divide.

The fix is to divide once, outside the loop, and multiply inside it:

```cpp
const double invbx = (v.bx > 0.0) ? 1.0 / v.bx : 0.0;   // hoisted: one division, once
...
dx -= v.bx * std::round(dx * invbx);                     // multiply, 8.4M times
```

That also deletes the branch. Note the trick: when `bx == 0` (no periodicity)
`invbx` is 0, so `round(dx*0) == 0` and `dx` is unchanged. The zero case falls
out of the arithmetic instead of needing an `if`. A branch that is *always*
taken the same way is nearly free — the predictor gets it right every time — but
removing it also removes a dependency that was blocking the vectorizer.

Similarly, the force never needs a normalised direction vector:

```cpp
// Original: normalise (3 divisions), then scale by |F|
direction = {dx/d, dy/d, dz/d};
force = forceMagnitude * direction;

// Rewrite: fold 1/r into the magnitude, multiply by the raw displacement
const double fOverR = 6.0 * eps4 * (2.0*sr12 - sr6) / r2;
fxi -= fOverR * dx;
```

`F⃗ = (F/r)·r⃗` is the same vector, computed with three fewer divisions and no
square root at all.

## The uncomfortable one: splitting the coordinates made it slower

### First, what the two layouts are

Every atom has an x, a y and a z. There are two ways to keep them.

**Array of Structures (AoS)** — what the repo started with. Each atom's three
numbers live together:

```
[x0 y0 z0][x1 y1 z1][x2 y2 z2]
  atom 0     atom 1     atom 2
```

**Structure of Arrays (SoA)** — three separate lists:

```
x: [x0 x1 x2]
y: [y0 y1 y2]
z: [z0 z1 z2]
```

Think of it as filing. **AoS is one folder per atom**, each folder holding three
sheets. **SoA is three filing cabinets** — every atom's x sheet in the first,
every y in the second, every z in the third.

Now ask two different questions:

| Question | AoS | SoA |
|---|---|---|
| "everything about atom 7" | open one folder | walk to three cabinets |
| "the x of atoms 0–31" | open 32 folders, take one sheet from each | one cabinet, 32 sheets in a row |

**Neither layout is better in the abstract.** It depends entirely on which
question you ask.

### Why the arrangement matters at all

Memory does not arrive one number at a time. It arrives in **64-byte chunks**
(a *cache line*). Ask for one number and its neighbours come along for free.
Three doubles is 24 bytes — so in AoS, an atom's x, y and z all land in a single
chunk.

Our inner loop asks the **first** question: it needs atom j's x, y *and* z to
work out a distance. AoS hands over all three in one fetch. SoA sends you to
three separate places.

### The measurement

Converting to SoA cost **13%**, consistently:

| | AoS | SoA | |
|---|---:|---:|---:|
| gather, N=4096 | 22.62 ms | 26.09 ms | 0.87× |
| gather, N=15625 | 315.78 ms | 362.46 ms | 0.87× |

Exactly as the filing-cabinet picture predicts: one folder beats three cabinets
when you want the whole folder.

SoA wins when you ask the *other* question — one coordinate across many atoms.
That is what a graphics card does: 32 workers move in lockstep, and each wants a
*different* atom's x at the same instant. SoA lays those 32 numbers in a row so
the hardware collects them in one go; AoS scatters them 24 bytes apart, so it
fetches 32 chunks and discards two-thirds of each. Same data, opposite verdict.

**The shipped kernel uses SoA anyway, and gives up 13%.** That is a deliberate
trade: the entire point of `LJKernelView` is to be a layout a CUDA kernel can
consume directly, and switching layouts later would mean rewriting every backend
and every test. Paying 13% now to avoid a rewrite later is a defensible call —
but it is a *call*, and you should know you are making it rather than absorb
"SoA is faster" as folklore. If this repo were never going to see a GPU, AoS
would be the right answer.

## What all of this has to do with CUDA

Strip the changes back to their essentials and the list is:

| Change | CPU benefit | Why a GPU *requires* it |
|---|---|---|
| `std::string` → `int` type ids | 1.10× | no `std::string` in a kernel |
| `std::map` → flat indexed table | (same) | no `std::map`; needs a raw pointer |
| `sqrt` + divisions → r² arithmetic | 2.45× | transcendentals are scarce on a GPU too |
| AoS → SoA | **0.87×** | coalesced global-memory access |
| `vector<array<double,3>>` → `double*` | — | device pointers only |

Three of the five help on CPU, one hurts, and every one of them is mandatory
for the port. That is the argument for doing the serial refactor first: it is
not preparation for the GPU work, it *is* the GPU work, done on hardware where
you can debug it with a print statement.

## Common Pitfalls

- **Optimising the cold path.** Everything after the cutoff test runs 74× less
  often. Beautiful improvements there are worth almost nothing.
- **Assuming the compiler is stupid.** It already fixed `std::pow`. Check the
  generated code, or measure, before hand-optimising arithmetic.
- **Assuming the compiler is smart.** It did *not* hoist the reciprocal box
  lengths, because `1.0/bx` and `dx/bx` are not equivalent under IEEE rounding
  and it is not allowed to make that substitution without `-ffast-math`.
- **Trusting "SoA is faster".** It depends entirely on the access pattern.

## Summary

- Ablate one change at a time; intuition about hot spots is unreliable.
- A cutoff loop has a hot pre-test path and a cold post-test path, here in a
  74:1 ratio. Optimise the pre-test path.
- Division and `sqrt` are not ordinary arithmetic — hoist reciprocals.
- SoA costs 13% on this CPU and is kept for GPU-portability reasons, knowingly.
- The refactor that makes the CPU faster is the same one that makes the kernel
  portable to a GPU at all.

## Further Reading

- [`benchmarks/bench_ablation.cpp`](../../benchmarks/bench_ablation.cpp) — reproduces both tables in this lesson
- [`src/engine/potentials/lj_kernels.cpp`](../../src/engine/potentials/lj_kernels.cpp) — the result
- Exercise: `exercises/02-serial-refactor/`

**Next:** [03_scatter_vs_gather.md](03_scatter_vs_gather.md) — three suspects are dealt with; the fourth is the two-sided write `f[i]`, `f[j]`. Lesson 3 answers: how do you reshape the loop so several workers can run it without colliding, and what does that cost?
