# Hints — Exercise 8

<details><summary>Hint 1 — the force</summary>

Zero. Exactly zero, for every input.

A uniform isotropic medium surrounding a point pulls equally in all directions,
so the contributions cancel by symmetry. The correction is a constant shift in
energy — and the gradient of a constant is zero.

</details>

<details><summary>Hint 2 — checking the energy</summary>

At ρ=0.8, N=4096, ε=σ=1, r_c=2.5, you should get about **−1754.5**, which is
roughly 8.7% of the truncated energy of −20105.5. If your answer is positive,
you have a sign error in `(sr9/3 − sr3)`; note that `sr3 > sr9/3` for
r_c > σ, so the bracket is negative.

</details>

<details><summary>Hint 3 — checking the pressure</summary>

`tailPressure` must be identical for N=512 and N=8000 at the same density —
it is a function of ρ and r_c only. At ρ=0.8, r_c=2.5 the value is about
**−0.6844**.

If yours changes with N, you have an extra factor of N: note that the formula
uses ρ², and ρ = N/V, so the N dependence has already cancelled.

</details>
