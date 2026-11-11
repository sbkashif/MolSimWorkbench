// mini_lj, chapter 2: two atoms, one distance. Just variables, types and arithmetic.
#include <cmath>
#include <iostream>

int main() {
    double epsilon = 1.0;   // well depth (reduced units)
    double sigma = 1.0;     // size of the atom
    double r = 1.2;         // distance between the two atoms

    double sr6 = std::pow(sigma / r, 6);
    double sr12 = sr6 * sr6;
    double energy = 4.0 * epsilon * (sr12 - sr6);

    // Size of the push. Positive = the atoms push apart, negative = they pull together.
    double force = 24.0 * epsilon * (2.0 * sr12 - sr6) / r;

    int numAtoms = 2;
    std::cout << "atoms  = " << numAtoms << std::endl;
    std::cout << "E(1.2) = " << energy << std::endl;
    std::cout << "F(1.2) = " << force << std::endl;
    return 0;
}
