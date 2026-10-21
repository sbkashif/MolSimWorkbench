#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <array>
#include <string>
#include <vector>

using Vec3 = std::array<double, 3>;

// The atoms and the box. Same public shape as src/builder/geometry.h.
class Geometry {
public:
    void addAtom(const std::string& atomType, const Vec3& coordinates);
    std::size_t getNumberOfAtoms() const { return atomTypes.size(); }

    const std::vector<std::string>& getAtoms() const { return atomTypes; }
    const std::vector<Vec3>& getPositions() const { return atomCoordinates; }

    void setBoxDimensions(const Vec3& dimensions) { boxDimensions = dimensions; }
    const Vec3& getBoxDimensions() const { return boxDimensions; }

private:
    std::vector<std::string> atomTypes;
    std::vector<Vec3> atomCoordinates;
    Vec3 boxDimensions = {0.0, 0.0, 0.0};
};

#endif
