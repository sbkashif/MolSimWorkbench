# Lesson 3: Scatter vs Gather

> ### In plain English
>
> The original loop computes the push between two atoms **once** and applies it
> to both of them. That is the smart way to do it with one worker — you never
> compute the same push twice.
>
> But it means a worker has to reach over and update an atom that some *other*
> worker might be updating at the same moment. When that happens, one of the two
> updates is silently thrown away.
>
> So we rewrite it: each worker owns certain atoms and works out everything
> about them by itself. Nobody ever touches an atom they do not own, so nothing
> can collide. The price is that every push now gets computed twice, once from
> each atom's side.
>
> **That rewrite makes the code slower for one worker (15.2 ms → 28.3 ms) and
> is the only reason eight workers are possible at all.**
>
> New words here: **scatter**, **gather**, **triangular loop** —
> see [GLOSSARY.md](GLOSSARY.md).

> **Where we are.** Lesson 2 made the loop 2.29× faster on one core without changing what it computes. The loop still writes to two atoms per pair, which is the fourth suspect from lesson 0, and it is what stands between us and threads.

## Learning Objectives

- Distinguish a *scatter* from a *gather* and say why it decides everything
- Explain why Newton's third law is a serial optimization and a parallel liability
- Price the gather formulation honestly (it costs 1.76× before threading begins)
- Recognise the gather loop as the shape a GPU kernel has to take

## Background

This is the one genuinely architectural decision in the course. Lesson 2 was
local rewriting — same loop, faster instructions. This lesson changes the shape
of the computation, and it costs performance to do it.

Everything after this point follows from the choice made here.

## Core Concepts

### The original loop is a scatter

```cpp
for (i = 0; i < n; ++i)
    for (j = i+1; j < n; ++j) {
        ...
        f[i] -= fOverR * dx;    // i: fixed across the whole inner loop
        f[j] += fOverR * dx;    // j: sweeps the entire array
    }
```

Each pair is visited once and its result is *scattered* to two destinations.
That is Newton's third law doing real work: F⃗ᵢⱼ = −F⃗ⱼᵢ, so computing the pair
once and applying it twice halves the arithmetic.

For serial code this is unambiguously correct, and it is why
`LJBackend::SerialTriangular` is the fastest single-threaded kernel here:

```
serial-triangular   16.14 ms
serial-gather       28.39 ms      1.76× more expensive
```

### And that is exactly what makes it unthreadable

Give the outer loop to several threads and consider `f[j]`.

Thread A is working on `i=10`. Thread B is working on `i=20`. Both inner loops
eventually reach `j=999`. Both execute:

```cpp
f[999] += something;
```

which is not one instruction. It is three:

```
    load    r ← f[999]
    add     r ← r + something
    store   f[999] ← r
```

Interleave two threads across those three steps and you get:

```
    Thread A: load f[999]  → 5.0
    Thread B: load f[999]  → 5.0          (A has not stored yet)
    Thread A: add  → 5.0 + 1.0 = 6.0
    Thread B: add  → 5.0 + 2.0 = 7.0
    Thread A: store 6.0
    Thread B: store 7.0                    ← A's contribution is gone
```

The correct answer was 8.0. You got 7.0. **Nothing crashed, nothing warned, and
the result looks entirely plausible.** This is a data race, and in C++ it is
undefined behaviour, not merely a wrong number.

`i` is safe — each thread owns its own `i` — so exactly half of each update is
correct. That is what makes the bug so slippery: the forces are *mostly* right.
Lesson 5 turns this on deliberately.

### Gather: change who does the writing

Rather than defending the write, remove the conflict. Let atom `i` loop over
**all** j and accumulate only into its own slot:

```cpp
for (i = 0; i < n; ++i) {
    double fxi = 0, fyi = 0, fzi = 0;      // registers, private by construction
    for (j = 0; j < n; ++j) {
        if (j == i) continue;
        ...
        fxi -= fOverR * dx;                 // only i is touched
    }
    f[i] = fxi;                             // one write, one owner
}
```

