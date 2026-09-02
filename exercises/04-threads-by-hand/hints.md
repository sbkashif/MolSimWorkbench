# Hints — Exercise 4

<details><summary>Hint 1 — check your partition on paper</summary>

n=10, T=3. chunk=3, rem=1.

| t | begin | end | count |
|---|---|---|---|
| 0 | 0 + min(0,1)=0 | 0+3+1 = 4 | 4 |
| 1 | 3 + min(1,1)=4 | 4+3+0 = 7 | 3 |
| 2 | 6 + min(2,1)=7 | 7+3+0 = 10 | 3 |

Covers 0–9 exactly once, 4+3+3 = 10. The naive version would give 3+3+3 = 9.

</details>

<details><summary>Hint 2 — the lambda capture</summary>

```cpp
pool.emplace_back([&, begin, end, t]() {   // begin/end/t BY VALUE
    partial[t] = gatherRange(begin, end);
});
```

`begin`, `end` and `t` change every iteration, so they must be copied. `partial`
and the position arrays are captured by reference deliberately — they outlive
the join and are not mutated by the spawning loop.

</details>

<details><summary>Hint 3 — structure</summary>

Write the per-range kernel as a lambda or static function first, test it
single-threaded (T=1 must equal exercise 3's answer), and only then wrap it in
threads. Debugging a parallel bug and a physics bug at once is twice the work.

</details>
