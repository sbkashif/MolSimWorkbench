#include "molecular_dynamics.h"
#include "force_calculator.h"
#include "../utils/trajectory_writer.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>

// Convert to class member
std::random_device rd;
std::mt19937 gen(rd());

MolecularDynamics::MolecularDynamics() 
    : currentEnergy(0.0), currentStep(0), trajectoryWriter(nullptr) {
    // Initialize default parameters
}

MolecularDynamics::~MolecularDynamics() {
    // No need to delete forceCalculator as it's a shared_ptr
}

// Set simulation box dimensions
void MolecularDynamics::setBox(double a, double b, double c, double alpha, double beta, double gamma) {
    params.box = SimulationBox(a, b, c, alpha, beta, gamma);
    params.usePBC = true;
    
    std::cout << "Simulation box set: " << a << " x " << b << " x " << c;
    if (alpha != 90.0 || beta != 90.0 || gamma != 90.0) {
        std::cout << " with angles " << alpha << ", " << beta << ", " << gamma;
    }
    std::cout << " Å" << std::endl;
    
    // If trajectory writer exists, update its box information
    if (trajectoryWriter) {
        trajectoryWriter->setBox(params.box);
    }
}

void MolecularDynamics::setBox(const SimulationBox& box) {
    setBox(box.a, box.b, box.c, box.alpha, box.beta, box.gamma);
}

// Set up trajectory writer for this simulation
void MolecularDynamics::setTrajectoryOutput(const std::string& filename, TrajectoryFormat format, int writeFrequency) {
    // Default to XYZ format for best compatibility
    TrajectoryFormat selectedFormat = TrajectoryFormat::XYZ;
    
    // Check for environment variable to override the format
    const char* formatEnv = std::getenv("TRAJECTORY_FORMAT");
    if (formatEnv) {
        std::string formatStr = formatEnv;
        if (formatStr == "XYZ") {
            selectedFormat = TrajectoryFormat::XYZ;
            std::cout << "Using XYZ trajectory format from environment variable" << std::endl;
        } else if (formatStr == "DCD") {
            selectedFormat = TrajectoryFormat::DCD;
            std::cout << "Using DCD trajectory format from environment variable" << std::endl;
        } else if (formatStr == "XTC") {
            selectedFormat = TrajectoryFormat::XTC;
            std::cout << "Using XTC trajectory format from environment variable" << std::endl;
        }
    } else if (format != TrajectoryFormat::XYZ) {
        // If not using XYZ but no environment override, log this
        selectedFormat = format;
        std::cout << "Note: Using " << (format == TrajectoryFormat::DCD ? "DCD" : "XTC") 
                  << " format. If visualization issues occur, use XYZ format instead." << std::endl;
    }
    
    // Check if an output path is specified in environment variable
    std::string outputFilename = filename;
    const char* outputPathEnv = std::getenv("OUTPUT_PATH");
    if (outputPathEnv) {
        std::string path = outputPathEnv;
        // Extract just the filename from the provided path
        size_t lastSlash = outputFilename.find_last_of("/\\");
        std::string baseFilename = (lastSlash != std::string::npos) ? 
                                  outputFilename.substr(lastSlash + 1) : 
                                  outputFilename;
        
        // Create the new path
        outputFilename = path + "/" + baseFilename;
        std::cout << "Writing trajectory to " << outputFilename << " (from environment variable)" << std::endl;
    }
    
    trajectoryWriter = std::make_unique<TrajectoryWriter>(outputFilename, selectedFormat);
    
    if (trajectoryWriter && trajectoryWriter->isOpen()) {
        trajectoryWriter->setWriteFrequency(writeFrequency);
        
        // Set box information if available
        if (params.usePBC) {
            trajectoryWriter->setBox(params.box);
        }
        
        std::string formatName;
        switch (selectedFormat) {
            case TrajectoryFormat::XYZ: formatName = "XYZ"; break;
            case TrajectoryFormat::DCD: formatName = "DCD"; break;
            case TrajectoryFormat::XTC: formatName = "XTC"; break;
            default: formatName = "Unknown";
        }
        
        std::cout << "Trajectory output set to " << outputFilename 
                  << " (" << formatName << " format)"
                  << ", writing every " << writeFrequency << " steps." << std::endl;
    }
    
    // If format is DCD, automatically generate a PDB structure file for visualization
    if (selectedFormat == TrajectoryFormat::DCD) {
        std::string pdbFilename = outputFilename.substr(0, outputFilename.find_last_of('.')) + "_structure.pdb";
        exportToPDB(pdbFilename);
        std::cout << "Created PDB structure file for DCD trajectory visualization: " << pdbFilename << std::endl;
    }
}

