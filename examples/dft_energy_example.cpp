#include <iostream>
#include <memory>
#include "../src/engine/potentials/dft_potential.h"
#include "../src/builder/geometry.h"
#include "../src/utils/yaml_parser.h"

int main() {
    std::cout << "=== DFT Energy Calculation Example ===" << std::endl;
    
    try {
        // Create a water molecule geometry
        Geometry waterMolecule;
        
        // Add water atoms (H2O)
        // Oxygen at origin
        waterMolecule.addAtom("O", {0.0, 0.0, 0.0});
        
        // Hydrogen atoms forming the typical water angle
        waterMolecule.addAtom("H", {0.757, 0.586, 0.0});
        waterMolecule.addAtom("H", {-0.757, 0.586, 0.0});
        
        std::cout << "Water molecule created with " << waterMolecule.getNumberOfAtoms() << " atoms:" << std::endl;
        for (size_t i = 0; i < waterMolecule.getNumberOfAtoms(); ++i) {
            const auto& pos = waterMolecule.getCoordinates(i);
            std::cout << "  " << waterMolecule.getAtomType(i) 
                      << " at (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
        }
        std::cout << std::endl;
        
        // Initialize DFT potential with parameters file
        std::cout << "Initializing DFT potential..." << std::endl;
        DFTPotential dftPotential("data/potentials/dft.yaml");
        std::cout << std::endl;
        
        // Calculate the DFT energy
        std::cout << "Calculating DFT energy..." << std::endl;
        double energy = dftPotential.calculatePotentialEnergy(waterMolecule);
        std::cout << std::endl;
        
        std::cout << "=== Final Results ===" << std::endl;
        std::cout << "Total DFT Energy: " << energy << " Hartree" << std::endl;
        std::cout << "Total DFT Energy: " << energy * 27.2114 << " eV" << std::endl;  // Convert to eV
        std::cout << "Total DFT Energy: " << energy * 627.509 << " kcal/mol" << std::endl;  // Convert to kcal/mol
        std::cout << std::endl;
        
        // Calculate forces
        std::cout << "Calculating forces..." << std::endl;
        std::vector<double> forces = dftPotential.calculateForces(waterMolecule);
        
        std::cout << "Forces on atoms (Hartree/Bohr):" << std::endl;
        for (size_t i = 0; i < waterMolecule.getNumberOfAtoms(); ++i) {
            std::cout << "  " << waterMolecule.getAtomType(i) << ": (" 
                      << forces[i*3] << ", " << forces[i*3+1] << ", " << forces[i*3+2] << ")" << std::endl;
        }
        
        // Calculate force magnitude for each atom
        std::cout << "\nForce magnitudes:" << std::endl;
        for (size_t i = 0; i < waterMolecule.getNumberOfAtoms(); ++i) {
            double fx = forces[i*3];
            double fy = forces[i*3+1];
            double fz = forces[i*3+2];
            double magnitude = std::sqrt(fx*fx + fy*fy + fz*fz);
            std::cout << "  " << waterMolecule.getAtomType(i) << ": " << magnitude << " Hartree/Bohr" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== DFT calculation completed successfully ===" << std::endl;
    return 0;
}
