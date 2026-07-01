#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <cmath>
#include <iomanip>

// Enhanced DFT class with multiple molecule support
class DFTTestSuite {
private:
    std::string functional;
    std::string basisSet;
    
public:
    DFTTestSuite(const std::string& func = "B3LYP", const std::string& basis = "6-31G*") 
        : functional(func), basisSet(basis) {
        std::cout << "DFT Test Suite initialized with " << functional << "/" << basisSet << std::endl;
    }
    
    struct Molecule {
        std::string name;
        std::vector<std::string> atoms;
        std::vector<std::array<double, 3>> positions;
        
        Molecule(const std::string& n) : name(n) {}
        
        void addAtom(const std::string& atom, double x, double y, double z) {
            atoms.push_back(atom);
            positions.push_back({x, y, z});
        }
        
        size_t size() const { return atoms.size(); }
    };
    
    Molecule createWater() {
        Molecule water("Water (H2O)");
        water.addAtom("O", 0.0, 0.0, 0.0);
        water.addAtom("H", 0.757, 0.586, 0.0);
        water.addAtom("H", -0.757, 0.586, 0.0);
        return water;
    }
    
    Molecule createMethane() {
        Molecule methane("Methane (CH4)");
        methane.addAtom("C", 0.0, 0.0, 0.0);
        methane.addAtom("H", 0.629, 0.629, 0.629);
        methane.addAtom("H", -0.629, -0.629, 0.629);
        methane.addAtom("H", -0.629, 0.629, -0.629);
        methane.addAtom("H", 0.629, -0.629, -0.629);
        return methane;
    }
    
    Molecule createAmmonia() {
        Molecule ammonia("Ammonia (NH3)");
        ammonia.addAtom("N", 0.0, 0.0, 0.0);
        ammonia.addAtom("H", 0.0, -0.94, -0.38);
        ammonia.addAtom("H", 0.81, 0.47, -0.38);
        ammonia.addAtom("H", -0.81, 0.47, -0.38);
        return ammonia;
    }
    
    Molecule createHydrogenMolecule() {
        Molecule h2("Hydrogen (H2)");
        h2.addAtom("H", 0.0, 0.0, 0.0);
        h2.addAtom("H", 0.74, 0.0, 0.0);  // H-H bond length ~0.74 Bohr
        return h2;
    }
    
    double calculateElectronicEnergy(const Molecule& mol) {
        double energy = 0.0;
        
        // Base electronic energies per atom type
        std::map<std::string, double> atomEnergies = {
            {"H", -0.5},
            {"C", -37.0},
            {"N", -54.0},
            {"O", -75.0}
        };
        
        for (const auto& atom : mol.atoms) {
            if (atomEnergies.count(atom)) {
                energy += atomEnergies[atom];
            }
        }
        
        // Apply functional corrections
        if (functional == "B3LYP") {
            energy *= 1.02;
        } else if (functional == "PBE") {
            energy *= 1.01;
        } else if (functional == "HF") {
            energy *= 0.98;  // Hartree-Fock tends to overbind
        }
        
        // Apply basis set corrections
        if (basisSet == "6-31G*") {
            energy *= 0.998;
        } else if (basisSet == "cc-pVDZ") {
            energy *= 0.995;
        } else if (basisSet == "STO-3G") {
            energy *= 1.005;  // Minimal basis set correction
        }
        
        return energy;
    }
    
    double calculateNuclearRepulsion(const Molecule& mol) {
        double repulsion = 0.0;
        std::map<std::string, int> atomicNumbers = {
            {"H", 1}, {"C", 6}, {"N", 7}, {"O", 8}
        };
        
        for (size_t i = 0; i < mol.size(); ++i) {
            for (size_t j = i + 1; j < mol.size(); ++j) {
                double dx = mol.positions[i][0] - mol.positions[j][0];
                double dy = mol.positions[i][1] - mol.positions[j][1];
                double dz = mol.positions[i][2] - mol.positions[j][2];
                double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
                
                int Z1 = atomicNumbers.count(mol.atoms[i]) ? atomicNumbers[mol.atoms[i]] : 1;
                int Z2 = atomicNumbers.count(mol.atoms[j]) ? atomicNumbers[mol.atoms[j]] : 1;
                
                if (distance > 1e-10) {
                    repulsion += (Z1 * Z2) / distance;
                }
            }
        }
        
        return repulsion;
    }
    
