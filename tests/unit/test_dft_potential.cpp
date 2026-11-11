// DFTPotential is not implemented: building one must fail, so that no run uses
// the table-based placeholder as if it were a DFT calculation.
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "engine/potentials/dft_potential.h"

TEST(DFTPotential, ConstructingIsANotImplementedError) {
    try {
        DFTPotential dft("dft.yaml");
        ADD_FAILURE() << "DFTPotential was built without an error";
    } catch (const std::runtime_error& err) {
        EXPECT_NE(std::string(err.what()).find("not implemented"), std::string::npos)
            << err.what();
    }
}
