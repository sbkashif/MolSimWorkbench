# Learning path

One route through every learning document in this repo, from "what is a C++
program" to "why the parallel LJ kernel looks the way it does". The aim is to
be able to **walk through the real code and understand every line**.

The path is shaped like a trapezium: wide at the start, narrow in the middle,
wide again at the end.

```
 WIDE    C++ concepts                      docs/learn-cpp/ 01–09
          │  one small program, mini_lj, grows one idea per chapter;
          │  every chapter ends on real lines of this repo
          ▼
 TAPER   this program                      docs/learn-cpp/10 → docs/explainer/01–04
          │  mini_lj turns out to be the repo's reference loop;
          │  then the whole program: geometry → potentials → MD loop
          ▼
         its parallel implementation       docs/learn-parallel/ 00–06
          │  the same loop, measured, rewritten, then threaded
          ▼
 WIDE    understanding it deeply           docs/learn-parallel/ 07–09, explainer 05,
                                           every section marked (deep dive later)
```

## Stage 1 — C++ concepts (wide)

[`learn-cpp/`](learn-cpp/README.md), chapters 1–9. Each chapter comes in a
technical version and a plain-language twin. The running program `mini_lj`
starts as two atoms (ch 2) and ends as classes, potentials, shared ownership,
a timer and two threads (ch 9).

## Stage 2 — this program, then its parallelism (taper)

1. [`learn-cpp/10`](learn-cpp/10_mini_lj_to_the_repo.md) — the bridge.
   `mini_lj` computes the same energy as the repo's reference loop, to every
   printed digit (-20105.493949 at N = 4096). Build and run the real thing.
2. [`explainer/01–04`](explainer/README.md) — how the whole program fits
   together: architecture, the LJ implementation, the integrator and MD loop,
   builders and YAML config. (`explainer/00_cpp_primer.md` is a review once
   you have done learn-cpp 1–7.)
3. [`learn-parallel/00–06`](learn-parallel/START_HERE.md) — the parallel
   implementation: measure first, the serial rewrite, scatter vs gather,
   threads by hand, the race, OpenMP. Exercises in `exercises/` are where you
   write it yourself.

## Stage 3 — understanding it deeply (wide again)

- [`learn-parallel/07–09`](learn-parallel/07_honest_speedup.md) — honest
  speedup, the physics outside the hot loop, the CUDA port and how GROMACS is
  built.
- [`explainer/05`](explainer/05_python_interface.md) — driving it all from
  Python through pybind11.
- Every section marked **(deep dive later)** anywhere on the path.

## Reference, not lessons

| | |
|---|---|
| [`learn-parallel/GLOSSARY.md`](learn-parallel/GLOSSARY.md) | every specialist term, in plain words |
| [`learn-parallel/PROGRESS.md`](learn-parallel/PROGRESS.md) | where you are in the parallel course |
| [`implementation/parallel_lj.md`](implementation/parallel_lj.md) | what was changed in the code for the parallel kernel: call path, backends, tests, known gaps |
