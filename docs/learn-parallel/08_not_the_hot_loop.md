# Lesson 8: What Does Not Belong in the Hot Loop

> **Where we are.** Lesson 7 settled the honest speedup of the pair loop: 4.03× on 8 threads. This lesson steps out of that loop and adds a term that costs almost nothing, to show that not every improvement belongs in the parallel kernel.

## Learning Objectives

- Derive why the dispersion tail correction contributes zero force
- Classify work by complexity before deciding what to parallelize
- Add a physically significant term without touching the parallel kernel
- Recognise when the uniform-density assumption behind it fails

## Background

Seven lessons of "make the loop faster" can leave you thinking every improvement
is a loop improvement. This lesson adds the single most physically significant
missing term in the repo — worth **8.73% of the total potential energy** — and it
never goes near the parallel kernel at all.

Knowing which work is O(N²), which is O(N), and which is O(1) is what tells you
where optimisation effort belongs. It is the same skill as lesson 2's pre-cutoff
vs post-cutoff split, one level up.

## Core Concepts

### Truncation is not free physics

The kernel discards every interaction beyond r_c = 2.5σ. Those interactions are
individually tiny — at r = 2.5σ the LJ energy is about −0.0163ε — but there are
a great many of them, and they are *all attractive*. The number of neighbours in
a shell grows as r², while the attractive tail decays as r⁻⁶, so the integrand
falls as r⁻⁴: convergent, but not negligible.

The standard fix assumes the fluid is uniform and isotropic beyond the cutoff —
g(r) = 1 for r > r_c — which turns the neglected sum into an integral with a
closed form:

$$E_{\text{tail}} = 2\pi N \rho \int_{r_c}^{\infty} r^2\, u(r)\, dr
= \frac{8}{3}\pi N \rho \varepsilon \sigma^3
\left[\frac{1}{3}\left(\frac{\sigma}{r_c}\right)^{9} - \left(\frac{\sigma}{r_c}\right)^{3}\right]$$

with a matching virial term for the pressure:

$$P_{\text{tail}} = \frac{16}{3}\pi \rho^2 \varepsilon \sigma^3
\left[\frac{2}{3}\left(\frac{\sigma}{r_c}\right)^{9} - \left(\frac{\sigma}{r_c}\right)^{3}\right]$$

This is GROMACS's `DispCorr = EnerPres`.

Measured on this repo at ρ = 0.8, r_c = 2.5σ:

| N | E (truncated) | E_tail | share | P_tail | cost |
|---:|---:|---:|---:|---:|---:|
| 512 | −2513.29 | −219.31 | 8.73% | −0.6844 | 0.012 ms |
| 1000 | −4914.96 | −428.35 | 8.72% | −0.6844 | 0.026 ms |
| 4096 | −20105.49 | −1754.51 | 8.73% | −0.6844 | 0.089 ms |
| 8000 | −39261.20 | −3426.77 | 8.73% | −0.6844 | 0.159 ms |

**Nearly 9% of the potential energy** was simply missing. That is far larger than
any floating-point concern in this course, and larger than most of the speedups.

Two sanity checks worth noticing: the share is constant with N (both E and E_tail
scale linearly at fixed density), and **P_tail does not depend on N at all** —
it is a function of ρ and r_c only, exactly as the formula says.

### The force contribution is exactly zero

This is the part that matters for parallelization, and it is worth being able to
derive rather than memorise.

Consider an atom at the origin and the region beyond r_c. Under the assumption
g(r) = 1, the material out there is **uniform and isotropic** — the same density
in every direction. Take any thin shell at radius r > r_c: the patch at
direction **n̂** pulls the atom one way, and the diametrically opposite patch at
**−n̂**, containing identically as much material at the same distance, pulls
exactly as hard the other way.

$$\vec{F}_{\text{tail}} = -\nabla \left(\text{something independent of position}\right) = \vec{0}$$

Every direction cancels. The correction shifts the energy by a constant and
changes the pressure, but exerts **no net force on anything**.

So it is not that we chose to leave it out of the kernel — it is not *allowed*
in the kernel:

```cpp
double LJPotential::calculateForcesAndEnergy(const Geometry& geometry, ...) {
    ...
    const double energy = ljComputeGather(view, backend, numThreads);
    for (std::size_t i = 0; i < n; ++i) {
        forces[i] = {sfx[i], sfy[i], sfz[i]};    // forces: nothing added. Ever.
    }
    return energy + tailCorrectionEnergy(geometry);   // energy: yes
}
```

