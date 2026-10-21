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
    argon.epsilon_ = 2.0;
    return 0;
}
