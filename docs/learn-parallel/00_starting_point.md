# Lesson 0: The Starting Point

## Learning Objectives

- Locate the exact loop that all nine lessons modify
- Identify, by reading, the three things in it that block parallelization
- Understand the seam (`lj_kernels.h`) that lets a CPU kernel and a GPU kernel coexist

## Where the time goes

Every MD step in this repo funnels through one place:

```
MolecularDynamics::run()                    src/engine/molecular_dynamics.cpp
  └─ ForceCalculator::calculateForces()     src/engine/force_calculator.cpp
       └─ LJPotential::calculateForces()    src/engine/potentials/lj_potential.cpp  ← HERE
```

A 5000-step run calls that innermost function 5000 times. At N=4096 it was
taking 34.9 ms per call, so ~3 minutes of a 3-minute simulation was spent
inside one double loop. That is the whole reason this course exists: there is
exactly one hot spot, and it is 30 lines long.

## The original loop

This is the code as it shipped, and it is still in the repo — preserved as
`LJPotential::calculateForcesReference()` so every later version can be tested
against it. Annotations are mine.

```cpp
for (size_t i = 0; i < atoms.size(); ++i) {
    for (size_t j = i + 1; j < atoms.size(); ++j) {      // (A) triangular: i < j
        double dx = positions[j][0] - positions[i][0];   // (B) AoS indexing
        double dy = positions[j][1] - positions[i][1];
        double dz = positions[j][2] - positions[i][2];

        if (boxDim[0] > 0.0) dx = dx - boxDim[0]*std::round(dx/boxDim[0]);
        if (boxDim[1] > 0.0) dy = dy - boxDim[1]*std::round(dy/boxDim[1]);
        if (boxDim[2] > 0.0) dz = dz - boxDim[2]*std::round(dz/boxDim[2]);

        double distanceSquared = dx*dx + dy*dy + dz*dz;
        double distance = std::sqrt(distanceSquared);    // (C) sqrt, every pair

        if (distance > cutoff || distance < 1e-10) continue;

        auto [epsilon, sigma] =
            getMixedParameters(atoms[i], atoms[j]);      // (D) TWO std::map
                                                         //     <std::string> lookups
                                                         //     + a sqrt. Per pair.
        std::array<double,3> direction = {dx/distance, dy/distance, dz/distance};

        calculatePairForce(epsilon, sigma, distance,
                           direction, forces[i], forces[j]);   // (E) writes BOTH i and j
    }
}
```

At N=4096 the inner body runs **8.4 million times** per call.

## Four suspects — deliberately not ranked

Below are the four things in that loop that could be costing you. They are
listed in the order they catch the eye, which is **not** the order that matters.

Ranking them is lesson 2's job, and it is done by measurement, because the
obvious answer turns out to be wrong. Read these as suspects, form your own
guess about which dominates, and write it down before you get to lesson 2. You
will find out whether you did better than I did — I was confident, and I was
wrong.

**(D) The parameter lookup.**
`getMixedParameters` takes two `std::string`s, does two red-black-tree descents
in a `std::map`, then computes `std::sqrt(eps1*eps2)` for the mixing rule. It
does this 8.4 million times to recompute *a constant* — for a single-component
LJ fluid the answer is identical every time.

(A red-black tree is the balanced binary search tree `std::map` is built from.
Looking up `"LJ"` walks from the root, comparing strings and chasing a pointer
to a separately-allocated node at each step, until it finds the key. Compare
that with what it becomes after the refactor: `eps4row[type[j]]`, a single
integer index into a contiguous array.)

Whatever it costs, it is a **hard blocker for the GPU**: there is no `std::map`
and no `std::string` in a CUDA kernel. So this one has to go regardless of what
the profiler says about it.

**(C) The `sqrt` and the divisions.** A square root for every pair, then used as
a divisor three more times to normalise the direction vector. Both the energy
and the force can be written in terms of r² alone, so the square root is never
actually needed. Note also the three divisions in the minimum-image convention
just above it — `dx / boxDim[0]` and friends — which run *before* the cutoff
test. Keep that timing in mind; it matters more than it looks.

**(B) Array-of-Structures layout.** `positions` is
`std::vector<std::array<double,3>>`, so x, y and z for one atom sit adjacent in
memory. The conventional wisdom is that Structure-of-Arrays (`x[]`, `y[]`,
`z[]`) is faster, and it is what a GPU needs for coalesced memory access.

