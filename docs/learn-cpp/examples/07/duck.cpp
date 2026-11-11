// C++ does not duck-type: having the right methods is not enough.
#include <vector>
#include "potential.h"

class Duck {   // same three methods as Potential, but does not inherit from it
public:
    double calculateEnergy(const Geometry&) { return 0.0; }
    void calculateForces(const Geometry&, std::vector<Vec3>&) {}
    std::string getName() const { return "duck"; }
};

int main() {
    Duck duck;
    Potential* p = &duck;
    return 0;
}
