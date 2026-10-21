#include <cmath>
#include <pybind11/pybind11.h>

namespace py = pybind11;

double lj_energy(double r, double epsilon, double sigma) {
    double sr6 = std::pow(sigma / r, 6);
    return 4.0 * epsilon * (sr6 * sr6 - sr6);
}

PYBIND11_MODULE(ljnamed, m) {
    m.doc() = "The Lennard-Jones pair energy, written in C++";
    m.def("lj_energy", &lj_energy, "LJ energy of one pair at distance r",
          py::arg("r"), py::arg("epsilon") = 1.0, py::arg("sigma") = 1.0);
}
