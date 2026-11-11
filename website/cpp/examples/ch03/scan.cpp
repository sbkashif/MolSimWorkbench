#include <cmath>
#include <iostream>
#include <vector>

double lj_energy(double r, double epsilon, double sigma) {
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

int main() {
    std::vector<double> distances;
    for (int i = 0; i <= 210; i++) {
        distances.push_back(0.9 + 0.01 * i);
    }
    std::cout << distances.size() << " distances, from " << distances[0]
              << " to " << distances[distances.size() - 1] << "\n";

    double best_r = distances[0];
    double best_u = lj_energy(best_r, 1.0, 1.0);
    for (double r : distances) {
        double u = lj_energy(r, 1.0, 1.0);
        if (u < best_u) {
            best_u = u;
            best_r = r;
        }
    }
    std::cout << "lowest energy " << best_u << " at r = " << best_r << "\n";
    return 0;
}
