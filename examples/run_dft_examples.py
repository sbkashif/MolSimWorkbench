#!/usr/bin/env python3
"""
Script to run and compare different DFT energy calculation examples.
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
        
        print("STDOUT:")
        print(result.stdout)
        
        if result.stderr:
            print("\nSTDERR:")
            print(result.stderr)
        
        print(f"\nReturn code: {result.returncode}")
        return result.returncode == 0, result.stdout, result.stderr
    
    except Exception as e:
        print(f"Error running command: {e}")
        return False, "", str(e)

def compile_examples():
    """Compile all DFT examples."""
    print("Compiling DFT examples...")
    
    examples = [
        ("simple_dft_example", "examples/simple_dft_example.cpp"),
        ("yaml_dft_example", "examples/yaml_dft_example.cpp")
    ]
    
    compiled = []
    for name, source in examples:
        cmd = [
            "g++", "-std=c++17", 
            "-o", f"build/{name}", 
            source
        ]
        
        success, stdout, stderr = run_command(cmd, f"Compiling {name}")
        if success:
            compiled.append(name)
            print(f"✓ {name} compiled successfully")
        else:
            print(f"✗ {name} compilation failed")
    
    return compiled

def run_examples(compiled_examples):
    """Run all successfully compiled examples."""
    results = {}
    
    for example in compiled_examples:
        cmd = f"./build/{example}"
        success, stdout, stderr = run_command(cmd, f"Running {example}")
        
        if success:
            # Extract energy from output
            energy = extract_energy(stdout)
            results[example] = {
                'success': True,
                'energy': energy,
                'output': stdout
            }
            print(f"✓ {example} ran successfully")
            if energy:
                print(f"  Energy: {energy} Hartree")
        else:
            results[example] = {
                'success': False,
                'error': stderr,
                'output': stdout
            }
            print(f"✗ {example} failed")
    
    return results

def extract_energy(output):
    """Extract the total energy from the output."""
    lines = output.split('\n')
    for line in lines:
        if 'Total Energy:' in line and 'Hartree' in line:
            try:
                # Extract number from line like "  Total Energy: -59.991 Hartree"
                parts = line.split()
                for i, part in enumerate(parts):
                    if part == 'Energy:' and i + 1 < len(parts):
                        return float(parts[i + 1])
            except (ValueError, IndexError):
                continue
    return None

def compare_results(results):
    """Compare results from different examples."""
    print(f"\n{'='*60}")
    print("COMPARISON OF RESULTS")
    print('='*60)
    
    energies = {}
    for example, result in results.items():
        if result['success'] and result['energy'] is not None:
            energies[example] = result['energy']
            print(f"{example:25}: {result['energy']:12.6f} Hartree")
    
    if len(energies) > 1:
        print(f"\n{'Energy Differences:'}")
        example_names = list(energies.keys())
        for i in range(len(example_names)):
            for j in range(i + 1, len(example_names)):
                name1, name2 = example_names[i], example_names[j]
                diff = abs(energies[name1] - energies[name2])
                print(f"{name1} vs {name2}: {diff:.6f} Hartree ({diff*627.509:.2f} kcal/mol)")

def check_prerequisites():
    """Check if all prerequisites are available."""
    print("Checking prerequisites...")
    
    # Check if in correct directory
    if not os.path.exists("CMakeLists.txt"):
        print("Error: Please run this script from the MolSimWorkbench root directory")
        return False
    
    # Check for required files
    required_files = [
        "examples/simple_dft_example.cpp",
        "examples/yaml_dft_example.cpp",
        "data/potentials/dft.yaml",
        "src/utils/yaml_parser.py"
    ]
    
    missing = []
    for file in required_files:
        if not os.path.exists(file):
            missing.append(file)
    
    if missing:
        print("Missing required files:")
        for file in missing:
            print(f"  - {file}")
        return False
    
    # Check for build directory
    if not os.path.exists("build"):
        print("Creating build directory...")
        os.makedirs("build")
    
    print("✓ All prerequisites satisfied")
    return True

def main():
    """Main function."""
    print("DFT Energy Calculation Examples Runner")
    print("=" * 40)
    
    if not check_prerequisites():
        return 1
    
    # Compile examples
    compiled = compile_examples()
    if not compiled:
        print("No examples compiled successfully.")
        return 1
    
    print(f"\nSuccessfully compiled {len(compiled)} examples: {', '.join(compiled)}")
    
    # Run examples
    results = run_examples(compiled)
    
    # Compare results
    compare_results(results)
    
    # Summary
    successful = sum(1 for r in results.values() if r['success'])
    print(f"\n{'='*60}")
    print("SUMMARY")
    print('='*60)
    print(f"Examples compiled: {len(compiled)}")
    print(f"Examples run successfully: {successful}/{len(compiled)}")
    
    if successful > 0:
        print("\n✓ DFT energy calculation examples completed successfully!")
        print("\nNext steps:")
        print("1. Review the example outputs above")
        print("2. Modify the YAML configuration file (data/potentials/dft.yaml)")
        print("3. Try different functionals (B3LYP, PBE, etc.)")
        print("4. Experiment with different molecular geometries")
        print("5. Read examples/DFT_README.md for detailed documentation")
    else:
        print("\n✗ Some examples failed. Check the error messages above.")
    
    return 0 if successful > 0 else 1

if __name__ == "__main__":
    sys.exit(main())
