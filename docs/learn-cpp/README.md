# Learn C++ — from a blank file to reading the parallel LJ kernel

The goal of this track is one thing: **walk through this repo's code and
understand every line**, including the parallel LJ kernel. It teaches only the
C++ that the code uses, starting from the Python you already know (binding vs
mutation, classes, inheritance), and every chapter ends on real lines in this
repo.

It is the wide first part of the whole path — see
[`../LEARNING_PATH.md`](../LEARNING_PATH.md) for how it leads into the
explainer and the parallelization course.

## One program, grown chapter by chapter: `mini_lj`

Each chapter adds one idea to a small Lennard-Jones program, `mini_lj`, and
each chapter starts from the code the previous one ended with:

```
ch02  two atoms, one distance                      E(1.2) = -0.890965
ch03  positions in a periodic box, references
ch04  N atoms in containers                        N=4096: E = -20105.493949  (= the repo's reference)
ch05  maths moved into functions and headers
ch06  Geometry and LJPotential become classes      (same shape as the repo's)
ch07  an abstract Potential, two potentials stacked
ch08  shared_ptr ownership, a ForceCalculator      (same shape as run_simulation.cpp)
ch09  a timer, lambdas, the first two threads      1.38x on 2 threads — and why not 2x
ch10  mini_lj *is* the repo's reference loop  ──►  explainer, then learn-parallel
```

The code for every step is in [`examples/NN/`](examples/). Compile and run it
yourself; every output quoted in a chapter came from running it on an Apple M1,
and yours should match (timings will differ).

## How each chapter is built

| File | Style |
|---|---|
| `NN_topic.md` | technical: real `file:line` refs, compiler output, exact terms |
| `NNp_topic_plain.md` | the same chapter in plain words, one kitchen picture throughout, step tables **Kitchen \| C++ \| Example** |

Every chapter opens with **"After this chapter you can say…"** and
**"Where we are"** (what the last chapter left unsolved), and closes with
**"Next"** (what this chapter leaves unsolved). Read them as one continuous
story.

Section markers:

- **(skip if …)** — optional for you if the condition holds.
- **(deep dive later)** — not needed to follow the thread on a first pass; come
  back once the whole path makes sense.

## Chapters

| # | Chapter | Lands on |
|---|---|---|
| 1 | [Program structure: compile, link, run](01_program_structure.md) · [plain](01p_program_structure_plain.md) | `src/tools/run_simulation.cpp`, `CMakeLists.txt` |
| 2 | [Variables and types](02_variables_and_types.md) · [plain](02p_variables_and_types_plain.md) | `calculatePairEnergy`, `double cutoff = 10.0;` |
| 3 | [Copies, references, pointers](03_copies_references_pointers.md) · [plain](03p_copies_references_pointers_plain.md) | `const Geometry&`, the `forces` out-parameter, `char* argv[]` |
| 4 | [Containers](04_containers.md) · [plain](04p_containers_plain.md) | `geometry.h` members, the lattice builder |
| 5 | [Functions and headers in practice](05_functions_and_headers.md) · [plain](05p_functions_and_headers_plain.md) | `getMixedParameters`, `calculatePairForce` |
| 6 | [Classes](06_classes.md) · [plain](06p_classes_plain.md) | `lj_potential.h`, `geometry.h` |
| 7 | [Inheritance and virtual functions](07_inheritance_and_virtual.md) · [plain](07p_inheritance_and_virtual_plain.md) | `Potential` → `LJPotential` |
| 8 | [Ownership and lifetime](08_ownership_and_lifetime.md) · [plain](08p_ownership_and_lifetime_plain.md) | `std::shared_ptr<ForceCalculator>`, `LJKernelView`'s pointers |
| 9 | [Templates, lambdas, and a first thread](09_templates_lambdas_threads.md) · [plain](09p_templates_lambdas_threads_plain.md) | the `Threads` backend in `lj_kernels.cpp` |
| 10 | [From mini_lj to the real repo](10_mini_lj_to_the_repo.md) · [plain](10p_mini_lj_to_the_repo_plain.md) | build and run the repo; hand-off to the explainer and learn-parallel |
| 11 | Back to Python: pybind11 *(deep dive later — not written yet)* | `src/bindings/molsim_bindings.cpp` |

Each chapter gives the exact compile command for its step. The `examples/NN/`
folders also hold small single-file toy programs next to `mini_lj`, so list the
`mini_lj` files explicitly rather than compiling `*.cpp`.
