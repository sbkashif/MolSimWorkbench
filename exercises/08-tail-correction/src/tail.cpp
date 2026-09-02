#include "tail.h"

#include <cmath>

namespace ex08 {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

double tailEnergy(std::size_t n, double volume, double epsilon,
                  double sigma, double cutoff) {
    // TODO(1): guard n == 0, volume <= 0, cutoff <= 0 -> return 0.0.
    //
    // TODO(2): rho = n / volume
    //          sr3 = (sigma/cutoff)^3        <- compute by multiplication
    //          sr9 = sr3*sr3*sr3
    //          return (8/3) * pi * n * rho * epsilon * sigma^3 * (sr9/3 - sr3)
    //
    // Sanity check as you go: for a normal LJ fluid this must come out
    // NEGATIVE. The tail you truncated was attractive, so restoring it lowers
    // the energy.
    (void)n; (void)volume; (void)epsilon; (void)sigma; (void)cutoff;
    return 0.0;
}

double tailPressure(std::size_t n, double volume, double epsilon,
                    double sigma, double cutoff) {
    // TODO(3): same guards, then
    //          return (16/3) * pi * rho^2 * epsilon * sigma^3 * ((2/3)*sr9 - sr3)
    //
    // Note this depends on rho and cutoff only -- NOT on n independently. Two
    // boxes at the same density must give the same tail pressure whatever their
    // size. A test checks exactly that, and it is a good check of your algebra.
    (void)n; (void)volume; (void)epsilon; (void)sigma; (void)cutoff;
    return 0.0;
}

double tailForceMagnitude(std::size_t n, double volume, double epsilon,
                          double sigma, double cutoff) {
    // TODO(4): The correction assumes g(r) = 1 beyond the cutoff -- the medium
    //          out there is UNIFORM and ISOTROPIC.
    //
    //          Take any thin shell at r > rc. The patch in direction +n pulls
    //          the atom one way; the patch at -n contains exactly as much
    //          material at exactly the same distance and pulls exactly as hard
    //          the other way. Integrate over all directions.
    //
    //          What is the net force? Write that value.
    //
    //          This is WHY the correction never enters the pair loop, and why
    //          adding it costs nothing in a parallel kernel.
    (void)n; (void)volume; (void)epsilon; (void)sigma; (void)cutoff;
    return -1.0;  // replace me
}

}  // namespace ex08
