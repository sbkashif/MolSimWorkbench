#include <array>
#include <cmath>
#include <iostream>
#include <vector>

double lj_energy(double r, double epsilon, double sigma, double cutoff) {
    if (r > cutoff) {
        return 0.0;
    }
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

double distance(std::array<double, 3> a, std::array<double, 3> b) {
    double dx = a[0] - b[0];
    double dy = a[1] - b[1];
    double dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double total_energy(std::vector<std::array<double, 3>> positions,
                    double epsilon, double sigma, double cutoff) {
    double total = 0.0;
    for (std::size_t i = 0; i < positions.size(); i++) {
        for (std::size_t j = i + 1; j < positions.size(); j++) {
            double r = distance(positions[i], positions[j]);
            total += lj_energy(r, epsilon, sigma, cutoff);
        }
    }
    return total;
}

int main() {
    std::vector<std::array<double, 3>> positions = {
        {0.0, 0.0, 0.0},
        {1.5, 0.0, 0.0},
        {0.0, 1.2, 0.0},
    };
    double argon_epsilon = 1.0;
    double argon_sigma = 1.0;
    double argon_cutoff = 2.5;
    double sticky_epsilon = 2.0;
    double sticky_sigma = 1.0;
    double sticky_cutoff = 2.5;

    std::cout << "argon:  U = "
              << total_energy(positions, argon_epsilon, argon_sigma, argon_cutoff) << "\n";
    std::cout << "sticky: U = "
              << total_energy(positions, sticky_epsilon, sticky_sigma, sticky_cutoff) << "\n";

    // Two slips. Both compile.
    std::cout << "argon, sigma and cutoff swapped: U = "
              << total_energy(positions, argon_epsilon, argon_cutoff, argon_sigma) << "\n";
    std::cout << "argon, with sticky's epsilon:    U = "
              << total_energy(positions, sticky_epsilon, argon_sigma, argon_cutoff) << "\n";
    return 0;
}
