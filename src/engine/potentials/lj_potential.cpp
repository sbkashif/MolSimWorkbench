#include "lj_potential.h"
#include <cmath>
#include <iostream>
#include <cstdlib>

LJPotential::LJPotential() {
    // Parallelism is OPT-IN. The default is the serial gather kernel, so a
    // stock build behaves identically (and deterministically) whether or not
    // OpenMP happened to be available at configure time.
    //
    // Turn it on explicitly with one of:
    //   - C++: setBackend(LJBackend::OpenMP)
    //   - env: MOLSIM_LJ_BACKEND=openmp
    // There is no YAML key or Python binding for this yet.
    backend = LJBackend::Serial;

    // Env-var override, purely for benchmarking convenience: it lets you A/B
    // the same binary without recompiling or editing a config.
    if (const char* env = std::getenv("MOLSIM_LJ_BACKEND")) {
        const std::string v(env);
        if (v == "openmp" || v == "omp") {
            backend = LJBackend::OpenMP;
        } else if (v == "openmp-private" || v == "private") {
            backend = LJBackend::OpenMPPrivate;
        } else if (v == "threads") {
            backend = LJBackend::Threads;
        } else if (v == "serial") {
            backend = LJBackend::Serial;
        } else {
            std::cerr << "Warning: unknown MOLSIM_LJ_BACKEND='" << v
                      << "' (expected serial|threads|openmp|openmp-private); using serial\n";
        }
    }
    if (const char* env = std::getenv("MOLSIM_LJ_THREADS")) {
        numThreads = std::atoi(env);
    }
}

LJPotential::~LJPotential() {
    // Destructor
}

void LJPotential::addAtomParameters(const std::string& atomType, double epsilon, double sigma) {
    atomParameters[atomType] = {epsilon, sigma};
    tablesDirty = true;
}

// ---------------------------------------------------------------------------
// Table construction. This is where the per-pair std::map<std::string> lookup
// of the reference implementation goes to die.
//
// Mixing rules are applied ONCE here, for every ordered type pair, and stored
// in a flat ntypes*ntypes array of plain doubles. The inner loop then does an
// integer index instead of two red-black tree descents and a sqrt.
// ---------------------------------------------------------------------------
void LJPotential::rebuildTables() {
    typeIds.clear();
    int next = 0;
    for (const auto& kv : atomParameters) {
        typeIds[kv.first] = next++;
    }
    ntypes = next;

    eps4Table.assign(static_cast<std::size_t>(ntypes) * ntypes, 0.0);
    sigma2Table.assign(static_cast<std::size_t>(ntypes) * ntypes, 0.0);

    for (const auto& a : atomParameters) {
        for (const auto& b : atomParameters) {
            const int i = typeIds[a.first];
            const int j = typeIds[b.first];
            // Same mixing rules as getMixedParameters(), evaluated once.
            auto [eps, sig] = getMixedParameters(a.first, b.first);
            const std::size_t idx = static_cast<std::size_t>(i) * ntypes + j;
            // Store 4*epsilon and sigma^2 so the kernel needs neither.
            // 4*x and x*x are exact-ish rescalings done outside the hot loop.
            eps4Table[idx] = 4.0 * eps;
            sigma2Table[idx] = sig * sig;
        }
    }
    tablesDirty = false;
}

