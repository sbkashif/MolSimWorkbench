#include <iostream>
#include <memory>
#include <string>
#include "../utils/yaml_parser.h"
#include "../engine/molecular_dynamics.h"
#include "../engine/force_calculator.h"
#include "../utils/trajectory_writer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: run_simulation <config_path> [trajectory_output]" << std::endl;
        return 1;
    }

    const std::string configPath = argv[1];
    const std::string trajectoryOutput = (argc >= 3) ? argv[2] : "water_trajectory.xyz";

    try {
        YAMLParser parser;
        SimulationConfig config = parser.parseSimulationConfig(configPath);

        Geometry geometry = parser.parseGeometry(config.geometryFile);

        auto forceCalculator = std::make_shared<ForceCalculator>();
        for (const auto& potentialFile : config.potentialConfigFiles) {
            forceCalculator->addPotential(parser.parsePotential(potentialFile));
        }

        MolecularDynamics dynamics;
        dynamics.setGeometry(geometry);
        dynamics.setForceCalculator(forceCalculator);
        dynamics.setSimulationParameters(config.simulationParams);
        dynamics.setTrajectoryOutput(trajectoryOutput, TrajectoryFormat::XYZ, 1);
        dynamics.initialize();
        dynamics.runSimulation();

        std::cout << "Simulation complete. Final energy: " << dynamics.getCurrentEnergy() << std::endl;
        std::cout << "Trajectory written to: " << trajectoryOutput << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Simulation failed: " << error.what() << std::endl;
        return 1;
    }
}
