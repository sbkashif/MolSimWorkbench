#include "yaml_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <filesystem>
#include <string>
#include "../engine/potentials/lj_potential.h"
#include "../engine/potentials/dft_potential.h"

YAMLParser::Loader YAMLParser::loader;

void YAMLParser::setLoader(Loader newLoader) {
    loader = std::move(newLoader);
}

YAMLParser::YAMLParser() {
    // Constructor
}

YAMLParser::~YAMLParser() {
    // Destructor
}

// Helper function to convert string to double, handling scientific notation
double stringToDouble(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to convert string to double: " + str);
    }
}

nlohmann::json YAMLParser::parseYAMLFile(const std::string& filename) {
    if (loader) {
        return nlohmann::json::parse(loader(resolvePath(filename)));
    }

    // Get the absolute path to the Python script
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string scriptPath;
    
    // Try multiple locations for the yaml_parser.py script
    std::vector<std::string> possiblePaths = {
        "yaml_parser.py",                                    // Current directory
        "../yaml_parser.py",                                 // Parent directory (from build/)
        "MolSimWorkbench/yaml_parser.py",                    // From parent of project
        "src/utils/yaml_parser.py",                         // Source directory
        (currentPath / "yaml_parser.py").string(),          // Absolute current
        (currentPath / ".." / "yaml_parser.py").string(),   // Absolute parent
        (currentPath / "MolSimWorkbench" / "yaml_parser.py").string(), // Absolute from parent
        (currentPath / "src" / "utils" / "yaml_parser.py").string()  // Absolute source
    };
    
    bool found = false;
    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path)) {
            scriptPath = std::filesystem::absolute(path).string();  // Use absolute path
            found = true;
            break;
        }
    }
    
    if (!found) {
        scriptPath = "yaml_parser.py";  // Default fallback
        std::cerr << "Warning: Python YAML parser script not found at any expected location" << std::endl;
    }
    
    // Resolve the input file path
    std::string resolvedFilePath = resolvePath(filename);
    
    // Build the command to execute the Python script
    std::string command = "python \"" + scriptPath + "\" \"" + resolvedFilePath + "\"";
    
    // Execute the command and capture output
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("Failed to execute Python YAML parser");
    }
    
    // Read the output
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // Parse the JSON output
    try {
        nlohmann::json jsonData = nlohmann::json::parse(result);
        return jsonData;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse YAML file: " + std::string(e.what()) + "\nOutput: " + result);
    }
}

