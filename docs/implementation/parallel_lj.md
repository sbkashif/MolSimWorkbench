# Parallel LJ — implementation notes

What was changed in the repo to make the LJ force loop parallel, and where it
lives. This is a record of the implementation, not a lesson: the reasoning and
the measurements behind each decision are taught in
[`docs/learn-parallel/`](../learn-parallel/START_HERE.md).

Part 1 is the plain-language tour (formerly `learn-parallel/CODE_TOUR.md`).
Part 2 is the technical reference: entry points, line numbers, tests, and
known gaps.

# Part 1 — Plain-language tour

---

## Before: one worker, one clipboard

Your `calculateForces` did the whole job itself:

```
for each pair of atoms:
    measure the distance (wrap around the box)
    too far apart?  ->  skip
    go and look up epsilon and sigma for these two atom types   <- walks to the
                                                                   filing cabinet
    compute the push
    update BOTH atoms
```

One worker, start to finish.

**That code is still there, unchanged.** It was renamed
`calculateForcesReference()`. Nothing calls it during a real run — it is the
answer sheet every new version gets checked against. Keeping it is why every
later claim in this course can be verified instead of trusted.

---

## After: a prep step, then a choice of work arrangements

```
calculateForces()                        <- the same function you always called
   │
   ├─ 1. buildView()          PREP THE WORKSHEET
   │
   ├─ 2. ljComputeGather()    DO THE JOB  (pick an arrangement)
   │
   └─ 3. copy the results back
```

### Step 1 — prep the worksheet

`buildView()`, in `src/engine/potentials/lj_potential.cpp`.

Before anyone starts work, translate the inputs into a form that is fast to use:

| your format | worksheet format |
|---|---|
| atom names as text — `"LJ"`, `"O"`, `"H"` | plain numbers — `0`, `1`, `2` |
| each atom's x, y, z kept together | three separate lists: all the x, all the y, all the z |
| look up ε and σ during the job | **one small table, worked out once up front** |

The last row is the important one. The worker no longer walks to the filing
cabinet mid-job — every answer is already written on the worksheet.

This costs one pass over the atoms. The job itself is millions of pairs, so the
prep is cheap by comparison.

Why bother? Your original format is comfortable for a person: an atom is a thing
with a name and three coordinates. It is awkward for fast work, and **impossible
for a graphics card**, which cannot handle text or objects at all. Somebody has
to translate, and doing it once beforehand beats doing it 8 million times inside
the loop.

### Step 2 — do the job

`src/engine/potentials/lj_kernels.cpp`, a new file. Five arrangements, all
computing identical physics:

| name | workers | method |
|---|---|---|
| `SerialTriangular` | 1 | **your original method** — each pair once, update both atoms |
| `Serial` | 1 | new method — each atom does all its own work, so pairs get computed twice |
| `Threads` | 8 | new method, workers managed by hand |
| `OpenMP` | 8 | new method, workers managed by the compiler |
| `OpenMPPrivate` | 8 | **your original method**, but each worker gets a private scratch sheet, summed at the end — *fastest* |

The middle three use the restructured method because it is the one that survives
onto a graphics card. `OpenMPPrivate` keeps *your* method and solves the
collision problem differently — by never sharing paper in the first place.

Why there needs to be more than one: your original method has two workers
reaching for the same atom at the same moment, which silently loses one of their
contributions. Lessons 3 and 5 are entirely about that.

### Step 3 — copy back

The worksheet's three lists are copied back into the shape the rest of the
program expects. Nothing downstream changed.

---

## The seam, and why it looks odd

`lj_kernels.h` defines `LJKernelView` — a bundle of nothing but plain numbers and
addresses. No text, no dictionaries, no objects that manage themselves.

That is deliberate and it is the whole architectural bet: it is exactly the
subset a graphics card can accept. Adding a GPU version later means adding one
new file beside `lj_kernels.cpp` and one new name to the list of arrangements.
No caller changes, and no `#ifdef` litter through `lj_potential.cpp`.

---

## Running it

```bash
# default: one worker, behaviour unchanged from before this course existed
./build/run_simulation data/lj_bulk_simulation.yaml

# eight workers
MOLSIM_LJ_BACKEND=openmp-private ./build/run_simulation data/lj_bulk_simulation.yaml

# also: serial | threads | openmp        and  MOLSIM_LJ_THREADS=4
```

**Parallelism is off by default.** A stock build behaves exactly as it did
before. You turn it on deliberately — from C++ with `setBackend()`, or from the
environment as above. (There is no YAML switch yet, and no Python one — see
*Known gaps* below.)

---

## What else was added

