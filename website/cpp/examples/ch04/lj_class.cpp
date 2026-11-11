#include <cmath>
#include <iostream>

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

private:
    double epsilon_;
    double sigma_;
    double cutoff_;
};

int main() {
    LJPotential argon(1.0, 1.0, 2.5);
    LJPotential sticky(2.0, 1.0, 2.5);
    std::cout << "argon:  U(1.5) = " << argon.energy(1.5) << "\n";
    std::cout << "sticky: U(1.5) = " << sticky.energy(1.5) << "\n";
    std::cout << "argon:  U(3.0) = " << argon.energy(3.0) << "\n";
    return 0;
}
