#!/usr/bin/env python3
"""
MolSimWorkbench DFT Example Runner

Demonstrates config-driven DFT calculations using YAML parameter files.
All DFT calculations are driven by configuration files for consistency and reproducibility.

Usage: python run_dft_example.py
"""

import subprocess
import sys
import os
from pathlib import Path

def run_command(cmd, description):
    """Run a command and return the result."""
    print(f"\n{'='*50}")
    print(f"Running: {description}")
    print(f"Command: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    print('='*50)
    
    try:
        if isinstance(cmd, str):
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, check=False)
        else:
            result = subprocess.run(cmd, capture_output=True, text=True, check=False)
        
        print(result.stdout)
        
        if result.stderr:
            print("\nSTDERR:")
            print(result.stderr)
        
        print(f"\nReturn code: {result.returncode}")
        return result.returncode == 0, result.stdout, result.stderr
    
    except Exception as e:
        print(f"Error running command: {e}")
        return False, "", str(e)

def compile_dft_example():
    """Compile the DFT example."""
    print("Compiling config-driven DFT example...")
    
    # Find project root (go up two levels from examples/dft/)
    project_root = Path(__file__).parent.parent.parent
    build_dir = project_root / "build"
    
    if not build_dir.exists():
        print(f"Build directory not found at {build_dir}")
        print("Please run: mkdir build && cd build && cmake .. && make")
        return False
    
    # Change to build directory
    original_dir = os.getcwd()
    os.chdir(build_dir)
    
    try:
        # Compile the DFT example
        success, stdout, stderr = run_command(
            f"g++ -std=c++17 -I../src -o dft_example ../examples/dft/dft_example.cpp -L. -lBuilderLib -lEngineLib",
            "Compiling DFT Example"
        )
        
        if not success:
            print("❌ Compilation failed")
            return False
            
        print("✅ DFT example compiled successfully")
        return True
        
    finally:
        os.chdir(original_dir)

def run_dft_example():
    """Run the config-driven DFT example."""
    project_root = Path(__file__).parent.parent.parent
    build_dir = project_root / "build"
    
    # Change to build directory to run
    original_dir = os.getcwd()
    os.chdir(build_dir)
    
    try:
        success, stdout, stderr = run_command(
            "./dft_example",
            "Running Config-Driven DFT Example"
        )
        
        if success:
            print("✅ DFT example completed successfully")
        else:
            print("❌ DFT example failed")
            
        return success
        
    finally:
        os.chdir(original_dir)

def main():
    """Run the config-driven DFT example."""
    print("=== MolSimWorkbench DFT Example (Config-Driven) ===")
    print("This example demonstrates DFT calculations using YAML configuration files.")
    print("All parameters are specified in data/potentials/dft.yaml\n")
    
    # Check if we're in the right directory structure
    project_root = Path(__file__).parent.parent.parent
    if not (project_root / "CMakeLists.txt").exists():
        print("❌ Error: Cannot find project root. Please run from the examples/dft/ directory.")
        return False
    
    print(f"📁 Project root: {project_root}")
    print(f"📄 DFT config: {project_root / 'data' / 'potentials' / 'dft.yaml'}")
    
    # Step 1: Compile the example
    if not compile_dft_example():
        return False
    
    # Step 2: Run the example
    if not run_dft_example():
        return False
    
    print("\n🎉 DFT example completed successfully!")
    print("The example demonstrated:")
    print("  • Loading DFT parameters from YAML configuration")
    print("  • Performing DFT energy calculations on molecular systems")
    print("  • Config-driven approach for reproducible simulations")
    print("  • Generation of visualization files for DFT results")
    print("\n📁 Output files generated in examples/dft/output/:")
    print("  • dft_optimization.xyz - Geometry optimization trajectory")
    print("  • water_structure.xyz - Final optimized molecular structure")
    print("  • dft_results.dat - Energy and force calculation results") 
    print("  • visualize_dft.vmd - VMD animation script for optimization")
    print("  • electron_density.cube - Electron density grid file")
    print("\n💡 To visualize:")
    print("  VMD: vmd -e output/visualize_dft.vmd (shows optimization animation)")
    print("  Python: python visualize_results.py (if matplotlib available)")
    
    return True

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
