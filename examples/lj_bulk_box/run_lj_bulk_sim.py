#!/usr/bin/env python3
"""
LJ Particle Box Simulation Example

Demonstrates how to run a full Lennard-Jones particle box simulation
with minimal setup: density, temperature, number of particles.
"""

import os
import sys

# Print header
print("=" * 80)
print("LENNARD-JONES PARTICLE BOX SIMULATION EXAMPLE")
print("=" * 80)

# Determine paths
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, '..', '..'))
build_dir = os.path.join(project_root, 'build')
output_dir = os.path.join(script_dir, 'output')

if project_root not in sys.path:
    sys.path.insert(0, project_root)

from molsimworkbench import Workbench

# Create output directory
if not os.path.exists(output_dir):
    os.makedirs(output_dir)
    print(f"Created output directory: {output_dir}\n")

# Check build directory
if not os.path.exists(build_dir):
    print(f"Build directory not found: {build_dir}")
    print("Please build the project first with:")
    print("  mkdir -p build && cd build && cmake .. && make")
    sys.exit(1)

# Run LJ bulk simulation
print("[1/3] Running LJ particle box simulation...")
trajectory_file = os.path.join(output_dir, "lj_bulk_trajectory.xyz")

try:
    # Run with LJ bulk config
    Workbench.run("data/lj_bulk_simulation.yaml", trajectory_file)
    print("✓ Simulation completed successfully!\n")
except RuntimeError as error:
    print(f"✗ Error: {error}")
    sys.exit(1)

# Check trajectory
print(f"[2/3] Trajectory written to: {trajectory_file}")
if os.path.exists(trajectory_file):
    with open(trajectory_file, 'r') as f:
        lines = f.readlines()
    num_frames = sum(1 for line in lines if line.strip().isdigit())
    print(f"✓ Trajectory contains {num_frames} frames\n")
else:
    print("✗ Trajectory file not found")
    sys.exit(1)

# Create VMD visualization script
print("[3/3] Setting up visualization...")
vmd_script_path = os.path.join(output_dir, "load_trajectory.vmd")

with open(vmd_script_path, 'w') as f:
    f.write("# VMD script for LJ particle box trajectory visualization\n")
    f.write(f"mol new {trajectory_file} type {{xyz}} first 0 last -1 step 1 waitfor 1\n")
    f.write("mol delrep 0 top\n")
    f.write("mol representation VDW 1.0 8.0\n")
    f.write("mol addrep top\n")
    f.write("animate style Loop\n")
    f.write("color Display Background white\n")
    f.write("display projection Orthographic\n")
    f.write("display resize 800 600\n")
    f.write("animate forward\n")

print(f"✓ VMD script created: {vmd_script_path}\n")

print("=" * 80)
print("Example complete!")
print("\nNext steps:")
print(f"  1. View trajectory in VMD:")
print(f"     cd {output_dir}")
print(f"     vmd load_trajectory.vmd")
print(f"\n  2. Edit {os.path.join(project_root, 'data/lj_bulk_simulation.yaml')} to change:")
print(f"     - num_particles: system size")
print(f"     - density: packing density")
print(f"     - temperature: initial velocity distribution")
print(f"     - total_steps: simulation duration")
print("=" * 80)
