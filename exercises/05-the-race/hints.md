# Hints — Exercise 5

<details><summary>Hint 1 — the racy version is the easy one</summary>

Take exercise 2's triangular kernel, change the loop index to
`std::ptrdiff_t`, and put the pragma on the outer loop. Change nothing else.
That is the whole task — the bug writes itself.

</details>

<details><summary>Hint 2 — private buffers</summary>

```cpp
std::vector<std::vector<double>> bufX(numThreads, std::vector<double>(n, 0.0));
// ... bufY, bufZ, and std::vector<double> partial(numThreads, 0.0)

#pragma omp parallel num_threads(numThreads)
{
    const int t = omp_get_thread_num();
    double* px = bufX[t].data();  // etc.
    double e = 0.0;
    #pragma omp for schedule(static, 16) nowait
    for (std::ptrdiff_t i = 0; i < (std::ptrdiff_t)n; ++i)
        for (std::ptrdiff_t j = i+1; j < (std::ptrdiff_t)n; ++j) {
            /* ... */
            px[i] -= fo*dx;   // this thread's buffer
            px[j] += fo*dx;   // also this thread's buffer -- safe
        }
    partial[t] = e;
}
// reduce
for (std::size_t i = 0; i < n; ++i) {
    double ax = 0;
    for (int t = 0; t < numThreads; ++t) ax += bufX[t][i];
    fx[i] = ax;
}
```

</details>

<details><summary>Hint 3 — what you should see in the demo</summary>

At N=4096 on 8 threads, roughly 2% of atoms come out wrong, the maximum relative
error is enormous (100× or more on individual components), the total energy
looks fine, the net force is off by only ~10⁻⁵ of the total, and the racy
version is the **fastest** of the lot.

If your race produces *no* errors, raise N or the thread count — the collision
probability has to be high enough to bite.

</details>
