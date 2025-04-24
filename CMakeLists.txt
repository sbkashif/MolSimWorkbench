cmake_minimum_required(VERSION 3.10)

project(MolSimWorkbench)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing support
enable_testing()

# Add include directories for our source code
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/builder
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# Find or fetch nlohmann/json for JSON parsing
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Add Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json googletest)

# Create a library for the builder components
add_library(BuilderLib STATIC
    src/builder/geometry.cpp
    src/builder/molecule_builder.cpp
    src/utils/yaml_parser.cpp
    src/utils/trajectory_writer.cpp  # Add trajectory writer
)
target_link_libraries(BuilderLib PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(BuilderLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${nlohmann_json_SOURCE_DIR}/include
)

# Create a library for the engine components
add_library(EngineLib STATIC
    src/engine/force_calculator.cpp
    src/engine/molecular_dynamics.cpp
    src/engine/potentials/lj_potential.cpp
)
target_link_libraries(EngineLib PUBLIC BuilderLib)

# Add the water LJ minimization test as a standalone executable
add_executable(WaterLJTest tests/unit/test_water_lj_unit.cpp)
target_link_libraries(WaterLJTest PRIVATE
    BuilderLib
    EngineLib
    gtest
    gtest_main
)
target_include_directories(WaterLJTest PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
)

# Copy Python YAML parser to build directory to ensure it's accessible at runtime
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

# Add subdirectory for unit tests
add_subdirectory(tests/unit)