# Lesson 1: Measure First

> **Where we are.** Lesson 0 found the hot loop and four suspects, but nothing has been changed yet. Before changing anything, this lesson builds the measuring tool every later number comes from.

## Learning Objectives

- Write a benchmark that measures the kernel rather than the weather
- Explain why minimum-of-N beats mean-of-N for timing
- Demonstrate that this repo's default N=125 cannot measure parallel speedup at all
- Put a correctness check *before* a timing number, always

## Background

The first rule is unglamorous: **a speedup you did not measure is a speedup you
did not get.** Parallel programming is unusually good at producing code that
feels faster, looks faster, and is not faster. The only defence is a harness you
trust, built before you change anything.

The second rule is that a benchmark measures whatever is slowest, which is often
not your code. On a laptop the competition includes your browser, Spotlight
indexing, thermal throttling, and the CPU's own decision about what clock speed
to run at. A good harness makes those visible instead of averaging them in.

## Core Concepts

### Warm-up: never time the first call

Here is the timing helper, with every unfamiliar piece of C++ explained inline.

```cpp
// "template <typename F>" means: this is a RECIPE, not a finished function.
// F is a blank to be filled in. Whatever you pass as `f`, the compiler
// generates a version of timeMs specialised for that exact type.
//
// We need this because `f` might be a lambda, a function pointer, or a
// struct with operator() -- each of those is a DIFFERENT type, and none of
// them has a name you could write down. The blank lets you accept all of them.
template <typename F>
double timeMs(F&& f, int reps) {         // F&& = take `f` without copying it
                                         // reps = how many timed runs to do

    f();                                 // WARM-UP. Called, deliberately NOT timed.
                                         // Everything below this line is measured;
                                         // this line is the price of admission.

    double best = 1e300;                 // a deliberately absurd starting value,
                                         // so the first real measurement is
                                         // guaranteed to be smaller than it

    for (int r = 0; r < reps; ++r) {
        const auto t0 = Clock::now();    // `auto` = "work out the type for me";
                                         // it is a timestamp, and its real type
                                         // name is long and uninteresting
        f();                             // the thing being measured
        const auto t1 = Clock::now();    // timestamp again

        // t1 - t0 is a duration. duration<double, std::milli> reinterprets it
        // in milliseconds-as-a-double, and .count() pulls out the plain number.
        best = std::min(best,
                 std::chrono::duration<double, std::milli>(t1 - t0).count());
                                         // keep whichever is smaller: the best
                                         // so far, or this run
    }
    return best;                          // the fastest of the `reps` runs
}
```

Note the clock is taken **inside** the loop, around each individual call. Timing
the whole loop once and dividing by `reps` would give you the average, which is
the thing we are trying to avoid.

### Why the warm-up is there

"The first call is unrepresentative" is easy to say and easy to disbelieve.
Here is the same kernel called eight times in a row, N=4096, nothing else
changing:

```
  call    ms
     1    7.48   <- first call
     2    4.90
     3    4.36
     4    4.30
     5    4.30
     6    4.36
     7    4.49
     8    4.90
```

**The first call is 74% slower than the steady state.** Include it and you would
report 7.48 ms for a kernel that costs 4.30 ms.

Three things are being paid for on that first call, none of which is your
kernel's actual work:

- **Page faults.** When you ask for memory, the operating system does not
  actually hand it over — it makes a note and waits. The real memory is only
  attached the first time you *touch* each part of it, and each of those
  attachments is an interruption. On the second call the memory is already
  yours.
- **Cold caches.** The processor keeps recently-used data in small fast
  storage near the cores. On the first call nothing is in there yet, so every
  number has to be fetched from main memory, which is far away and slow. By the
  second call much of it is already close by.
- **Worker startup.** The 8 workers do not exist until something asks for them.
  Creating them costs real time. They are then kept alive and reused, so later
  calls do not pay it. Charging that one-time cost to the kernel would make
  parallelism look far worse than it is.

Notice also that call 2 (4.90 ms) is still slightly elevated, and it does not
truly settle until call 3 or 4. One warm-up removes most of the distortion, and
taking the **minimum** of several runs mops up the rest — which is the next
section.

### Minimum, not mean

`timeMs` returns the *fastest* of N runs. That looks like cheating and isn't.

Interference is one-sided. The OS descheduling your thread, a background
process stealing a core, the CPU dropping its clock — every one of these makes a
run *slower*. Nothing makes a run spuriously faster. So the distribution of
measured times is the true cost plus a non-negative noise term, and the minimum
is the best estimate of the true cost.

