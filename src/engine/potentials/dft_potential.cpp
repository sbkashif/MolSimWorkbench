#include "dft_potential.h"
#include "../../utils/yaml_parser.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <nlohmann/json.hpp>

DFTPotential::DFTPotential(const std::string& parametersFile) {
    loadParameters(parametersFile);
}

double DFTPotential::calculateEnergy(const Geometry& geometry) {
    return calculatePotentialEnergy(geometry);
}

void DFTPotential::calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) {
    const std::vector<double> flatForces = calculateForces(geometry);
    const size_t numAtoms = geometry.getNumberOfAtoms();

    forces.assign(numAtoms, {0.0, 0.0, 0.0});
    for (size_t i = 0; i < numAtoms; ++i) {
        forces[i][0] = flatForces[i * 3];
        forces[i][1] = flatForces[i * 3 + 1];
        forces[i][2] = flatForces[i * 3 + 2];
    }
}

void DFTPotential::loadParameters(const std::string& parametersFile) {
    try {
        YAMLParser parser;
        nlohmann::json jsonData = parser.parseYAMLFile(parametersFile);
        
        // Load DFT parameters from YAML
        functional = jsonData["parameters"]["functional"].get<std::string>();
        basisSet = jsonData["parameters"]["basis_set"].get<std::string>();
        scfConvergence = jsonData["parameters"]["scf_convergence"].get<double>();
        maxScfCycles = jsonData["parameters"]["max_scf_cycles"].get<int>();
        gridQuality = jsonData["parameters"]["grid_quality"].get<std::string>();
        
        std::cout << "DFT Parameters loaded:" << std::endl;
        std::cout << "  Functional: " << functional << std::endl;
        std::cout << "  Basis Set: " << basisSet << std::endl;
        std::cout << "  SCF Convergence: " << scfConvergence << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading DFT parameters: " << e.what() << std::endl;
        // Set default parameters
        functional = "B3LYP";
        basisSet = "6-31G*";
        scfConvergence = 1.0e-6;
        maxScfCycles = 100;
        gridQuality = "fine";
    }

    // This is a placeholder, not DFT: the functional name only selects one of
    // three tables of per-atom energies in calculateElectronicEnergy. Checked
    // outside the try block so the error is not swallowed above.
    if (functional != "B3LYP" && functional != "PBE" && functional != "HF") {
        throw std::invalid_argument(
            "DFT functional \"" + functional + "\" is not recognised. The DFT "
            "potential is a placeholder that performs no DFT calculation; it has "
            "per-atom energy tables labelled B3LYP, PBE and HF only");
    }
}

double DFTPotential::calculatePotentialEnergy(const Geometry& geometry) {
    // Simplified DFT energy calculation for demonstration
    // In reality, this would involve solving the Kohn-Sham equations
    
    double electronicEnergy = calculateElectronicEnergy(geometry);
    double nuclearRepulsion = calculateNuclearRepulsion(geometry);
    
    double totalEnergy = electronicEnergy + nuclearRepulsion;
    
    std::cout << "DFT Energy Components:" << std::endl;
    std::cout << "  Electronic Energy: " << electronicEnergy << " Hartree" << std::endl;
    std::cout << "  Nuclear Repulsion: " << nuclearRepulsion << " Hartree" << std::endl;
    std::cout << "  Total Energy: " << totalEnergy << " Hartree" << std::endl;
    
    return totalEnergy;
}

