#include <cmath>
#include <iostream>

double lj_energy(double r, double epsilon, double sigma) {
    std::cout << "  inside lj_energy: r is at " << &r << "\n";
    r = 99.0;
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

int main() {
    double r = 1.5;
    std::cout << "in main: r is at " << &r << "\n";
    double energy = lj_energy(r, 1.0, 1.0);
    std::cout << "in main: r is still " << r << "\n";
    return 0;
}
