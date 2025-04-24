# MolSimWorkbench Tests

This directory contains the test suite for the MolSimWorkbench project. Tests are implemented using Google Test and are automatically discovered and run via CTest.

## Test Organization

- **Unit Tests**: Located in `tests/unit/`, these tests verify individual components of the simulation framework
- **Integration Tests**: Some tests verify the integration between multiple components

## Current Test Coverage

### Water Lennard-Jones Tests (`test_water_lj_unit.cpp`)

These tests verify the functionality of water molecule simulations using Lennard-Jones potentials:

- **GeometryLoaded**: Verifies that water molecule geometry can be correctly loaded and parsed
- **InitialEnergyCalculation**: Tests the initial energy calculation for a water molecule
- **EnergyMinimization**: Verifies the energy minimization procedure works correctly
- **MolecularDynamics**: Tests the molecular dynamics simulation for a water molecule

## Running Tests

You can run all tests using CTest:

```bash
# From the build directory
ctest
```

To run specific tests:

```bash
# Run tests with verbose output
ctest -V

# Run tests matching a pattern
ctest -R WaterLJ
```

## Adding New Tests

To add new tests:

1. Create a new test file in the `tests/unit/` directory
2. Add the test executable to `tests/unit/CMakeLists.txt`
3. The test will be automatically discovered by CTest

## Test Limitations and Future Improvements

- More comprehensive test coverage for edge cases
- Performance tests for larger systems
- Tests for different potential functions (pending implementation)
- Tests for different molecule types (pending implementation)