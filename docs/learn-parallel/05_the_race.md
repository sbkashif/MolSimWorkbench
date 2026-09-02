# Lesson 5: The Race

> ### In plain English
>
> "Add my push to atom 999" is not one step. It is three: read the current
> total, add your bit, write it back. If two workers do that at the same moment
> they both read the *old* total, and whoever writes second wipes out the other
> one's contribution.
>
> Nothing crashes. You get a number that looks completely reasonable and is
> quietly wrong. In this chapter, 2% of atoms came out wrong while the total
> energy and every other summary check looked perfect — and the broken version
> was the **fastest** one, because skipping safety is fast.
>
> Then we fix it three ways and compare what each costs.
>
> New words here: **data race**, **atomic**, **deterministic** —
> see [GLOSSARY.md](GLOSSARY.md).

## Learning Objectives

- Produce a real data race and observe what it does to physical results
- Explain why aggregate checks (energy, net force) fail to detect it
- Compare the three standard fixes on cost, memory, and portability
- Understand why `schedule(dynamic)` can make correct code non-reproducible

## Background

Lesson 3 argued that the triangular scatter races when threaded. Arguments are
easy to nod along to. This lesson runs it.

Everything below was measured on the LJ box at N=4096 with 8 threads, against
the serial scatter as ground truth.

## The experiment

Take the triangular kernel, put `#pragma omp parallel for` on the outer loop,
and change nothing else — the `f[j] +=` write is left unprotected:

```cpp
#pragma omp parallel for schedule(dynamic,16) num_threads(8)
for (long i = 0; i < n; ++i)
    for (long j = i+1; j < n; ++j) {
        ...
        f.x[i] -= fo*dx;   // safe: i belongs to this thread
        f.x[j] += fo*dx;   // RACE: j belongs to nobody
    }
```

| variant | ms | max rel error | run-to-run |
|---|---:|---:|---|
| serial scatter (correct) | 13.97 | — | deterministic |
| **racy parallel scatter** | **3.13** | **1.94e+02** | **19/19 runs differ** |
| fix: `#pragma omp atomic` | 4.04 | 1.3e-11 | deterministic |
| fix: per-thread buffers | 3.13 | 8.1e-12 | deterministic |
| fix: gather | 5.84 | 0 | deterministic |

Read the second row carefully.

**The broken version is the fastest.** Of course it is — it is doing strictly
less work than every correct version. A race does not slow your program down;
it speeds it up, by skipping the synchronisation that correctness requires.
Performance is not evidence of correctness. It is often evidence against it.

**The maximum relative error is 194** — one force component is wrong by
19,400%.

## Why nothing catches it

Here is the part that should worry you:

```
86 of 4096 atoms have a wrong x-force (2.1%)
total |error| / total |force| = 0.000
```

**97.9% of atoms are exactly right.** Every update to `f[i]` was safe, and most
updates to `f[j]` happened not to collide. Only the unlucky 2% got corrupted.

And because the errors are individually small in absolute terms and scattered in
sign, the *aggregate* error is indistinguishable from zero. Which means:

- **Total potential energy: looks correct.** The energy accumulator is not the
  thing that raced.
- **Net force ΣF = 0: passes.** Lost contributions come in ± pairs and cancel.
- **The trajectory: looks physical.** Atoms move, the box stays filled, nothing
  blows up.
- **Energy conservation in NVE: drifts** — but slowly, and drift has a dozen
  innocent explanations (timestep too large, cutoff too short, thermostat
  coupling). You would blame the physics for weeks.

The only check that catches this is a **per-atom comparison against a known-good
implementation.** That is exactly what
`tests/unit/test_lj_kernels.cpp::AllBackendsMatchReferenceForces` does, and it
is why the original implementation was kept in the repo rather than deleted.

Aggregate assertions are how races survive code review.

### It is also undefined behaviour

Not just "a wrong number". In the C++ memory model, two threads writing the same
non-atomic object without synchronisation is UB. The compiler is entitled to
assume it does not happen, and may optimise accordingly — keeping `f.x[j]` in a
register across iterations, reordering the store, vectorising in a way that
assumes exclusivity. "It gave the right answer on my machine" is not a defence;
it may stop doing so under a different optimisation level.

### Finding it with ThreadSanitizer

```bash
cmake -B build-tsan -S . -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build-tsan
./build-tsan/tests/unit/MolSimTests --gtest_filter='LJKernels.*'
```

TSan reports the exact pair of conflicting accesses with both stack traces. It
costs ~10× runtime and ~5× memory, so it is a debugging tool, not a build
setting. Note that TSan and OpenMP interact badly on some toolchains — it will
report false positives inside the OpenMP runtime itself. The hand-rolled
`std::thread` backend is the cleaner target.