SimulationConfig YAMLParser::parseSimulationConfig(const std::string& filename) {
    try {
        std::cout << "Starting to parse simulation config: " << filename << std::endl;
        nlohmann::json jsonData = parseYAMLFile(filename);
        std::cout << "Successfully parsed JSON from YAML" << std::endl;
        
        SimulationConfig config;
        
        // Extract simulation type
        std::cout << "Extracting simulation type..." << std::endl;
        if (jsonData.contains("simulation") && jsonData["simulation"].contains("type")) {
            config.simulationType = jsonData["simulation"]["type"].get<std::string>();
        } else {
            throw std::runtime_error("Missing simulation type in configuration");
        }
        
        // Extract system mode (default: geometry_file for backward compatibility)
        std::cout << "Extracting system mode..." << std::endl;
        config.systemMode = "geometry_file";  // Default
        if (jsonData["simulation"].contains("system_mode")) {
            config.systemMode = jsonData["simulation"]["system_mode"].get<std::string>();
            std::cout << "System mode set to: " << config.systemMode << std::endl;
        } else {
            std::cout << "No system_mode found, using default: geometry_file" << std::endl;
        }
        
        // Extract file references (required for geometry_file mode, optional for lj_particle_box)
        std::cout << "Extracting file references..." << std::endl;
        for (const auto& potentialFile : jsonData["potential_config_files"]) {
            // Add data/ prefix to make it relative to the project root
            config.potentialConfigFiles.push_back("data/" + potentialFile.get<std::string>());
        }
        
        // Only require geometry file for geometry_file mode
        if (config.systemMode == "geometry_file" && jsonData.contains("geometry_file")) {
            config.geometryFile = "data/" + jsonData["geometry_file"].get<std::string>();
        }
        
        // Parse LJ box configuration if in lj_particle_box mode
        std::cout << "Parsing system mode specific config..." << std::endl;
        if (config.systemMode == "lj_particle_box" && jsonData["simulation"].contains("lj_box")) {
            auto& lj = jsonData["simulation"]["lj_box"];
            config.ljBoxConfig.numParticles = lj.contains("num_particles") ? lj["num_particles"].get<std::size_t>() : 125;
            config.ljBoxConfig.density = lj.contains("density") ? lj["density"].get<double>() : 0.8;
            config.ljBoxConfig.temperature = lj.contains("temperature") ? lj["temperature"].get<double>() : 1.0;
            config.ljBoxConfig.initialization = lj.contains("initialization") ? lj["initialization"].get<std::string>() : "cubic_lattice";
            
            if (lj.contains("equilibration_steps")) {
                config.ljBoxConfig.equilibrationSteps = lj["equilibration_steps"].get<std::size_t>();
            }
            if (lj.contains("production_steps")) {
                config.ljBoxConfig.productionSteps = lj["production_steps"].get<std::size_t>();
            }
            if (lj.contains("random_seed")) {
                config.ljBoxConfig.randomSeed = lj["random_seed"].get<unsigned int>();
            }
        }
        
        // Extract simulation parameters - match with the actual SimulationParameters structure
        std::cout << "Parsing simulation parameters..." << std::endl;
        SimulationParameters& params = config.simulationParams;
        
        // Map YAML fields to SimulationParameters structure
        // Basic parameters
        std::cout << "Parsing timestep..." << std::endl;
        if (jsonData["simulation"]["timestep"].is_number()) {
            params.timeStep = jsonData["simulation"]["timestep"].get<double>();
        } else if (jsonData["simulation"]["timestep"].is_string()) {
            params.timeStep = stringToDouble(jsonData["simulation"]["timestep"].get<std::string>());
        }
        
        std::cout << "Parsing temperature..." << std::endl;
        if (jsonData["simulation"]["temperature"].is_number()) {
            params.temperature = jsonData["simulation"]["temperature"].get<double>();
        } else if (jsonData["simulation"]["temperature"].is_string()) {
            params.temperature = stringToDouble(jsonData["simulation"]["temperature"].get<std::string>());
        }
        
        // Convert total_steps to totalTime
        std::cout << "Parsing total steps..." << std::endl;
        if (jsonData["simulation"].contains("total_steps")) {
            int totalSteps = jsonData["simulation"]["total_steps"].get<int>();
            params.totalTime = totalSteps * params.timeStep;
        }
        
        // Minimization parameters
        std::cout << "Setting minimization flag..." << std::endl;
        if (config.simulationType == "minimization") {
            params.useMinimization = true;
        } else {
            params.useMinimization = false;
        }
        
        // Set minimization method based on algorithm string
        std::cout << "Parsing minimization algorithm..." << std::endl;
        std::string minimizationAlgo = jsonData["simulation"]["minimization"]["algorithm"].get<std::string>();
        if (minimizationAlgo == "steepest_descent") {
            params.minimization.method = MinimizationMethod::STEEPEST_DESCENT;
        } else if (minimizationAlgo == "conjugate_gradient") {
            params.minimization.method = MinimizationMethod::CONJUGATE_GRADIENT;
        } else if (minimizationAlgo == "lbfgs") {
            params.minimization.method = MinimizationMethod::LBFGS;
        }
        
        // Set other minimization parameters
        std::cout << "Parsing minimization maxIterations..." << std::endl;
        params.minimization.maxIterations = jsonData["simulation"]["minimization"]["max_iterations"].get<int>();
        
        std::cout << "Parsing minimization energy_tolerance..." << std::endl;
        if (jsonData["simulation"]["minimization"]["energy_tolerance"].is_number()) {
            params.minimization.tolerance = jsonData["simulation"]["minimization"]["energy_tolerance"].get<double>();
        } else if (jsonData["simulation"]["minimization"]["energy_tolerance"].is_string()) {
            // Handle scientific notation in strings like "1e-6"
            std::string toleranceStr = jsonData["simulation"]["minimization"]["energy_tolerance"].get<std::string>();
            params.minimization.tolerance = stringToDouble(toleranceStr);
        }
        
        std::cout << "Parsing minimization force_tolerance..." << std::endl;
        if (jsonData["simulation"]["minimization"]["force_tolerance"].is_number()) {
            params.minimization.forceTolerance = jsonData["simulation"]["minimization"]["force_tolerance"].get<double>();
        } else if (jsonData["simulation"]["minimization"]["force_tolerance"].is_string()) {
            // Handle scientific notation in strings like "1e-4"
            std::string forceToleranceStr = jsonData["simulation"]["minimization"]["force_tolerance"].get<std::string>();
            params.minimization.forceTolerance = stringToDouble(forceToleranceStr);
        }
        
        // Phase control and diagnostics (optional)
        std::cout << "Parsing phase control parameters..." << std::endl;
        if (jsonData["simulation"].contains("equilibration_steps")) {
            params.equilibrationSteps = jsonData["simulation"]["equilibration_steps"].get<std::size_t>();
        }
        if (jsonData["simulation"].contains("production_steps")) {
            params.productionSteps = jsonData["simulation"]["production_steps"].get<std::size_t>();
        }
        if (jsonData["simulation"].contains("random_seed")) {
            params.randomSeed = jsonData["simulation"]["random_seed"].get<unsigned int>();
        }
        if (jsonData["simulation"].contains("energy_drift_tolerance")) {
            params.energyDriftTolerance = jsonData["simulation"]["energy_drift_tolerance"].get<double>();
        }
        
        // Integrator and thermostat plugs (optional). Either a bare name,
        //   integrator: "leapfrog"
        // or a block with parameters,
        //   thermostat: { type: "langevin", friction: 1.0 }
        //   thermostat: { type: "berendsen", tau: 0.1 }
        const auto& sim = jsonData["simulation"];
        if (sim.contains("integrator")) {
            const auto& node = sim["integrator"];
            params.integrator = node.is_string() ? node.get<std::string>()
                                                 : node.at("type").get<std::string>();
        }
        if (sim.contains("thermostat")) {
            const auto& node = sim["thermostat"];
            if (node.is_null()) {
                params.thermostat = "none";           // `thermostat: null` or `~`
            } else if (node.is_string()) {
                params.thermostat = node.get<std::string>();
            } else {
                params.thermostat = node.at("type").get<std::string>();
                if (node.contains("tau")) {
                    params.thermostatTau = node["tau"].get<double>();
                }
                if (node.contains("friction")) {
                    params.langevinFriction = node["friction"].get<double>();
                }
            }
        }
        
        std::cout << "Simulation config parsing complete" << std::endl;
        return config;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing simulation config: " << e.what() << std::endl;
        throw; // Rethrow to let the caller handle it
    }
}

