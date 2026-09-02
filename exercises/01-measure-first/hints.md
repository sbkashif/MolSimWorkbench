# Hints — Exercise 1

<details><summary>Hint 1 — try without this first</summary>

The warm-up is a bare `f();` with no clock around it. It is the easiest TODO and
the easiest to forget, and the test counts invocations specifically to catch it.

</details>

<details><summary>Hint 2</summary>

Seed your running minimum with something larger than any real measurement —
`std::numeric_limits<double>::max()` — then `best = std::min(best, elapsed)`
inside the loop.

Take `t0` and `t1` *inside* the loop, around each individual call. Timing the
whole loop and dividing gives you the mean, which is what we are avoiding.

</details>

<details><summary>Hint 3 — near-solution</summary>

```cpp
double timeMs(const std::function<void()>& f, int reps) {
    f();                              // warm-up, untimed
    if (reps < 1) reps = 1;
    double best = std::numeric_limits<double>::max();
    for (int r = 0; r < reps; ++r) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        f();
        const auto t1 = std::chrono::high_resolution_clock::now();
        best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    return best;
}
```

</details>