## The three fixes

### 1. Atomics — correct, and coarse

```cpp
#pragma omp atomic
f.x[j] += fo*dx;
```

**4.04 ms.** Every update becomes a locked read-modify-write. Surprisingly
cheap here (only 29% over the racy version) because collisions are rare — with
4096 atoms spread over 8 threads, two threads rarely want the same `j` at the
same instant. In a denser conflict pattern, or on a GPU with thousands of
threads, this degrades badly. It also needs six `atomic` directives, one per
component, which is easy to get wrong.

### 2. Per-thread private buffers — the CPU answer

Give every thread its own complete force array, then sum them:

```cpp
std::vector<std::vector<double>> bufX(T, std::vector<double>(n, 0.0));
#pragma omp parallel num_threads(T)
{
    double* fx = bufX[omp_get_thread_num()].data();
    #pragma omp for schedule(static,16) nowait
    for (long i = 0; i < n; ++i)
        for (long j = i+1; j < n; ++j) { ...
            fx[i] -= fo*dx;
            fx[j] += fo*dx;    // this thread's buffer -- nobody else's
        }
}
// then sum the T buffers per atom
```

**3.13 ms — tied for fastest, and correct.** It keeps Newton's third law (half
the arithmetic) *and* removes the conflict, by not sharing the memory at all.

The cost is memory: `T × N × 3` doubles = **0.8 MB** at 8 threads and N=4096.
Fine here. At 100,000 GPU threads it would be 72 GB, which is why this technique
stops at the CPU boundary.

This is what LAMMPS and GROMACS actually do for CPU threading, and it is now
`LJBackend::OpenMPPrivate` — the fastest kernel in this repo at **3.77 ms**
end-to-end, 4.03× over the best serial code.

### 3. Gather — the portable answer

**5.84 ms.** The slowest of the three fixes, because it pays the 2× arithmetic
penalty from lesson 3. It is kept as the primary teaching path because it is the
only one of the three that survives contact with a GPU: no extra memory, no
atomics, no reduction.

**The best CPU kernel and the best GPU kernel are different kernels.** That is
not a contradiction to resolve, it is the actual situation, and the seam in
`lj_kernels.h` exists so both can live in the same codebase.

## The subtle one: scheduling can break reproducibility

While adding `OpenMPPrivate`, the determinism test failed — with no race at all.

The per-thread-buffer version is race-free by construction. But each atom's
final force is a sum over T buffers, and floating-point addition is not
associative. With `schedule(dynamic)`, the OS decides which thread grabs which
chunk, and that decision differs every run. Same values, partitioned differently,
summed in a different order, **rounded differently.** The forces changed in the
last bits between identical runs.

The fix is `schedule(static, 16)`: round-robin chunks, so the partition is fixed
at compile time. Balanced enough for a triangular loop, and reproducible.

```cpp
#pragma omp for schedule(static, 16) nowait     // static, deliberately
```

This is a distinction worth internalising: **race-free is not the same as
deterministic.** A correct parallel program can still give you different
last-digit answers on every run, and for reproducible science that is often
unacceptable. `reduction(+:)` has exactly the same property (lesson 6).

## Common Pitfalls

- **Testing only aggregates.** Energy and ΣF both pass here. Compare per-atom.
- **Trusting a fast result.** The broken kernel was the fastest one.
- **Assuming races are rare because the answer looked fine.** 2.1% of atoms were
  wrong on a run that produced a physically plausible trajectory.
- **Assuming races vary visibly.** They vary — 19/19 runs differed here — but
  the *distribution* can be stable enough that casual re-running looks
  consistent.
- **Reaching for `atomic` first.** It is the easiest fix and rarely the right
  one. Restructure so the conflict cannot occur.

## Summary

- The racy kernel was fastest, physically plausible, and wrong on 2.1% of atoms.
- Aggregate checks do not detect it; per-atom comparison against a reference does.
- Atomics: correct, coarse, scales badly. Per-thread buffers: fastest on CPU,
  memory-hungry, does not port. Gather: slowest, portable, race-impossible.
- Race-free ≠ deterministic: `schedule(dynamic)` reorders a float reduction.

## Further Reading

- Exercise: `exercises/05-the-race/` — you will write the race yourself
- [`src/engine/potentials/lj_kernels.cpp`](../../src/engine/potentials/lj_kernels.cpp) — `computePrivateScatter`

**Next:** [06_openmp.md](06_openmp.md)
