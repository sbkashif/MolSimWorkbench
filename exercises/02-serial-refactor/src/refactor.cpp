#include "refactor.h"

#include <cmath>

namespace ex02 {

MixingTable buildMixingTable(const std::vector<double>& epsilons,
                             const std::vector<double>& sigmas,
                             const std::string& rule) {
    // ---- ALREADY WRITTEN FOR YOU: setting up the empty table ---------------
    //
    // `epsilons` and `sigmas` hold ONE value per atom type. What the kernel
    // needs instead is one value per PAIR of types, because the mixing rule
    // combines two of them. With T types that is T*T combinations.

    MixingTable t;
    t.ntypes = static_cast<int>(epsilons.size());        // how many atom types

    // .assign(count, value) means "make this vector hold `count` items, and set
    // every one of them to `value`". So these two lines create T*T slots and
    // fill them all with 0.0 -- an empty grid, ready to be written into.
    //
    // They do NOT compute anything. Calling .assign again later would just wipe
    // the grid back to zeros.
    t.eps4.assign(static_cast<std::size_t>(t.ntypes) * t.ntypes, 0.0);
    t.sigma2.assign(static_cast<std::size_t>(t.ntypes) * t.ntypes, 0.0);

    // ---- YOUR JOB: fill those slots in ------------------------------------
    //
    // The vectors are FLAT, but they represent a GRID. With 2 atom types:
    //
    //            b=0   b=1                stored as:
    //     a=0  [  A  |  B  ]              [ A, B, C, D ]
    //     a=1  [  C  |  D  ]                0  1  2  3
    //
    // The slot for "row a, column b" is at index   a * t.ntypes + b
    // (check it: row 1, column 0 -> 1*2 + 0 = index 2 = C.)
    //
    // TODO(1): visit EVERY combination of a and b -- a loop inside a loop --
    //          and for each one:
    //
    //            1. combine the two types' values using the mixing rule:
    //                 "geometric"   eps = sqrt(epsilons[a] * epsilons[b])
    //                               sig = sqrt(sigmas[a]   * sigmas[b])
    //                 "arithmetic"  eps = 0.5 * (epsilons[a] + epsilons[b])
    //                               sig = 0.5 * (sigmas[a]   + sigmas[b])
    //                 anything else leave the slot at 0.0
    //
    //            2. store the PRE-CHEWED versions, not the raw ones:
    //                 t.eps4  [a * t.ntypes + b] = 4.0 * eps;
    //                 t.sigma2[a * t.ntypes + b] = sig * sig;
    //
    // Step 2 is the whole point of this exercise. Storing 4*eps and sigma^2
    // means the inner loop -- which runs millions of times -- never has to
    // multiply by 4 or square anything. You pay that cost once, here.
    //
    // The `rule` check is the same for every pair, so it can sit outside the
    // loops if you prefer. This runs once at startup, so either is fine.
    if (rule == "geometric") {
        t.eps4.assign(static_cast<std::size_t>(t.ntypes) * t.ntypes, 0.0);
        t.sigma2.assign(static_cast<std::size_t>(t.ntypes) * t.ntypes, 0.0);
       
    (void)sigmas;   // delete these two once you actually use sigmas and rule
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
    //
    //   const double invbx = (bx > 0.0) ? 1.0 / bx : 0.0;   // and by, bz
    //
    // Then inside the loop use   dx -= bx * std::round(dx * invbx);
    //
    // Two things are happening here.
    //
    // (a) DIVISION -> MULTIPLICATION. dx/bx and dx*(1/bx) are the same value,
    //     but the reciprocal is now computed once instead of 8.4 million times.
    //     Division is ~15-20 cycles and blocks; multiplication is ~4 and
    //     pipelines.
    //
    // (b) THE BRANCH MOVED OUTSIDE THE LOOP. A box length of 0 means "no
    //     wrapping on this axis". The original guarded that with an `if` on
    //     every pair. Here the `?:` makes the decision ONCE, and the loop body
    //     needs no test at all -- because with bx == 0 we set invbx = 0, and:
    //
    //         dx * 0.0        = 0.0
    //         round(0.0)      = 0.0
    //         bx * 0.0        = 0.0
    //         dx -= 0.0       -> dx unchanged
    //
    //     The wrapping term collapses to zero on its own.
    //
    // The guard is NOT optional. Writing 1.0/bx unguarded when bx == 0 gives
    // infinity, then 0.0 * infinity = NaN, and every force in the system
    // becomes garbage. Choosing 0.0 for the no-box case is what makes the
    // arithmetic safely do nothing.

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
