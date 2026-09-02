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

## What the loop is actually computing

For every pair of atoms a distance *r* apart, the Lennard-Jones energy is

$$U(r) = 4\varepsilon\left[\left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^{6}\right]$$

and the force between them is the slope of that, with a minus sign:

$$F(r) = -\frac{dU}{dr} = \frac{24\varepsilon}{r}\left[2\left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^{6}\right]$$

The r⁻¹² term is the hard repulsion when atoms get too close; the r⁻⁶ term is
the gentle attraction that pulls them together. This repo uses reduced units,
ε = σ = 1, so those simplify to `4(r⁻¹² − r⁻⁶)` and `(24/r)(2r⁻¹² − r⁻⁶)`.

### Two actual atoms

Put atom A at the origin and atom B at r = 1.2:

```
(σ/r)⁶  = (1/1.2)⁶  = 0.334898
(σ/r)¹² = 0.334898² = 0.112157

U = 4 × (0.112157 − 0.334898) = −0.890965      attractive (negative)
F = (24/1.2) × (2×0.112157 − 0.334898) = −2.211693
```

Now walk B further away and watch what happens:

| r | U(r) | F(r) | |
|---:|---:|---:|---|
| 1.12 | −1.000 | 0.000 | the bottom of the well — atoms sit here |
| 1.2 | −0.891 | −2.212 | strong pull |
| 2.0 | −0.0615 | −0.182 | 14× weaker |
| 2.5 | −0.0163 | −0.039 | **the cutoff — we stop here** |
| 3.0 | −0.0055 | — | thrown away |

By r = 2.5 the energy is 1.6% of what it was at the bottom of the well. That is
why there is a cutoff: past that point you are doing full arithmetic to compute
almost nothing. **Remember that the code has to measure the distance before it
can know the answer is too small to matter.** That single fact is the key to
this whole chapter.

## What one worker actually does, per pair

Picture a single worker with a checklist. For **every** pair of atoms they must:

```
  1. subtract the two positions                    3 subtractions
  2. wrap around the box (nearest copy)            3 divisions, 3 roundings,
                                                   3 multiplications, 3 branches
  3. square and add up                             3 multiplications, 2 additions
  4. take the square root                          1 square root
  ─────────────────────────────────────────────────────────────────────
  5. is it closer than 2.5?  NO  → throw it away and start the next pair
                             YES → carry on:
  ─────────────────────────────────────────────────────────────────────
  6. look up ε and σ for this pair of atom types   2 std::map lookups + 1 sqrt
  7. compute the push and apply it                 the actual physics
```

At N = 4096 there are 4096 × 4095 / 2 = **8,386,560 pairs**. And of those, only
**113,910 — 1.36% — are closer than 2.5.**

So steps 1–5 run **8,386,560 times**.
Steps 6–7 run **113,910 times**.

**That is a ratio of 74 to 1.** Your worker spends the overwhelming majority of
their day carefully measuring the distance to atoms that turn out to be too far
away to care about, then throwing the measurement in the bin.

Anything you save in steps 1–5 you save 74 times over. Anything you save in
steps 6–7, you save once.

## Four suspects — deliberately not ranked

Here are the four things that could be costing you. They are listed in the order
they catch the eye, which is **not** the order that matters.

Ranking them is lesson 2's job, and it is done by measurement, because the
obvious answer is wrong. Form your own guess and write it down before you get
there. Hint: look again at which steps are above the line and which are below it.

**(D) The parameter lookup — step 6.**
`getMixedParameters` takes two atom names as text (`"LJ"`, `"O"`, `"H"`), looks
each one up in a `std::map`, and computes `sqrt(ε₁ × ε₂)` for the mixing rule.
For a single-component fluid the answer is *identical every single time*, so
this is a lot of work to rediscover a constant.

(A `std::map` is not an index into a list. It is a tree: to find `"LJ"` you start
at the top, compare text, follow a pointer to another node somewhere else in
memory, compare again, and so on until you land on it. Each hop can be a slow
trip to main memory. After the refactor this becomes `eps4row[type[j]]` — one
integer index straight into a contiguous array.)

This is the obvious suspect. Note carefully **which side of the cutoff test it
sits on** before you decide how much it costs.

Either way it has to go, because it is a hard blocker for the GPU: there is no
`std::map` and no text in a graphics-card kernel.

**(C) The square root and the divisions — steps 2 and 4.**
A square root for every pair, then used as a divisor three more times to make a
unit direction vector. Both the energy and the force can be written using r²
alone, so the square root is never actually needed.

And look at step 2: `dx / boxDim[0]` and its two friends. **Three divisions per
pair, before the cutoff test.** Division is one of the slowest things a processor
does — roughly 15–20× the cost of a multiplication, and unlike multiplication it
cannot be overlapped with other work.

**(B) How the positions are stored — everything.**
`positions[i]` currently keeps atom i's x, y and z together, side by side. The
usual advice is to split them into three separate lists (all the x's, then all
the y's, then all the z's), because that is what a graphics card needs.

Whether that is faster *here*, for one worker on a CPU, is worth measuring
rather than assuming. Lesson 2 measures it. The answer is not the one the usual
advice predicts.

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
- Of 8,386,560 pairs, only 113,910 (1.36%) are close enough to matter — but the
  code must measure every one to find that out. Work before the cutoff test is
  paid 74× more often than work after it.
- Four suspects: the text-keyed parameter lookup, the square root and the three
  divisions that wrap coordinates around the box, how positions are stored, and
  the two-sided write that breaks under multiple workers.
- The first three are addressed by a *serial* rewrite that is also the GPU port.
  Which of them actually dominates is measured in lesson 2, and it is not the
  one most people pick.
- The fourth is a formulation choice, not an optimization — lesson 3.
- The original code is preserved as `calculateForcesReference()` and is the
  yardstick for everything else.

**Next:** [01_measure_first.md](01_measure_first.md) — before changing anything, learn to measure it.
