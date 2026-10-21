# Lesson 9: Where This Goes

> ### In plain English
>
> Two things happen here, and neither needs anything installed.
>
> First: the graphics-card version of this kernel is almost the same code. You
> delete the outer loop and replace it with "which atom am I?" — because
> chapter 3 already restructured it so each worker owns its own atoms. That
> restructuring was the port; everything else is mechanical.
>
> Second: where your toy kernel sits relative to real MD software. The short
> version is that the biggest win available here is not parallelism at all — the
> loop checks 8.4 million pairs of atoms and only 114,000 of them are close
> enough to matter. Skipping the other 98.6% would beat all eight of your cores
> using one. That is what real codes do, and it is deliberately not built here.
>
> New words here: **kernel**, **warp**, **coalescing**, **shared memory**,
> **cell list** — see [GLOSSARY.md](GLOSSARY.md).

> **Where we are.** Lessons 0–8 took one CPU loop from 34.85 ms to 3.77 ms and showed which parts of that were serial, parallel, and physics. This lesson widens out: what the same kernel looks like on a GPU, and where it sits in production MD codes.

> **Read-only.** Nothing in this lesson is built and nothing needs installing.
> There is no CUDA toolkit on this machine and the course does not require one.
> The point is to show that the work you have already done is most of the port,
> and to place your toy kernel on the map of how production MD is actually built.

## Learning Objectives

- Translate the gather kernel into CUDA and see how little changes
- Explain why SoA finally pays off, after costing 13% on CPU
- Describe a block reduction and why it is the hard part
- Locate this kernel inside GROMACS's architecture, and see why that architecture exists

## Part 1: The kernel ports almost verbatim

Here is `computeRange` from `lj_kernels.cpp`, and the same thing as a CUDA
kernel. The differences are marked; there are three.

```cpp
// ---- CPU (what you have) --------------------------------------------------
double computeRange(const LJKernelView& v, size_t i0, size_t i1) {
    double energy = 0.0;
    for (size_t i = i0; i < i1; ++i) {                      // (1)
        double fxi = 0, fyi = 0, fzi = 0;
        const double xi = v.x[i], yi = v.y[i], zi = v.z[i];
        for (size_t j = 0; j < v.n; ++j) {
            if (j == i) continue;
            double dx = v.x[j] - xi, dy = v.y[j] - yi, dz = v.z[j] - zi;
            dx -= v.bx * std::round(dx * invbx);            // (2)
            /* ... */
            const double r2 = dx*dx + dy*dy + dz*dz;
            if (r2 > v.cutoff2 || r2 < 1e-20) continue;
            const double sr2 = sig2/r2, sr6 = sr2*sr2*sr2, sr12 = sr6*sr6;
            const double fOverR = 6.0*eps4*(2.0*sr12 - sr6)/r2;
            fxi -= fOverR*dx; fyi -= fOverR*dy; fzi -= fOverR*dz;
        }
        v.fx[i] = fxi; v.fy[i] = fyi; v.fz[i] = fzi;
        energy += /* ... */;                                // (3)
    }
    return energy;
}

// ---- CUDA (what it becomes) -----------------------------------------------
__global__ void ljGatherKernel(LJKernelView v, double* blockEnergy) {
    const size_t i = blockIdx.x * blockDim.x + threadIdx.x;   // (1) no outer loop
    if (i >= v.n) return;                                     //     bounds guard

    double fxi = 0, fyi = 0, fzi = 0;
    const double xi = v.x[i], yi = v.y[i], zi = v.z[i];
    for (size_t j = 0; j < v.n; ++j) {
        if (j == i) continue;
        double dx = v.x[j] - xi, dy = v.y[j] - yi, dz = v.z[j] - zi;
        dx -= v.bx * rint(dx * invbx);                        // (2) rint, not std::round
        /* ... IDENTICAL ... */
        fxi -= fOverR*dx; fyi -= fOverR*dy; fzi -= fOverR*dz;
    }
    v.fx[i] = fxi; v.fy[i] = fyi; v.fz[i] = fzi;
    blockReduceSum(ei, blockEnergy);                          // (3) the hard part
}
```

1. **The outer loop becomes the thread index.** This is the entire structural
   change, and it only works because lesson 3 chose the gather form. A
   triangular `i<j` loop has no such translation.
2. **`std::round` → `rint`.** Cosmetic. CUDA provides `rint`/`__drcp_rn`; the
   arithmetic is the same.
3. **The reduction is real work.** Everything else was free; this is not.

Launching it:

```cpp
const int threadsPerBlock = 256;                     // = num_threads(T)
const int blocks = (n + threadsPerBlock - 1) / threadsPerBlock;   // = ceil, like lesson 4's remainder handling
ljGatherKernel<<<blocks, threadsPerBlock>>>(deviceView, dBlockEnergy);
cudaDeviceSynchronize();                             // = th.join()
```

