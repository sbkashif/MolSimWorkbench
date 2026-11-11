#include "gather.h"

#include <cmath>

namespace ex03 {

double computeGather(const std::vector<double>& x,
                     const std::vector<double>& y,
                     const std::vector<double>& z,
                     double eps4, double sigma2,
                     double bx, double by, double bz,
                     double cutoff,
                     std::vector<double>& fx,
                     std::vector<double>& fy,
                     std::vector<double>& fz) {
    const std::size_t n = x.size();
    fx.assign(n, 0.0);
    fy.assign(n, 0.0);
    fz.assign(n, 0.0);

    // TODO(1): hoist invbx/invby/invbz and cutoff2 as in exercise 2.

    // TODO(2): for each i:
    //            double fxi = 0, fyi = 0, fzi = 0, ei = 0;   <-- LOCAL scalars
    //            for each j != i:  ... accumulate into fxi/fyi/fzi and ei ...
    //            fx[i] = fxi;  fy[i] = fyi;  fz[i] = fzi;    <-- ONE write
    //
    //          Only atom i is ever written. Do NOT touch fx[j].
    //          The local scalars are the point: they live in registers, they
    //          are private to this iteration by construction, and on a GPU
    //          they are exactly what a thread's registers hold.

    // TODO(3): every pair is visited twice (as i-j and as j-i), so the energy
    //          you accumulated is double counted. Return HALF of it.
    (void)y; (void)z; (void)eps4; (void)sigma2;
    (void)bx; (void)by; (void)bz; (void)cutoff;
    return 0.0;  // replace me
}

double gatherArithmeticRatio(std::size_t n) {
    // TODO(4): triangular evaluates n(n-1)/2 pairs; gather evaluates n(n-1).
    //          Return the ratio. (Guard n < 2.)
    (void)n;
    return 0.0;  // replace me
}

}  // namespace ex03
