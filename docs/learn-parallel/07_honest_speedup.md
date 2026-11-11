# Lesson 7: Honest Speedup

> **Where we are.** Lessons 2–6 produced a table of backends with speedups as high as 9.24×. This lesson asks which of those numbers you are allowed to quote, and what limits them.

## Learning Objectives

- Choose a baseline you can defend, and recognise the flattering one
- Apply Amdahl's law to real measurements and extract the serial fraction
- Distinguish strong scaling from weak scaling
- Explain a scaling curve that bends, and one that goes backwards

## Background

This is the lesson that decides whether the previous six were worth anything.
It is very easy to produce a large speedup number and very easy for that number
to be meaningless. Everything here is about the denominator.

## Core Concepts

### The baseline problem

The final results, N=4096, 8 threads:

| backend | ms/call | vs reference | **vs best serial** |
|---|---:|---:|---:|
| reference (original) | 34.85 | 1.00× | 0.44× |
| serial-triangular | 15.22 | 2.29× | **1.00× ← the baseline** |
| serial-gather | 28.29 | 1.23× | 0.54× |
| `std::thread` gather ×8 | 6.56 | 5.31× | 2.32× |
| openmp gather ×8 | 5.97 | 5.84× | 2.55× |
| openmp private ×8 | 3.77 | **9.24×** | **4.03×** |

The two right-hand columns describe the same programs. One says the work
produced a **9.24× speedup**; the other says **4.03×**. Both are arithmetically
correct. Only one is honest.

The reference implementation is slow for reasons that have nothing to do with
parallelism: it divides three times per pair in the minimum-image convention,
takes a square root it does not need, and normalises a direction vector it then
immediately rescales. Fixing those is a *serial* improvement worth 2.29×. If you
divide by the reference, that 2.29× gets silently re-labelled as a benefit of
threading. It is not. A reader who then buys an 8-core machine expecting 9× will
get 4×.

**Rule: the denominator is the fastest correct single-threaded implementation you
know how to write.** Not the code you started with. Not the parallel code run on
one thread — that carries the gather form's 2× penalty and would flatter you
almost as much (28.29/3.77 = 7.5×).

This is why `bench_lj.cpp` prints both columns and labels one of them as the
flattering one. It is much harder to fool yourself when the honest number is on
the same line.

### Amdahl's law, applied to actual data

If a fraction *s* of the runtime is inherently serial, then with *T* processors:

$$S(T) = \frac{1}{s + \frac{1-s}{T}} \qquad S(\infty) = \frac{1}{s}$$

Here is the gather kernel scaling, measured:

| threads | ms | speedup vs 1 thread | efficiency |
|---:|---:|---:|---:|
| 1 | 28.92 | 1.00× | 100% |
| 2 | 14.76 | 1.96× | 98% |
| 3 | 10.29 | 2.81× | 94% |
| 4 | 7.74 | 3.74× | **93%** |
| 5 | 10.21 | 2.83× | 57% |
| 6 | 8.64 | 3.35× | 56% |
| 7 | 7.56 | 3.83× | 55% |
| 8 | 6.84 | 4.23× | 53% |

Take the 4-thread point and solve for *s*:

$$\frac{1}{3.74} = s + \frac{1-s}{4} \;\Rightarrow\; s \approx 0.024$$

**About 2.4% serial.** That is the AoS→SoA marshalling in `buildView()`, the
force scatter-back, and the fork/join overhead — all the O(N) work wrapped
around an O(N²) kernel.

Amdahl then caps this program at **1/0.024 ≈ 42×**, no matter how many cores you
add. So on 8 cores Amdahl is *not* what is limiting you: it predicts 7.0× at
T=8, and you measured 4.23×. Something else is responsible.

### The something else: your cores are not the same

Look at where the efficiency column falls off a cliff — between 4 and 5 threads,
from 93% to 57%. And note that **5 threads (10.21 ms) is slower than 4 threads
(7.74 ms)**.

