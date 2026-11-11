#include <cmath>
#include <iostream>

double lj_energy(double r) {
    double sr6 = std::pow(1.0 / r, 6);
    return 4.0 * (sr6 * sr6 - sr6);
}

int main() {
    std::cout << "U(1.0)  = " << lj_energy(1.0) << "\n";
    std::cout << "U(1.12) = " << lj_energy(1.122462) << "\n";
    std::cout << "U(1.5)  = " << lj_energy(1.5) << "\n";
    std::cout << "U(2.5)  = " << lj_energy(2.5) << "\n";
    return 0;
}
