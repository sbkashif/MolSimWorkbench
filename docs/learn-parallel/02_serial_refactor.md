# Lesson 2: The Serial Refactor — and Why It Is Also the Port

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

  (It is still a latent problem. With 30 atom types, as a protein force field
  would have, this line gets slower — and it remains a hard blocker for the GPU
  regardless of cost.)
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
| A original | 28.08 | — |
| B **pre-cutoff fix only** (divisions → multiplies, branches gone) | 15.20 | **1.85×** |
| C post-cutoff fix only (no `sqrt`, no normalisation) | 22.13 | 1.27× |
| D both | 11.46 | 2.45× |

The minimum-image convention was the hot spot all along.

### Why division is special

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

## The uncomfortable one: SoA is slower here

Converting to structure-of-arrays — separate `x[]`, `y[]`, `z[]` instead of
`std::vector<std::array<double,3>>` — cost **13%**, consistently:

| | AoS | SoA | |
|---|---:|---:|---:|
| gather, N=4096 | 22.62 ms | 26.09 ms | 0.87× |
| gather, N=15625 | 315.78 ms | 362.46 ms | 0.87× |

The reason is straightforward once you look at the access pattern. The inner
loop needs **all three components of atom j**. In AoS those are three adjacent
doubles — one cache line, one load stream. In SoA they live in three separate
arrays, so you touch three cache lines and run three streams. AoS wins.

SoA pays off when you need *one component across many atoms* (SIMD across the
j loop), or on a GPU, where consecutive threads in a warp read consecutive `j`
and SoA makes those loads coalesce into a single transaction. Neither applies
to a scalar CPU loop that wants x, y and z together.

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

- [`src/engine/potentials/lj_kernels.cpp`](../../src/engine/potentials/lj_kernels.cpp) — the result
- Exercise: `exercises/02-serial-refactor/`

**Next:** [03_scatter_vs_gather.md](03_scatter_vs_gather.md)
