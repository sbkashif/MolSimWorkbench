# 02 — LJ Implementation: From Equation to Code

## The equation you know

$$V_{LJ}(r) = 4\varepsilon \left[ \left(\frac{\sigma}{r}\right)^{12} - \left(\frac{\sigma}{r}\right)^{6} \right]$$

The force on atom $i$ due to atom $j$ is $\mathbf{F}_i = -\nabla_i V_{LJ}$, which for a pair along unit vector $\hat{r}_{ij}$ gives:

$$F(r) = \frac{48\varepsilon}{r} \left[ \left(\frac{\sigma}{r}\right)^{12} - \frac{1}{2}\left(\frac{\sigma}{r}\right)^{6} \right]$$

Now let's see every piece of that in `src/engine/potentials/lj_potential.cpp`.

---

## Class structure

`LJPotential` inherits from the abstract `Potential` base class defined in `src/engine/force_calculator.h`. The two methods it must implement are:

```cpp
double calculateEnergy(const Geometry& geometry);
void   calculateForces(const Geometry& geometry, std::vector<std::array<double,3>>& forces);
```

The class stores per-atom-type parameters:

```cpp
struct AtomParameters { double epsilon; double sigma; };
std::map<std::string, AtomParameters> atomParameters;
double cutoff = 10.0;          // default, overridden by config
std::string mixingRule = "geometric";
```

For the LJ-bulk run those are loaded from `data/potentials/lj_bulk.yaml`:

```yaml
atom_parameters:
  "LJ": { epsilon: 1.0, sigma: 1.0 }
cutoff: 2.5   # in reduced units
mixing_rule: "geometric"
```

---

## Pair energy — `calculatePairEnergy()`

This is the direct translation of the LJ equation:

```cpp
double LJPotential::calculatePairEnergy(double epsilon, double sigma, double distance) {
    double sr6  = std::pow(sigma / distance, 6);
    double sr12 = sr6 * sr6;
    return 4.0 * epsilon * (sr12 - sr6);
}
```

`sr6` is $(\sigma/r)^6$, `sr12` is $(\sigma/r)^{12}$. Straightforward.

---

## Pair force — `calculatePairForce()`

```cpp
void LJPotential::calculatePairForce(double epsilon, double sigma, double distance,
                                     const std::array<double,3>& direction,
                                     std::array<double,3>& force1,
                                     std::array<double,3>& force2) {
    double sr6      = std::pow(sigma / distance, 6);
    double sr12     = sr6 * sr6;
    double forceMag = 48.0 * epsilon * (sr12 - 0.5 * sr6) / (distance * distance);

    for (int d = 0; d < 3; ++d) {
        double f   = forceMag * direction[d];
        force1[d] += f;    // atom i pushed away from j
        force2[d] -= f;    // Newton's third law
    }
}
```

`direction` is the vector $\mathbf{r}_j - \mathbf{r}_i$ (unnormalised). Dividing by $r^2$ rather than $r$ absorbs one power of $r$ from the normalisation, so the formula matches $F(r)/r \cdot \hat{r}$ correctly.

Newton's third law is applied in the same call: `force2[d] -= f`. This means the double-loop only needs to run over $i < j$.

---

## Cutoff

Every pair loop starts with:

```cpp
if (distance > cutoff || distance < 1e-10) continue;
```

`cutoff = 2.5` in reduced units. Pairs beyond that distance contribute nothing. The `1e-10` guard prevents division by zero if two particles overlap.

The potential is **not** shifted to zero at the cutoff in the current implementation (`shift_potential: false` in the config). This means there is a small discontinuity in energy at $r = r_c$, which is acceptable for bulk NVE benchmarks but would need addressing for precise thermodynamics.

---

## Periodic boundary conditions — minimum image convention

In a simulation box with PBC you don't want to compare $i$ to all images of $j$; you just want the nearest image. The code applies the **minimum image convention** before computing distance:

```cpp
if (boxDim[0] > 0.0) dx -= boxDim[0] * std::round(dx / boxDim[0]);
if (boxDim[1] > 0.0) dy -= boxDim[1] * std::round(dy / boxDim[1]);
if (boxDim[2] > 0.0) dz -= boxDim[2] * std::round(dz / boxDim[2]);
```

`std::round(dx / L)` gives the nearest integer, i.e. which box image is closest. Subtracting `n * L` folds the displacement back into $[-L/2, +L/2]$.

This block appears identically in both `calculateEnergy()` and `calculateForces()`. If `boxDim` is zero (no PBC), the `if` guards skip it entirely.

---

## Mixing rules

When two *different* atom types interact, you need combined $\varepsilon$ and $\sigma$. The method `getMixedParameters()` handles this:

```cpp
// geometric (default):
epsilon_mix = sqrt(eps_i * eps_j);
sigma_mix   = sqrt(sig_i * sig_j);

// arithmetic (Lorentz-Berthelot):
epsilon_mix = sqrt(eps_i * eps_j);
sigma_mix   = 0.5 * (sig_i + sig_j);
```

For the LJ-bulk simulation there is only one atom type (`"LJ"`) with $\varepsilon = \sigma = 1$, so mixing rules are irrelevant in practice. They matter once you add water (O + H) or heterogeneous systems.

---

## What is NOT yet implemented

Looking at the YAML config and the code comments:
- **Tail corrections** — long-range correction integral beyond cutoff (`tail_correction: false`)
- **Neighbor lists** — the config mentions `neighbor_list_radius` and `update_frequency` but the code still uses an $O(N^2)$ pair loop
- **Potential shift** — `shift_potential: false`; adding this would eliminate the energy discontinuity at cutoff

These are the natural next performance/accuracy improvements flagged in the codebase.

---

## Key files

| File | Role |
|------|------|
| [src/engine/potentials/lj_potential.h](../../src/engine/potentials/lj_potential.h) | Class declaration, public API |
| [src/engine/potentials/lj_potential.cpp](../../src/engine/potentials/lj_potential.cpp) | Full implementation |
| [src/engine/force_calculator.h](../../src/engine/force_calculator.h) | `Potential` base class |
| [data/potentials/lj_bulk.yaml](../../data/potentials/lj_bulk.yaml) | LJ params for bulk run |

---

Next: [03_integrator_and_md.md](03_integrator_and_md.md) — how the forces get turned into motion.