// ---------------------------------------------------------------------------
// Marshalling: Geometry (AoS + std::string) -> flat SoA the kernel can take.
//
// This costs O(N) per call, against an O(N^2) kernel, so it is cheap here.
// It is NOT cheap on a GPU, where the equivalent is a host->device memcpy every
// timestep. The fix there is to keep the buffers resident on the device and
// only copy back on output frames -- see docs/learn-parallel/09.
// ---------------------------------------------------------------------------
void LJPotential::buildView(const Geometry& geometry, LJKernelView& view) {
    if (tablesDirty) rebuildTables();

    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& boxDim = geometry.getBoxDimensions();
    const std::size_t n = atoms.size();

    sx.resize(n); sy.resize(n); sz.resize(n);
    sfx.assign(n, 0.0); sfy.assign(n, 0.0); sfz.assign(n, 0.0);
    stype.resize(n);

    for (std::size_t i = 0; i < n; ++i) {
        sx[i] = positions[i][0];
        sy[i] = positions[i][1];
        sz[i] = positions[i][2];
        auto it = typeIds.find(atoms[i]);
        // An unparameterised type maps to id -1; we point it at a zeroed row
        // below so it contributes exactly nothing, matching the reference
        // implementation's behaviour of returning {0,0} for missing params.
        stype[i] = (it != typeIds.end()) ? it->second : -1;
    }

    // Any atom with no parameters is remapped onto a spare all-zero type row.
    bool hasUnknown = false;
    for (std::size_t i = 0; i < n; ++i) {
        if (stype[i] < 0) { hasUnknown = true; break; }
    }
    if (hasUnknown) {
        const int zeroId = ntypes;
        const int newN = ntypes + 1;
        std::vector<double> e(static_cast<std::size_t>(newN) * newN, 0.0);
        std::vector<double> s(static_cast<std::size_t>(newN) * newN, 0.0);
        for (int i = 0; i < ntypes; ++i)
            for (int j = 0; j < ntypes; ++j) {
                e[static_cast<std::size_t>(i) * newN + j] = eps4Table[static_cast<std::size_t>(i) * ntypes + j];
                s[static_cast<std::size_t>(i) * newN + j] = sigma2Table[static_cast<std::size_t>(i) * ntypes + j];
            }
        eps4Table.swap(e);
        sigma2Table.swap(s);
        ntypes = newN;
        for (std::size_t i = 0; i < n; ++i)
            if (stype[i] < 0) stype[i] = zeroId;
    }

    view.x = sx.data();  view.y = sy.data();  view.z = sz.data();
    view.fx = sfx.data(); view.fy = sfy.data(); view.fz = sfz.data();
    view.type = stype.data();
    view.n = n;
    view.eps4 = eps4Table.data();
    view.sigma2 = sigma2Table.data();
    view.ntypes = ntypes;
    view.bx = boxDim[0]; view.by = boxDim[1]; view.bz = boxDim[2];
    view.cutoff2 = cutoff * cutoff;
}

double LJPotential::calculateEnergy(const Geometry& geometry) {
    if (geometry.getAtoms().empty()) return 0.0;
    LJKernelView view;
    buildView(geometry, view);
    // The gather kernel produces energy and forces together; for an energy-only
    // query we simply discard the forces it wrote into the scratch buffers.
    //
    // The tail correction is added here and NOWHERE in calculateForces --
    // its force contribution is exactly zero.
    return ljComputeGather(view, backend, numThreads) + tailCorrectionEnergy(geometry);
}

void LJPotential::calculateForces(const Geometry& geometry,
                                  std::vector<std::array<double, 3>>& forces) {
    const std::size_t n = geometry.getAtoms().size();
    if (forces.size() != n) forces.resize(n, {0.0, 0.0, 0.0});
    if (n == 0) return;

    LJKernelView view;
    buildView(geometry, view);
    ljComputeGather(view, backend, numThreads);

    // Scatter SoA back into the AoS interface the rest of the engine expects.
    for (std::size_t i = 0; i < n; ++i) {
        forces[i] = {sfx[i], sfy[i], sfz[i]};
    }
}


// ===========================================================================
// LONG-RANGE DISPERSION CORRECTION
//
// Truncating the LJ potential at r_c discards every interaction beyond it.
// The standard fix assumes the fluid is uniform and isotropic out there --
// g(r) = 1 for r > r_c -- which turns the neglected sum into an integral that
// has a closed form:
//
//   E_tail = (2*pi/V) * sum_{a,b} N_a N_b * 4*eps_ab *
//                [ sigma_ab^12 / (9 r_c^9)  -  sigma_ab^6 / (3 r_c^3) ]
//
// For one component this is the familiar
//   E_tail = (8/3) pi N rho eps sigma^3 [ (1/3)(sigma/r_c)^9 - (sigma/r_c)^3 ]
//
// THE FORCE CORRECTION IS ZERO. A uniform isotropic shell of material beyond
// r_c pulls equally in every direction, so it exerts no net force on the atom
// at the centre. That is why this function exists outside the pair loop and
// costs nothing: it is O(number of atom types squared), evaluated once.
// ===========================================================================

