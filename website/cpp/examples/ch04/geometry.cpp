#include <array>
#include <cmath>
#include <iostream>
#include <vector>

struct Geometry {
    std::vector<std::array<double, 3>> positions;
};

class LJPotential {
public:
    LJPotential(double epsilon, double sigma, double cutoff) {
        epsilon_ = epsilon;
        sigma_ = sigma;
        cutoff_ = cutoff;
    }

    double energy(double r) const {
        if (r > cutoff_) {
            return 0.0;
        }
        double sr6 = std::pow(sigma_ / r, 6);
        return 4.0 * epsilon_ * (sr6 * sr6 - sr6);
    }

    double total_energy(const Geometry& geometry) const {
        std::cout << "  inside: geometry is at " << &geometry << "\n";
        const std::vector<std::array<double, 3>>& p = geometry.positions;
        double total = 0.0;
        for (std::size_t i = 0; i < p.size(); i++) {
            for (std::size_t j = i + 1; j < p.size(); j++) {
                double dx = p[i][0] - p[j][0];
                double dy = p[i][1] - p[j][1];
                double dz = p[i][2] - p[j][2];
                total += energy(std::sqrt(dx * dx + dy * dy + dz * dz));
            }
        }
        return total;
    }

private:
    double epsilon_;
    double sigma_;
    double cutoff_;
};

int main() {
    Geometry geometry;
    geometry.positions = {{0.0, 0.0, 0.0}, {1.5, 0.0, 0.0}, {0.0, 1.2, 0.0}};
    geometry.positions.push_back({3.0, 3.0, 3.0});
    std::cout << "in main: geometry is at " << &geometry << "\n";

    LJPotential argon(1.0, 1.0, 2.5);
    double total = argon.total_energy(geometry);
    std::cout << "total U = " << total << "\n";
    return 0;
}
