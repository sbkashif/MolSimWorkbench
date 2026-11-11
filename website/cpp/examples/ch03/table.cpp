#include <cmath>
#include <iostream>
#include <vector>

double lj_energy(double r, double epsilon, double sigma) {
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

int main() {
    std::vector<double> distances = {1.0, 1.122462, 1.5, 2.5};
    std::cout << "first: " << distances[0] << ", count: " << distances.size() << "\n";
    for (double r : distances) {
        std::cout << "U(" << r << ") = " << lj_energy(r, 1.0, 1.0) << "\n";
    }
    return 0;
}
