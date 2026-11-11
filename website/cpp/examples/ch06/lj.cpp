#include "lj.h"

#include <cmath>

LJPotential::LJPotential(double epsilon, double sigma, double cutoff) {
    epsilon_ = epsilon;
    sigma_ = sigma;
    cutoff_ = cutoff;
}

double LJPotential::energy(double r) const {
    if (r > cutoff_) {
        return 0.0;
    }
    double sr6 = std::pow(sigma_ / r, 6);
    return 4.0 * epsilon_ * (sr6 * sr6 - sr6);
}
