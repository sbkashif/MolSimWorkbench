# ---------------------------------------------------------------------------
# Parallelization curriculum exercises (docs/learn-parallel/).
#
# These are LEARNING exercises: their tests are expected to FAIL until you
# implement the stubs. They are therefore built only when MOLSIM_BUILD_LEARN=ON
# and are registered with CTest under the label "learn" so the project's own
# test suite stays green.
#
#   cmake -B build -S . -DMOLSIM_BUILD_LEARN=ON
#   cmake --build build --parallel
#   cd build && ctest -L learn --output-on-failure      # your progress
#   cd build && ctest -LE learn                         # the repo's own tests
# ---------------------------------------------------------------------------

# Helper: one GoogleTest binary per exercise.
function(add_exercise name dir)
    cmake_parse_arguments(EX "OPENMP" "" "SOURCES;TESTS" ${ARGN})

    add_executable(${name} ${EX_SOURCES} ${EX_TESTS})
    target_link_libraries(${name} PRIVATE BuilderLib EngineLib gtest gtest_main)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/src
    )
    if(EX_OPENMP AND OpenMP_CXX_FOUND)
        target_link_libraries(${name} PRIVATE OpenMP::OpenMP_CXX)
    endif()

    gtest_discover_tests(${name}
        PROPERTIES LABELS "learn"
        DISCOVERY_MODE PRE_TEST
    )
endfunction()

# Helper: a runnable demo/study binary (no tests, never in CTest).
function(add_exercise_tool name dir source)
    cmake_parse_arguments(EX "OPENMP" "" "EXTRA" ${ARGN})
    add_executable(${name} ${dir}/src/${source} ${EX_EXTRA})
    target_link_libraries(${name} PRIVATE BuilderLib EngineLib)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/src
    )
    if(EX_OPENMP AND OpenMP_CXX_FOUND)
        target_link_libraries(${name} PRIVATE OpenMP::OpenMP_CXX)
    endif()
endfunction()

add_exercise(ex01_tests 01-measure-first
    SOURCES 01-measure-first/src/timing.cpp
    TESTS   01-measure-first/tests/test_timing.cpp)

add_exercise(ex02_tests 02-serial-refactor
    SOURCES 02-serial-refactor/src/refactor.cpp
    TESTS   02-serial-refactor/tests/test_refactor.cpp)

add_exercise(ex03_tests 03-scatter-vs-gather
    SOURCES 03-scatter-vs-gather/src/gather.cpp
    TESTS   03-scatter-vs-gather/tests/test_gather.cpp)

add_exercise(ex04_tests 04-threads-by-hand
    SOURCES 04-threads-by-hand/src/pool.cpp
    TESTS   04-threads-by-hand/tests/test_pool.cpp)

add_exercise(ex05_tests 05-the-race OPENMP
    SOURCES 05-the-race/src/race.cpp
    TESTS   05-the-race/tests/test_race.cpp)

add_exercise(ex06_tests 06-openmp OPENMP
    SOURCES 06-openmp/src/omp_kernel.cpp
    TESTS   06-openmp/tests/test_omp_kernel.cpp)

add_exercise(ex07_tests 07-scaling
    SOURCES 07-scaling/src/scaling.cpp
    TESTS   07-scaling/tests/test_scaling.cpp)

add_exercise(ex08_tests 08-tail-correction
    SOURCES 08-tail-correction/src/tail.cpp
    TESTS   08-tail-correction/tests/test_tail.cpp)

# Runnable studies referenced by the exercise READMEs.
add_exercise_tool(ex05_demo     05-the-race demo.cpp           OPENMP EXTRA 05-the-race/src/race.cpp)
add_exercise_tool(ex06_schedule 06-openmp   schedule_study.cpp OPENMP EXTRA 06-openmp/src/omp_kernel.cpp)
add_exercise_tool(ex07_scaling  07-scaling  scaling_study.cpp         EXTRA 07-scaling/src/scaling.cpp)