double DFTPotential::calculateElectronicEnergy(const Geometry& geometry) {
    // More realistic DFT electronic energy calculation
    // Based on reference atomic energies and molecular corrections
    
    double energy = 0.0;
    size_t numAtoms = geometry.getNumberOfAtoms();
    
    // Reference atomic energies (approximate values in Hartree)
    std::map<std::string, double> atomicEnergies;
    
    // Set atomic energies based on functional
    if (functional == "B3LYP") {
        atomicEnergies = {
            {"H", -0.502},    // B3LYP/6-31G* hydrogen
            {"C", -37.845},   // B3LYP/6-31G* carbon
            {"N", -54.583},   // B3LYP/6-31G* nitrogen
            {"O", -75.064}    // B3LYP/6-31G* oxygen
        };
    } else if (functional == "PBE") {
        atomicEnergies = {
            {"H", -0.498},    // PBE hydrogen
            {"C", -37.821},   // PBE carbon
            {"N", -54.548},   // PBE nitrogen
            {"O", -75.032}    // PBE oxygen
        };
    } else if (functional == "HF") {
        atomicEnergies = {
            {"H", -0.500},    // Hartree-Fock hydrogen
            {"C", -37.689},   // HF carbon (less correlation)
            {"N", -54.401},   // HF nitrogen
            {"O", -74.810}    // HF oxygen
        };
    }
    // Any other functional was rejected in loadParameters.
    
    // Calculate atomic contributions
    for (size_t i = 0; i < numAtoms; ++i) {
        const std::string& atomType = geometry.getAtomType(i);
        
        if (atomicEnergies.count(atomType)) {
            energy += atomicEnergies[atomType];
        } else {
            // Default for unknown atoms
            energy -= 10.0;
        }
    }
    
    // Apply basis set corrections
    double basisCorrection = 1.0;
    if (basisSet == "STO-3G") {
        basisCorrection = 1.015;  // Minimal basis needs correction
    } else if (basisSet == "6-31G") {
        basisCorrection = 1.005;  // Split valence
    } else if (basisSet == "6-31G*" || basisSet == "6-31G(d)") {
        basisCorrection = 1.000;  // Reference basis
    } else if (basisSet == "6-31G**" || basisSet == "6-31G(d,p)") {
        basisCorrection = 0.998;  // Polarization functions improve energy
    } else if (basisSet == "cc-pVDZ") {
        basisCorrection = 0.995;  // Correlation consistent
    } else if (basisSet == "cc-pVTZ") {
        basisCorrection = 0.992;  // Triple zeta
    } else if (basisSet == "aug-cc-pVDZ") {
        basisCorrection = 0.990;  // Augmented basis
    }
    
    energy *= basisCorrection;
    
    // Add molecular bonding corrections (simplified)
    double bondingCorrection = calculateBondingCorrection(geometry);
    energy += bondingCorrection;
    
    return energy;
}

double DFTPotential::calculateNuclearRepulsion(const Geometry& geometry) {
    // Calculate nuclear-nuclear repulsion energy
    double repulsionEnergy = 0.0;
    size_t numAtoms = geometry.getNumberOfAtoms();
    
    // Atomic numbers map
    std::map<std::string, int> atomicNumbers = {
        {"H", 1}, {"C", 6}, {"N", 7}, {"O", 8}
    };
    
    for (size_t i = 0; i < numAtoms; ++i) {
        for (size_t j = i + 1; j < numAtoms; ++j) {
            const auto& pos1 = geometry.getCoordinates(i);
            const auto& pos2 = geometry.getCoordinates(j);
            
            // Calculate distance between atoms i and j
            double dx = pos1[0] - pos2[0];
            double dy = pos1[1] - pos2[1];
            double dz = pos1[2] - pos2[2];
            double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            // Get atomic numbers
            const std::string& atom1 = geometry.getAtomType(i);
            const std::string& atom2 = geometry.getAtomType(j);
            
            int Z1 = atomicNumbers.count(atom1) ? atomicNumbers[atom1] : 1;
            int Z2 = atomicNumbers.count(atom2) ? atomicNumbers[atom2] : 1;
            
            // Nuclear repulsion: Z1 * Z2 / r (in atomic units)
            if (distance > 1e-10) {  // Avoid division by zero
                repulsionEnergy += (Z1 * Z2) / distance;
            }
        }
    }
    
    return repulsionEnergy;
}

