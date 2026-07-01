# Water Molecule Visualization Example
#
# This example demonstrates how to:
# 1. Run a simulation on a water molecule
# 2. Visualize the resulting trajectory file
#
# Prerequisites:
# - VMD (Visual Molecular Dynamics) installed for visualization
# - The MolSimWorkbench built successfully

import os
import sys

# Print header
print("=" * 80)
print("WATER MOLECULE SIMULATION AND VISUALIZATION EXAMPLE")
print("=" * 80)

# Step 1: Determine the path to the project root and build directory
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, '..', '..'))  # Go up two levels from examples/lj/
build_dir = os.path.join(project_root, 'build')
output_dir = os.path.join(script_dir, 'output')

if project_root not in sys.path:
    sys.path.insert(0, project_root)

from molsimworkbench import Workbench

# Create output directory if it doesn't exist
if not os.path.exists(output_dir):
    os.makedirs(output_dir)
    print(f"Created output directory: {output_dir}")

# Check if the build directory exists
if not os.path.exists(build_dir):
    print(f"Build directory not found at {build_dir}")
    print("Please build the project first with:")
    print("  mkdir -p build && cd build && cmake .. && make")
    sys.exit(1)

# Step 2: Run simulation through the Python binding-backed API
print("\n[1/3] Running water molecule simulation...")
trajectory_file = os.path.join(output_dir, "water_trajectory.xyz")

try:
    Workbench.run("data/simulation_config.yaml", trajectory_file)
    print("Simulation completed successfully!")
except RuntimeError as error:
    print(f"Error running simulation: {error}")
    sys.exit(1)

print(f"\n[2/3] Trajectory file generated at: {trajectory_file}")

# Step 4: Create VMD script for visualization
print("\n[3/3] Setting up visualization...")
vmd_script_path = os.path.join(output_dir, "load_trajectory.vmd")

# Create VMD script
with open(vmd_script_path, 'w', encoding='utf-8') as f:
    f.write("# VMD script for water molecule visualization\n")
    f.write(f"mol new {trajectory_file} type {{xyz}} first 0 last -1 step 1 waitfor 1\n")
    f.write("mol delrep 0 top\n")
    f.write("mol representation CPK 1.0 0.3 12.0 12.0\n")
    f.write("mol addrep top\n")
    f.write("mol representation Lines 1.0\n")
    f.write("mol addrep top\n")
    f.write("animate style Loop\n")
    f.write("color Display Background white\n")
    f.write("axes location Off\n")
    f.write("display projection Orthographic\n")
    f.write("display resize 800 600\n")
    f.write("animate speed 0.5\n")
    f.write("animate forward\n")

print(f"VMD script created: {vmd_script_path}")

  
print("\nExample complete! Output files are available in:")
print(f"  {output_dir}")