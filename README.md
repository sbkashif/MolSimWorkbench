# MolSimWorkbench

MolSimWorkbench is a molecular dynamics simulation package designed to support various potential models, including classical Lennard-Jones potentials with plans to add ab initio and machine learning potentials. This project allows users to define and combine different potential functions, enabling flexible simulations of molecular systems.

## Features

- **Modular Architecture**: Separation of molecular building, potential energy calculation, and dynamics simulation components
- **Lennard-Jones Potential**: Implementation of Lennard-Jones potential for molecular simulations
- **Molecule Builder**: A simple builder module to create molecules, such as water, with the ability to manage coordinates and force field parameters
- **YAML Configuration**: Use YAML files to define force field parameters, molecular geometries, and simulation settings
- **Trajectory Generation**: Output of simulation trajectories in standard formats readable by visualization tools
- **Comprehensive Testing**: Google Test integration with unit and simulation tests

## Project Structure

```
MolSimWorkbench
├── src
│   ├── engine                  # Simulation engine components
│   │   ├── molecular_dynamics.cpp/h
│   │   ├── force_calculator.cpp/h
│   │   └── potentials
│   │       └── lj_potential.cpp/h
│   ├── builder                 # Molecular structure components
│   │   ├── molecule_builder.cpp/h
│   │   └── geometry.cpp/h
│   └── utils                   # Utility functions
│       ├── yaml_parser.cpp/h
│       ├── trajectory_writer.cpp/h
│       └── yaml_parser.py
├── data                        # Configuration data
│   ├── simulation_config.yaml
│   ├── geometry
│   │   └── water.yaml
│   └── potentials
│       └── lj_only.yaml
├── examples                    # Example simulations
│   └── water_lj_sim.py
├── tests                       # Testing framework
│   └── unit
│       ├── CMakeLists.txt
│       └── test_water_lj_unit.cpp
├── CMakeLists.txt
└── README.md
```

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/MolSimWorkbench.git
   cd MolSimWorkbench
   ```

2. Build the project using CMake:
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Run the tests to verify the installation:
   ```
   cd build
   ctest
   ```

## Usage

### Running a Water Molecule Simulation

The project includes a ready-to-use water molecule simulation using Lennard-Jones potentials:

```bash
# From the build directory
./WaterLJTest
```

This will run a simple simulation and output a trajectory file that can be visualized with VMD or other molecular visualization software.

### Using the Python Interface

The `examples` directory contains a Python script demonstrating how to set up and run a water molecule simulation:

```bash
python examples/water_lj_sim.py
```

### Configuration Files

The simulation behavior can be modified by editing the YAML configuration files:

- `data/geometry/water.yaml`: Defines water molecule geometry
- `data/potentials/lj_only.yaml`: Defines Lennard-Jones parameters
- `data/simulation_config.yaml`: Defines simulation parameters such as time step and number of steps

## Future Plans

- Integration of ab initio methods for more accurate potential energy calculations
- Machine learning potential support for efficient simulations of complex systems
- Additional minimization algorithms and integrators
- Enhanced visualization and analysis tools
- Parallel computing support for larger systems

## Contributing

Contributions are welcome! Please feel free to submit a pull request or open an issue for any suggestions or improvements.

## License

This project is licensed under the MIT License. See the LICENSE file for details.