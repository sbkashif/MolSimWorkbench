#include <cmath>
#include <iostream>

int main() {
    double r = 1.5;
    double sr6 = std::pow(1.0 / r, 6);
    double energy = 4.0 * (sr6 * sr6 - sr6);
    std::cout << "U(" << r << ") = " << energy << "\n";
    std::cout << "r      is " << sizeof(r) << " bytes at " << &r << "\n";
    std::cout << "sr6    is " << sizeof(sr6) << " bytes at " << &sr6 << "\n";
    std::cout << "energy is " << sizeof(energy) << " bytes at " << &energy << "\n";
    return 0;
}
