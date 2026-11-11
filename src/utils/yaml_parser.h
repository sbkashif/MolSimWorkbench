#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <functional>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "../builder/geometry.h"
#include "../builder/lj_particle_box_builder.h"
#include "../engine/force_calculator.h"
#include "../engine/molecular_dynamics.h"

// Full simulation configuration including file references
struct SimulationConfig {
    std::string simulationType;             // "dynamics" or "minimization"
    std::string systemMode;                 // "geometry_file" (default) or "lj_particle_box"
    std::vector<std::string> potentialConfigFiles;
    std::string geometryFile;               // For geometry_file mode
    LJBoxConfig ljBoxConfig;                // For lj_particle_box mode (populated only if systemMode = "lj_particle_box")
    SimulationParameters simulationParams;
};

class YAMLParser {
public:
    // Turns a YAML file into JSON text.
    using Loader = std::function<std::string(const std::string&)>;

    YAMLParser();
    ~YAMLParser();

    // Replaces the default loader, which starts a separate Python process.
    // A web browser cannot start one, so the browser build sets a loader from
    // the Python it is already running in.
    static void setLoader(Loader loader);
    
    // Parse the main simulation configuration
    SimulationConfig parseSimulationConfig(const std::string& filename);
    
    // Parse geometry from a YAML file
    Geometry parseGeometry(const std::string& filename);
    
    // Parse a potential from a YAML file
    std::shared_ptr<Potential> parsePotential(const std::string& filename);
    
    // Generic method to parse any YAML file to JSON
    nlohmann::json parseYAMLFile(const std::string& filename);
    
private:
    static Loader loader;

    // Specialized parsers for different potential types
    std::shared_ptr<Potential> parseClassicalPotential(const nlohmann::json& json);
    std::shared_ptr<Potential> parseQuantumPotential(const nlohmann::json& json, const std::string& sourcePath);
    std::shared_ptr<Potential> parseMLPotential(const nlohmann::json& json, const std::string& sourcePath);
    
    // Helper methods
    std::string resolvePath(const std::string& relativePath);
};

#endif // YAML_PARSER_H