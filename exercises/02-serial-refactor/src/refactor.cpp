#include "refactor.h"

#include <cmath>

namespace ex02 {

MixingTable buildMixingTable(const std::vector<double>& epsilons,
                             const std::vector<double>& sigmas,
                             const std::string& rule) {
    MixingTable t;
    t.ntypes = static_cast<int>(epsilons.size());
    t.eps4.assign(static_cast<std::size_t>(t.ntypes) * t.ntypes, 0.0);
    t.sigma2.assign(static_cast<std::size_t>(t.ntypes) * t.ntypes, 0.0);

    // TODO(1): for every ordered pair (a, b), apply the mixing rule and store
    //          4*eps_ab   in t.eps4[a*ntypes + b]
    //          sigma_ab^2 in t.sigma2[a*ntypes + b]
    //
    //   "geometric"  -> eps_ab = sqrt(eps_a * eps_b), sig_ab = sqrt(sig_a * sig_b)
    //   "arithmetic" -> eps_ab = 0.5*(eps_a + eps_b), sig_ab = 0.5*(sig_a + sig_b)
    //   anything else -> leave zeros
    //
    // Storing 4*eps and sigma^2 means the inner loop never has to compute them.
    (void)sigmas;
    (void)rule;
    return t;
}

double computeTriangularFast(const std::vector<double>& x,
                             const std::vector<double>& y,
                             const std::vector<double>& z,
                             const std::vector<int>& type,
                             const MixingTable& table,
                             double bx, double by, double bz,
                             double cutoff,
                             std::vector<double>& fx,
                             std::vector<double>& fy,
                             std::vector<double>& fz) {
    const std::size_t n = x.size();
    fx.assign(n, 0.0);
    fy.assign(n, 0.0);
    fz.assign(n, 0.0);

    // TODO(2): hoist the reciprocal box lengths OUT of the loop.
    //   const double invbx = (bx > 0.0) ? 1.0 / bx : 0.0;   // and by, bz
    // Then inside the loop use   dx -= bx * std::round(dx * invbx);
    // When bx == 0 this leaves dx alone, with no branch. Work out why.

    // TODO(3): hoist cutoff*cutoff out of the loop too.

    // TODO(4): the i<j double loop. For each pair:
    //   - displacement, then minimum image using the hoisted reciprocals
    //   - r2 = dx*dx + dy*dy + dz*dz
    //   - skip if r2 > cutoff2 || r2 < 1e-20        <-- NO sqrt anywhere
    //   - look up eps4 and sigma2 from the flat table by integer index
    //   - sr2 = sigma2/r2;  sr6 = sr2*sr2*sr2;  sr12 = sr6*sr6;   <-- no std::pow
    //   - energy += eps4 * (sr12 - sr6)
    //   - fOverR = 6.0 * eps4 * (2.0*sr12 - sr6) / r2
    //       (24*epsilon == 6*eps4, and dividing by r2 instead of r folds in
    //        the 1/r that would otherwise normalise the direction vector)
    //   - scatter:  f[i] -= fOverR * d;   f[j] += fOverR * d;
    //
    // Hoisting the eps4/sigma2 ROW for atom i out of the j loop is worth doing.
    (void)type; (void)table; (void)by; (void)bz; (void)cutoff;
    (void)y; (void)z;

    return 0.0;  // replace with the accumulated energy
}

}  // namespace ex02
