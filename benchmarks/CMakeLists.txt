# Benchmark harness for the parallelization curriculum (docs/learn-parallel/).
# Deliberately NOT registered with CTest: benchmarks are run by hand, and a
# timing-dependent test in CI is a flaky test.
add_executable(bench_lj bench_lj.cpp)
target_link_libraries(bench_lj PRIVATE BuilderLib EngineLib)
target_include_directories(bench_lj PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Reproduces lesson 2's ablation table and the atom-type sweep.
add_executable(bench_ablation bench_ablation.cpp)
target_link_libraries(bench_ablation PRIVATE BuilderLib EngineLib)
target_include_directories(bench_ablation PRIVATE ${CMAKE_SOURCE_DIR}/src)