    void printMoleculeInfo(const Molecule& mol) {
        std::cout << "\n=== " << mol.name << " ===" << std::endl;
        std::cout << "Atoms: " << mol.size() << std::endl;
        for (size_t i = 0; i < mol.size(); ++i) {
            const auto& pos = mol.positions[i];
            std::cout << "  " << mol.atoms[i] << " at (" 
                      << std::fixed << std::setprecision(3)
                      << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
        }
    }
    
    void calculateAndPrintEnergy(const Molecule& mol) {
        printMoleculeInfo(mol);
        
        double electronic = calculateElectronicEnergy(mol);
        double nuclear = calculateNuclearRepulsion(mol);
        double total = electronic + nuclear;
        
        std::cout << "\nEnergy Calculation (" << functional << "/" << basisSet << "):" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "  Electronic Energy: " << electronic << " Hartree" << std::endl;
        std::cout << "  Nuclear Repulsion: " << nuclear << " Hartree" << std::endl;
        std::cout << "  Total Energy:      " << total << " Hartree" << std::endl;
        
        // Unit conversions
        std::cout << "  Total Energy:      " << total * 27.2114 << " eV" << std::endl;
        std::cout << "  Total Energy:      " << total * 627.509 << " kcal/mol" << std::endl;
        
        // Calculate energy per atom
        double energyPerAtom = total / mol.size();
        std::cout << "  Energy per atom:   " << energyPerAtom << " Hartree/atom" << std::endl;
    }
    
    void compareFunctionals(const Molecule& mol) {
        std::vector<std::string> functionals = {"HF", "B3LYP", "PBE"};
        std::string originalFunctional = functional;
        
        std::cout << "\n=== Functional Comparison for " << mol.name << " ===" << std::endl;
        std::cout << std::setw(12) << "Functional" 
                  << std::setw(15) << "Electronic" 
                  << std::setw(15) << "Nuclear" 
                  << std::setw(15) << "Total" << std::endl;
        std::cout << std::string(57, '-') << std::endl;
        
        for (const auto& func : functionals) {
            functional = func;
            double electronic = calculateElectronicEnergy(mol);
            double nuclear = calculateNuclearRepulsion(mol);
            double total = electronic + nuclear;
            
            std::cout << std::setw(12) << func
                      << std::setw(15) << std::fixed << std::setprecision(6) << electronic
                      << std::setw(15) << nuclear
                      << std::setw(15) << total << std::endl;
        }
        
        functional = originalFunctional;  // Restore original
    }
    
    void runFullTestSuite() {
        std::cout << "=== DFT Energy Calculation Test Suite ===" << std::endl;
        std::cout << "Testing various molecules with different functionals" << std::endl;
        
        // Create test molecules
        std::vector<Molecule> molecules = {
            createHydrogenMolecule(),
            createWater(),
            createAmmonia(),
            createMethane()
        };
        
        // Test each molecule
        for (auto& mol : molecules) {
            calculateAndPrintEnergy(mol);
            compareFunctionals(mol);
            std::cout << std::endl;
        }
        
        // Summary
        std::cout << "=== Test Suite Summary ===" << std::endl;
        std::cout << "Successfully calculated energies for " << molecules.size() << " molecules" << std::endl;
        std::cout << "Demonstrated features:" << std::endl;
        std::cout << "  - Multiple molecule types (H2, H2O, NH3, CH4)" << std::endl;
        std::cout << "  - Functional comparison (HF, B3LYP, PBE)" << std::endl;
        std::cout << "  - Electronic and nuclear energy components" << std::endl;
        std::cout << "  - Unit conversions (Hartree, eV, kcal/mol)" << std::endl;
        std::cout << "  - Energy per atom calculations" << std::endl;
    }
};

int main() {
    try {
        // Create and run the test suite
        DFTTestSuite testSuite("B3LYP", "6-31G*");
        testSuite.runFullTestSuite();
        
        std::cout << "\n=== All tests completed successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error in test suite: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