You have written all three of those lines before, in lesson 4, in C++.

### Adding it to this repo

The seam is already in place, which was the entire point:

```
src/engine/potentials/
    lj_kernels.h      ← unchanged: LJKernelView is already POD
    lj_kernels.cpp    ← unchanged
    lj_kernels.cu     ← new sibling
```

`LJBackend` gains a `Cuda` enum value, `ljComputeGather` gains a `case`, and
CMake gains an `enable_language(CUDA)` behind an `option()`. **No caller
changes. No `#ifdef` inside `lj_potential.cpp`.** That is what the narrow POD
seam bought.

## Part 2: Where SoA finally pays

Lesson 2 measured structure-of-arrays as **13% slower** on CPU and kept it
anyway, promising it would pay off here. This is the payoff.

A GPU does not issue loads per thread; it issues them per **warp** — 32 threads
in lockstep. When those 32 threads request 32 *contiguous* addresses, the
hardware merges them into a few wide memory transactions. That is *coalescing*,
and it is the single most important performance property of a CUDA kernel.

- **AoS:** thread i reads `pos[j].x` at byte offset `24*j`. Consecutive threads
  read addresses 24 bytes apart — strided, so the hardware fetches 32 separate
  cache lines and discards two-thirds of every one.
- **SoA:** thread i reads `x[j]` at offset `8*j`. Consecutive threads read
  consecutive doubles — one contiguous run, fully coalesced.

The layout that cost 13% on a scalar CPU loop is worth several-fold on a GPU.
This is the clearest example in the course of an architectural decision that is
correct *only* when you know where the code is going.

### Shared-memory tiling (deep dive later)

The naive kernel reads all N positions from global memory for every one of the N
threads — N² global reads. But every thread in a block wants the *same* j values.
So load a tile of positions into `__shared__` memory once per block and let all
256 threads read from there:

```cpp
__shared__ double sx[256], sy[256], sz[256];
for (int tile = 0; tile < gridDim.x; ++tile) {
    sx[threadIdx.x] = v.x[tile*blockDim.x + threadIdx.x];   // coalesced load
    /* sy, sz */
    __syncthreads();                                        // tile is ready
    for (int k = 0; k < blockDim.x; ++k) { /* use sx[k], sy[k], sz[k] */ }
    __syncthreads();                                        // before overwriting
}
```

Global traffic drops by a factor of `blockDim.x`. This is the canonical N-body
optimisation from *GPU Gems 3*, chapter 31, and it is why all-pairs N-body is the
standard GPU teaching kernel — it maps onto the hardware almost too well.

### The block reduction (deep dive later)

Summing the energy is where CUDA stops resembling OpenMP. `reduction(+:acc)` was
one word. On a GPU there is no global `+=` across 100,000 threads; you build a
tree:

```cpp
__device__ void blockReduceSum(double val, double* out) {
    __shared__ double s[256];
    s[threadIdx.x] = val;
    __syncthreads();
    for (int stride = blockDim.x/2; stride > 0; stride >>= 1) {
        if (threadIdx.x < stride) s[threadIdx.x] += s[threadIdx.x + stride];
        __syncthreads();
    }
    if (threadIdx.x == 0) atomicAdd(out, s[0]);    // one atomic per BLOCK
}
```

Log₂(256) = 8 steps instead of 256 serial adds, and one atomic per block rather
than per thread. Note that this is **not deterministic** — blocks finish in
arbitrary order and `atomicAdd` on doubles sums in that order. Exactly the
lesson-6 caveat about `reduction(+:)`, with sharper teeth.

### Do not memcpy every timestep

The trap that ruins naive GPU ports:

```cpp
for (int step = 0; step < 5000; ++step) {
    cudaMemcpy(d_pos, h_pos, ..., cudaMemcpyHostToDevice);   // ✗ 5000 times
    ljGatherKernel<<<...>>>(...);
    cudaMemcpy(h_force, d_force, ..., cudaMemcpyDeviceToHost);
    integrate(h_pos, h_force);                               // on the CPU
}
```

PCIe is roughly 100× slower than device memory. Copying 4096 atoms twice per step
can easily cost more than the kernel. The fix is to **keep the data resident** and
move the integrator onto the device too:

```cpp
cudaMemcpy(d_pos, h_pos, ..., HostToDevice);          // once
for (int step = 0; step < 5000; ++step) {
    ljGatherKernel<<<...>>>(...);
    integrateKernel<<<...>>>(...);                    // Verlet on the GPU
    if (step % outputFreq == 0)
        cudaMemcpy(h_pos, d_pos, ..., DeviceToHost);  // only on output frames
}
```

The CPU version of this idea is already in the repo: `sx`, `sy`, `sz` are
`LJPotential` *members*, not locals, so a 5000-step run allocates them once. Same
principle, milder stakes.

### If you do run it on Perlmutter (deep dive later)

