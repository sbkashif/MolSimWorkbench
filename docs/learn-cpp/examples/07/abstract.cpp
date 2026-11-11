// A class with a pure virtual method cannot be made into an object.
#include "potential.h"

class Unfinished : public Potential {
public:
    double calculateEnergy(const Geometry&) override { return 0.0; }
    std::string getName() const override { return "unfinished"; }
    // calculateForces is missing
};

int main() {
    Potential p;      // error: Potential is abstract
    Unfinished u;     // error: Unfinished is still abstract
    return 0;
}