This is an Apple Silicon chip: **4 performance cores and 4 efficiency cores.**
`hardware_concurrency()` returns 8 and tells you nothing about that. Threads 1–4
land on P-cores and scale almost perfectly. Thread 5 lands on an E-core running
at roughly half the speed, and with `schedule(static)` every other thread waits
at the barrier for it to finish its equal share.

Two consequences worth carrying with you:

1. **"Number of cores" is not a scalar on modern hardware.** Phones, laptops and
   most new server parts are heterogeneous. A model that assumes identical
   processors will mispredict.
2. **The fix is scheduling, not fewer threads.** `guided` brings 5 threads to
   7.06 ms and 8 threads to 5.84 ms (lesson 6). Load balancing recovers most of
   the loss because E-cores simply take fewer chunks.

The scaling curve bending at exactly the P-core count is a *hardware fingerprint*.
When you see a knee, ask what changed at that thread count.

### Strong vs weak scaling (deep dive later)

The table above is **strong scaling**: fixed problem, more processors. It answers
"can I get my answer sooner?" It is bounded by Amdahl and degrades as *T* grows,
because the per-processor work shrinks toward the overhead.

**Weak scaling** grows the problem with the processors — 2× the cores and 2× the
work, asking "can I solve a bigger problem in the same time?" It answers a
different question and usually looks much better, because the parallel work grows
while the serial part often does not (Gustafson's law).

For MD this distinction is the difference between two real goals:

- *Strong*: run my 4096-atom system for more nanoseconds. Hard — you eventually
  run out of atoms to divide among cores.
- *Weak*: simulate a bigger system at the same ns/day. Easier, and what
  supercomputers are actually built for.

Be explicit about which you are reporting. A weak-scaling plot presented as
strong scaling is one of the most common forms of accidental dishonesty in
performance work.

Note also that the parallel efficiency depends on N. From lesson 1:

| N | serial-triangular | OpenMP ×8 | speedup |
|---:|---:|---:|---:|
| 125 | 0.045 ms | 0.052 ms | **0.87×** |
| 1000 | 1.201 | 0.526 | 2.28× |
| 4096 | 15.160 | 5.943 | 2.55× |

The same code is a loss at N=125 and a win at N=4096. **A speedup figure without
a problem size attached is not a measurement.**

### The checklist

When you report a number, state all of it:

- [ ] What is the baseline, and is it the fastest serial code you can write?
- [ ] What problem size? What density, cutoff, precision?
- [ ] What hardware, and are the cores homogeneous?
- [ ] Minimum or mean of how many runs? Release build?
- [ ] Were results verified against a reference, per element?
- [ ] Strong or weak scaling?

`bench_lj.cpp` prints most of this in its header for exactly this reason.

## Common Pitfalls

- **Dividing by the original code.** Flatters by however much serial work you
  also did — 2.29× here.
- **Dividing by the parallel code on one thread.** Hides the formulation penalty.
  Gather on 1 thread is *worse* than the triangular serial kernel.
- **Reporting efficiency without saying against what.** Efficiency against
  1-thread gather (53% at T=8) and against best serial are different numbers.
- **Extrapolating a straight line.** The curve here is linear to 4 threads and
  then breaks. Predicting 64 cores from an 8-core run is guesswork.
- **Ignoring the size dependence.** Speedup is a function of N, not a constant.

## Summary

- Report speedup against the **fastest correct serial code**: 4.03×, not 9.24×.
- Amdahl gives s ≈ 2.4% here, capping the program at ~42× — so Amdahl is not the
  binding constraint at 8 cores.
- What *is* binding is heterogeneous cores: 93% efficiency on 4 P-cores, 53% once
  E-cores join, and a 5-thread run slower than a 4-thread run under `static`.
- Strong and weak scaling answer different questions. Say which one you ran.
- A speedup with no problem size attached means nothing.

## Further Reading

- Exercise: `exercises/07-scaling/` — produce your own scaling study

**Next:** [08_not_the_hot_loop.md](08_not_the_hot_loop.md) — the pair loop is done. Lesson 8 answers: what about work that is *not* in the hot loop, and how do you tell when something should not be parallelized at all?
