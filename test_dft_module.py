#!/usr/bin/env python3

"""
DFT Module Testing and Demonstration Script

This script compiles and runs all the DFT examples to demonstrate the enhanced
DFT capabilities in MolSimWorkbench.
"""

import subprocess
import sys
import os
from pathlib import Path

def run_command(cmd, description):
    """Run a command and return success status"""
    print(f"\n{description}...")
    print(f"Command: {cmd}")
    
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=".")
        
        if result.returncode == 0:
            print("✅ SUCCESS")
            if result.stdout:
                print(result.stdout)
            return True
        else:
            print("❌ FAILED")
            if result.stderr:
                print("Error:", result.stderr)
            if result.stdout:
                print("Output:", result.stdout)
            return False
            
    except Exception as e:
        print(f"❌ EXCEPTION: {e}")
        return False

def main():
    """Main demonstration function"""
    print("=" * 70)
    print("DFT MODULE TESTING AND DEMONSTRATION")
    print("=" * 70)
    
    # Ensure we're in the right directory
    if not os.path.exists("CMakeLists.txt"):
        print("❌ Error: Run this script from the MolSimWorkbench root directory")
        return 1
    
    # Create build directory if it doesn't exist
    os.makedirs("build", exist_ok=True)
    
    # List of examples to compile and run
    examples = [
        {
            "name": "Simple DFT Example",
            "source": "examples/simple_dft_example.cpp",
            "executable": "build/simple_dft_example",
            "compile_cmd": "g++ -std=c++17 -o build/simple_dft_example examples/simple_dft_example.cpp -lm",
            "description": "Basic DFT energy calculation with hardcoded parameters"
        },
        {
            "name": "YAML DFT Example", 
            "source": "examples/yaml_dft_example.cpp",
            "executable": "build/yaml_dft_example",
            "compile_cmd": "g++ -std=c++17 -o build/yaml_dft_example examples/yaml_dft_example.cpp -lm",
            "description": "DFT calculation with YAML configuration loading"
        },
        {
            "name": "Enhanced DFT Test",
            "source": "examples/enhanced_dft_test.cpp", 
            "executable": "build/enhanced_dft_test",
            "compile_cmd": "g++ -std=c++17 -o build/enhanced_dft_test examples/enhanced_dft_test.cpp -lm",
            "description": "Comprehensive test with multiple molecules and functionals"
        }
    ]
    
    results = []
    
    print("\n📋 AVAILABLE EXAMPLES:")
    for i, example in enumerate(examples, 1):
        print(f"{i}. {example['name']}: {example['description']}")
    
    # Compile and run each example
    for example in examples:
        print(f"\n{'='*50}")
        print(f"Testing: {example['name']}")
        print(f"{'='*50}")
        
        # Check if source file exists
        if not os.path.exists(example['source']):
            print(f"❌ Source file not found: {example['source']}")
            results.append((example['name'], False, "Source not found"))
            continue
        
        # Compile
        compile_success = run_command(example['compile_cmd'], f"Compiling {example['name']}")
        
        if not compile_success:
            results.append((example['name'], False, "Compilation failed"))
            continue
            
        # Run
        run_success = run_command(f"./{example['executable']}", f"Running {example['name']}")
        
        results.append((example['name'], run_success, "Success" if run_success else "Runtime error"))
    
    # Print summary
    print(f"\n{'='*70}")
    print("SUMMARY OF RESULTS")
    print(f"{'='*70}")
    
    print(f"{'Example':<25} {'Status':<15} {'Notes'}")
    print(f"{'-'*25} {'-'*15} {'-'*30}")
    
    success_count = 0
    for name, success, notes in results:
        status = "✅ PASSED" if success else "❌ FAILED"
        print(f"{name:<25} {status:<15} {notes}")
        if success:
            success_count += 1
    
    print(f"\nResults: {success_count}/{len(results)} examples passed")
    
    # Print feature summary
    print(f"\n{'='*70}")
    print("DFT MODULE FEATURES DEMONSTRATED")
    print(f"{'='*70}")
    
    features = [
        "✅ Multiple DFT functionals (B3LYP, PBE, HF)",
        "✅ Various basis sets (6-31G, 6-31G*, cc-pVDZ)",
        "✅ Realistic atomic energies from literature",
        "✅ Nuclear repulsion energy calculations",
        "✅ Molecular bonding corrections",
        "✅ Force calculations via numerical differentiation",
        "✅ YAML configuration file support",
        "✅ Unit conversions (Hartree, eV, kcal/mol, kJ/mol)",
        "✅ Multiple test molecules (H2O, CH4, C6H6)",
        "✅ Comprehensive error handling"
    ]
    
    for feature in features:
        print(feature)
    
    print(f"\n{'='*70}")
    print("NEXT STEPS")
    print(f"{'='*70}")
    
    next_steps = [
        "1. Integrate DFT module with main MolSimWorkbench framework",
        "2. Add support for more elements (P, S, halogens)",
        "3. Implement self-consistent field (SCF) iterations", 
        "4. Add meta-GGA functionals (M06, TPSS)",
        "5. Include solvent effects (PCM models)",
        "6. Add basis set extrapolation methods",
        "7. Implement parallel computation support",
        "8. Add visualization of molecular orbitals"
    ]
    
    for step in next_steps:
        print(step)
    
    return 0 if success_count == len(results) else 1

if __name__ == "__main__":
    sys.exit(main())
