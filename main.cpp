#include <iostream>
#include <memory>
#include <vector>
#include "src/builder/molecule_builder.h"
#include "src/engine/molecular_dynamics.h"
#include "src/engine/force_calculator.h"
#include "src/utils/yaml_parser.h"

int main() {
    try {
        std::cout << "Starting molecular simulation..." << std::endl;
        
        // Parse simulation configuration
        YAMLParser yamlParser;
        auto simulationConfig = yamlParser.parseSimulationConfig("data/simulation_config.yaml");
        
        // Create force calculator and load potentials
        auto forceCalculator = std::make_shared<ForceCalculator>();
        for (const auto& potentialFile : simulationConfig.potentialConfigFiles) {
            std::cout << "Loading potential from: " << potentialFile << std::endl;
            auto potential = yamlParser.parsePotential(potentialFile);
            forceCalculator->addPotential(potential);
        }
        
        // Load geometry from separate file
        std::cout << "Loading geometry from: " << simulationConfig.geometryFile << std::endl;
        auto geometry = yamlParser.parseGeometry(simulationConfig.geometryFile);
        
        // Initialize molecular dynamics engine
        MolecularDynamics mdEngine;
        mdEngine.setGeometry(geometry);
        mdEngine.setForceCalculator(forceCalculator);
        mdEngine.setSimulationParameters(simulationConfig.simulationParams);
        
        // Initialize the simulation
        mdEngine.initialize();
        
        // Run simulation based on type (dynamics or minimization)
        if (simulationConfig.simulationType == "dynamics") {
            std::cout << "Running molecular dynamics simulation..." << std::endl;
            mdEngine.runSimulation();
        } else if (simulationConfig.simulationType == "minimization") {
            std::cout << "Running energy minimization..." << std::endl;
            bool converged = mdEngine.minimizeEnergy();
            if (converged) {
                std::cout << "Minimization converged successfully!" << std::endl;
            } else {
                std::cout << "Minimization did not converge within the maximum iterations." << std::endl;
            }
        }
        
        std::cout << "Simulation completed successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}