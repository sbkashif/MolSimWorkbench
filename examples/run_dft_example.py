#!/usr/bin/env python3

import subprocess
import sys
import os

def compile_and_run_dft_example():
    """Compile and run the DFT energy calculation example"""
    
    print("=== DFT Energy Calculation Example (Python Wrapper) ===")
    print()
    
    # Check if we're in the correct directory
    if not os.path.exists("CMakeLists.txt"):
        print("Error: Please run this script from the MolSimWorkbench root directory")
        return False
    
    try:
        # Build the project
        print("Building the project...")
        build_result = subprocess.run(
            ["cmake", "--build", "build"], 
            capture_output=True, 
            text=True
        )
        
        if build_result.returncode != 0:
            print("Build failed:")
            print(build_result.stderr)
            return False
        
        print("Build successful!")
        print()
        
        # Compile the DFT example
        print("Compiling DFT example...")
        compile_cmd = [
            "g++", 
            "-std=c++17",
            "-I.", 
            "-Ibuild/_deps/json-src/include",
            "examples/dft_energy_example.cpp",
            "src/engine/potentials/dft_potential.cpp",
            "src/builder/geometry.cpp", 
            "src/utils/yaml_parser.cpp",
            "build/libBuilderLib.a",
            "build/libEngineLib.a",
            "-o", "build/dft_example"
        ]
        
        compile_result = subprocess.run(compile_cmd, capture_output=True, text=True)
        
        if compile_result.returncode != 0:
            print("Compilation failed:")
            print(compile_result.stderr)
            return False
        
        print("Compilation successful!")
        print()
        
        # Run the DFT example
        print("Running DFT energy calculation...")
        print("-" * 50)
        
        run_result = subprocess.run(
            ["./build/dft_example"], 
            capture_output=True, 
            text=True,
            cwd="."
        )
        
        if run_result.returncode != 0:
            print("Execution failed:")
            print("STDOUT:", run_result.stdout)
            print("STDERR:", run_result.stderr)
            return False
        
        # Print the output
        print(run_result.stdout)
        
        if run_result.stderr:
            print("Warnings/Errors:")
            print(run_result.stderr)
        
        return True
        
    except Exception as e:
        print(f"An error occurred: {e}")
        return False

def main():
    """Main function"""
    success = compile_and_run_dft_example()
    
    if success:
        print("\n=== Example completed successfully! ===")
        print()
        print("This example demonstrates:")
        print("1. Loading DFT parameters from YAML file")
        print("2. Creating a water molecule geometry")
        print("3. Calculating DFT energy (simplified model)")
        print("4. Computing atomic forces")
        print("5. Unit conversions (Hartree to eV and kcal/mol)")
        print()
        print("Note: This is a simplified DFT implementation for demonstration.")
        print("Real DFT calculations require solving the Kohn-Sham equations.")
        return 0
    else:
        print("\n=== Example failed ===")
        return 1

if __name__ == "__main__":
    sys.exit(main())