Now every pair is computed **twice**, once as (i,j) and once as (j,i). Twice the
arithmetic. In exchange:

- `f[i]` is written by exactly one thread — no race is *possible*
- no atomics, no locks
- no per-thread scratch buffers and no reduction pass
- the accumulator lives in a register, not memory

The parallelization strategy becomes trivial because the data dependency was
designed out, rather than guarded against.

### Why this is the GPU shape

Take the gather loop and delete the outer `for`:

```cpp
// CPU
for (size_t i = 0; i < n; ++i) {
    double fxi = 0, fyi = 0, fzi = 0;
    for (size_t j = 0; j < n; ++j) { /* ... */ }
    f[i] = fxi;
}

// CUDA — same body, no outer loop
__global__ void ljGather(...) {
    size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    double fxi = 0, fyi = 0, fzi = 0;
    for (size_t j = 0; j < n; ++j) { /* ... identical ... */ }
    f[i] = fxi;
}
```

That is the whole port. The outer loop becomes the thread index; the body is
unchanged.

The triangular version has no such translation. `i < j` gives threads wildly
unequal amounts of work (thread 0 does n−1 pairs, the last does 1), and the
`f[j]` write would need atomics — which on a GPU, with thousands of threads
contending, is far worse than doing 2× the arithmetic. GPUs have arithmetic to
spare and hate synchronisation. The gather form trades exactly the right way.

### Paying for it

Be clear-eyed: **the gather form starts 1.76× in the hole.** Threading has to
win that back before showing any profit at all. On this machine:

| | ms | vs best serial |
|---|---:|---:|
| serial-triangular (best serial) | 16.14 | 1.00× |
| serial-gather, 1 thread | 28.39 | 0.57× — *worse* |
| gather, OpenMP × 2 | 14.76 | 1.09× — barely ahead |
| gather, OpenMP × 8 | 6.57 | 2.46× |

You need **two threads just to break even** with good serial code. That is not a
failure of the approach; it is the honest cost of buying scalability. On 8 cores
it is a clear win, on 2 it is a wash, and on 1 it is a straight loss.

On a GPU with thousands of cores, the 2× is irrelevant and the absence of
synchronisation is everything. The formulation is chosen for where it is going.

## Common Pitfalls

- **"I'll just add `#pragma omp atomic` to the `f[j]` line."** It is correct, and
  it is slow — every update becomes a locked read-modify-write, and contention
  scales badly. Lesson 5 measures it.
- **Per-thread force buffers.** Also correct: give each thread its own full
  `f[]`, then sum them. This is what LAMMPS and GROMACS do on CPU. It costs
  `nthreads × N × 3` doubles of memory and a reduction pass — reasonable at 8
  threads, unreasonable at 10,000 GPU threads.
- **Forgetting `if (j == i) continue;`.** Self-interaction gives r² = 0, and
  `sig2/0.0` is `inf`, not a crash. The `r2 < 1e-20` guard catches it too, but
  relying on that is relying on a floating-point accident.
- **Assuming gather is always right.** With a neighbour list the pair count
  drops so far that the 2× penalty starts to dominate, and per-thread buffers
  become competitive again. The right answer depends on the algorithm around it.

## Summary

- Scatter (`f[i]` and `f[j]`) is faster serially and races under threads.
- Gather (`f[i]` only) costs 1.76× and makes races structurally impossible.
- The gather loop *is* a CUDA kernel with the outer loop deleted.
- You need 2 threads to break even; the formulation is chosen for 8+ and for GPU.

## Further Reading

- Exercise: `exercises/03-scatter-vs-gather/`

**Next:** [04_threads_by_hand.md](04_threads_by_hand.md) — the loop is now safe to split. Lesson 4 answers: how do you actually start threads, divide the atoms between them, and collect their results, by hand?