| path | what |
|---|---|
| `benchmarks/bench_lj.cpp` | times every arrangement and checks each against the answer sheet |
| `benchmarks/bench_ablation.cpp` | reproduces lesson 2's two tables |
| `tests/unit/test_lj_kernels.cpp` | 11 tests: every arrangement must match the original, for any worker count, reproducibly |
| tail correction | in `lj_potential.cpp` — the one piece of *physics* added. Opt-in, off by default, worth 8.73% of the energy, contributes exactly zero force |

---

# Part 2 — Technical reference

Line numbers verified on 2026-11-10.

## Call path

```
LJPotential::calculateForces          src/engine/potentials/lj_potential.cpp:159
└─ LJPotential::calculateForcesAndEnergy  lj_potential.cpp:166   what the MD loop reaches, via ForceCalculator
  └─ LJPotential::buildView           lj_potential.cpp:91    AoS + strings -> SoA + int type ids
       └─ rebuildTables (if dirty)    lj_potential.cpp:56    pre-mixed eps4 / sigma2 tables
  └─ ljComputeGather(view, backend, numThreads)
                                      src/engine/potentials/lj_kernels.cpp:273   dispatch on LJBackend
  └─ copy SoA forces back to AoS      lj_potential.cpp:177
  └─ + tail correction energy         lj_potential.cpp:180
```

`calculateEnergy` (`lj_potential.cpp:147`) takes the same path and adds the
optional tail correction (`:156`).

## Backends

| `LJBackend` | Function | Lines in `lj_kernels.cpp` | Form |
|---|---|---|---|
| `SerialTriangular` | `computeTriangular` | 120–168 | scatter, i<j, 1 thread |
| `Serial` (default) | `computeRange(v, 0, n)` | 29–106 | gather, 1 thread |
| `Threads` | `computeRange` per slice, `std::thread` pool | 302–333 | gather, balanced static split |
| `OpenMP` | `computeRange` per atom | 335–362 | gather, `schedule(guided)` + `reduction(+:acc)` |
| `OpenMPPrivate` | `computePrivateScatter` | 184–268 | scatter into per-thread buffers, `schedule(static, 16)`, ordered reduction |

The seam is `LJKernelView` (`lj_kernels.h:25-54`): raw pointers, ints and
doubles only, so a CUDA backend could take it unchanged.

## Selecting a backend

- C++: `lj.setBackend(LJBackend::OpenMPPrivate)`, `lj.setNumThreads(n)` (`lj_potential.h:35`, `:38`).
- Environment, read once in the `LJPotential` constructor (`lj_potential.cpp:19-36`):
  `MOLSIM_LJ_BACKEND=serial|threads|openmp|openmp-private`, `MOLSIM_LJ_THREADS=n`.
- `numThreads < 1` means the OpenMP default; it is capped at N (`lj_kernels.cpp:276-278`).
- Without OpenMP at build time, the OpenMP backends fall back to serial code (`#else` branches).

## Correctness contract

`tests/unit/test_lj_kernels.cpp`, 11 tests:

| Suite | Tests |
|---|---|
| `LJKernels` | all backends match the reference forces and energy; result independent of thread count; repeated runs bit-identical; net force zero; empty and single-atom systems; unparameterised type contributes nothing |
| `LJTailCorrection` | changes no force; lowers the energy; matches the analytic formula; off by default and needs a box |

The reference is the original serial code, kept as
`calculateEnergyReference` / `calculateForcesReference` (`lj_potential.cpp:277`, `:307`).

## Measured (2026-10-07, Apple M1, 4 P + 4 E cores, `bench_lj 4096 7`)

```
backend               ms/call    vs ref           energy      max|df|      max rel
reference              35.057     1.00x -20105.493949019            -            -
serial-triangular      15.060     2.33x -20105.493949019    1.137e-13    1.490e-11
serial-gather          28.336     1.24x -20105.493949019    1.137e-13    1.490e-11
std::thread             6.594     5.32x -20105.493949019    1.137e-13    1.490e-11
openmp-gather           6.726     5.21x -20105.493949019    1.137e-13    1.490e-11
openmp-private          3.763     9.32x -20105.493949019    1.279e-13    1.158e-11
```

Honest speedup (against `serial-triangular`): `openmp-private` 4.00×.

## Known gaps

| Gap | Where | Note |
|---|---|---|
| No YAML switch | `src/utils/yaml_parser.cpp` | there is no config key for the backend |
| No Python switch | `src/bindings/molsim_bindings.cpp` | `setBackend` / `LJBackend` are not exposed; from Python only the environment variables work |
| Demo config too small | `data/lj_bulk_simulation.yaml` uses 125 atoms | threading is a net loss below ~1000 atoms (lesson 01) |
| O(N) marshalling every call | `buildView` | cheap against the O(N²) kernel on CPU; on a GPU it would be a host→device copy per step (lesson 09) |
| No neighbour list | — | intentional scope (`CLAUDE.md`); 98.6% of pairs are outside the cutoff |