(`calculateForces` just calls this, and `calculateEnergy` adds the same term
to its own kernel call.)

And the test asserts it *bitwise*, not approximately:

```cpp
TEST(LJTailCorrection, DoesNotChangeAnyForce) {
    ...
    EXPECT_DOUBLE_EQ(without[i][k], with[i][k]);
}
```

Because the forces are untouched, the **trajectory is untouched**. Turning the
correction on changes reported energies and pressures, not dynamics. In an NVT
or NVE run the atoms move identically. It matters when you compute an equation
of state, a phase boundary, or a surface tension — quantities where 9% of the
energy is the whole answer.

### Complexity is the thing to look at first

| Work | Complexity | Calls at N=4096 | Time | Parallelize? |
|---|---|---:|---:|---|
| pair loop | O(N²) | 8,386,560 | 3.77 ms | **yes** |
| AoS→SoA marshalling | O(N) | 4,096 | ~0.02 ms | no |
| tail correction | O(N) count + O(T²) formula | 4,096 | 0.089 ms | no |
| parameter table build | O(T²), once | 1 | ~0 | no |

The tail correction costs **2.4% of the kernel time** for **8.73% of the energy**.
Threading it would be pure loss: the fork/join overhead alone (tens of µs, from
lesson 1) is comparable to the entire computation.

That is the transferable point. Before optimising anything, ask how often it
runs. Lesson 2 found a 74:1 ratio *inside* one loop; this is the same question
asked about the program. **An O(1) term is not worth a thread, however important
the physics it carries.**

(The 0.089 ms here is technically O(N), because counting atoms per type walks
the array. Caching the counts would make it genuinely O(1). It is not worth
doing at 2.4%, which is itself the lesson.)

### When the assumption breaks (deep dive later)

The correction rests on g(r) = 1 beyond r_c: uniform density, isotropic. That
holds for a bulk homogeneous fluid, which is exactly what
`LJParticleBoxBuilder::createCubicLattice` produces. It fails for:

- **Interfaces** — a liquid/vapour surface has density varying along one axis,
  so the cancellation argument fails and there *is* a net force. This is why
  surface tension calculations need care.
- **Membranes, slabs, anything inhomogeneous** — same problem.
- **Small cutoffs**, where g(r) is still structured at r_c and is nowhere near 1.

For those, GROMACS offers LJ-PME (`vdwtype = PME`), which handles the long-range
dispersion on a mesh instead of assuming uniformity. That has a completely
different parallelization profile — it is an FFT, and FFTs are communication
bound rather than compute bound. Lesson 9 picks that up.

**This is why the correction is opt-in and off by default.** It is right for a
bulk box and wrong for an interface, and the code cannot tell which you have.

```cpp
lj.setTailCorrection(true);    // you assert the fluid is homogeneous
```

## Common Pitfalls

- **Adding a tail force.** There isn't one. If your correction changes a
  trajectory, it is wrong.
- **Using it at an interface.** The uniform-density assumption is the whole
  derivation; without it the term is meaningless.
- **Double counting.** The sum over ordered type pairs (a,b) already counts each
  unordered pair twice, which the 2π prefactor accounts for. Check against the
  single-component closed form — that is what
  `LJTailCorrection.MatchesTheAnalyticFormula` does.
- **Parallelizing it.** 0.089 ms. The fork/join costs more than the work.

## Summary

- The truncated dispersion tail was worth 8.73% of the potential energy.
- Its force contribution is exactly zero by isotropy, so it never enters the
  pair loop — asserted bitwise in the tests.
- It costs 2.4% of the kernel time and must not be parallelized.
- Classify work by complexity *before* optimising: O(N²) deserves threads,
  O(N) and O(1) do not.
- It is opt-in because it is valid for bulk fluids and invalid at interfaces.

## Further Reading

- Allen & Tildesley, *Computer Simulation of Liquids*, 2nd ed., §2.8
- [GROMACS manual — dispersion correction](https://manual.gromacs.org/current/reference-manual/functions/long-range-vdw.html)
- Exercise: `exercises/08-tail-correction/`

**Next:** [09_where_this_goes.md](09_where_this_goes.md) — the CPU story is complete. Lesson 9 answers: what does this kernel become on a GPU, and why do production codes like GROMACS look the way they do?
