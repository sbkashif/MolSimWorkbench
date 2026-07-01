# Unit tests
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

target_include_directories(MolSimTests PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
)

# Register tests with CTest
gtest_discover_tests(MolSimTests)