// Helper method to initialize velocities according to Maxwell-Boltzmann distribution
void MolecularDynamics::initializeVelocities(double temperature) {
    size_t numAtoms = geometry.getNumberOfAtoms();
    velocities.resize(numAtoms, {0.0, 0.0, 0.0});
    
    // Set up random number generator with Gaussian (normal) distribution
    // Mean velocity is 0, and standard deviation is sqrt(kB * T / m)
    // Using kB = 1 and m = 1 in reduced units (standard in MD)
    double stddev = std::sqrt(temperature);
    std::normal_distribution<double> dist(0.0, stddev);

    // A nonzero random_seed makes the run reproducible; 0 keeps the
    // random_device seed, so every run differs.
    if (params.randomSeed != 0) {
        gen.seed(params.randomSeed);
    }
    
    // Assign random velocities from the distribution
    std::cout << "Initializing velocities for temperature: " << temperature << " K" << std::endl;
    for (size_t i = 0; i < numAtoms; ++i) {
        for (int d = 0; d < 3; ++d) {
            velocities[i][d] = dist(gen);
        }
    }
    
    // Calculate center of mass velocity
    std::array<double, 3> vcom = {0.0, 0.0, 0.0};
    for (const auto& v : velocities) {
        vcom[0] += v[0];
        vcom[1] += v[1];
        vcom[2] += v[2];
    }
    
    // Normalize by number of atoms
    vcom[0] /= numAtoms;
    vcom[1] /= numAtoms;
    vcom[2] /= numAtoms;
    
    // Remove center of mass motion
    for (auto& v : velocities) {
        v[0] -= vcom[0];
        v[1] -= vcom[1];
        v[2] -= vcom[2];
    }
    
    // Calculate current kinetic energy and temperature
    double kineticEnergy = 0.0;
    for (const auto& v : velocities) {
        kineticEnergy += v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    }
    kineticEnergy *= 0.5;  // Assuming mass = 1 for all atoms
    
    // Scale velocities to exactly match target temperature
    double currentTemp = 2.0 * kineticEnergy / (3 * numAtoms);
    double scaleFactor = std::sqrt(temperature / currentTemp);
    
    std::cout << "Initial temperature before scaling: " << currentTemp << " K" << std::endl;
    
    for (auto& v : velocities) {
        v[0] *= scaleFactor;
        v[1] *= scaleFactor;
        v[2] *= scaleFactor;
    }
    
    // Recalculate kinetic energy to verify
    kineticEnergy = 0.0;
    for (const auto& v : velocities) {
        kineticEnergy += v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    }
    kineticEnergy *= 0.5;
    
    double finalTemp = 2.0 * kineticEnergy / (3 * numAtoms);
    std::cout << "Initial temperature after scaling: " << finalTemp << " K" << std::endl;
    std::cout << "Initial kinetic energy: " << kineticEnergy << std::endl;
}

void MolecularDynamics::setGeometry(const Geometry& geometry) {
    this->geometry = geometry;
    
    // Initialize velocities and forces arrays based on geometry size
    size_t numAtoms = geometry.getNumberOfAtoms();
    velocities.resize(numAtoms, {0.0, 0.0, 0.0});
    forces.resize(numAtoms, {0.0, 0.0, 0.0});
}

