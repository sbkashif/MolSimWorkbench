"""
MolSimWorkbench - Python API for Molecular Simulation Experiments

A molecular simulation workbench for educational purposes that allows 
experimentation with different potentials (LJ, DFT) through both C++ 
and Python interfaces.
"""

import os
import sys
import subprocess
from pathlib import Path

class MolSimWorkbench:
    """Python API for the MolSimWorkbench"""
    
    def __init__(self):
        """Initialize the workbench"""
        self.project_root = Path(__file__).parent
        self.build_dir = self.project_root / "build"
        self.examples_dir = self.project_root / "examples"
        
    def run_lj_simulation(self, output_dir=None):
        """Run a Lennard-Jones water simulation
        
        Args:
            output_dir: Directory to store output files (default: examples/output)
        
        Returns:
            Path to trajectory file
        """
        # Ensure build directory exists
        if not self.build_dir.exists():
            raise RuntimeError("Build directory not found. Please build the project first.")
            
        # Run the water LJ simulation
        water_lj_script = self.examples_dir / "lj" / "water_lj_sim.py"
        if not water_lj_script.exists():
            raise RuntimeError(f"Water LJ simulation script not found: {water_lj_script}")
            
        result = subprocess.run([sys.executable, str(water_lj_script)], 
                              capture_output=True, text=True, cwd=str(self.project_root))
        
        if result.returncode != 0:
            raise RuntimeError(f"Simulation failed: {result.stderr}")
            
        return self.examples_dir / "lj" / "output" / "water_trajectory.xyz"
    
    def run_tests(self):
        """Run all unit tests"""
        os.chdir(self.build_dir)
        result = subprocess.run(["ctest", "--verbose"], capture_output=True, text=True)
        
        if result.returncode != 0:
            raise RuntimeError(f"Tests failed: {result.stderr}")
            
        return result.stdout
    
    def get_available_potentials(self):
        """Get list of available potential types"""
        potentials_dir = self.project_root / "data" / "potentials"
        potentials = []
        
        for pot_file in potentials_dir.glob("*.yaml"):
            potentials.append(pot_file.stem)
            
        return potentials

# Convenience function for direct usage
def create_workbench():
    """Create a new MolSimWorkbench instance"""
    return MolSimWorkbench()

if __name__ == "__main__":
    # Example usage
    wb = create_workbench()
    print("Available potentials:", wb.get_available_potentials())
    
    # Run a simple LJ simulation
    try:
        trajectory = wb.run_lj_simulation()
        print(f"Simulation completed. Trajectory saved to: {trajectory}")
    except Exception as e:
        print(f"Error: {e}")