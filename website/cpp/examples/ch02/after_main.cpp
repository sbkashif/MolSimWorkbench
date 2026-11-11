#include <cmath>
#include <iostream>

int main() {
    std::cout << lj_energy(1.5) << "\n";
    return 0;
}

double lj_energy(double r) {
    double sr6 = std::pow(1.0 / r, 6);
    return 4.0 * (sr6 * sr6 - sr6);
}