void MolecularDynamics::setForceCalculator(std::shared_ptr<ForceCalculator> forceCalc) {
    this->forceCalculator = forceCalc;
}

void MolecularDynamics::setSimulationParameters(const SimulationParameters& params) {
    this->params = params;
    
    // If PBC is enabled, ensure that trajectory writer knows about the box
    if (params.usePBC && trajectoryWriter) {
        trajectoryWriter->setBox(params.box);
    }
}

void MolecularDynamics::initialize() {
    // Validate that necessary components are set
    if (!forceCalculator) {
        throw std::runtime_error("Force calculator not set before initialization");
    }
    
    if (geometry.getNumberOfAtoms() == 0) {
        throw std::runtime_error("No atoms in geometry before initialization");
    }
    
    // Initialize velocities using Maxwell-Boltzmann distribution
    initializeVelocities(params.temperature);
    
    // Initialize forces array
    size_t numAtoms = geometry.getNumberOfAtoms();
    forces.resize(numAtoms, {0.0, 0.0, 0.0});
    
    // Calculate initial energy and forces
    currentEnergy = calculatePotentialEnergy();
    calculateForces();
    
    currentStep = 0;
}

void MolecularDynamics::runSimulation() {
    // Perform energy minimization if requested
    if (params.useMinimization) {
        bool converged = minimizeEnergy();
        std::cout << "Energy minimization " 
                  << (converged ? "converged" : "did not converge") 
                  << std::endl;
    }
    
    // Write initial frame to trajectory if writer is set up
    if (trajectoryWriter && trajectoryWriter->isOpen()) {
        trajectoryWriter->writeFrameWithVelocities(geometry, velocities, 0.0, "Initial configuration");
    }
    
    // Main simulation loop
    int totalSteps = static_cast<int>(params.totalTime / params.timeStep);
    
    for (currentStep = 0; currentStep < totalSteps; ++currentStep) {
        // Integrate equations of motion using velocity Verlet
        integrateVelocityVerlet();
        
        // Apply temperature control
        applyTemperatureControl();
        
        // Calculate potential energy
        double potentialEnergy = calculatePotentialEnergy();
        
        // Calculate kinetic energy
        double kineticEnergy = 0.0;
        for (const auto& v : velocities) {
            kineticEnergy += v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
        }
        kineticEnergy *= 0.5;  // Assuming mass = 1 for all atoms
        
        // Total energy = potential + kinetic
        currentEnergy = potentialEnergy + kineticEnergy;
        
        // Output if needed
        if (currentStep % 10 == 0) {
            double currentTemp = 2.0 * kineticEnergy / (3 * geometry.getNumberOfAtoms());
            std::cout << "Step " << currentStep 
                      << ", Potential Energy: " << potentialEnergy
                      << ", Kinetic Energy: " << kineticEnergy
                      << ", Total Energy: " << currentEnergy
                      << ", Temperature: " << currentTemp
                      << std::endl;
        }
        
        // Write frame to trajectory if needed
        if (trajectoryWriter && trajectoryWriter->isOpen() && 
            trajectoryWriter->shouldWriteFrame(currentStep)) {
            
            double time = currentStep * params.timeStep;
            std::string comment = "Step " + std::to_string(currentStep) + 
                                  ", PE: " + std::to_string(potentialEnergy) +
                                  ", KE: " + std::to_string(kineticEnergy);
                                  
            trajectoryWriter->writeFrameWithVelocities(geometry, velocities, time, comment);
        }
    }
    
    // Write final frame to trajectory
    if (trajectoryWriter && trajectoryWriter->isOpen()) {
        double time = currentStep * params.timeStep;
        trajectoryWriter->writeFrameWithVelocities(geometry, velocities, time, "Final configuration");
    }
}

