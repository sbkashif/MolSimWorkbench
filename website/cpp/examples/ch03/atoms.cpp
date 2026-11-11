#include <array>
#include <cmath>
#include <iostream>
#include <vector>

double lj_energy(double r, double epsilon, double sigma) {
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

double distance(std::array<double, 3> a, std::array<double, 3> b) {
    double dx = a[0] - b[0];
    double dy = a[1] - b[1];
    double dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

int main() {
    std::vector<std::array<double, 3>> positions = {
        {0.0, 0.0, 0.0},
        {1.5, 0.0, 0.0},
        {0.0, 1.2, 0.0},
    };
    double total = 0.0;
    for (std::size_t i = 0; i < positions.size(); i++) {
        for (std::size_t j = i + 1; j < positions.size(); j++) {
            double r = distance(positions[i], positions[j]);
            double u = lj_energy(r, 1.0, 1.0);
            std::cout << "pair " << i << "-" << j << ": r = " << r
                      << ", U = " << u << "\n";
            total += u;
        }
    }
    std::cout << "total U = " << total << "\n";
    return 0;
}
