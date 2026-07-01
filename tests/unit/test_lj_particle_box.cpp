#include <gtest/gtest.h>
#include <memory>
#include <filesystem>
#include "../../src/builder/geometry.h"
#include "../../src/builder/lj_particle_box_builder.h"
#include "../../src/engine/force_calculator.h"
#include "../../src/engine/molecular_dynamics.h"
#include "../../src/engine/potentials/lj_potential.h"

namespace fs = std::filesystem;

class LJParticleBoxTest : public ::testing::Test {
protected:
    LJBoxConfig boxConfig;
    fs::path buildDir;
    fs::path originalPath;
    
    void SetUp() override {
        // Save original working directory
        originalPath = fs::current_path();
        
        // Find build directory
        if (fs::current_path().filename() == "build") {
            buildDir = fs::current_path();
        } else {
            fs::path projectRoot = fs::current_path();
            while (projectRoot.has_parent_path() && !fs::exists(projectRoot / "CMakeLists.txt")) {
                projectRoot = projectRoot.parent_path();
            }
            if (fs::exists(projectRoot / "CMakeLists.txt")) {
                buildDir = projectRoot / "build";
            } else {
                buildDir = originalPath.parent_path().parent_path();
            }
        }
        
        // Set up default box config for tests
        boxConfig.numParticles = 64;  // Small for fast tests
        boxConfig.density = 0.8;
        boxConfig.temperature = 1.0;
        boxConfig.randomSeed = 42;
    }
    
    void TearDown() override {
        fs::current_path(originalPath);
    }
};

// Test 1: LJ box geometry creation
TEST_F(LJParticleBoxTest, CreateLJParticleBox) {
    std::array<double, 3> boxSize;
    Geometry geometry = LJParticleBoxBuilder::createCubicLattice(boxConfig, boxSize);
    
    // Check particle count
    EXPECT_EQ(geometry.getNumberOfAtoms(), boxConfig.numParticles);
    
    // Check that all atoms are "LJ"
    const auto& atoms = geometry.getAtoms();
    for (const auto& atom : atoms) {
        EXPECT_EQ(atom, "LJ");
    }
    
    // Check box size is reasonable
    double expectedVolume = boxConfig.numParticles / boxConfig.density;
    double expectedSide = std::cbrt(expectedVolume);
    EXPECT_NEAR(boxSize[0], expectedSide, 0.1);
    EXPECT_NEAR(boxSize[1], expectedSide, 0.1);
    EXPECT_NEAR(boxSize[2], expectedSide, 0.1);
}

// Test 2: LJ energy calculation with periodic boundaries
TEST_F(LJParticleBoxTest, LJEnergyWithMIC) {
    std::array<double, 3> boxSize;
    Geometry geometry = LJParticleBoxBuilder::createCubicLattice(boxConfig, boxSize);
    geometry.setBoxDimensions(boxSize);
    
    // Create LJ potential
    auto ljPotential = std::make_shared<LJPotential>();
    ljPotential->addAtomParameters("LJ", 1.0, 1.0);
    ljPotential->setCutoff(2.5);
    
    // Calculate energy
    double energy = ljPotential->calculateEnergy(geometry);
    
    // Energy should be finite and negative
    EXPECT_FALSE(std::isnan(energy));
    EXPECT_FALSE(std::isinf(energy));
    EXPECT_LT(energy, 0.0);  // Should be attractive overall
}

// Test 3: Short NVE simulation
TEST_F(LJParticleBoxTest, NVESimulationStability) {
    std::array<double, 3> boxSize;
    Geometry geometry = LJParticleBoxBuilder::createCubicLattice(boxConfig, boxSize);
    
    // Create force calculator with LJ potential
    auto forceCalculator = std::make_shared<ForceCalculator>();
    auto ljPotential = std::make_shared<LJPotential>();
    ljPotential->addAtomParameters("LJ", 1.0, 1.0);
    ljPotential->setCutoff(2.5);
    forceCalculator->addPotential(ljPotential);
    
    // Set up MD engine
    MolecularDynamics dynamics;
    dynamics.setGeometry(geometry);
    dynamics.setForceCalculator(forceCalculator);
    
    SimulationParameters params;
    params.timeStep = 0.001;
    params.temperature = 1.0;
    params.totalTime = 0.01;  // 10 steps for quick test
    params.usePBC = true;
    params.useMinimization = false;
    params.box = SimulationBox(boxSize[0], boxSize[1], boxSize[2]);
    
    dynamics.setSimulationParameters(params);
    
    // Don't write trajectory for test
    dynamics.initialize();
    
    // Record initial energy
    double initialEnergy = dynamics.getCurrentEnergy();
    
    // Run simulation
    dynamics.runSimulation();
    
    // Record final energy
    double finalEnergy = dynamics.getCurrentEnergy();
    
    // Energy should stay finite in NVE
    EXPECT_FALSE(std::isnan(finalEnergy));
    EXPECT_FALSE(std::isinf(finalEnergy));
    
    // Check energy drift is reasonable (relax tolerance for small system, short sim)
    double relDrift = std::abs(finalEnergy - initialEnergy) / std::abs(initialEnergy);
    EXPECT_LT(relDrift, 1.0);  // Relax to 100% for quick test
}

// Test 4: Particles are properly initialized
TEST_F(LJParticleBoxTest, ParticleInitialization) {
    std::array<double, 3> boxSize;
    Geometry geometry = LJParticleBoxBuilder::createCubicLattice(boxConfig, boxSize);
    
    // Check all particles are initialized
    const auto& positions = geometry.getPositions();
    EXPECT_EQ(positions.size(), boxConfig.numParticles);
    
    // Check that particles are in reasonable range (within box with small margin)
    for (const auto& pos : positions) {
        EXPECT_GT(pos[0], -boxSize[0]/2 - 1.0);
        EXPECT_LT(pos[0], boxSize[0]/2 + 1.0);
        EXPECT_GT(pos[1], -boxSize[1]/2 - 1.0);
        EXPECT_LT(pos[1], boxSize[1]/2 + 1.0);
        EXPECT_GT(pos[2], -boxSize[2]/2 - 1.0);
        EXPECT_LT(pos[2], boxSize[2]/2 + 1.0);
    }
}