bool MolecularDynamics::minimizeEnergy() {
    // Choose minimization method based on parameters
    switch (params.minimization.method) {
        case MinimizationMethod::STEEPEST_DESCENT:
            return minimizeSteepestDescent();
        case MinimizationMethod::CONJUGATE_GRADIENT:
            return minimizeConjugateGradient();
        case MinimizationMethod::LBFGS:
            return minimizeLBFGS();
        default:
            return minimizeSteepestDescent();
    }
}

double MolecularDynamics::calculatePotentialEnergy() {
    if (!forceCalculator) {
        throw std::runtime_error("Force calculator not set");
    }
    
    return forceCalculator->calculateEnergy(geometry);
}

void MolecularDynamics::calculateForces() {
    if (!forceCalculator) {
        throw std::runtime_error("Force calculator not set");
    }
    
    forceCalculator->calculateForces(geometry, forces);
}

bool MolecularDynamics::minimizeSteepestDescent() {
    // Implementation of steepest descent minimization
    double energyTolerance = params.minimization.tolerance;
    double forceTolerance = params.minimization.forceTolerance;
    int maxIterations = params.minimization.maxIterations;
    double stepSize = params.minimization.initialStepSize;
    
    // Calculate initial energy and forces
    double energy = calculatePotentialEnergy();
    calculateForces();
    
    // Write initial frame to trajectory if writer is set up
    if (trajectoryWriter && trajectoryWriter->isOpen()) {
        trajectoryWriter->writeFrame(geometry, 0.0, "Minimization start");
    }
    
    // Iterate until convergence or max iterations
    for (int iter = 0; iter < maxIterations; ++iter) {
        // Move atoms along the negative gradient (forces)
        for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
            std::array<double, 3> oldPos = geometry.getCoordinates(i);
            std::array<double, 3> newPos = {
                oldPos[0] + stepSize * forces[i][0],
                oldPos[1] + stepSize * forces[i][1],
                oldPos[2] + stepSize * forces[i][2]
            };
            geometry.setCoordinates(i, newPos);
        }
        
        // Calculate new energy and forces
        double newEnergy = calculatePotentialEnergy();
        calculateForces();
        
        // Check for convergence
        double maxForce = 0.0;
        for (const auto& f : forces) {
            double forceMag = std::sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
            maxForce = std::max(maxForce, forceMag);
        }
        
        double energyChange = std::abs(newEnergy - energy);
        energy = newEnergy;
        
        // Output minimization progress periodically
        if (iter % 10 == 0) {
            std::cout << "Minimization step " << iter 
                      << ", Energy: " << energy
                      << ", Max force: " << maxForce 
                      << std::endl;
            
            // Write frame to trajectory if set up and it's a multiple of the frequency
            if (trajectoryWriter && trajectoryWriter->isOpen() && 
                iter % trajectoryWriter->getWriteFrequency() == 0) {
                trajectoryWriter->writeFrame(geometry, iter, "Minimization step " + std::to_string(iter));
            }
        }
        
        // Check for convergence
        if (energyChange < energyTolerance && maxForce < forceTolerance) {
            currentEnergy = energy;
            
            // Write final minimization frame
            if (trajectoryWriter && trajectoryWriter->isOpen()) {
                trajectoryWriter->writeFrame(geometry, iter, "Minimization complete");
            }
            
            return true;  // Converged
        }
        
        // Adjust step size if needed
        if (params.minimization.adaptiveStepSize) {
            // Simple step size adaptation (could be improved)
            if (newEnergy < energy) {
                stepSize *= 1.2;  // Increase step size
            } else {
                stepSize *= 0.5;  // Decrease step size
            }
            
            // Clamp step size
            stepSize = std::min(stepSize, params.minimization.maxStepSize);
        }
    }
    
    currentEnergy = energy;
    
    // Write final minimization frame even if not converged
    if (trajectoryWriter && trajectoryWriter->isOpen()) {
        trajectoryWriter->writeFrame(geometry, maxIterations, "Minimization not converged");
    }
    
    return false;  // Did not converge
}

