# Hints — Exercise 7

<details><summary>Hint 1 — sanity-check the inversion</summary>

`amdahlSerialFraction(amdahlSpeedup(s, n), n)` must return `s` for any valid
`s` and `n >= 2`. Write that round trip yourself before running the tests; it
catches almost every algebra slip.

</details>

<details><summary>Hint 2 — chooseBaseline</summary>

Filter on `threads <= 1` first, then take the minimum `ms`. The test
deliberately includes a parallel entry that is faster than every serial one; if
you forget to filter, you will return it.

</details>

<details><summary>Hint 3 — what the study should show</summary>

Using the repo's numbers: 4 threads gives a speedup of ~3.74× over 1 thread, so
`s ≈ 0.024` and Amdahl caps the program at ~42×. Since you measured only ~4.2×
on 8 threads, Amdahl is **not** the limiting factor — heterogeneous cores are.
Being able to rule Amdahl out is as useful as being able to blame it.

</details>
