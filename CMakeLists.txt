cmake_minimum_required(VERSION 3.15)

project(MolSimWorkbench)

# Default to an optimised build. Without this, a plain `cmake -B build -S .`
# compiles with no optimisation, and every timing in docs/learn-parallel/ is
# meaningless (mini_lj runs ~12x slower at -O0). Pass -DCMAKE_BUILD_TYPE=Debug
# explicitly when you want a debug build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The static libraries are linked into the Python module, a shared library;
# on Linux that needs position-independent code (macOS builds it by default).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable testing support
enable_testing()

# Fetch dependencies
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Add Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.1
)

set(PYBIND11_FINDPYTHON ON)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json googletest pybind11)

# Make gtest_discover_tests() available to subdirectories
include(GoogleTest)


# ---------------------------------------------------------------------------
# OpenMP (optional) - powers the opt-in parallel LJ backend.
#
# Apple Clang does not ship an OpenMP runtime, so on macOS we point CMake at
# Homebrew's libomp. If it is not found the build still succeeds; the OpenMP
# backend simply falls back to the serial kernel at runtime.
# ---------------------------------------------------------------------------
if(APPLE AND NOT DEFINED OpenMP_ROOT)
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE _brew_libomp
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_brew_libomp AND EXISTS "${_brew_libomp}")
        set(OpenMP_ROOT "${_brew_libomp}")
    endif()
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP ${OpenMP_CXX_VERSION} found - parallel LJ backend available (opt-in)")
else()
    message(STATUS "OpenMP not found - LJ OpenMP backend will fall back to serial")
endif()

# Create a library for the builder components
add_library(BuilderLib STATIC
    src/builder/geometry.cpp
    src/builder/molecule_builder.cpp
    src/builder/lj_particle_box_builder.cpp
    src/utils/yaml_parser.cpp
    src/utils/trajectory_writer.cpp  # Add trajectory writer
)
target_link_libraries(BuilderLib PUBLIC nlohmann_json::nlohmann_json)
# GCC 8 ships std::filesystem in a separate library.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_link_libraries(BuilderLib PUBLIC stdc++fs)
endif()
target_include_directories(BuilderLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${nlohmann_json_SOURCE_DIR}/include
)

# Create a library for the engine components
add_library(EngineLib STATIC
    src/engine/force_calculator.cpp
    src/engine/molecular_dynamics.cpp
    src/engine/md_state.cpp
    src/engine/integrators/integrator.cpp
    src/engine/integrators/velocity_verlet_integrator.cpp
    src/engine/integrators/leapfrog_integrator.cpp
    src/engine/thermostats/thermostat.cpp
    src/engine/thermostats/velocity_rescale_thermostat.cpp
    src/engine/thermostats/langevin_thermostat.cpp
    src/engine/potentials/lj_potential.cpp
    src/engine/potentials/lj_kernels.cpp
    src/engine/potentials/dft_potential.cpp
)
target_link_libraries(EngineLib PUBLIC BuilderLib)
if(OpenMP_CXX_FOUND)
    target_link_libraries(EngineLib PUBLIC OpenMP::OpenMP_CXX)
endif()

add_executable(run_simulation
    src/tools/run_simulation.cpp
)
target_link_libraries(run_simulation PRIVATE BuilderLib EngineLib)

pybind11_add_module(_molsim
    src/bindings/molsim_bindings.cpp
)
target_link_libraries(_molsim PRIVATE BuilderLib EngineLib)
target_include_directories(_molsim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Copy Python YAML parser to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/yaml_parser.py
    ${CMAKE_CURRENT_BINARY_DIR}/yaml_parser.py
    COPYONLY
)

# Copy configuration files to build directory for easier test execution
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/potentials)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data/geometry)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/simulation_config.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/data/simulation_config.yaml
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/potentials/lj_only.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/data/potentials/lj_only.yaml
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/geometry/water.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/data/geometry/water.yaml
    COPYONLY
)

# Benchmarks (parallelization curriculum)
add_subdirectory(benchmarks)

# ---------------------------------------------------------------------------
# Parallelization curriculum (docs/learn-parallel/).
#
# OFF by default: the exercise tests are DESIGNED to fail until the learner
# implements the stubs, so building them into the default `ctest` run would
# make CI permanently red. Turn on explicitly:
#
#   cmake -B build -S . -DMOLSIM_BUILD_LEARN=ON
#   cd build && ctest -L learn --output-on-failure
# ---------------------------------------------------------------------------
option(MOLSIM_BUILD_LEARN "Build the parallelization curriculum exercises and capstone" OFF)
if(MOLSIM_BUILD_LEARN)
    message(STATUS "Parallelization curriculum ENABLED (ctest -L learn)")
    add_subdirectory(exercises)
    add_subdirectory(project)
endif()

# Unit tests
add_subdirectory(tests/unit)