bool MolecularDynamics::minimizeConjugateGradient() {
    // Placeholder for conjugate gradient method
    std::cout << "Conjugate gradient minimization not yet implemented. Using steepest descent instead." << std::endl;
    return minimizeSteepestDescent();
}

bool MolecularDynamics::minimizeLBFGS() {
    // Placeholder for L-BFGS method
    std::cout << "L-BFGS minimization not yet implemented. Using steepest descent instead." << std::endl;
    return minimizeSteepestDescent();
}

void MolecularDynamics::integrateVelocityVerlet() {
    // Velocity Verlet integration
    double dt = params.timeStep;
    double dtSquaredHalf = 0.5 * dt * dt;
    
    // First half of velocity Verlet
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        // Update positions: r(t+dt) = r(t) + v(t)*dt + 0.5*f(t)*dt^2
        std::array<double, 3> pos = geometry.getCoordinates(i);
        std::array<double, 3> newPos = {
            pos[0] + velocities[i][0] * dt + forces[i][0] * dtSquaredHalf,
            pos[1] + velocities[i][1] * dt + forces[i][1] * dtSquaredHalf,
            pos[2] + velocities[i][2] * dt + forces[i][2] * dtSquaredHalf
        };
        geometry.setCoordinates(i, newPos);
        
        // Half-update velocities: v(t+dt/2) = v(t) + 0.5*f(t)*dt
        velocities[i][0] += forces[i][0] * 0.5 * dt;
        velocities[i][1] += forces[i][1] * 0.5 * dt;
        velocities[i][2] += forces[i][2] * 0.5 * dt;
    }
    
    // Apply periodic boundary conditions if needed
    if (params.usePBC) {
        applyPeriodicBoundaryConditions();
    }
    
    // Calculate forces at new positions
    calculateForces();
    
    // Second half of velocity Verlet
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        // Complete velocity update: v(t+dt) = v(t+dt/2) + 0.5*f(t+dt)*dt
        velocities[i][0] += forces[i][0] * 0.5 * dt;
        velocities[i][1] += forces[i][1] * 0.5 * dt;
        velocities[i][2] += forces[i][2] * 0.5 * dt;
    }
}

// Apply periodic boundary conditions to keep all atoms within the simulation box
void MolecularDynamics::applyPeriodicBoundaryConditions() {
    // Only apply if PBC is enabled
    if (!params.usePBC) return;
    
    // For now, we only implement simple PBC for orthorhombic boxes (90° angles)
    // More complex triclinic boxes would require matrix transformations
    
    // Process each atom
    for (size_t i = 0; i < geometry.getNumberOfAtoms(); ++i) {
        std::array<double, 3> pos = geometry.getCoordinates(i);
        bool changed = false;
        
        // Apply PBC in each dimension
        if (pos[0] < 0) {
            pos[0] += params.box.a;
            changed = true;
        } else if (pos[0] >= params.box.a) {
            pos[0] -= params.box.a;
            changed = true;
        }
        
        if (pos[1] < 0) {
            pos[1] += params.box.b;
            changed = true;
        } else if (pos[1] >= params.box.b) {
            pos[1] -= params.box.b;
            changed = true;
        }
        
        if (pos[2] < 0) {
            pos[2] += params.box.c;
            changed = true;
        } else if (pos[2] >= params.box.c) {
            pos[2] -= params.box.c;
            changed = true;
        }
        
        // Update position if needed
        if (changed) {
            geometry.setCoordinates(i, pos);
        }
    }
}

