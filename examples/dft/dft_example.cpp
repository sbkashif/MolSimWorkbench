#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <cmath>
#include <fstream>
#include <sstream>
#include <memory>
#include <stdexcept>

// Use the Python YAML parser for configuration
class YAMLDFTConfig {
private:
    std::string functional;
    std::string basisSet;
    double scfConvergence;
    int maxScfCycles;
    std::string gridQuality;
    
public:
    YAMLDFTConfig(const std::string& yamlFile) {
        loadFromYAML(yamlFile);
    }
    
    void loadFromYAML(const std::string& yamlFile) {
        // Use the Python script to parse YAML (located in build directory)
        std::string pythonScript = "yaml_parser.py";
        std::string command = "python \"" + pythonScript + "\" \"" + yamlFile + "\"";
        
        // Execute and capture output
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to execute Python YAML parser");
        }
        
        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // Simple JSON parsing (looking for specific keys)
        parseJSONConfig(result);
        
        std::cout << "Loaded DFT Parameters from " << yamlFile << ":" << std::endl;
        std::cout << "  Functional: " << functional << std::endl;
        std::cout << "  Basis Set: " << basisSet << std::endl;
        std::cout << "  SCF Convergence: " << scfConvergence << std::endl;
        std::cout << "  Max SCF Cycles: " << maxScfCycles << std::endl;
        std::cout << "  Grid Quality: " << gridQuality << std::endl << std::endl;
    }
    
    void parseJSONConfig(const std::string& jsonStr) {
        // Simple parsing for our specific YAML structure
        // In a real implementation, use a proper JSON library
        
        functional = extractValue(jsonStr, "functional", "B3LYP");
        basisSet = extractValue(jsonStr, "basis_set", "6-31G*");
        gridQuality = extractValue(jsonStr, "grid_quality", "fine");
        
        std::string scfStr = extractValue(jsonStr, "scf_convergence", "1.0e-6");
        scfConvergence = std::stod(scfStr);
        
        std::string maxCyclesStr = extractValue(jsonStr, "max_scf_cycles", "100");
        maxScfCycles = std::stoi(maxCyclesStr);
    }
    
    std::string extractValue(const std::string& json, const std::string& key, const std::string& defaultValue) {
        // Simple string extraction - in real code, use proper JSON parser
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) {
            return defaultValue;
        }
        
        // Find the value after the colon
        pos = json.find(":", pos);
        if (pos == std::string::npos) {
            return defaultValue;
        }
        
        // Skip whitespace and quotes
        pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\"')) {
            pos++;
        }
        
        // Extract the value until comma, closing brace, or quote
        std::string value;
        while (pos < json.length() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\"') {
            value += json[pos];
            pos++;
        }
        
        // Trim whitespace
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }
        
        return value.empty() ? defaultValue : value;
    }
    
    std::string getFunctional() const { return functional; }
    std::string getBasisSet() const { return basisSet; }
    double getScfConvergence() const { return scfConvergence; }
};

// Enhanced DFT calculator that uses YAML configuration
class ConfigurableDFTPotential {
private:
    YAMLDFTConfig config;
    
public:
    ConfigurableDFTPotential(const std::string& configFile) : config(configFile) {}
    
    double calculateElectronicEnergy(const std::vector<std::string>& atoms, 
                                   const std::vector<std::array<double, 3>>& positions) {
        double energy = 0.0;
        
        // Simple model: energy scales with number of electrons
        for (const auto& atom : atoms) {
            if (atom == "H") {
                energy -= 0.5;  // Hydrogen electronic energy
            } else if (atom == "O") {
                energy -= 75.0;  // Oxygen electronic energy
            } else if (atom == "C") {
                energy -= 37.0;  // Carbon electronic energy
            } else if (atom == "N") {
                energy -= 54.0;  // Nitrogen electronic energy
            }
        }
        
        // Apply functional-specific corrections
        std::string functional = config.getFunctional();
        if (functional == "B3LYP") {
            energy *= 1.02;  // B3LYP correction factor
        } else if (functional == "PBE") {
            energy *= 1.01;  // PBE correction factor
        } else if (functional == "PW91") {
            energy *= 1.005; // PW91 correction factor
        }
        
        // Apply basis set corrections
        std::string basisSet = config.getBasisSet();
        if (basisSet == "6-31G*") {
            energy *= 0.998;  // 6-31G* correction
        } else if (basisSet == "cc-pVDZ") {
            energy *= 0.995;  // cc-pVDZ correction
        }
        
        return energy;
    }
    
