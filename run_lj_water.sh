#!/bin/bash

# Sample run script for Lennard-Jones water simulation
# This script demonstrates running a complete MD simulation using LJ potentials

echo "=================================================="
echo "Lennard-Jones Water Simulation"
echo "=================================================="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: Run this script from the MolSimWorkbench root directory"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build

echo "📋 Configuration:"
echo "  - Potential: Lennard-Jones (LJ)"
echo "  - Molecule: Water (H2O)"
echo "  - Simulation: Molecular Dynamics"
echo "  - Output: Trajectory file"
echo ""

# Build the project
echo "🔨 Building the project..."
cd build
cmake .. > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

make > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed"
    exit 1
fi
echo "✅ Build successful"

# Check if executable exists
if [ ! -f "WaterLJTest" ]; then
    echo "❌ WaterLJTest executable not found"
    exit 1
fi

# Display configuration files being used
echo ""
echo "📄 Configuration files:"
echo "  - Simulation config: data/simulation_config.yaml"
echo "  - Water geometry: data/geometry/water.yaml"
echo "  - LJ parameters: data/potentials/lj_only.yaml"
echo ""

# Show current configuration
echo "🔍 Current LJ simulation parameters:"
echo "==================================="
cd ..
if [ -f "data/simulation_config.yaml" ]; then
    grep -A 10 "simulation:" data/simulation_config.yaml | head -15
fi
echo ""

# Run the LJ simulation
echo "🚀 Running LJ water simulation..."
echo "================================="
cd build
time ./WaterLJTest

# Check if trajectory file was created
if [ -f "water_trajectory.xyz" ]; then
    echo ""
    echo "✅ Simulation completed successfully!"
    echo "📊 Results:"
    echo "  - Trajectory file: build/water_trajectory.xyz"
    
    # Show trajectory file info
    line_count=$(wc -l < water_trajectory.xyz)
    echo "  - Trajectory frames: $((line_count / 5))"  # 3 atoms + 2 header lines per frame
    echo "  - File size: $(du -h water_trajectory.xyz | cut -f1)"
    
    echo ""
    echo "📁 Output files created:"
    ls -la *.xyz 2>/dev/null || echo "  No XYZ files found"
    ls -la *.dat 2>/dev/null || echo "  No data files found"
    
    echo ""
    echo "🎬 Visualization:"
    echo "  To view trajectory in VMD:"
    echo "    vmd build/water_trajectory.xyz"
    echo "  Or use any molecular viewer that supports XYZ format"
    
else
    echo "❌ Simulation failed - no trajectory file generated"
    exit 1
fi

echo ""
echo "=================================================="
echo "LJ Water Simulation Completed Successfully!"
echo "=================================================="
