#include <iostream>

#include "lj.h"

int main() {
    LJPotential argon(1.0, 1.0, 2.5);
    std::cout << "U(1.5) = " << argon.energy(1.5) << "\n";
    return 0;
}
