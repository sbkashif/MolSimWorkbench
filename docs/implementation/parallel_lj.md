# Code Tour

What changed in the repo, in plain language, using the worker analogy. Read this
if you know the original serial code and want to see what was done to it.

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
before. You turn it on deliberately — from C++ with `setBackend()`, from the
environment as above, or from YAML.

---

## What else was added

| path | what |
|---|---|
| `benchmarks/bench_lj.cpp` | times every arrangement and checks each against the answer sheet |
| `benchmarks/bench_ablation.cpp` | reproduces lesson 2's two tables |
| `tests/unit/test_lj_kernels.cpp` | 11 tests: every arrangement must match the original, for any worker count, reproducibly |
| tail correction | in `lj_potential.cpp` — the one piece of *physics* added. Opt-in, off by default, worth 8.73% of the energy, contributes exactly zero force |