void MolecularDynamics::applyTemperatureControl() {
    // Simple velocity scaling for temperature control
    // More sophisticated methods like Nose-Hoover thermostat could be implemented
    
    // Calculate current kinetic energy
    double kineticEnergy = 0.0;
    for (const auto& v : velocities) {
        kineticEnergy += v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    }
    kineticEnergy *= 0.5;  // Assuming mass = 1 for all atoms
    
    // Calculate current temperature (kB = 1 in reduced units)
    double currentTemp = 2.0 * kineticEnergy / (3 * geometry.getNumberOfAtoms());
    
    // Scale velocities to match target temperature
    if (currentTemp > 1e-10) {  // Avoid division by zero
        double scaleFactor = std::sqrt(params.temperature / currentTemp);
        for (auto& v : velocities) {
            v[0] *= scaleFactor;
            v[1] *= scaleFactor;
            v[2] *= scaleFactor;
        }
    }
}

bool MolecularDynamics::exportToPDB(const std::string& filename) const {
    try {
        // Generate PDB content
        std::string pdbContent = generatePDBContent();
        
        // Write to file
        std::ofstream pdbFile(filename);
        if (!pdbFile.is_open()) {
            std::cerr << "Error: Could not open PDB file " << filename << " for writing." << std::endl;
            return false;
        }
        
        pdbFile << pdbContent;
        pdbFile.close();
        
        std::cout << "PDB structure file written to: " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error writing PDB file: " << e.what() << std::endl;
        return false;
    }
}

std::string MolecularDynamics::generatePDBContent() const {
    std::stringstream ss;
    
    // PDB Header
    ss << "TITLE     Molecular Simulation Generated by MolSimWorkbench\n";
    ss << "REMARK    Simulation box: " 
       << params.box.a << " x " << params.box.b << " x " << params.box.c 
       << " Angstroms\n";
    
    // Crystal record (simulation box)
    if (params.usePBC) {
        // Format: CRYST1  a      b      c     alpha  beta  gamma  space group  Z
        ss << "CRYST1"
           << std::fixed << std::setprecision(3)
           << std::setw(9) << params.box.a
           << std::setw(9) << params.box.b
           << std::setw(9) << params.box.c
           << std::setw(7) << params.box.alpha
           << std::setw(7) << params.box.beta
           << std::setw(7) << params.box.gamma
           << " P 1           1\n";
    }
    
    // Model start
    ss << "MODEL        1\n";
    
    // Atom records
    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    
    for (size_t i = 0; i < atoms.size(); ++i) {
        // Format: ATOM  serial atom  resName chainID resSeq  x       y       z       occupancy tempFactor element
        int serial = static_cast<int>(i) + 1;
        std::string element = atoms[i];
        
        // Handle atom name field (right-justified 4-char field)
        std::string atomName = element;
        if (atomName.length() == 1) {
            atomName = " " + atomName + "  "; // Pad single-char elements like H, O
        } else if (atomName.length() == 2) {
            atomName = " " + atomName + " ";  // Pad two-char elements like Na, Cl
        }
        
        // Determine residue name based on atom type
        std::string resName = "UNK"; // Default unknown
        if (element == "O" || element == "H") {
            resName = "HOH"; // Water
        } else if (element == "Na" || element == "Cl") {
            resName = "ION"; // Ions
        } else if (element == "C" || element == "N" || element == "S") {
            resName = "AAA"; // Generic organic
        }
        
        ss << "ATOM  "
           << std::setw(5) << serial << " "
           << std::left << std::setw(4) << atomName << std::right
           << std::setw(4) << resName << " "
           << "A"                            // Chain ID
           << std::setw(4) << 1              // Residue sequence
           << "    "                         // Extra space
           << std::fixed << std::setprecision(3)
           << std::setw(8) << positions[i][0]
           << std::setw(8) << positions[i][1]
           << std::setw(8) << positions[i][2]
           << std::setw(6) << std::setprecision(2) << 1.00  // Occupancy
           << std::setw(6) << std::setprecision(2) << 0.00  // Temperature factor
           << std::setw(12) << element           // Element symbol
           << "\n";
    }
    
    // Model end
    ss << "TER   " << std::setw(5) << (atoms.size() + 1) << "      " << "HOH A" << std::setw(4) << 1 << "\n";
    ss << "ENDMDL\n";
    ss << "END\n";
    
    return ss.str();
}