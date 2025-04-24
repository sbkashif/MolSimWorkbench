#include "geometry.h"
#include <vector>
#include <string>
#include <map>
#include <stdexcept>

Geometry::Geometry() {}

void Geometry::addAtom(const std::string& atomType, const std::array<double, 3>& coordinates) {
    atomTypes.push_back(atomType);
    atomCoordinates.push_back(coordinates);
}

size_t Geometry::getNumberOfAtoms() const {
    return atomTypes.size();
}

const std::string& Geometry::getAtomType(size_t index) const {
    return atomTypes.at(index);
}

const std::array<double, 3>& Geometry::getCoordinates(size_t index) const {
    return atomCoordinates.at(index);
}

void Geometry::setCoordinates(size_t index, const std::array<double, 3>& newCoordinates) {
    if (index < atomCoordinates.size()) {
        atomCoordinates[index] = newCoordinates;
    } else {
        throw std::out_of_range("Atom index out of range");
    }
}

void Geometry::setBoxDimensions(const std::array<double, 3>& dimensions) {
    boxDimensions = dimensions;
}