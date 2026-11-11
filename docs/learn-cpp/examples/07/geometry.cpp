#include "geometry.h"

void Geometry::addAtom(const std::string& atomType, const Vec3& coordinates) {
    atomTypes.push_back(atomType);
    atomCoordinates.push_back(coordinates);
}
