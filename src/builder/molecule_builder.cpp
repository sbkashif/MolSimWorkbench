#include "molecule_builder.h"
#include "geometry.h"
#include <vector>
#include <string>
#include <iostream>
#include <array>

MoleculeBuilder::MoleculeBuilder() {
    // Default constructor
}

void MoleculeBuilder::addAtom(const std::string& atomType, const std::vector<double>& coordinates) {
    atomTypes.push_back(atomType);
    
    // Convert std::vector<double> to std::array<double, 3>
    std::array<double, 3> coordArray = {0.0, 0.0, 0.0};
    for (size_t i = 0; i < 3 && i < coordinates.size(); ++i) {
        coordArray[i] = coordinates[i];
    }
    
    atomCoordinates.push_back(coordArray);
}

Geometry MoleculeBuilder::buildMolecule() {
    Geometry geometry;
    for (size_t i = 0; i < atomTypes.size(); ++i) {
        geometry.addAtom(atomTypes[i], atomCoordinates[i]);
    }
    return geometry;
}