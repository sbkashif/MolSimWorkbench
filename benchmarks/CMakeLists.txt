# Benchmark harness for the parallelization curriculum (docs/learn-parallel/).
# Deliberately NOT registered with CTest: benchmarks are run by hand, and a
# timing-dependent test in CI is a flaky test.
add_executable(bench_lj bench_lj.cpp)
target_link_libraries(bench_lj PRIVATE BuilderLib EngineLib)
target_include_directories(bench_lj PRIVATE ${CMAKE_SOURCE_DIR}/src)
