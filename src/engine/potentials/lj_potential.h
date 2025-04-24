#ifndef LJ_POTENTIAL_H
#define LJ_POTENTIAL_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <array>
#include "../force_calculator.h"
#include "../../builder/geometry.h"

// Class for Lennard-Jones potential
class LJPotential : public Potential {
public:
    LJPotential();
    ~LJPotential() override;
    
    // Potential interface implementation
    double calculateEnergy(const Geometry& geometry) override;
    void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) override;
    std::string getName() const override { return "Lennard-Jones"; }
    
    // LJ-specific methods
    void addAtomParameters(const std::string& atomType, double epsilon, double sigma);
    void setCutoff(double cutoff) { this->cutoff = cutoff; }
    void setMixingRule(const std::string& rule) { this->mixingRule = rule; }
    
private:
    // Atom parameters
    struct AtomParameters {
        double epsilon;
        double sigma;
    };
    
    std::map<std::string, AtomParameters> atomParameters;
    double cutoff = 10.0;  // Default cutoff
    std::string mixingRule = "geometric";  // Default mixing rule
    
    // Helper methods
    std::pair<double, double> getMixedParameters(const std::string& type1, const std::string& type2);
    double calculatePairEnergy(double epsilon, double sigma, double distance);
    void calculatePairForce(double epsilon, double sigma, double distance, 
                           const std::array<double, 3>& direction,
                           std::array<double, 3>& force1,
                           std::array<double, 3>& force2);
};

#endif // LJ_POTENTIAL_H