double DFTPotential::calculateBondingCorrection(const Geometry& geometry) {
    // Calculate bonding energy corrections based on molecular connectivity
    // This is a simplified model based on bond distances and types
    
    double bondingEnergy = 0.0;
    size_t numAtoms = geometry.getNumberOfAtoms();
    
    // Typical bond distances (in Bohr) and energies (in Hartree)
    std::map<std::pair<std::string, std::string>, std::pair<double, double>> bondParameters = {
        // {atom1, atom2} -> {typical_distance, bond_energy}
        {{"H", "H"}, {1.4, -0.174}},      // H-H bond
        {{"C", "H"}, {2.05, -0.168}},     // C-H bond
        {{"N", "H"}, {1.95, -0.159}},     // N-H bond
        {{"O", "H"}, {1.83, -0.189}},     // O-H bond
        {{"C", "C"}, {2.91, -0.347}},     // C-C bond
        {{"C", "N"}, {2.77, -0.308}},     // C-N bond
        {{"C", "O"}, {2.68, -0.360}},     // C-O bond
        {{"N", "N"}, {2.74, -0.159}},     // N-N bond
        {{"N", "O"}, {2.64, -0.201}},     // N-O bond
        {{"O", "O"}, {2.68, -0.159}}      // O-O bond
    };
    
    for (size_t i = 0; i < numAtoms; ++i) {
        for (size_t j = i + 1; j < numAtoms; ++j) {
            const std::string& atom1 = geometry.getAtomType(i);
            const std::string& atom2 = geometry.getAtomType(j);
            
            // Calculate distance
            const auto& pos1 = geometry.getCoordinates(i);
            const auto& pos2 = geometry.getCoordinates(j);
            double dx = pos1[0] - pos2[0];
            double dy = pos1[1] - pos2[1];
            double dz = pos1[2] - pos2[2];
            double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            // Check for bond (try both orderings)
            std::pair<std::string, std::string> atomPair1 = {atom1, atom2};
            std::pair<std::string, std::string> atomPair2 = {atom2, atom1};
            
            auto it = bondParameters.find(atomPair1);
            if (it == bondParameters.end()) {
                it = bondParameters.find(atomPair2);
            }
            
            if (it != bondParameters.end()) {
                double typicalDistance = it->second.first;
                double bondEnergy = it->second.second;
                
                // Apply bond if distance is within reasonable range (0.8 to 1.5 times typical)
                if (distance > 0.8 * typicalDistance && distance < 1.5 * typicalDistance) {
                    // Use Morse potential-like correction
                    double alpha = 1.5;  // Morse parameter
                    double deltaR = distance - typicalDistance;
                    double correction = bondEnergy * (1.0 - std::exp(-alpha * deltaR));
                    bondingEnergy += correction;
                }
            }
        }
    }
    
    return bondingEnergy;
}

std::vector<double> DFTPotential::calculateForces(const Geometry& geometry) {
    size_t numAtoms = geometry.getNumberOfAtoms();
    std::vector<double> forces(numAtoms * 3, 0.0);
    
    // Simplified force calculation using finite differences
    const double delta = 0.001;  // Small displacement for numerical derivatives
    
    double baseEnergy = calculatePotentialEnergy(geometry);
    
    for (size_t i = 0; i < numAtoms; ++i) {
        for (int coord = 0; coord < 3; ++coord) {
            // Create displaced geometry
            Geometry displacedGeometry = geometry;
            auto currentPos = displacedGeometry.getCoordinates(i);
            currentPos[coord] += delta;
            displacedGeometry.setCoordinates(i, currentPos);
            
            // Calculate energy at displaced position
            double displacedEnergy = calculateElectronicEnergy(displacedGeometry) + 
                                   calculateNuclearRepulsion(displacedGeometry);
            
            // Force is negative gradient: F = -dE/dx
            forces[i * 3 + coord] = -(displacedEnergy - baseEnergy) / delta;
        }
    }
    
    return forces;
}