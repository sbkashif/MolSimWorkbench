#ifndef MOLECULE_BUILDER_H
#define MOLECULE_BUILDER_H

#include <vector>
#include <string>
#include <array>
#include <map>
#include "geometry.h"

class MoleculeBuilder {
public:
    MoleculeBuilder();
    void addAtom(const std::string& atomType, const std::vector<double>& coordinates);
    Geometry buildMolecule();
    
private:
    std::vector<std::string> atomTypes;
    std::vector<std::array<double, 3>> atomCoordinates;
};

#endif // MOLECULE_BUILDER_H