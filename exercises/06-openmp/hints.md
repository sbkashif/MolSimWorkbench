# Hints — Exercise 6

<details><summary>Hint 1 — avoid copy-pasting the body four times</summary>

Write the per-atom work as a lambda returning that atom's energy contribution
and writing its own force:

```cpp
auto body = [&](std::ptrdiff_t i) -> double { /* ... */ return ei; };
```

then each `case` is a three-line pragma + loop calling `body(i)`. One kernel,
four schedules — and if you find a bug you fix it once.

</details>

<details><summary>Hint 2 — signed index</summary>

`for (std::ptrdiff_t i = 0; i < (std::ptrdiff_t)n; ++i)`. Unsigned loop
variables are only allowed from OpenMP 3.0 and some compilers still object.

</details>

<details><summary>Hint 3 — what the schedule study should show</summary>

On a machine with 4 performance and 4 efficiency cores you should see
`schedule(static)` scale nearly perfectly to 4 threads, then get *worse* at 5,
while `guided` keeps improving. If all your cores are identical you will not see
the knee — which is a valid result, and you should say so in your answer.

</details>
