#ifndef MD_INTEGRATOR_H
#define MD_INTEGRATOR_H

#include <vector>
#include <memory>
#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include "md_parameters.h"
#include "../../builder/geometry.h"
#include "../force_calculator.h"

class MDIntegrator {
public:
    MDIntegrator(const MDParameters& params = MDParameters())
        : params(params), stepCount(0), time(0.0), energy(0.0), 
          geometry(nullptr), forceCalculator(nullptr) {}
    
    virtual ~MDIntegrator() = default;
    
    // Initialize the integrator with geometry and force calculator
    virtual void initialize(Geometry& geom, std::shared_ptr<ForceCalculator> forceCalc) {
        geometry = &geom;
        forceCalculator = forceCalc;
        stepCount = 0;
        time = 0.0;
        
        // Initialize velocities and forces
        size_t numAtoms = geometry->getNumberOfAtoms();
        velocities.resize(numAtoms, {0.0, 0.0, 0.0});
        forces.resize(numAtoms, {0.0, 0.0, 0.0});
        
        // Get initial velocities from atom properties or initialize to zero
        for (size_t i = 0; i < numAtoms; ++i) {
            std::string vx = geometry->getProperty(i, "vx");
            std::string vy = geometry->getProperty(i, "vy");
            std::string vz = geometry->getProperty(i, "vz");
            
            if (!vx.empty() && !vy.empty() && !vz.empty()) {
                velocities[i][0] = std::stod(vx);
                velocities[i][1] = std::stod(vy);
                velocities[i][2] = std::stod(vz);
            }
        }
        
        // Calculate initial energy and forces
        energy = forceCalculator->calculateEnergy(*geometry);
        forceCalculator->calculateForces(*geometry, forces);
        
        // Open output files if needed
        if (params.writeTrajectory) {
            trajectoryFile.open(params.trajectoryFile);
            if (!trajectoryFile) {
                std::cerr << "Error opening trajectory file: " << params.trajectoryFile << std::endl;
            }
        }
        
        if (params.writeEnergy) {
            energyFile.open(params.energyFile);
            if (!energyFile) {
                std::cerr << "Error opening energy file: " << params.energyFile << std::endl;
            } else {
                // Write energy file header
                energyFile << "# Step\tTime(ps)\tEnergy(kJ/mol)\tTemperature(K)" << std::endl;
            }
        }
        
        // Write initial frame
        writeOutput(true);
        
        std::cout << "Initialized MD integrator with " << numAtoms << " atoms" << std::endl;
        std::cout << "Initial energy: " << energy << " kJ/mol" << std::endl;
    }
    
    // Initialize velocities with Maxwell-Boltzmann distribution
    virtual void initializeVelocities(double temperature = -1.0) {
        if (temperature < 0.0) {
            temperature = params.temperature;
        }
        
        if (!geometry) {
            std::cerr << "Error: Geometry not initialized" << std::endl;
            return;
        }
        
        // Set up random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> distribution(0.0, 1.0);
        
        // Boltzmann constant in kJ/(mol*K)
        const double kB = 8.3144621e-3;
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        
        // Generate random velocities from a Gaussian distribution
        for (size_t i = 0; i < numAtoms; ++i) {
            // For simplicity, assuming mass of 1.0 for all atoms
            // In a real implementation, you would use actual atomic masses
            double mass = 1.0;
            double sigma = std::sqrt(kB * temperature / mass);
            
            for (int d = 0; d < 3; ++d) {
                velocities[i][d] = distribution(gen) * sigma;
            }
        }
        
        // Remove center-of-mass motion
        removeCenterOfMassMotion();
        
        // Update velocity properties in geometry
        for (size_t i = 0; i < numAtoms; ++i) {
            geometry->setProperty(i, "vx", std::to_string(velocities[i][0]));
            geometry->setProperty(i, "vy", std::to_string(velocities[i][1]));
            geometry->setProperty(i, "vz", std::to_string(velocities[i][2]));
        }
        
        std::cout << "Initialized velocities at temperature " << temperature << " K" << std::endl;
        std::cout << "Current temperature: " << calculateTemperature() << " K" << std::endl;
    }
    
    // Perform a single integration step
    virtual void step() = 0;
    
    // Run the simulation for the specified number of steps
    virtual void run() {
        if (!geometry || !forceCalculator) {
            std::cerr << "Error: Integrator not initialized properly" << std::endl;
            return;
        }
        
        std::cout << "Running MD simulation for " << params.maxSteps << " steps" << std::endl;
        
        for (int i = 0; i < params.maxSteps; ++i) {
            // Take a step
            step();
            
            // Increment step counter and time
            stepCount++;
            time += params.timeStep;
            
            // Write output if needed
            writeOutput(false);
            
            // Print progress every 100 steps
            if ((stepCount % 100) == 0) {
                std::cout << "Step " << stepCount << "/" << params.maxSteps 
                          << ", Time: " << time << " ps, Energy: " << energy 
                          << " kJ/mol, Temperature: " << calculateTemperature() << " K" << std::endl;
            }
        }
        
        // Write final frame
        writeOutput(true);
        
        std::cout << "MD simulation completed after " << stepCount << " steps" << std::endl;
        std::cout << "Final time: " << time << " ps" << std::endl;
        std::cout << "Final energy: " << energy << " kJ/mol" << std::endl;
        std::cout << "Final temperature: " << calculateTemperature() << " K" << std::endl;
        
        // Close output files
        if (trajectoryFile.is_open()) {
            trajectoryFile.close();
        }
        
        if (energyFile.is_open()) {
            energyFile.close();
        }
    }
    