Whether it is faster *here*, on a scalar CPU loop, is a question worth actually
measuring rather than assuming. Lesson 2 measures it. The answer is not the one
the conventional wisdom predicts.

**(A)+(E) The triangular loop with a two-sided write.** This is the subtle one,
and it is the reason parallelizing is a *design* decision rather than a pragma.

`i < j` visits each pair once and applies the result to both atoms via Newton's
third law — half the arithmetic, and the right call for serial code. But look at
the write:

```cpp
forces[i] -= f;    // i is fixed for this whole inner loop — safe
forces[j] += f;    // j ranges over everything — NOT safe
```

Hand the outer loop to two threads. Thread A holds `i=10`, thread B holds
`i=20`, and both eventually reach `j=999`. Both read `forces[999]`, both add to
it, both write it back. One update is silently lost.

Nothing crashes. You get a plausible-looking force array that is slightly wrong,
and 5000 steps later your "conserved" energy has drifted. Lesson 5 does this on
purpose so you can watch it happen.

## Where parallelism gets inserted

Not into the code above. That code stays exactly as it is, forever, as the
correctness baseline. The new path is a sibling:

```
LJPotential::calculateForces(geometry, forces)
    │
    ├─ buildView()              AoS + strings  →  flat SoA + integer type ids
    │                           (this is the marshalling; O(N), done once per call)
    │
    └─ ljComputeGather(view, backend, numThreads)      ← src/engine/potentials/lj_kernels.cpp
           │
           ├─ SerialTriangular   i<j scatter, fastest serial          (lesson 3)
           ├─ Serial             gather, one thread                  (lesson 3)
           ├─ Threads            gather, hand-rolled std::thread pool (lesson 4)
           ├─ OpenMP             gather, #pragma omp parallel for     (lesson 6)
           └─ [Cuda]             gather, one CUDA thread per atom     (lesson 9, not built)
```

The seam is `LJKernelView` in `lj_kernels.h`. It is deliberately made of
nothing but raw pointers, `int`s and `double`s:

```cpp
struct LJKernelView {
    const double *x, *y, *z;      // SoA positions, not vector<array<double,3>>
    const int    *type;           // integer ids, not std::string
    double       *fx, *fy, *fz;
    std::size_t   n;
    const double *eps4, *sigma2;  // flat ntypes×ntypes tables, mixing pre-applied
    int           ntypes;
    double        bx, by, bz, cutoff2;
};
```

That is not a style preference. It is precisely the subset of C++ that can be
handed to a CUDA kernel as device pointers. Every backend above — including the
one that does not exist yet — takes this same struct. Adding a GPU backend later
means adding a `.cu` file and one `case` label; no caller changes.

The alternative, `#ifdef __CUDACC__` scattered through `lj_potential.cpp`, is
how this kind of code becomes unreadable. Keeping the seam narrow is the whole
architectural bet.

## Parallelism is opt-in

The default is still serial:

```cpp
LJPotential::LJPotential() {
    backend = LJBackend::Serial;     // you must ask for parallelism
}
```

Three ways to ask:

```cpp
lj.setBackend(LJBackend::OpenMP);
```
```bash
MOLSIM_LJ_BACKEND=openmp ./build/run_simulation data/lj_bulk_simulation.yaml
```
```yaml
simulation:
  parallel:
    backend: openmp
```

## Try it now

```bash
cmake -B build -S . && cmake --build build --parallel
./build/benchmarks/bench_lj 4096 7
```

You should see all four backends agreeing on the energy to nine decimal places
and disagreeing wildly on speed. Everything that follows explains that table.

## Summary

- One loop, `LJPotential::calculateForces`, is essentially the entire runtime.
- It has four suspects: a per-pair `std::map<std::string>` lookup, an unnecessary
  `sqrt` (plus three divisions in the minimum-image convention), an AoS layout,
  and a two-sided write that races under threading.
- The first three are addressed by a *serial* rewrite that is also the GPU port.
  Which of them actually dominates is measured in lesson 2, and it is not the
  one most people pick.
- The fourth is a formulation choice, not an optimization — lesson 3.
- The original code is preserved as `calculateForcesReference()` and is the
  yardstick for everything else.

**Next:** [01_measure_first.md](01_measure_first.md) — before changing anything, learn to measure it.
