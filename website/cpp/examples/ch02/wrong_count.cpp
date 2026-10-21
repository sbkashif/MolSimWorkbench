#include <cmath>
#include <iostream>

double lj_energy(double r, double epsilon, double sigma) {
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

int main() {
    std::cout << lj_energy(1.5) << "\n";
    return 0;
}