    double calculateNuclearRepulsion(const std::vector<std::string>& atoms, 
                                   const std::vector<std::array<double, 3>>& positions) {
        double repulsionEnergy = 0.0;
        std::map<std::string, int> atomicNumbers = {
            {"H", 1}, {"C", 6}, {"N", 7}, {"O", 8}
        };
        
        for (size_t i = 0; i < atoms.size(); ++i) {
            for (size_t j = i + 1; j < atoms.size(); ++j) {
                // Calculate distance
                double dx = positions[i][0] - positions[j][0];
                double dy = positions[i][1] - positions[j][1];
                double dz = positions[i][2] - positions[j][2];
                double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
                
                // Get atomic numbers
                int Z1 = atomicNumbers.count(atoms[i]) ? atomicNumbers[atoms[i]] : 1;
                int Z2 = atomicNumbers.count(atoms[j]) ? atomicNumbers[atoms[j]] : 1;
                
                // Nuclear repulsion: Z1 * Z2 / r
                if (distance > 1e-10) {
                    repulsionEnergy += (Z1 * Z2) / distance;
                }
            }
        }
        
        return repulsionEnergy;
    }
    
    double calculateTotalEnergy(const std::vector<std::string>& atoms, 
                              const std::vector<std::array<double, 3>>& positions) {
        std::cout << "Computing DFT energy with " << config.getFunctional() 
                  << "/" << config.getBasisSet() << "..." << std::endl;
        
        double electronicEnergy = calculateElectronicEnergy(atoms, positions);
        double nuclearRepulsion = calculateNuclearRepulsion(atoms, positions);
        
        std::cout << "Energy Components:" << std::endl;
        std::cout << "  Electronic Energy: " << electronicEnergy << " Hartree" << std::endl;
        std::cout << "  Nuclear Repulsion: " << nuclearRepulsion << " Hartree" << std::endl;
        
        double totalEnergy = electronicEnergy + nuclearRepulsion;
        std::cout << "  Total Energy: " << totalEnergy << " Hartree" << std::endl;
        
        return totalEnergy;
    }
    
    std::vector<double> calculateForces(const std::vector<std::string>& atoms, 
                                      const std::vector<std::array<double, 3>>& positions) {
        std::vector<double> forces(atoms.size() * 3, 0.0);
        
        const double delta = 0.001;  // Small displacement for numerical derivatives
        double baseEnergy = calculateTotalEnergy(atoms, positions);
        
        std::cout << "\nCalculating forces via numerical differentiation..." << std::endl;
        
        for (size_t i = 0; i < atoms.size(); ++i) {
            for (int coord = 0; coord < 3; ++coord) {
                // Create displaced positions
                auto displacedPositions = positions;
                displacedPositions[i][coord] += delta;
                
                // Calculate energy at displaced position
                double electronicEnergy = calculateElectronicEnergy(atoms, displacedPositions);
                double nuclearRepulsion = calculateNuclearRepulsion(atoms, displacedPositions);
                double displacedEnergy = electronicEnergy + nuclearRepulsion;
                
                // Force is negative gradient: F = -dE/dx
                forces[i * 3 + coord] = -(displacedEnergy - baseEnergy) / delta;
            }
        }
        
        return forces;
    }
};

