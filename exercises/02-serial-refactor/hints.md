# Hints — Exercise 2

<details><summary>Hint 1 — the branchless minimum image</summary>

If `bx == 0` then `invbx == 0`, so `std::round(dx * 0.0)` is `0.0`, and
`dx -= bx * 0.0` leaves `dx` unchanged. The non-periodic case falls out of the
arithmetic. No `if` required.

</details>

<details><summary>Hint 2 — why 6.0 * eps4</summary>

The force magnitude is `24ε(2(σ/r)¹² − (σ/r)⁶)/r`. You stored `eps4 = 4ε`, and
`24ε = 6 · 4ε = 6 · eps4`.

You want a vector, not a magnitude. `F⃗ = F · r̂ = F · r⃗/r`, so
`F⃗ = (F/r) · r⃗`. Dividing by `r2` instead of `r` gives you exactly that extra
`1/r` for free — which is why no square root and no normalisation are needed.

</details>

<details><summary>Hint 3 — the loop skeleton</summary>

```cpp
const double invbx = (bx > 0.0) ? 1.0/bx : 0.0;   // + by, bz
const double cutoff2 = cutoff * cutoff;
double energy = 0.0;

for (std::size_t i = 0; i < n; ++i) {
    const double xi = x[i], yi = y[i], zi = z[i];
    const double* e4row = table.eps4.data()   + type[i] * table.ntypes;
    const double* s2row = table.sigma2.data() + type[i] * table.ntypes;
    for (std::size_t j = i + 1; j < n; ++j) {
        double dx = x[j] - xi, dy = y[j] - yi, dz = z[j] - zi;
        dx -= bx * std::round(dx * invbx);
        dy -= by * std::round(dy * invby);
        dz -= bz * std::round(dz * invbz);
        const double r2 = dx*dx + dy*dy + dz*dz;
        if (r2 > cutoff2 || r2 < 1e-20) continue;
        const double e4 = e4row[type[j]], s2 = s2row[type[j]];
        const double sr2 = s2/r2, sr6 = sr2*sr2*sr2, sr12 = sr6*sr6;
        energy += e4 * (sr12 - sr6);
        const double fo = 6.0*e4*(2.0*sr12 - sr6)/r2;
        fx[i] -= fo*dx; fy[i] -= fo*dy; fz[i] -= fo*dz;
        fx[j] += fo*dx; fy[j] += fo*dy; fz[j] += fo*dz;
    }
}
return energy;
```

</details>
