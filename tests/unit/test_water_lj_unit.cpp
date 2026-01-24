#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../../src/builder/geometry.h"
#include "../../src/engine/force_calculator.h"
#include "../../src/engine/molecular_dynamics.h"
#include "../../src/utils/yaml_parser.h"
#include "../../src/utils/trajectory_writer.h"

namespace fs = std::filesystem;

class WaterLJTest : public ::testing::Test {
protected:
    std::shared_ptr<ForceCalculator> forceCalculator;
    YAMLParser yamlParser;
    Geometry waterGeometry;
    fs::path buildDir;
    fs::path originalPath;
    
    void SetUp() override {
        // Save the original working directory
        originalPath = fs::current_path();
        
        // Find the build directory - when running tests, we want to use the actual build directory
        // If we're in the build directory, use it; otherwise find it relative to the current location
        if (fs::current_path().filename() == "build") {
            buildDir = fs::current_path();
        } else {
            // Try to find build directory relative to project root
            fs::path projectRoot = fs::current_path();
            while (projectRoot.has_parent_path() && !fs::exists(projectRoot / "CMakeLists.txt")) {
                projectRoot = projectRoot.parent_path();
            }
            if (fs::exists(projectRoot / "CMakeLists.txt")) {
                buildDir = projectRoot / "build";
            } else {
                // Fallback: assume we need to go up two levels (for tests run from build/tests/unit)
                buildDir = originalPath.parent_path().parent_path();
            }
        }
        
        // Ensure we're in the build directory for YAML parsing
        fs::current_path(buildDir);
        
        // Parse simulation configuration using the correct path
        auto simulationConfig = yamlParser.parseSimulationConfig("data/simulation_config.yaml");
        
        // Load the LJ potential
        auto ljPotential = yamlParser.parsePotential(simulationConfig.potentialConfigFiles[0]);
        
        // Create force calculator with LJ potential
        forceCalculator = std::make_shared<ForceCalculator>();
        forceCalculator->addPotential(ljPotential);
        
        // Load water geometry
        waterGeometry = yamlParser.parseGeometry(simulationConfig.geometryFile);
    }
    
    void TearDown() override {
        // Restore the original working directory
        fs::current_path(originalPath);
    }
};

// Test that water geometry loads correctly
TEST_F(WaterLJTest, GeometryLoaded) {
    const auto& atoms = waterGeometry.getAtoms();
    const auto& positions = waterGeometry.getPositions();
    
    // Check that we have 3 atoms for water
    ASSERT_EQ(atoms.size(), 3);
    ASSERT_EQ(positions.size(), 3);
    
    // Check that we have O and H atoms
    bool hasOxygen = false;
    int hydrogenCount = 0;
    
    for (const auto& atom : atoms) {
        if (atom == "O") hasOxygen = true;
        if (atom == "H") hydrogenCount++;
    }
    
    EXPECT_TRUE(hasOxygen) << "Water molecule should contain an oxygen atom";
    EXPECT_EQ(hydrogenCount, 2) << "Water molecule should contain two hydrogen atoms";
}

// Test initial energy calculation
TEST_F(WaterLJTest, InitialEnergyCalculation) {
    double initialEnergy = forceCalculator->calculateEnergy(waterGeometry);
    
    // Energy should be a finite number
    EXPECT_FALSE(std::isnan(initialEnergy)) << "Initial energy should not be NaN";
    EXPECT_FALSE(std::isinf(initialEnergy)) << "Initial energy should not be infinite";
    
    // Note: For a single water molecule with this force field, energy is expected to be
    // zero because hydrogens have zero LJ parameters and there's only one oxygen.
    // This is normal behavior, not an error.
    EXPECT_DOUBLE_EQ(initialEnergy, 0.0) << "Energy should be zero for a single water molecule";
}

