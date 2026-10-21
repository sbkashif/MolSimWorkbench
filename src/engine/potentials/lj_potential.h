#ifndef LJ_POTENTIAL_H
#define LJ_POTENTIAL_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <array>
#include "../force_calculator.h"
#include "../../builder/geometry.h"
#include "lj_kernels.h"

// Class for Lennard-Jones potential
class LJPotential : public Potential {
public:
    LJPotential();
    ~LJPotential() override;

    // Potential interface implementation.
    // These dispatch to lj_kernels via the currently selected backend.
    double calculateEnergy(const Geometry& geometry) override;
    void calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces) override;
    double calculateForcesAndEnergy(const Geometry& geometry,
                                    std::vector<std::array<double, 3>>& forces) override;
    std::string getName() const override { return "Lennard-Jones"; }

    // LJ-specific methods
    void addAtomParameters(const std::string& atomType, double epsilon, double sigma);
    void setCutoff(double cutoff) { this->cutoff = cutoff; tablesDirty = true; }
    void setMixingRule(const std::string& rule) { this->mixingRule = rule; tablesDirty = true; }

    // ---- Backend selection (see docs/learn-parallel/) ----------------------
    // Parallelism is OPT-IN: the default is LJBackend::Serial. Callers must ask
    // for a parallel backend explicitly, so a stock build is unchanged.
    void setBackend(LJBackend backend) { this->backend = backend; }
    LJBackend getBackend() const { return backend; }
    // 0 or negative means "use the runtime default" (omp_get_max_threads).
    void setNumThreads(int n) { this->numThreads = n; }
    int getNumThreads() const { return numThreads; }

    // ---- Long-range dispersion correction ----------------------------------
    // Analytic correction for the interactions truncated at the cutoff,
    // assuming g(r) = 1 beyond r_c (a uniform, isotropic fluid).
    // GROMACS calls this DispCorr = EnerPres. OPT-IN, default off.
    //
    // It contributes to ENERGY and PRESSURE only. The force contribution is
    // exactly zero by symmetry, so this never enters the pair loop -- see
    // docs/learn-parallel/08_not_the_hot_loop.md.
    void setTailCorrection(bool on) { tailCorrection = on; }
    bool getTailCorrection() const { return tailCorrection; }

    // Both are O(1): they depend only on N, V, r_c and the mixed parameters.
    double tailCorrectionEnergy(const Geometry& geometry);
    double tailCorrectionPressure(const Geometry& geometry);

    // ---- Reference implementation ------------------------------------------
    // The ORIGINAL triangular (i<j) loop, kept verbatim as the correctness
    // baseline that every parallel backend is tested against. Slow on purpose:
    // it does a std::map<std::string> lookup per pair. Do not "optimise" it --
    // its entire value is being the thing we did not touch.
    double calculateEnergyReference(const Geometry& geometry);
    void calculateForcesReference(const Geometry& geometry,
                                  std::vector<std::array<double, 3>>& forces);

private:
    // Atom parameters
    struct AtomParameters {
        double epsilon;
        double sigma;
    };

    std::map<std::string, AtomParameters> atomParameters;
    double cutoff = 10.0;  // Default cutoff
    std::string mixingRule = "geometric";  // Default mixing rule

    LJBackend backend;
    int numThreads = 0;
    bool tailCorrection = false;

    // ---- Flat, kernel-ready state -----------------------------------------
    // Built once (and rebuilt only when parameters change), NOT per pair.
    bool tablesDirty = true;
    std::map<std::string, int> typeIds;   // atom type name -> dense integer id
    std::vector<double> eps4Table;        // ntypes*ntypes, 4*epsilon_ij
    std::vector<double> sigma2Table;      // ntypes*ntypes, sigma_ij^2
    int ntypes = 0;

    // Persistent SoA scratch. Kept as members so a 5000-step MD run allocates
    // these once rather than 5000 times. On a GPU this is the same idea taken
    // further: the buffers stay resident on the device across timesteps.
    std::vector<double> sx, sy, sz, sfx, sfy, sfz;
    std::vector<int> stype;

    void rebuildTables();
    // Marshal Geometry (AoS + strings) into the flat SoA scratch buffers.
    void buildView(const Geometry& geometry, LJKernelView& view);

    // Helper methods (used by the reference implementation)
    std::pair<double, double> getMixedParameters(const std::string& type1, const std::string& type2);
    double calculatePairEnergy(double epsilon, double sigma, double distance);
    void calculatePairForce(double epsilon, double sigma, double distance,
                           const std::array<double, 3>& direction,
                           std::array<double, 3>& force1,
                           std::array<double, 3>& force2);
};

#endif // LJ_POTENTIAL_H