int main() {
    std::cout << "=== DFT Energy Calculation with YAML Configuration ===" << std::endl << std::endl;
    
    try {
        // Create a water molecule (H2O)
        std::vector<std::string> atoms = {"O", "H", "H"};
        std::vector<std::array<double, 3>> positions = {
            {0.0, 0.0, 0.0},        // Oxygen at origin
            {0.757, 0.586, 0.0},    // First hydrogen
            {-0.757, 0.586, 0.0}    // Second hydrogen
        };
        
        std::cout << "Water molecule geometry:" << std::endl;
        for (size_t i = 0; i < atoms.size(); ++i) {
            std::cout << "  " << atoms[i] << " at (" 
                      << positions[i][0] << ", " << positions[i][1] << ", " << positions[i][2] 
                      << ") Bohr" << std::endl;
        }
        std::cout << std::endl;
        
        // Initialize DFT calculator with YAML configuration
        ConfigurableDFTPotential dft("data/potentials/dft.yaml");
        
        // Calculate energy
        std::cout << "Calculating DFT energy..." << std::endl;
        double energy = dft.calculateTotalEnergy(atoms, positions);
        
        std::cout << std::endl << "=== Energy Results ===" << std::endl;
        std::cout << "Total DFT Energy: " << energy << " Hartree" << std::endl;
        std::cout << "Total DFT Energy: " << energy * 27.2114 << " eV" << std::endl;
        std::cout << "Total DFT Energy: " << energy * 627.509 << " kcal/mol" << std::endl;
        
        // Calculate forces
        std::vector<double> forces = dft.calculateForces(atoms, positions);
        
        std::cout << std::endl << "=== Force Results ===" << std::endl;
        std::cout << "Forces on atoms (Hartree/Bohr):" << std::endl;
        for (size_t i = 0; i < atoms.size(); ++i) {
            double fx = forces[i*3];
            double fy = forces[i*3+1];
            double fz = forces[i*3+2];
            double magnitude = std::sqrt(fx*fx + fy*fy + fz*fz);
            
            std::cout << "  " << atoms[i] << ": (" 
                      << fx << ", " << fy << ", " << fz << ") |F| = " << magnitude << std::endl;
        }
        
        // Create output directory for visualization files
        std::string outputDir = "../examples/dft/output";
        system(("mkdir -p " + outputDir).c_str());
        
        // Perform geometry optimization trajectory
        std::cout << std::endl << "=== Geometry Optimization ===" << std::endl;
        std::vector<std::vector<std::array<double, 3>>> trajectory;
        std::vector<double> energyTrajectory;
        
        // Start with slightly distorted geometry
        auto currentPositions = positions;
        currentPositions[1][0] += 0.2;  // Distort H1 position
        currentPositions[2][0] -= 0.15; // Distort H2 position
        currentPositions[1][1] -= 0.1;  // Distort H1 y-position
        
        std::cout << "Starting from distorted geometry..." << std::endl;
        std::cout << "Optimization steps:" << std::endl;
        
        // Simple optimization loop (steepest descent)
        double stepSize = 0.05;
        int maxSteps = 10;
        
        for (int step = 0; step < maxSteps; ++step) {
            // Calculate current energy and forces
            double currentEnergy = dft.calculateTotalEnergy(atoms, currentPositions);
            std::vector<double> currentForces = dft.calculateForces(atoms, currentPositions);
            
            // Store trajectory point
            trajectory.push_back(currentPositions);
            energyTrajectory.push_back(currentEnergy);
            
            std::cout << "Step " << step + 1 << ": Energy = " << currentEnergy 
                      << " Hartree, Max Force = ";
            
            // Find maximum force magnitude
            double maxForce = 0.0;
            for (size_t i = 0; i < atoms.size(); ++i) {
                double fx = currentForces[i*3];
                double fy = currentForces[i*3+1]; 
                double fz = currentForces[i*3+2];
                double forceMag = std::sqrt(fx*fx + fy*fy + fz*fz);
                if (forceMag > maxForce) maxForce = forceMag;
            }
            std::cout << maxForce << " Hartree/Bohr" << std::endl;
            
            // Update positions using forces (steepest descent)
            for (size_t i = 0; i < atoms.size(); ++i) {
                currentPositions[i][0] -= stepSize * currentForces[i*3];
                currentPositions[i][1] -= stepSize * currentForces[i*3+1];
                currentPositions[i][2] -= stepSize * currentForces[i*3+2];
            }
            
            // Check convergence
            if (maxForce < 0.1) {
                std::cout << "Geometry converged!" << std::endl;
                break;
            }
        }
        
        // Write geometry optimization trajectory
        std::ofstream trajFile(outputDir + "/dft_optimization.xyz");
        for (size_t step = 0; step < trajectory.size(); ++step) {
            trajFile << atoms.size() << "\n";
            trajFile << "DFT Geometry Optimization Step " << step + 1 
                     << ", Energy = " << energyTrajectory[step] << " Hartree\n";
            for (size_t i = 0; i < atoms.size(); ++i) {
                trajFile << atoms[i] << " " 
                         << trajectory[step][i][0] << " " 
                         << trajectory[step][i][1] << " " 
                         << trajectory[step][i][2] << "\n";
            }
        }
        trajFile.close();
        
        // Generate molecular structure file for visualization (final optimized geometry)
        std::ofstream structureFile(outputDir + "/water_structure.xyz");
        structureFile << atoms.size() << "\n";
        structureFile << "Water molecule - DFT optimized geometry\n";
        for (size_t i = 0; i < atoms.size(); ++i) {
            structureFile << atoms[i] << " " 
                         << currentPositions[i][0] << " " 
                         << currentPositions[i][1] << " " 
                         << currentPositions[i][2] << "\n";
        }
        structureFile.close();
        
        // Generate energy and force data file
        std::ofstream dataFile(outputDir + "/dft_results.dat");
        dataFile << "# DFT Calculation Results\n";
        dataFile << "# Energy (Hartree): " << energy << "\n";
        dataFile << "# Energy (eV): " << energy * 27.2114 << "\n";
        dataFile << "# Energy (kcal/mol): " << energy * 627.509 << "\n";
        dataFile << "#\n";
        dataFile << "# Forces on atoms (Hartree/Bohr):\n";
        dataFile << "# Atom X Y Z |F|\n";
        for (size_t i = 0; i < atoms.size(); ++i) {
            double fx = forces[i*3];
            double fy = forces[i*3+1];
            double fz = forces[i*3+2];
            double magnitude = std::sqrt(fx*fx + fy*fy + fz*fz);
            dataFile << atoms[i] << " " << fx << " " << fy << " " << fz << " " << magnitude << "\n";
        }
        dataFile.close();
        
        // Generate VMD visualization script
        std::ofstream vmdScript(outputDir + "/visualize_dft.vmd");
        vmdScript << "# VMD script for DFT results visualization\n";
        vmdScript << "# Load optimization trajectory\n";
        vmdScript << "mol new dft_optimization.xyz type {xyz}\n";
        vmdScript << "mol delrep 0 top\n";
        vmdScript << "mol representation CPK 1.5 0.3 12.0 12.0\n";
        vmdScript << "mol addrep top\n";
        vmdScript << "mol representation DynamicBonds 3.0 0.1 12.0\n";
        vmdScript << "mol addrep top\n";
        vmdScript << "# Animation settings\n";
        vmdScript << "animate style Loop\n";
        vmdScript << "animate speed 0.8\n";
        vmdScript << "color Display Background white\n";
        vmdScript << "axes location Off\n";
        vmdScript << "display projection Orthographic\n";
        vmdScript << "display resize 800 600\n";
        vmdScript << "# Add labels for atoms\n";
        vmdScript << "label add Atoms 0/0\n";
        vmdScript << "label add Atoms 0/1\n";
        vmdScript << "label add Atoms 0/2\n";
        vmdScript << "# Color atoms by element\n";
        vmdScript << "color Name O red\n";
        vmdScript << "color Name H white\n";
        vmdScript << "# Start animation\n";
        vmdScript << "animate forward\n";
        vmdScript.close();
        
        // Generate simple electron density grid file for educational purposes
        std::ofstream densityFile(outputDir + "/electron_density.cube");
        densityFile << "DFT Electron Density (Educational Example)\n";
        densityFile << "Generated by MolSimWorkbench DFT module\n";
        densityFile << atoms.size() << " -2.0 -2.0 -2.0\n";  // Grid origin
        densityFile << "20 0.2 0.0 0.0\n";  // Grid dimensions
        densityFile << "20 0.0 0.2 0.0\n";
        densityFile << "20 0.0 0.0 0.2\n";
        // Write atom positions in cube format (use final optimized geometry)
        for (size_t i = 0; i < atoms.size(); ++i) {
            int atomic_num = (atoms[i] == "O") ? 8 : 1;
            densityFile << atomic_num << " 0.0 " 
                       << currentPositions[i][0] << " " 
                       << currentPositions[i][1] << " " 
                       << currentPositions[i][2] << "\n";
        }
        // Simple grid data (educational - not real electron density)
        for (int i = 0; i < 20*20*20; ++i) {
            if (i % 6 == 0) densityFile << "\n";
            densityFile << "0.001 ";
        }
        densityFile.close();
        
        std::cout << std::endl << "=== Output Files Generated ===" << std::endl;
        std::cout << "Optimization trajectory: " << outputDir << "/dft_optimization.xyz" << std::endl;
        std::cout << "Final structure: " << outputDir << "/water_structure.xyz" << std::endl;
        std::cout << "Results data: " << outputDir << "/dft_results.dat" << std::endl;
        std::cout << "VMD script: " << outputDir << "/visualize_dft.vmd" << std::endl;
        std::cout << "Density cube: " << outputDir << "/electron_density.cube" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << std::endl << "=== Calculation completed successfully ===" << std::endl;
    std::cout << std::endl << "Features demonstrated:" << std::endl;
    std::cout << "1. Loading DFT parameters from YAML configuration file" << std::endl;
    std::cout << "2. Functional-dependent energy calculations" << std::endl;
    std::cout << "3. Basis set corrections" << std::endl;
    std::cout << "4. Nuclear repulsion energy" << std::endl;
    std::cout << "5. Force calculations via numerical differentiation" << std::endl;
    std::cout << "6. Unit conversions (Hartree to eV and kcal/mol)" << std::endl;
    std::cout << "7. Geometry optimization with trajectory output" << std::endl;
    std::cout << "8. Visualization file generation (XYZ, VMD, cube formats)" << std::endl;
    std::cout << std::endl << "Note: This is a simplified DFT model for demonstration." << std::endl;
    std::cout << "Real DFT requires solving the Kohn-Sham equations iteratively." << std::endl;
    
    return 0;
}
