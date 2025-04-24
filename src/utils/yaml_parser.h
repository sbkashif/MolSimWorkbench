#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "../builder/geometry.h"
#include "../engine/force_calculator.h"
#include "../engine/molecular_dynamics.h"

// Full simulation configuration including file references
struct SimulationConfig {
    std::string simulationType;             // "dynamics" or "minimization"
    std::vector<std::string> potentialConfigFiles;
    std::string geometryFile;
    SimulationParameters simulationParams;
};

class YAMLParser {
public:
    YAMLParser();
    ~YAMLParser();
    
    // Parse the main simulation configuration
    SimulationConfig parseSimulationConfig(const std::string& filename);
    
    // Parse geometry from a YAML file
    Geometry parseGeometry(const std::string& filename);
    
    // Parse a potential from a YAML file
    std::shared_ptr<Potential> parsePotential(const std::string& filename);
    
private:
    // Generic method to parse any YAML file to JSON
    nlohmann::json parseYAMLFile(const std::string& filename);
    
    // Specialized parsers for different potential types
    std::shared_ptr<Potential> parseClassicalPotential(const nlohmann::json& json);
    std::shared_ptr<Potential> parseQuantumPotential(const nlohmann::json& json);
    std::shared_ptr<Potential> parseMLPotential(const nlohmann::json& json);
    
    // Helper methods
    std::string resolvePath(const std::string& relativePath);
};

#endif // YAML_PARSER_H