double LJPotential::tailCorrectionEnergy(const Geometry& geometry) {
    if (!tailCorrection) return 0.0;

    const auto& box = geometry.getBoxDimensions();
    const double volume = box[0] * box[1] * box[2];
    // No box means no density, and the uniform-fluid assumption is meaningless.
    if (volume <= 0.0 || cutoff <= 0.0) return 0.0;

    // Count atoms of each parameterised type.
    std::map<std::string, double> counts;
    for (const auto& t : geometry.getAtoms()) {
        if (atomParameters.find(t) != atomParameters.end()) counts[t] += 1.0;
    }

    const double rc3 = cutoff * cutoff * cutoff;
    const double rc9 = rc3 * rc3 * rc3;

    double sum = 0.0;
    for (const auto& a : counts) {
        for (const auto& b : counts) {
            auto [eps, sig] = getMixedParameters(a.first, b.first);
            const double s3 = sig * sig * sig;
            const double s6 = s3 * s3;
            const double s12 = s6 * s6;
            sum += a.second * b.second * 4.0 * eps * (s12 / (9.0 * rc9) - s6 / (3.0 * rc3));
        }
    }

    constexpr double kPi = 3.14159265358979323846;
    return 2.0 * kPi / volume * sum;
}

double LJPotential::tailCorrectionPressure(const Geometry& geometry) {
    if (!tailCorrection) return 0.0;

    const auto& box = geometry.getBoxDimensions();
    const double volume = box[0] * box[1] * box[2];
    if (volume <= 0.0 || cutoff <= 0.0) return 0.0;

    std::map<std::string, double> counts;
    for (const auto& t : geometry.getAtoms()) {
        if (atomParameters.find(t) != atomParameters.end()) counts[t] += 1.0;
    }

    const double rc3 = cutoff * cutoff * cutoff;
    const double rc9 = rc3 * rc3 * rc3;

    // P_tail = (16 pi / (3 V^2)) sum_ab N_a N_b eps_ab
    //              [ (2/3) sigma^12 / r_c^9  -  sigma^6 / r_c^3 ]
    double sum = 0.0;
    for (const auto& a : counts) {
        for (const auto& b : counts) {
            auto [eps, sig] = getMixedParameters(a.first, b.first);
            const double s3 = sig * sig * sig;
            const double s6 = s3 * s3;
            const double s12 = s6 * s6;
            sum += a.second * b.second * eps *
                   ((2.0 / 3.0) * s12 / rc9 - s6 / rc3);
        }
    }

    constexpr double kPi = 3.14159265358979323846;
    return 16.0 * kPi / (3.0 * volume * volume) * sum;
}

// ===========================================================================
// REFERENCE IMPLEMENTATION BELOW
//
// This is the original triangular (i<j) loop, unchanged. It is the baseline
// that tests/unit/test_lj_kernels.cpp compares every backend against, and the
// "before" picture for docs/learn-parallel/00_starting_point.md.
// ===========================================================================

double LJPotential::calculateEnergyReference(const Geometry& geometry) {
    double totalEnergy = 0.0;

    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& boxDim = geometry.getBoxDimensions();

    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
            double dx = positions[j][0] - positions[i][0];
            double dy = positions[j][1] - positions[i][1];
            double dz = positions[j][2] - positions[i][2];

            if (boxDim[0] > 0.0) dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
            if (boxDim[1] > 0.0) dy = dy - boxDim[1] * std::round(dy / boxDim[1]);
            if (boxDim[2] > 0.0) dz = dz - boxDim[2] * std::round(dz / boxDim[2]);

            double distanceSquared = dx*dx + dy*dy + dz*dz;
            double distance = std::sqrt(distanceSquared);

            if (distance > cutoff || distance < 1e-10) continue;

            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
            totalEnergy += calculatePairEnergy(epsilon, sigma, distance);
        }
    }

    return totalEnergy;
}

