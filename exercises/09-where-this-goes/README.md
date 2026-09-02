# Exercise 9: No Code

[Lesson 9](../../docs/learn-parallel/09_where_this_goes.md) is read-only — there
is no CUDA toolkit on this machine and the course does not need one.

Work through these on paper instead. They are all answerable from measurements
you have already taken.

### 1. Launch configuration

You have N = 4096 atoms and choose 256 threads per block.
How many blocks? Write the general expression for arbitrary N, and say what goes
wrong if you use integer division without rounding up.
(You solved the same problem in exercise 4 — it is the remainder again.)

### 2. Coalescing

A warp of 32 threads runs `i = 0..31` and each reads position `j = 100`.
Then each reads `j = 101`, and so on.

- With AoS (`struct {double x,y,z;} pos[]`), what byte addresses does the warp
  touch when reading the x components of `j = 100..131`? What is the stride?
- With SoA (`double x[]`), same question.
- Which one lets the hardware merge the requests into fewer transactions?
- Lesson 2 measured SoA as **13% slower** on this CPU. Reconcile that with your
  answer.

### 3. The reduction

`#pragma omp parallel for reduction(+:e)` is one clause. Sketch the equivalent
CUDA block reduction in `__shared__` memory.

- How many `__syncthreads()` calls for a 256-thread block?
- Why `atomicAdd` once per *block* rather than once per thread?
- Is the result deterministic? Compare with lessons 5 and 6.

### 4. The transfer trap

A 5000-step run at N=4096 copying positions and forces each way every step.
That is 4096 × 3 × 8 bytes each way per step.

- Total bytes over PCIe?
- At ~10 GB/s effective, how long is that, and how does it compare to 5000 kernel
  launches at ~0.1 ms?
- What has to move onto the device to avoid it?

### 5. The one that matters

Only **1.36%** of pairs fall inside the cutoff (lesson 2). Threading bought
**4.03×** (lesson 7).

- Roughly what would a cell list buy on a single core?
- So which should you implement first, and why is this course's kernel still the
  right thing to have learned on?
- `CLAUDE.md` lists neighbour lists as deliberate omitted scope. Argue both sides
  in three sentences.

### 6. Back to GROMACS

You said at the start that beyond the cutoff there are long-range corrections.

- Why does this repo need only the O(1) dispersion correction, and no PME?
- PME is a 3-D FFT. Why does GROMACS put it on *separate ranks* from the
  short-range work, and which of the properties you measured in lessons 3, 5 and
  7 explains that?
