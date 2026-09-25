# Progress

Where you are in the course, and the exact next thing to do. Update as you go.

_Last updated: 2026-09-25_

---

## Status

| | |
|---|---|
| **Lessons read** | 00 ✅ · 01 ✅ · 02 ✅ (partly — see below) |
| **Exercises done** | 01 ✅ 6/6 passing |
| **Exercise in progress** | **02 — currently does not compile** |
| **Branch** | `parallelization-curriculum` |

### Right now: exercise 02 is broken

`exercises/02-serial-refactor/src/refactor.cpp` has **4 opening braces and 3
closing braces**. You opened `if (rule == "geometric") {` inside
`buildMixingTable` and never closed it. That is the only thing stopping the
build.

Also in that block: `t.eps4.assign(..., 0.0)` re-fills the table with zeros. The
table was already zeroed four lines above, so those two lines undo nothing and
achieve nothing. You need to *write computed values into* the slots, not
re-zero them.

**Next action, in order:**

1. Close the brace. `cmake --build build-learn --target ex02_tests` goes quiet.
2. Finish TODO(1) — fill the mixing table. Check with:
   ```bash
   cd build-learn && ctest -L learn -R "Ex02Refactor.MixingTable" --output-on-failure
   ```
   Three tests, they only cover TODO(1), so the rest of the exercise cannot
   distract you.
3. Then TODO(2)–(4) — the r² kernel itself.

The stub comments for TODO(1) and TODO(2) were expanded with full explanations
(flat-list-as-grid, and why the branchless minimum image works). Re-read them
before starting; they are much longer than when you first opened the file.

---

## What you have covered

**Lesson 00 — the starting point.** The LJ equations, two atoms worked through at
r = 1.2, the per-pair checklist, the 74:1 ratio, the spring explanation for the
`-=` / `+=` pair, and the four suspects.

**Lesson 01 — measure first.** Warm-up, minimum-not-mean, why N=125 cannot
measure parallel speedup, and the annotated `timeMs` helper.

**Exercise 01 — done, 6/6.** `timeMs`, `speedup`, `parallelEfficiency`.
Note to self: the first pass was written with autocomplete and not understood;
the second pass was. **Turn autocomplete off for these exercises** — the tests
cannot tell the difference between code you understand and code you accepted.

**Lesson 02 — the ablation result.** You guessed (D), the `std::map` lookup. So
did I. The answer is **(C)**, the square root and the three divisions.

---

## Numbers established (measured on this machine, N=4096, ρ=0.8, r_c=2.5)

```
pairs evaluated      8,386,560
inside the cutoff      113,910   (1.36%)  -> 74 : 1
```

**Backends**

| | ms/call | |
|---|---:|---|
| reference (original code) | 34.85 | the yardstick, not a backend |
| serial-triangular | 15.22 | **honest baseline** |
| serial-gather | 28.29 | restructuring's price tag |
| std::thread ×8 | 6.56 | |
| openmp-gather ×8 | 5.97 | |
| openmp-private ×8 | **3.77** | fastest — 4.03× over best serial |

**Ablation (step gains)**

| fix | suspect | gain |
|---|---|---:|
| integer type ids, flat table | (D) | 1.10× |
| `std::pow` → multiplies | (E) | 1.01× — free, compiler already did it |
| drop sqrt, work in r² | **(C)** | **2.51× ← the answer** |
| structure-of-arrays | (B) | **0.83× — made it slower**, kept for the GPU |

**Other facts worth not re-deriving**

- First call 7.48 ms vs 4.30 ms steady state → 74% overstatement without a warm-up
- At N=125, OpenMP (0.052 ms) is *slower* than serial (0.045 ms)
- `schedule(static)` on 5 threads (9.43 ms) is slower than on 4 (7.62 ms); `guided` fixes it
- Racy scatter: 2.1% of atoms wrong, aggregate error ≈ 0, and it is the fastest variant
- Tail correction: 8.73% of the energy, exactly zero force
- Map lookup cost grows 25.6 → 125.2 ns from 1 to 100 atom types; flat table stays flat

---

## Errors you found in the lessons

Kept as a record, because finding them was the most useful reading you did.

1. **False ranking.** Lesson 00 called the `std::map` lookup "the single biggest
   cost" — it is 1.10×. Rewritten as unranked suspects.
2. **Wrong hot path.** Both lessons claimed the lookup runs 8.4 M times. It sits
   *after* the cutoff test, so it runs 113,910 times. The cost estimate was
   right; the frequency estimate was wrong by 74×.
3. **"Newton's third law" as an explanation.** Named the rule instead of making
   it intuitive. Replaced with the spring.
4. **Bogus redundancy argument.** "900 distinct answers, still 127× redundant"
   implied the problem *shrinks* with more atom types — the opposite of the
   conclusion two paragraphs below. Deleted.
5. **Label collision.** Lesson 00 labels suspects (A)–(E); lesson 02's table
   independently used A–D, so row "B" (the biggest gain) was suspect (C). This
   is why you concluded (B). Variants are named now.
6. **"SoA lets the CPU vectorize."** Measured 13% *slower*. Corrected.
7. **Unexplained jargon.** `template`, `auto`, `numeric_limits`, AoS/SoA all used
   before being defined. Glossary added, plus definitions at first use.
8. **Vacuous tests.** 15 of 64 exercise tests passed against empty stubs — the
   consistency ones passed because a kernel that does nothing is trivially
   deterministic. All 64 now fail until implemented.

---

## Still to come

| | |
|---|---|
| 03 | scatter vs gather — where the parallel half begins |
| 04 | threads by hand |
| 05 | the race (break it on purpose) |
| 06 | OpenMP |
| 07 | honest speedup |
| 08 | the O(1) tail correction |
| 09 | CUDA + GROMACS (read-only) |
| capstone | `project/` |

## Open question

Lesson 04 is the one that turns "parallel" from a word into machinery. If it has
not clicked by the end of it, say so and it gets rewritten — that offer is still
open.
