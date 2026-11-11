# Capstone: implement your own parallel LJ backend (project/README.md).
add_executable(capstone_tests
    src/student_kernel.cpp
    tests/test_capstone.cpp
)
target_link_libraries(capstone_tests PRIVATE BuilderLib EngineLib gtest gtest_main)
target_include_directories(capstone_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/exercises/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(OpenMP_CXX_FOUND)
    target_link_libraries(capstone_tests PRIVATE OpenMP::OpenMP_CXX)
endif()
gtest_discover_tests(capstone_tests PROPERTIES LABELS "learn" DISCOVERY_MODE PRE_TEST)

add_executable(capstone_report src/student_kernel.cpp src/capstone_report.cpp)
target_link_libraries(capstone_report PRIVATE BuilderLib EngineLib)
target_include_directories(capstone_report PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/exercises/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(OpenMP_CXX_FOUND)
    target_link_libraries(capstone_report PRIVATE OpenMP::OpenMP_CXX)
endif()
