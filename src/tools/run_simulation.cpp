#include <iostream>
#include <memory>
#include <string>
#include "../utils/yaml_parser.h"
#include "../builder/lj_particle_box_builder.h"
#include "../engine/molecular_dynamics.h"
#include "../engine/force_calculator.h"
#include "../utils/trajectory_writer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: run_simulation <config_path> [trajectory_output]" << std::endl;
        return 1;
    }

    const std::string configPath = argv[1];
    const std::string trajectoryOutput = (argc >= 3) ? argv[2] : "trajectory.xyz";

    try {
        YAMLParser parser;
        SimulationConfig config = parser.parseSimulationConfig(configPath);

        // Load geometry based on system mode
        Geometry geometry;
        std::array<double, 3> boxSize;
        
        if (config.systemMode == "lj_particle_box") {
            std::cout << "\n=== LJ Particle Box Mode ===" << std::endl;
            // Initialize LJ particle box
            geometry = LJParticleBoxBuilder::createCubicLattice(config.ljBoxConfig, boxSize);
            config.simulationParams.usePBC = true;
            // Set box dimensions in simulation parameters
            config.simulationParams.box = SimulationBox(boxSize[0], boxSize[1], boxSize[2]);
            std::cout << "LJ box initialized: " << config.ljBoxConfig.numParticles << " particles\n" << std::endl;
        } else {
            std::cout << "\n=== Geometry File Mode ===" << std::endl;
            // Load from geometry file (default mode)
            geometry = parser.parseGeometry(config.geometryFile);
            std::cout << "Loaded geometry from: " << config.geometryFile << "\n" << std::endl;
        }

        // Load potentials
        auto forceCalculator = std::make_shared<ForceCalculator>();
        for (const auto& potentialFile : config.potentialConfigFiles) {
            forceCalculator->addPotential(parser.parsePotential(potentialFile));
        }

        // Set up and run molecular dynamics
        MolecularDynamics dynamics;
        dynamics.setGeometry(geometry);
        dynamics.setForceCalculator(forceCalculator);
        dynamics.setSimulationParameters(config.simulationParams);
        dynamics.setTrajectoryOutput(trajectoryOutput, TrajectoryFormat::XYZ, 1);
        dynamics.initialize();
        
        // Run simulation with phase control if specified
        if (config.simulationParams.equilibrationSteps > 0 || config.simulationParams.productionSteps > 0) {
            std::cout << "\n=== Running with Phase Control ===" << std::endl;
            if (config.simulationParams.equilibrationSteps > 0) {
                std::cout << "Equilibration phase: " << config.simulationParams.equilibrationSteps << " steps" << std::endl;
            }
            if (config.simulationParams.productionSteps > 0) {
                std::cout << "Production phase: " << config.simulationParams.productionSteps << " steps" << std::endl;
            }
        }
        
        dynamics.runSimulation();

        std::cout << "\nSimulation complete." << std::endl;
        std::cout << "Final energy: " << dynamics.getCurrentEnergy() << std::endl;
        std::cout << "Trajectory written to: " << trajectoryOutput << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Simulation failed: " << error.what() << std::endl;
        return 1;
    }
}
