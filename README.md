# MolSimWorkbench

A molecular simulation workbench for educational purposes that enables experimentation with different potential types.

## Features

- **Modular Architecture**: Separation of molecular building, potential energy calculation, and dynamics simulation components
- **Molecule Builder**: A simple builder module to create molecules, such as water.
- **YAML Configuration**: Use YAML files for a one-shop-stop defintion of all force field parameters, molecular geometries, and simulation settings
- **Python API**: Easy-to-use Python interface for running simulations
- **C++ Engine**: High-performance C++ backend for molecular dynamics
- **Unit Tests**: Comprehensive test suite for CI/CD integration

## Quick Start

### Building the Project

```bash
mkdir build && cd build
cmake ..
make
```

### Python API Usage

```python
from molsimworkbench import MolSimWorkbench

# Create workbench instance
wb = MolSimWorkbench()

# Check available potentials
print("Available potentials:", wb.get_available_potentials())

# Run LJ simulation
# Currently a hard-coded function. API improvement would be needed later.
trajectory = wb.run_lj_simulation()

```

### Running Examples

#### Lennard-Jones Water Simulation
```bash
cd examples/lj
python water_lj_sim.py
```

#### Basic DFT optimization of water
```bash
cd examples/dft
python run_dft_example.py
```

### Running Tests

#### C++ Unit Tests
```bash
cd build
ctest --verbose
```

#### Python API Tests
```bash
python tests/test_python_api.py
```

## Repository Structure

```
MolSimWorkbench/
├── src/                    # C++ source code
│   ├── engine/            # MD engine and potentials
│   ├── builder/           # Geometry and molecule builder
│   └── utils/             # Utilities (YAML parser, trajectory writer)
├── examples/              # Example simulations
│   ├── lj/               # Lennard-Jones examples
│   └── dft/              # DFT examples
├── tests/                 # Test suite
│   ├── unit/             # C++ unit tests
│   └── test_python_api.py # Python API tests
├── data/                  # Configuration files
│   ├── potentials/       # Potential definitions
│   └── geometry/         # Molecular geometries
├── molsimworkbench.py    # Python API
└── build/                # Build directory
```

## Supported Potentials

- **lj_only**: Lennard-Jones potential for classical molecular dynamics
- **dft**: Density Functional Theory potential for electronic structure

## Future work
- **classical**: Full hamiltoninian found in a typical forcefield
- **ml_potential**: Machine learning potentials (future)

## Trajectory Visualization

Generated XYZ trajectory files can be visualized using Visual Molecular Dynamics (VMD).

## CI/CD Integration

The workbench is designed for CI/CD with:
- Unit tests using GoogleTest (C++)
- Python API tests using pytest
- Trajectory files stored in `build/` for artifact collection
- Deterministic test environments

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## Educational Use

This workbench is designed for learning:
- Molecular dynamics concepts
- Different potential energy functions
- Trajectory analysis
- Scientific computing with C++ and Python

## Requirements

- C++17 compatible compiler
- Python 3.6+
- CMake 3.12+
- PyYAML (for YAML parsing)