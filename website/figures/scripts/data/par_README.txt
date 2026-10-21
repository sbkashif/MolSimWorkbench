Measured 2026-10-21 on Apple M4 (4 performance + 6 efficiency cores), macOS 15.7.4,
Apple clang 17, -O3, OpenMP 5.1 from Homebrew libomp.
par_step_split_before.csv: website/parallel/examples/step_split.cpp, one run, on the engine
  before forces and energy were computed in one pass (energy was a second pair loop per step).
par_step_split.csv: step_split.cpp, fastest of three runs per entry, serial (default) backend.
par_step_split_openmp.csv: `step_split openmp`, fastest of three runs, OpenMPPrivate, 10 threads.
par_thread_sweep.csv: website/parallel/examples/thread_sweep.cpp, faster of two runs per entry.
