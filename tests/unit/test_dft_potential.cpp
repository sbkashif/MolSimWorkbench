// The DFT potential is a placeholder: the functional name selects one of three
// per-atom energy tables (B3LYP, PBE, HF). Any other name is rejected at load.
#include <gtest/gtest.h>

#include <stdexcept>
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

TEST(DFTFunctional, UnknownFunctionalIsAnError) {
    for (const std::string name : {"PW91", "PBE0"}) {
        YAMLParser::setLoader([name](const std::string&) {
            return R"({"parameters": {"functional": ")" + name +
                   R"(", "basis_set": "6-31G*", "scf_convergence": 1e-6,)"
                   R"( "max_scf_cycles": 100, "grid_quality": "fine"}})";
        });
        try {
            DFTPotential dft("dft.yaml");
            ADD_FAILURE() << name << " loaded without an error";
        } catch (const std::invalid_argument& err) {
            EXPECT_NE(std::string(err.what()).find("is not recognised"), std::string::npos)
                << err.what();
        }
        YAMLParser::setLoader(nullptr);
    }
}