void LJPotential::calculateForcesReference(const Geometry& geometry,
                                           std::vector<std::array<double, 3>>& forces) {
    if (forces.size() != geometry.getAtoms().size()) {
        forces.resize(geometry.getAtoms().size(), {0.0, 0.0, 0.0});
    } else {
        for (auto& force : forces) {
            force = {0.0, 0.0, 0.0};
        }
    }

    const auto& atoms = geometry.getAtoms();
    const auto& positions = geometry.getPositions();
    const auto& boxDim = geometry.getBoxDimensions();

    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
            double dx = positions[j][0] - positions[i][0];
            double dy = positions[j][1] - positions[i][1];
            double dz = positions[j][2] - positions[i][2];

            if (boxDim[0] > 0.0) dx = dx - boxDim[0] * std::round(dx / boxDim[0]);
            if (boxDim[1] > 0.0) dy = dy - boxDim[1] * std::round(dy / boxDim[1]);
            if (boxDim[2] > 0.0) dz = dz - boxDim[2] * std::round(dz / boxDim[2]);

            double distanceSquared = dx*dx + dy*dy + dz*dz;
            double distance = std::sqrt(distanceSquared);

            if (distance > cutoff || distance < 1e-10) continue;

            auto [epsilon, sigma] = getMixedParameters(atoms[i], atoms[j]);
            std::array<double, 3> direction = {dx/distance, dy/distance, dz/distance};
            calculatePairForce(epsilon, sigma, distance, direction, forces[i], forces[j]);
        }
    }
}

std::pair<double, double> LJPotential::getMixedParameters(const std::string& type1, const std::string& type2) {
    if (atomParameters.find(type1) == atomParameters.end() ||
        atomParameters.find(type2) == atomParameters.end()) {
        return {0.0, 0.0};  // Return zero parameters if missing
    }

    const auto& params1 = atomParameters[type1];
    const auto& params2 = atomParameters[type2];

    double mixedEpsilon, mixedSigma;

    if (mixingRule == "geometric") {
        mixedEpsilon = std::sqrt(params1.epsilon * params2.epsilon);
        mixedSigma = std::sqrt(params1.sigma * params2.sigma);
    } else if (mixingRule == "arithmetic") {
        mixedEpsilon = 0.5 * (params1.epsilon + params2.epsilon);
        mixedSigma = 0.5 * (params1.sigma + params2.sigma);
    } else {
        mixedEpsilon = 0.0;
        mixedSigma = 0.0;
    }

    return {mixedEpsilon, mixedSigma};
}

double LJPotential::calculatePairEnergy(double epsilon, double sigma, double distance) {
    if (distance <= 0.0) return 0.0;

    double sigma_r = sigma / distance;
    double sigma_r6 = std::pow(sigma_r, 6);
    double sigma_r12 = sigma_r6 * sigma_r6;

    return 4.0 * epsilon * (sigma_r12 - sigma_r6);
}

void LJPotential::calculatePairForce(double epsilon, double sigma, double distance,
                                    const std::array<double, 3>& direction,
                                    std::array<double, 3>& force1,
                                    std::array<double, 3>& force2) {
    if (distance <= 0.0) return;

    double sigma_r = sigma / distance;
    double sigma_r6 = std::pow(sigma_r, 6);
    double sigma_r12 = sigma_r6 * sigma_r6;

    double forceMagnitude = 24.0 * epsilon * (2.0 * sigma_r12 - sigma_r6) / distance;

    for (int i = 0; i < 3; ++i) {
        force1[i] -= forceMagnitude * direction[i];
        force2[i] += forceMagnitude * direction[i];
    }
}