NERSC Perlmutter GPU nodes carry 4 NVIDIA A100s (Ampere, compute capability 8.0),
40 or 80 GB HBM2e each. Compile for `-arch=sm_80`; the default `cudatoolkit`
module is currently 13.x. Profile with Nsight Systems for the timeline and Nsight
Compute for per-kernel occupancy and memory throughput. Check the current module
names in the NERSC docs rather than trusting this paragraph — they move.

## Part 3: How GROMACS is actually built

Your kernel is a teaching artefact. It is worth knowing exactly how it differs
from production, because the differences are all consequences of things you have
now measured.

### The elephant: 98.6% of your work is wasted

From lesson 2, at ρ=0.8 and r_c=2.5σ:

```
pairs evaluated:   8,386,560
pairs inside cutoff:  113,910   (1.36%)
```

**A cell list would beat all 8 threads on one core.** Bin atoms into cells of
side ≥ r_c, and each atom only tests the 27 neighbouring cells — O(N) instead of
O(N²). At N=4096 that is a ~74× reduction in pair tests, against the 4× you got
from threading.

`CLAUDE.md` marks neighbour lists as deliberate omitted scope, and this course
respects that. But you should hold the ranking clearly:

> **Algorithm beats parallelism.** A better algorithm on one core beat eight
> cores running a worse one. Parallelize *after* you have the right algorithm,
> not instead of finding it.

All-pairs is the right *teaching* kernel — it is the canonical GPU example, it
has no data structure to distract from the parallelism, and it is what the NERSC
notebooks use. It is not the right production kernel.

### What GROMACS does instead

**Verlet buffered cluster pair lists.** Not atom-atom lists but *cluster*-cluster
lists, typically 4×4 or 8×8 atoms, sized so a cluster pair maps onto a SIMD width
or a GPU warp. A buffer (`verlet-buffer-tolerance`) is added to the cutoff so the
list survives several steps before rebuilding (`nstlist`). This is the
cell-list idea, reshaped so that the *inner loop is a fixed-size block* the
hardware can vectorize — which is why it beats a naive neighbour list on both CPU
and GPU.

**PP / PME rank separation.** Short-range particle-particle work and long-range
electrostatics have incompatible parallel profiles:

| | short-range (PP) | long-range (PME) |
|---|---|---|
| algorithm | cluster pair list | 3-D FFT on a mesh |
| scales with | local atom count | global grid, all-to-all |
| bottleneck | arithmetic | **communication** |
| GPU | excellent | awkward (FFT transposes) |

An FFT needs all-to-all communication; a pair loop needs almost none. Run them on
the same ranks and the FFT's communication stalls the pair loop. So GROMACS
assigns *separate ranks* to PME (`-npme`), letting each scale on its own terms.

This repo has no charges, so there is no PME — the long-range term that applies
here is lesson 8's dispersion correction, which is O(1) and free. **Your kernel
is the PP half, with the pair list removed.**

**Multi-level parallelism.** Production MD stacks: SIMD within a core → OpenMP
threads within a socket → MPI ranks across nodes → GPU offload of the PP kernel.
You have now done the second of those four, and read enough about the fourth to
recognise it.

## Summary

- The gather kernel becomes a CUDA kernel by deleting the outer loop. That is
  possible only because of the formulation chosen in lesson 3.
- SoA cost 13% on CPU and buys coalesced memory access on GPU — the decision was
  only correct given where the code was headed.
- The reduction is the genuinely hard part; everything else is mechanical.
- Keep data resident on the device; do not memcpy per timestep.
- A cell list (~74× here) would beat threading (~4×). **Algorithm first.**
- GROMACS's architecture — cluster lists, PP/PME split — is a direct consequence
  of the trade-offs you measured in lessons 3, 5 and 7.

## Further Reading

- [*GPU Gems 3*, Ch. 31 — Fast N-Body Simulation with CUDA](https://developer.download.nvidia.com/compute/cuda/1.1-Beta/x86_website/projects/nbody/doc/nbody_gems3_ch31.pdf)
- [NERSC Perlmutter architecture](https://docs.nersc.gov/systems/perlmutter/architecture/)
- [GROMACS long-range VdW](https://manual.gromacs.org/current/reference-manual/functions/long-range-vdw.html)
- Páll et al., *Tackling Exascale Software Challenges in MD with GROMACS* — [arXiv:1506.00716](https://arxiv.org/pdf/1506.00716)

**Next:** the capstone, [`project/README.md`](../../project/README.md) — the lessons are done, and it answers: can you write all four backends yourself against the real `LJKernelView` seam, and explain why each one is as fast as it is? To widen out afterwards, [../explainer/05_python_interface.md](../explainer/05_python_interface.md) shows how this same engine is driven from Python.

**Back to:** [README.md](README.md) · **Capstone:** [`project/`](../../project/)
