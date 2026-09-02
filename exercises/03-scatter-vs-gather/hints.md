# Hints — Exercise 3

<details><summary>Hint 1</summary>

The inner loop is identical to exercise 2's, except that `j` starts at 0 rather
than `i+1`, you need `if (j == i) continue;`, and there is no `f[j]` write.

</details>

<details><summary>Hint 2 — the energy factor</summary>

Pair (3,7) is evaluated when `i=3, j=7` and again when `i=7, j=3`. Both add the
same energy, so your accumulator ends up at exactly twice the true value.
Return `0.5 * accumulated`.

The forces do *not* need halving: when `i=3` you compute the force on atom 3
from atom 7, and when `i=7` you compute the force on atom 7 from atom 3. Those
are different numbers, and each is written once. Only the scalar energy is
double counted.

</details>

<details><summary>Hint 3 — the ratio</summary>

`n(n-1) / (n(n-1)/2) = 2` exactly, for every n ≥ 2. Gather always does precisely
twice the pair evaluations. Return `2.0` (and something sensible for n < 2).

</details>