The mean estimates "true cost plus average interference", which is a property of
your machine's mood, not of your kernel. Report the minimum; if you want to know
how noisy the machine is, report the spread separately.

### Correctness first, and in the same program

`benchmarks/bench_lj.cpp` computes the max deviation from the reference forces
*before* it prints a single timing:

```
backend               ms/call    vs ref           energy      max|df|      max rel
reference              34.891     1.00x -20105.493949019            -            -
serial-triangular      16.142     2.16x -20105.493949019    1.137e-13    1.490e-11
openmp                  6.571     5.31x -20105.493949019    1.137e-13    1.490e-11
```

An infinitely fast kernel that returns the wrong forces is worth nothing, and
"fast" is exactly what a subtly-broken parallel kernel looks like. Keeping both
columns in one table means you cannot read the speedup without also seeing
whether the answer survived.

**A note on that `max rel` column.** The kernels are *not* bitwise identical to
the reference, and that is expected. The reference computes `(σ/r)^6` with
`std::pow`; the kernels compute `(σ²/r²)³` with two multiplies. Algebraically
equal, but floating-point arithmetic is not associative, so they round
differently — about 1.5e-11 relative. That is ~10⁵ ULP: far too large to call
"identical", and far too small to perturb a trajectory. Knowing which of those
two statements applies is the job.

### Your test system cannot measure parallelism

`data/lj_bulk_simulation.yaml` uses `num_particles: 125`. Here is every backend
across problem sizes, on this machine:

| N | serial-triangular | OpenMP ×8 | speedup |
|---:|---:|---:|---:|
| **125** | **0.045 ms** | **0.052 ms** | **0.87× — slower!** |
| 512 | 0.549 | 0.276 | 1.99× |
| 1000 | 1.201 | 0.526 | 2.28× |
| 2000 | 4.044 | 1.586 | 2.55× |
| 4096 | 15.160 | 5.943 | 2.55× |
| 8000 | 55.297 | 27.287 | 2.03× |

At N=125, **parallelism makes it slower.** Spawning and joining 8 threads costs
more than the 0.045 ms of work there is to share. If you had "tested
parallelization" on the repo's default config, you would have concluded that
threading does not work.

There is a floor to how small a parallel task can usefully be, it is on the
order of tens of microseconds, and N=125 is below it. **Every exercise in this
course uses N ≥ 4000.**

### While you are here: confirm the complexity (deep dive later)

The same table proves the loop is O(N²). From N=4096 to N=8000, N grows 1.95×
and time grows 55.30/15.16 = 3.65×. And 1.95² = 3.80. That is the quadratic
you expected, and it is worth confirming rather than assuming — if it had come
out linear, something would have been badly wrong with the cutoff.

## Common Pitfalls

- **Timing a debug build.** `-O0` can be 10× slower and changes which things
  dominate. Always benchmark `-DCMAKE_BUILD_TYPE=Release`. If your speedup
  vanishes when you optimize, you were measuring the compiler.
- **Letting the optimizer delete your work.** If a benchmark computes something
  and never uses it, the compiler may remove it and time an empty loop. Here the
  forces get written through a pointer into a live `std::vector`, so they
  survive — but check, especially in micro-benchmarks.
- **One run.** A single measurement has no error bar and is indistinguishable
  from noise.
- **Comparing across machines, or across a lunch break.** Thermal state matters.
  An earlier version of `bench_lj.cpp` re-timed the parallel backends at the end
  to print a second table; those runs came out ~2× slower purely from heat.
  Measure once, store the number, do arithmetic on the stored number.

## Summary

- Warm up, then take the **minimum** of several runs.
- Print correctness next to speed, so you cannot read one without the other.
- Algebraically-equal floating-point rewrites are not bitwise equal; know the
  size of the difference (~1.5e-11 here) and whether it matters (it does not).
- N=125 is below the floor where threading can pay for itself. Use N ≥ 4000.
- Confirm your complexity while you have the data in front of you.

## Further Reading

- [`benchmarks/bench_lj.cpp`](../../benchmarks/bench_lj.cpp) — the harness this lesson describes
- Exercise: `exercises/01-measure-first/`

**Next:** [02_serial_refactor.md](02_serial_refactor.md) — with a harness you trust, lesson 2 answers: which of lesson 0's four suspects actually costs the time, and how much can one core gain before any threads?
