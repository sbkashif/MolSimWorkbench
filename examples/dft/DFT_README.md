# DFT Examples README

> **Not implemented.** `DFTPotential` raises a not-implemented error when
> built, so this example stops at that error. It is kept for when a real DFT
> backend exists.

This directory contains a config-driven example demonstrating DFT (Density Functional Theory) calculations using MolSimWorkbench.

## Config-Driven Approach

All DFT calculations in MolSimWorkbench use YAML configuration files for consistency and reproducibility. This approach ensures:
- **Reproducible Results**: All parameters are externally defined
- **Easy Experimentation**: Modify YAML files without recompilation
- **Production Ready**: Suitable for automated workflows and CI/CD
- **Educational Value**: Clear separation between code logic and simulation parameters

## Available Example

### Config-Driven DFT Example (`dft_example.cpp`)
- **Purpose**: Production-ready DFT calculations with YAML configuration
- **Features**: 
  - External configuration via YAML files
  - Flexible parameter adjustment
  - Complete workflow from setup to calculation
  - Educational comments and documentation
- **Target Audience**: All users - from beginners to advanced researchers

## Python Runner

### `run_dft_example.py`
Automated runner that compiles and executes the DFT example.

## Configuration Files

DFT parameters are loaded from:
- `../../data/potentials/dft.yaml` - Main DFT configuration
- `../../data/geometry/water.yaml` - Molecular geometry definitions

## Usage

### Quick Start (Recommended)
```bash
# Run the config-driven DFT example
python run_dft_example.py
```

### Manual Compilation
```bash
# From build directory
g++ -std=c++17 -I../src -o dft_example ../examples/dft/dft_example.cpp -L. -lBuilderLib -lEngineLib
```

### Direct Execution
```bash
# From build directory
./dft_example
```

## Configuration Customization

To experiment with different DFT parameters:

1. **Modify DFT Settings**: Edit `data/potentials/dft.yaml`
   ```yaml
   dft:
     functional: "B3LYP"        # Exchange-correlation functional
     basis_set: "6-31G*"        # Basis set
     convergence: 1.0e-6        # SCF convergence threshold
     max_iterations: 100        # Maximum SCF iterations
   ```

2. **Modify Geometry**: Edit `data/geometry/water.yaml`
   ```yaml
   molecules:
     - name: "water"
       atoms:
         - element: "O"
           position: [0.0, 0.0, 0.0]
         - element: "H"
           position: [0.757, 0.586, 0.0]
   ```

3. **Run Example**: `python run_dft_example.py`

## Dependencies

The example requires:
- C++17 compatible compiler
- Compiled MolSimWorkbench libraries (BuilderLib, EngineLib)
- YAML configuration files in `data/` directory
- Python (for automated runner)

## Output

The example produces:
- DFT energy calculation results
- System information and parameters
- Timing and performance data
- Configuration validation output

## Educational Benefits

This config-driven approach teaches:
- **Best Practices**: Production-ready simulation setup
- **Parameter Management**: External configuration management
- **Reproducibility**: Scientific computing reproducibility standards
- **Workflow Design**: Professional computational chemistry workflows

## Examples Included

### 1. Simple DFT Example (`simple_dft_example.cpp`)
A minimal, self-contained example that demonstrates basic DFT energy calculation without external dependencies.

**Features:**
- Hardcoded DFT parameters (B3LYP functional, 6-31G* basis set)
- Electronic energy calculation for common atoms (H, C, N, O)
- Nuclear repulsion energy calculation
- Water molecule test case
- Unit conversions (Hartree to eV and kcal/mol)

**Compilation and execution:**
```bash
cd MolSimWorkbench
g++ -std=c++17 -o build/simple_dft_example examples/simple_dft_example.cpp
./build/simple_dft_example
```

### 2. YAML-Configured DFT Example (`yaml_dft_example.cpp`)
A more advanced example that loads DFT parameters from YAML configuration files and includes force calculations.

**Features:**
- Loads parameters from `data/potentials/dft.yaml`
- Functional-dependent energy corrections
- Basis set corrections
- Force calculation via numerical differentiation
- Enhanced error handling

**Compilation and execution:**
```bash
cd MolSimWorkbench
g++ -std=c++17 -o build/yaml_dft_example examples/yaml_dft_example.cpp
./build/yaml_dft_example
```

### 3. Full Framework Integration (`dft_energy_example.cpp`)
Integration with the complete MolSimWorkbench framework (requires successful build).