    // Getters
    int getStepCount() const { return stepCount; }
    double getTime() const { return time; }
    double getEnergy() const { return energy; }
    double getTemperature() const { return calculateTemperature(); }
    const std::vector<std::array<double, 3>>& getVelocities() const { return velocities; }
    const std::vector<std::array<double, 3>>& getForces() const { return forces; }
    
    // Set/get parameters
    void setParameters(const MDParameters& newParams) { params = newParams; }
    const MDParameters& getParameters() const { return params; }

protected:
    MDParameters params;
    int stepCount;
    double time;
    double energy;
    Geometry* geometry;
    std::shared_ptr<ForceCalculator> forceCalculator;
    std::vector<std::array<double, 3>> velocities;
    std::vector<std::array<double, 3>> forces;
    std::ofstream trajectoryFile;
    std::ofstream energyFile;
    
    // Calculate current temperature
    double calculateTemperature() const {
        if (!geometry || velocities.empty()) {
            return 0.0;
        }
        
        // Boltzmann constant in kJ/(mol*K)
        const double kB = 8.3144621e-3;
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        double kineticEnergy = 0.0;
        
        for (size_t i = 0; i < numAtoms; ++i) {
            // For simplicity, assuming mass of 1.0 for all atoms
            double mass = 1.0;
            
            for (int d = 0; d < 3; ++d) {
                kineticEnergy += 0.5 * mass * velocities[i][d] * velocities[i][d];
            }
        }
        
        // T = 2*Ekin / (3*N*kB)
        double temperature = 2.0 * kineticEnergy / (3.0 * numAtoms * kB);
        return temperature;
    }
    
    // Remove center-of-mass motion
    void removeCenterOfMassMotion() {
        if (!geometry || velocities.empty()) {
            return;
        }
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        std::array<double, 3> totalMomentum = {0.0, 0.0, 0.0};
        double totalMass = 0.0;
        
        // Calculate total momentum and mass
        for (size_t i = 0; i < numAtoms; ++i) {
            // For simplicity, assuming mass of 1.0 for all atoms
            double mass = 1.0;
            totalMass += mass;
            
            for (int d = 0; d < 3; ++d) {
                totalMomentum[d] += mass * velocities[i][d];
            }
        }
        
        // Calculate center of mass velocity
        std::array<double, 3> comVelocity = {
            totalMomentum[0] / totalMass,
            totalMomentum[1] / totalMass,
            totalMomentum[2] / totalMass
        };
        
        // Subtract center of mass velocity from all atoms
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                velocities[i][d] -= comVelocity[d];
            }
        }
    }
    
    // Scale velocities to match target temperature
    void scaleVelocities(double targetTemperature) {
        if (!geometry || velocities.empty()) {
            return;
        }
        
        double currentTemperature = calculateTemperature();
        if (currentTemperature < 1.0e-10) {
            // Temperature too low, can't scale
            return;
        }
        
        double scaleFactor = std::sqrt(targetTemperature / currentTemperature);
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                velocities[i][d] *= scaleFactor;
            }
        }
    }
    
    // Apply thermostat (Berendsen)
    void applyThermostat() {
        if (!params.useConstantTemperature) {
            return;
        }
        
        double currentTemperature = calculateTemperature();
        if (currentTemperature < 1.0e-10) {
            return;
        }
        
        // Berendsen thermostat scaling factor
        double lambda = std::sqrt(1.0 + (params.timeStep / params.couplingConstant) * 
                                 (params.temperature / currentTemperature - 1.0));
        
        size_t numAtoms = geometry->getNumberOfAtoms();
        for (size_t i = 0; i < numAtoms; ++i) {
            for (int d = 0; d < 3; ++d) {
                velocities[i][d] *= lambda;
            }
        }
    }
    
    // Write output files (trajectory and energy)
    void writeOutput(bool forceWrite) {
        if ((!forceWrite && stepCount % params.trajectoryFrequency != 0) &&
            (!forceWrite && stepCount % params.energyFrequency != 0)) {
            return;
        }
        
        // Write trajectory frame
        if ((forceWrite || stepCount % params.trajectoryFrequency == 0) && 
            trajectoryFile.is_open()) {
            size_t numAtoms = geometry->getNumberOfAtoms();
            trajectoryFile << numAtoms << std::endl;
            trajectoryFile << "# Step: " << stepCount << " Time: " << time 
                          << " ps Energy: " << energy << " kJ/mol" << std::endl;
            
            for (size_t i = 0; i < numAtoms; ++i) {
                std::string element = geometry->getElement(i);
                std::array<double, 3> coords = geometry->getCoordinates(i);
                
                trajectoryFile << element << " " 
                              << coords[0] << " " 
                              << coords[1] << " " 
                              << coords[2] << std::endl;
            }
        }
        
        // Write energy data
        if ((forceWrite || stepCount % params.energyFrequency == 0) && 
            energyFile.is_open()) {
            double temperature = calculateTemperature();
            energyFile << stepCount << "\t" 
                      << time << "\t" 
                      << energy << "\t" 
                      << temperature << std::endl;
        }
    }
};

#endif // MD_INTEGRATOR_H