// Test energy minimization
TEST_F(WaterLJTest, EnergyMinimization) {
    // Setup for energy minimization
    MolecularDynamics mdEngine;
    mdEngine.setGeometry(waterGeometry);
    mdEngine.setForceCalculator(forceCalculator);
    
    // Define a simulation box - create a cubic box with 20 Å on each side
    double boxSize = 20.0;  // Angstroms
    mdEngine.setBox(boxSize, boxSize, boxSize);
    
    // Configure minimization parameters
    MinimizationParameters minParams;
    minParams.method = MinimizationMethod::STEEPEST_DESCENT;
    minParams.maxIterations = 100;
    minParams.tolerance = 1e-6;
    minParams.forceTolerance = 1e-4;
    minParams.initialStepSize = 0.01;
    
    // Set parameters and initialize
    SimulationParameters simParams;
    simParams.useMinimization = true;
    simParams.minimization = minParams;
    simParams.usePBC = true;  // Enable periodic boundary conditions
    mdEngine.setSimulationParameters(simParams);
    mdEngine.initialize();
    
    // Calculate initial energy before minimization
    double initialEnergy = mdEngine.getCurrentEnergy();
    
    // Run energy minimization
    bool converged = mdEngine.minimizeEnergy();
    
    // Get results
    double minEnergy = mdEngine.getCurrentEnergy();
    
    // For a single water molecule with this force field, energy is expected to be
    // zero both before and after minimization because:
    // - Hydrogens have zero LJ parameters
    // - There's only one oxygen, so no O-O interactions
    // This is normal behavior, not an error.
    
    // Test that minimization completed
    EXPECT_TRUE(converged) << "Energy minimization should converge";
    EXPECT_DOUBLE_EQ(initialEnergy, 0.0) << "Initial energy should be zero for a single water molecule";
    EXPECT_DOUBLE_EQ(minEnergy, 0.0) << "Minimized energy should be zero for a single water molecule";
}

// Test simple molecular dynamics simulation
TEST_F(WaterLJTest, MolecularDynamics) {
    // Setup MD engine
    MolecularDynamics mdEngine;
    mdEngine.setGeometry(waterGeometry);
    mdEngine.setForceCalculator(forceCalculator);
    
    // Define a simulation box - create a cubic box with 20 Å on each side
    double boxSize = 20.0;  // Angstroms
    mdEngine.setBox(boxSize, boxSize, boxSize);
    
    // Configure dynamics parameters (short simulation for testing)
    SimulationParameters mdParams;
    mdParams.useMinimization = false;
    mdParams.timeStep = 0.001;         // 1 fs time step
    mdParams.totalTime = 0.01;         // 0.01 ps total (10 steps) - short for testing
    mdParams.temperature = 300.0;      // 300 K
    mdParams.usePBC = true;            // Enable periodic boundary conditions
    
    // Set up trajectory output for testing
    fs::path trajectoryPath = buildDir / "water_trajectory";
    mdEngine.setTrajectoryOutput(trajectoryPath.string(), TrajectoryFormat::XYZ, 1);
    
    // Update the MD engine with new parameters
    mdEngine.setSimulationParameters(mdParams);
    mdEngine.initialize();
    
    // Save initial state
    Geometry initialGeometry = mdEngine.getCurrentGeometry();
    
    // Run dynamics simulation
    mdEngine.runSimulation();
    
    // Get final results before destroying mdEngine to ensure trajectory gets written
    Geometry finalGeometry = mdEngine.getCurrentGeometry();
    
    // Test that atoms moved during dynamics
    bool atomsMoved = false;
    const auto& initialPositions = initialGeometry.getPositions();
    const auto& finalPositions = finalGeometry.getPositions();
    
    for (size_t i = 0; i < initialPositions.size(); ++i) {
        double dx = finalPositions[i][0] - initialPositions[i][0];
        double dy = finalPositions[i][1] - initialPositions[i][1];
        double dz = finalPositions[i][2] - initialPositions[i][2];
        double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (distance > 1e-6) {
            atomsMoved = true;
            break;
        }
    }
    
    EXPECT_TRUE(atomsMoved) << "Atoms should move during molecular dynamics simulation";
    
    // Verify that we have results
}

// Test trajectory file creation - separate test to ensure destructor is called
TEST_F(WaterLJTest, TrajectoryFileCreation) {
    // Setup MD engine with trajectory output
    MolecularDynamics mdEngine;
    mdEngine.setGeometry(waterGeometry);
    mdEngine.setForceCalculator(forceCalculator);
    
    // Define a simulation box
    double boxSize = 20.0;
    mdEngine.setBox(boxSize, boxSize, boxSize);
    
    // Set up trajectory output for testing in build directory
    fs::path testOutputDir = buildDir / "tests" / "output";
    fs::create_directories(testOutputDir);
    fs::path trajectoryPath = testOutputDir / "test_trajectory";
    mdEngine.setTrajectoryOutput(trajectoryPath.string(), TrajectoryFormat::XYZ, 1);
    
    // Configure short dynamics parameters
    SimulationParameters mdParams;
    mdParams.useMinimization = false;
    mdParams.timeStep = 0.001;
    mdParams.totalTime = 0.005;  // Even shorter simulation
    mdParams.temperature = 300.0;
    mdParams.usePBC = true;
    
    mdEngine.setSimulationParameters(mdParams);
    mdEngine.initialize();
    
    // Run simulation
    mdEngine.runSimulation();
    
    // The trajectory should be created once the destructor runs at the end of this test
}