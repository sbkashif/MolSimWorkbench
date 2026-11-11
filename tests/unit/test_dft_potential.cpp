// The DFT backend has atomic-energy tables for B3LYP, PBE and HF only. Any
// other functional name is computed with the B3LYP values; it must say so.
#include <gtest/gtest.h>

#include <string>

#include "builder/geometry.h"
#include "engine/potentials/dft_potential.h"
#include "utils/yaml_parser.h"

namespace {

// Builds a DFTPotential for `functional` without a file on disk (the loader
// hook stands in for the YAML), and returns what it printed to stderr.
std::string stderrWhenLoading(const std::string& functional, double* waterEnergy) {
    YAMLParser::setLoader([functional](const std::string&) {
        return R"({"parameters": {"functional": ")" + functional +
               R"(", "basis_set": "6-31G*", "scf_convergence": 1e-6,)"
               R"( "max_scf_cycles": 100, "grid_quality": "fine"}})";
    });
    testing::internal::CaptureStderr();
    DFTPotential dft("dft.yaml");
    const std::string err = testing::internal::GetCapturedStderr();
    YAMLParser::setLoader(nullptr);

    Geometry water;
    water.addAtom("O", {0.0, 0.0, 0.0});
    water.addAtom("H", {0.9572, 0.0, 0.0});
    water.addAtom("H", {-0.2399, 0.9266, 0.0});
    *waterEnergy = dft.calculateEnergy(water);
    return err;
}

}  // namespace

TEST(DFTFunctional, TabledFunctionalsLoadQuietly) {
    double e = 0.0;
    for (const char* name : {"B3LYP", "PBE", "HF"}) {
        EXPECT_EQ(stderrWhenLoading(name, &e).find("Warning"), std::string::npos) << name;
    }
}

TEST(DFTFunctional, UntabledFunctionalWarnsAndUsesB3LYP) {
    double b3lyp = 0.0, pw91 = 0.0, pbe0 = 0.0;
    stderrWhenLoading("B3LYP", &b3lyp);
    const std::string errPW91 = stderrWhenLoading("PW91", &pw91);
    const std::string errPBE0 = stderrWhenLoading("PBE0", &pbe0);

    EXPECT_NE(errPW91.find("\"PW91\" has no atomic-energy table"), std::string::npos) << errPW91;
    EXPECT_NE(errPBE0.find("\"PBE0\" has no atomic-energy table"), std::string::npos) << errPBE0;
    EXPECT_DOUBLE_EQ(pw91, b3lyp);
    EXPECT_DOUBLE_EQ(pbe0, b3lyp);
}