**Features:**
- Full integration with Geometry and YAMLParser classes
- Complete DFTPotential class implementation
- Framework-consistent API usage

**Note:** This example requires the main project to build successfully.

## Configuration

### DFT Parameters (`data/potentials/dft.yaml`)
The DFT potential can be configured using the YAML file:

```yaml
potential_type: "quantum"
method: "dft"
parameters:
  functional: "B3LYP"        # Exchange-correlation functional
  basis_set: "6-31G*"        # Basis set specification
  scf_convergence: 1.0e-6    # SCF convergence criteria
  max_scf_cycles: 100        # Maximum SCF iterations
  grid_quality: "fine"       # Integration grid quality
```

**Supported functionals:**
- B3LYP (hybrid functional)
- PBE (GGA functional)
- PW91 (GGA functional)

**Supported basis sets:**
- 6-31G* (split-valence with polarization)
- cc-pVDZ (correlation-consistent double-zeta)

## Implementation Details

### Energy Calculation Approach
The DFT energy is calculated as:
```
E_total = E_electronic + E_nuclear_repulsion
```

Where:
- **E_electronic**: Simplified electronic energy based on atom types and functional corrections
- **E_nuclear_repulsion**: Classical Coulomb repulsion between nuclei: Σ(i<j) Z_i*Z_j/r_ij

### Force Calculation
Forces are computed using numerical differentiation:
```
F_i = -∂E/∂r_i ≈ -(E(r_i + δ) - E(r_i))/δ
```

### Simplifications
This implementation uses several simplifications for demonstration purposes:

1. **Electronic Energy**: Based on atomic contributions rather than solving Kohn-Sham equations
2. **Basis Sets**: Represented by correction factors rather than actual orbital expansions
3. **Exchange-Correlation**: Applied as multiplicative corrections rather than functionals of electron density
4. **SCF Procedure**: Not implemented (would require iterative solution of Kohn-Sham equations)

## Example Output

```
=== DFT Energy Calculation with YAML Configuration ===

Water molecule geometry:
  O at (0, 0, 0) Bohr
  H at (0.757, 0.586, 0) Bohr
  H at (-0.757, 0.586, 0) Bohr

Loaded DFT Parameters from data/potentials/dft.yaml:
  Functional: B3LYP
  Basis Set: 6-31G*
  SCF Convergence: 1e-06
  Max SCF Cycles: 100
  Grid Quality: fine

Computing DFT energy with B3LYP/6-31G*...
Energy Components:
  Electronic Energy: -77.365 Hartree
  Nuclear Repulsion: 17.374 Hartree
  Total Energy: -59.991 Hartree

=== Energy Results ===
Total DFT Energy: -59.991 Hartree
Total DFT Energy: -1632.44 eV
Total DFT Energy: -37644.9 kcal/mol

=== Force Results ===
Forces on atoms (Hartree/Bohr):
  O: (-0.008, -10.688, 0.009) |F| = 10.688
  H: (7.335, 5.343, 0.005) |F| = 9.075
  H: (-7.343, 5.343, 0.005) |F| = 9.082
```

## Units and Conversions

- **Energy**: Hartree (atomic units)
  - 1 Hartree = 27.2114 eV
  - 1 Hartree = 627.509 kcal/mol
- **Distance**: Bohr (atomic units)
  - 1 Bohr = 0.529177 Å
- **Forces**: Hartree/Bohr

## Extending the Implementation

To create a more realistic DFT implementation:

1. **Add SCF iterations**: Implement self-consistent field procedure
2. **Implement basis functions**: Use actual Gaussian basis sets
3. **Add exchange-correlation functionals**: Implement LDA, GGA, and hybrid functionals
4. **Grid integration**: Add numerical integration for exchange-correlation energy
5. **Electron density**: Calculate and update electron density iteratively

## References

- Kohn, W., & Sham, L. J. (1965). Physical Review, 140(4A), A1133.
- Parr, R. G., & Yang, W. (1989). Density-functional theory of atoms and molecules.
- Becke, A. D. (1993). Journal of Chemical Physics, 98(7), 5648.

## Troubleshooting

### Common Issues

1. **YAML parsing errors**: Ensure Python is available and yaml_parser.py exists
2. **Compilation errors**: Check C++17 support and include paths
3. **Missing files**: Verify data/potentials/dft.yaml exists

### Debug Mode
Add `-DCMAKE_BUILD_TYPE=Debug` to cmake for additional debugging information.