Geometry YAMLParser::parseGeometry(const std::string& filename) {
    nlohmann::json jsonData = parseYAMLFile(filename);
    Geometry geometry;
    
    // Extract molecule data
    std::string moleculeName = jsonData["molecule"]["name"];
    
    // Add atoms
    for (const auto& atomJson : jsonData["molecule"]["atoms"]) {
        std::string atomType = atomJson["type"];
        std::array<double, 3> position = {
            atomJson["position"][0],
            atomJson["position"][1],
            atomJson["position"][2]
        };
        
        geometry.addAtom(atomType, position);
    }
    
    // Set box dimensions if available
    if (jsonData.contains("box")) {
        std::array<double, 3> boxDimensions = {
            jsonData["box"]["dimensions"][0],
            jsonData["box"]["dimensions"][1],
            jsonData["box"]["dimensions"][2]
        };
        geometry.setBoxDimensions(boxDimensions);
    }
    
    return geometry;
}

std::shared_ptr<Potential> YAMLParser::parsePotential(const std::string& filename) {
    nlohmann::json jsonData = parseYAMLFile(filename);
    std::string resolvedSourcePath = resolvePath(filename);
    
    // Determine potential type
    std::string potentialType = jsonData["potential_type"];
    
    if (potentialType == "classical") {
        return parseClassicalPotential(jsonData);
    }
    else if (potentialType == "quantum") {
        return parseQuantumPotential(jsonData, resolvedSourcePath);
    }
    else if (potentialType == "machine_learning") {
        return parseMLPotential(jsonData, resolvedSourcePath);
    }
    else {
        throw std::runtime_error("Unknown potential type: " + potentialType);
    }
}

std::shared_ptr<Potential> YAMLParser::parseClassicalPotential(const nlohmann::json& json) {
    // For now, we'll only implement Lennard-Jones potential parsing
    // Loop through each component in the potential
    for (const auto& component : json["components"]) {
        std::string componentType = component["type"];
        
        if (componentType == "lennard_jones") {
            // Create a new LJ potential
            auto ljPotential = std::make_shared<LJPotential>();
            
            // Set the cutoff
            double cutoff = component["cutoff"];
            ljPotential->setCutoff(cutoff);
            
            // Parse atom parameters
            for (auto it = component["atom_parameters"].begin(); it != component["atom_parameters"].end(); ++it) {
                std::string atomType = it.key();
                double epsilon = it.value()["epsilon"];
                double sigma = it.value()["sigma"];
                
                ljPotential->addAtomParameters(atomType, epsilon, sigma);
            }
            
            // Set mixing rule if available
            if (component.contains("mixing_rule")) {
                ljPotential->setMixingRule(component["mixing_rule"]);
            }
            
            return ljPotential;
        }
        // Add other component types here (electrostatic, bond, etc.)
    }
    
    throw std::runtime_error("No recognized component found in classical potential");
}

std::shared_ptr<Potential> YAMLParser::parseQuantumPotential(const nlohmann::json& json, const std::string& sourcePath) {
    std::string method = "dft";
    if (json.contains("method") && json["method"].is_string()) {
        method = json["method"].get<std::string>();
    }

    if (method != "dft") {
        throw std::runtime_error("Unsupported quantum method: " + method);
    }

    return std::make_shared<DFTPotential>(sourcePath);
}

std::shared_ptr<Potential> YAMLParser::parseMLPotential(const nlohmann::json& json, const std::string& sourcePath) {
    // TODO: Implement machine learning potential parsing
    throw std::runtime_error("ML potential parsing not yet implemented");
}

std::string YAMLParser::resolvePath(const std::string& relativePath) {
    // Try multiple paths to find the file
    std::vector<std::string> pathsToTry = {
        relativePath,
        "MolSimWorkbench/" + relativePath,  // From parent directory
        "../" + relativePath,
        "../../" + relativePath
    };
    
    for (const auto& path : pathsToTry) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    // If none of the paths worked, return the original path
    // (the Python script will handle additional path resolution)
    return relativePath;
}