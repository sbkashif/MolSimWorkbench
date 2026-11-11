#ifndef EX08_TAIL_H
#define EX08_TAIL_H

#include <cstddef>

namespace ex08 {

// E_tail = (8/3) pi N rho eps sigma^3 [ (1/3)(sigma/rc)^9 - (sigma/rc)^3 ]
// with rho = n/volume. Returns 0 for non-physical input.
double tailEnergy(std::size_t n, double volume, double epsilon,
                  double sigma, double cutoff);

// P_tail = (16/3) pi rho^2 eps sigma^3 [ (2/3)(sigma/rc)^9 - (sigma/rc)^3 ]
double tailPressure(std::size_t n, double volume, double epsilon,
                    double sigma, double cutoff);

// The magnitude of the force the truncated tail exerts on an atom.
// Think about the symmetry of a uniform isotropic medium before answering.
double tailForceMagnitude(std::size_t n, double volume, double epsilon,
                          double sigma, double cutoff);

}  // namespace ex08
#endif
