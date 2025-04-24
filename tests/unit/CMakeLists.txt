# Unit tests configuration
include(GoogleTest)

# Create a test for the Water LJ simulation (using the standalone executable approach)
add_test(NAME WaterLJSimulation COMMAND WaterLJTest)

# Create proper unit tests with Google Test
add_executable(MolSimTests
    test_water_lj_unit.cpp
    # Add more test files here as needed
)

target_link_libraries(MolSimTests
    PRIVATE
    BuilderLib
    EngineLib
    gtest
    gtest_main
)

# Register the tests with CTest
gtest_discover_tests(MolSimTests)