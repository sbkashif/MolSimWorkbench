#ifndef MOLECULAR_DYNAMICS_H
#define MOLECULAR_DYNAMICS_H

#include <vector>
#include <string>
#include <array>
#include <memory>
#include <iostream>
#include <cstdlib> // For getenv
#include "../builder/geometry.h"
#include "force_calculator.h"
#include "../utils/trajectory_writer.h" // Include for SimulationBox and TrajectoryFormat

// Forward declarations
class TrajectoryWriter;

// Enumeration for minimization methods
enum class MinimizationMethod {
    STEEPEST_DESCENT,
    CONJUGATE_GRADIENT,
    LBFGS
};

// Structure to hold energy minimization parameters
struct MinimizationParameters {
    MinimizationMethod method = MinimizationMethod::STEEPEST_DESCENT;
    double tolerance = 1e-6;            // Convergence tolerance (energy)
    double forceTolerance = 1e-4;       // Convergence tolerance (forces)
    int maxIterations = 1000;           // Maximum iterations
    double initialStepSize = 0.01;      // Initial step size
    double maxStepSize = 0.2;           // Maximum step size
    bool adaptiveStepSize = true;       // Use adaptive step size
};

// Structure to hold simulation parameters
struct SimulationParameters {
    double timeStep = 0.001;            // Time step in picoseconds
    double temperature = 300.0;         // Temperature in Kelvin
    double totalTime = 1.0;             // Total simulation time in picoseconds
    bool useMinimization = true;        // Perform energy minimization before dynamics
    MinimizationParameters minimization;// Energy minimization parameters
    
    // Periodic boundary conditions
    bool usePBC = false;                // Use periodic boundary conditions
    SimulationBox box;                  // Simulation box dimensions
    
    // Phase control for LJ box simulations
    std::size_t equilibrationSteps = 0; // Equilibration steps (NVE can skip with 0)
    std::size_t productionSteps = 0;    // Production steps (if 0, use totalTime)
    
    // Diagnostics
    double energyDriftTolerance = 0.01; // Relative energy drift threshold (NVE)
    unsigned int randomSeed = 0;        // Random seed for reproducibility (0 = auto)
};

class MolecularDynamics {
public:
    MolecularDynamics();
    ~MolecularDynamics();

    // Setup methods
    void setGeometry(const Geometry& geometry);
    void setForceCalculator(std::shared_ptr<ForceCalculator> forceCalc);
    void setSimulationParameters(const SimulationParameters& params);
    
    // Box handling methods
    void setBox(double a, double b, double c, double alpha = 90.0, double beta = 90.0, double gamma = 90.0);
    void setBox(const SimulationBox& box);
    const SimulationBox& getBox() const { return params.box; }
    
    // Trajectory file output methods
    void setTrajectoryOutput(const std::string& filename, TrajectoryFormat format = TrajectoryFormat::DCD, int writeFrequency = 1);
    
    // Initialization and simulation methods
    void initialize();
    void runSimulation();
    
    // Energy minimization methods
    bool minimizeEnergy();
    double calculatePotentialEnergy();
    void calculateForces();
    
    // Results and statistics
    double getCurrentEnergy() const { return currentEnergy; }
    const std::vector<std::array<double, 3>>& getCurrentForces() const { return forces; }
    const Geometry& getCurrentGeometry() const { return geometry; }

    // Export initial structure to PDB format for visualization
    bool exportToPDB(const std::string& filename) const;

private:
    // Molecular system data
    Geometry geometry;
    std::vector<std::array<double, 3>> velocities;
    std::vector<std::array<double, 3>> forces;
    
    // Simulation settings
    SimulationParameters params;
    std::shared_ptr<ForceCalculator> forceCalculator;
    
    // Trajectory output
    std::unique_ptr<TrajectoryWriter> trajectoryWriter;
    
    // Current state
    double currentEnergy;
    double potentialEnergy = 0.0;  // set by calculateForces(), same positions as `forces`
    int currentStep;
    
    // Helper methods for minimization
    bool minimizeSteepestDescent();
    bool minimizeConjugateGradient();
    bool minimizeLBFGS();
    
    // Helper methods for dynamics
    void initializeVelocities(double temperature);
    void integrateVelocityVerlet();
    void applyTemperatureControl();
    void applyPeriodicBoundaryConditions();

    // Helper methods to write PDB files  
    std::string generatePDBContent() const;
};

#endif // MOLECULAR_DYNAMICS_H