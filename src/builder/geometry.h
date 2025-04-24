#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include <string>
#include <array>
#include <map>

// Forward declaration to avoid circular dependencies
class YAMLParser;

class Geometry {
public:
    Geometry();
    
    // Atom methods
    void addAtom(const std::string& atomType, const std::array<double, 3>& coordinates);
    size_t getNumberOfAtoms() const;
    const std::vector<std::string>& getAtoms() const { return atomTypes; }
    const std::vector<std::array<double, 3>>& getPositions() const { return atomCoordinates; }
    const std::string& getAtomType(size_t index) const;
    const std::array<double, 3>& getCoordinates(size_t index) const;
    void setCoordinates(size_t index, const std::array<double, 3>& newCoordinates);
    
    // Box methods
    void setBoxDimensions(const std::array<double, 3>& dimensions);
    const std::array<double, 3>& getBoxDimensions() const { return boxDimensions; }
    
private:
    std::vector<std::string> atomTypes;
    std::vector<std::array<double, 3>> atomCoordinates;
    std::array<double, 3> boxDimensions = {0.0, 0.0, 0.0};
};

#endif // GEOMETRY_H