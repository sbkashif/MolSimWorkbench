#include <cmath>
#include <iostream>

double lj_energy(double r, double epsilon, double sigma) {
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

int main() {
    std::cout << "eps=1, sigma=1:   U(1.5) = " << lj_energy(1.5, 1.0, 1.0) << "\n";
    std::cout << "eps=2, sigma=1:   U(1.5) = " << lj_energy(1.5, 2.0, 1.0) << "\n";
    std::cout << "eps=1, sigma=1.2: U(1.5) = " << lj_energy(1.5, 1.0, 1.2) << "\n";
    return 0;
}
