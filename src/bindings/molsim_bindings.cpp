#include <memory>
#include <string>
#include <array>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "../builder/geometry.h"
#include "../builder/lj_particle_box_builder.h"
#include "../engine/force_calculator.h"
#include "../engine/molecular_dynamics.h"
#include "../engine/potentials/lj_potential.h"
#include "../engine/potentials/dft_potential.h"
#include "../engine/integrators/integrator.h"
#include "../engine/integrators/velocity_verlet_integrator.h"
#include "../engine/integrators/leapfrog_integrator.h"
#include "../engine/thermostats/thermostat.h"
#include "../engine/thermostats/velocity_rescale_thermostat.h"
#include "../engine/thermostats/langevin_thermostat.h"
#include "../utils/yaml_parser.h"
#include "../utils/trajectory_writer.h"

namespace py = pybind11;

// Set by the build from project(VERSION) in CMakeLists.txt.
#ifndef MOLSIM_VERSION
#define MOLSIM_VERSION "unknown"
#endif

PYBIND11_MODULE(_molsim, module) {
    module.doc() = "MolSimWorkbench C++ bindings";
    module.attr("__version__") = MOLSIM_VERSION;

    py::enum_<MinimizationMethod>(module, "MinimizationMethod")
        .value("STEEPEST_DESCENT", MinimizationMethod::STEEPEST_DESCENT)
        .value("CONJUGATE_GRADIENT", MinimizationMethod::CONJUGATE_GRADIENT)
        .value("LBFGS", MinimizationMethod::LBFGS);

    py::enum_<TrajectoryFormat>(module, "TrajectoryFormat")
        .value("XYZ", TrajectoryFormat::XYZ)
        .value("DCD", TrajectoryFormat::DCD)
        .value("XTC", TrajectoryFormat::XTC);

    py::class_<SimulationBox>(module, "SimulationBox")
        .def(py::init<double, double, double, double, double, double>(),
             py::arg("a") = 0.0,
             py::arg("b") = 0.0,
             py::arg("c") = 0.0,
             py::arg("alpha") = 90.0,
             py::arg("beta") = 90.0,
             py::arg("gamma") = 90.0)
        .def_readwrite("a", &SimulationBox::a)
        .def_readwrite("b", &SimulationBox::b)
        .def_readwrite("c", &SimulationBox::c)
        .def_readwrite("alpha", &SimulationBox::alpha)
        .def_readwrite("beta", &SimulationBox::beta)
        .def_readwrite("gamma", &SimulationBox::gamma);

    py::class_<MinimizationParameters>(module, "MinimizationParameters")
        .def(py::init<>())
        .def_readwrite("method", &MinimizationParameters::method)
        .def_readwrite("tolerance", &MinimizationParameters::tolerance)
        .def_readwrite("forceTolerance", &MinimizationParameters::forceTolerance)
        .def_readwrite("maxIterations", &MinimizationParameters::maxIterations)
        .def_readwrite("initialStepSize", &MinimizationParameters::initialStepSize)
        .def_readwrite("maxStepSize", &MinimizationParameters::maxStepSize)
        .def_readwrite("adaptiveStepSize", &MinimizationParameters::adaptiveStepSize);

    py::class_<SimulationParameters>(module, "SimulationParameters")
        .def(py::init<>())
        .def_readwrite("timeStep", &SimulationParameters::timeStep)
        .def_readwrite("temperature", &SimulationParameters::temperature)
        .def_readwrite("totalTime", &SimulationParameters::totalTime)
        .def_readwrite("useMinimization", &SimulationParameters::useMinimization)
        .def_readwrite("minimization", &SimulationParameters::minimization)
        .def_readwrite("usePBC", &SimulationParameters::usePBC)
        .def_readwrite("box", &SimulationParameters::box)
        .def_readwrite("equilibrationSteps", &SimulationParameters::equilibrationSteps)
        .def_readwrite("productionSteps", &SimulationParameters::productionSteps)
        .def_readwrite("energyDriftTolerance", &SimulationParameters::energyDriftTolerance)
        .def_readwrite("randomSeed", &SimulationParameters::randomSeed)
        .def_readwrite("integrator", &SimulationParameters::integrator)
        .def_readwrite("thermostat", &SimulationParameters::thermostat)
        .def_readwrite("thermostatTau", &SimulationParameters::thermostatTau)
        .def_readwrite("langevinFriction", &SimulationParameters::langevinFriction);

    py::class_<SimulationConfig>(module, "SimulationConfig")
        .def(py::init<>())
        .def_readwrite("simulationType", &SimulationConfig::simulationType)
        .def_readwrite("systemMode", &SimulationConfig::systemMode)
        .def_readwrite("potentialConfigFiles", &SimulationConfig::potentialConfigFiles)
        .def_readwrite("geometryFile", &SimulationConfig::geometryFile)
        .def_readwrite("ljBoxConfig", &SimulationConfig::ljBoxConfig)
        .def_readwrite("simulationParams", &SimulationConfig::simulationParams);

    py::class_<Geometry>(module, "Geometry")
        .def(py::init<>())
        .def("addAtom", &Geometry::addAtom)
        .def("getNumberOfAtoms", &Geometry::getNumberOfAtoms)
        .def("getAtoms", &Geometry::getAtoms, py::return_value_policy::copy)
        .def("getPositions", &Geometry::getPositions, py::return_value_policy::copy)
        .def("getAtomType", &Geometry::getAtomType, py::return_value_policy::copy)
        .def("getCoordinates", &Geometry::getCoordinates, py::return_value_policy::copy)
        .def("setCoordinates", &Geometry::setCoordinates)
        .def("setBoxDimensions", &Geometry::setBoxDimensions)
        .def("getBoxDimensions", &Geometry::getBoxDimensions, py::return_value_policy::copy);

    py::class_<Potential, std::shared_ptr<Potential>>(module, "Potential");

    py::class_<LJPotential, Potential, std::shared_ptr<LJPotential>>(module, "LJPotential")
        .def(py::init<>())
        .def("addAtomParameters", &LJPotential::addAtomParameters)
        .def("setCutoff", &LJPotential::setCutoff)
        .def("setMixingRule", &LJPotential::setMixingRule)
        .def("calculateEnergy", &LJPotential::calculateEnergy)
        .def("getName", &LJPotential::getName);

    py::class_<DFTPotential, Potential, std::shared_ptr<DFTPotential>>(module, "DFTPotential")
        .def(py::init<const std::string&>())
        .def("calculateEnergy", &DFTPotential::calculateEnergy)
        .def("calculatePotentialEnergy", &DFTPotential::calculatePotentialEnergy)
        .def("calculateFlatForces", py::overload_cast<const Geometry&>(&DFTPotential::calculateForces))
        .def("getName", &DFTPotential::getName);

    py::class_<ForceCalculator, std::shared_ptr<ForceCalculator>>(module, "ForceCalculator")
        .def(py::init<>())
        .def("addPotential", &ForceCalculator::addPotential)
        .def("calculateEnergy", &ForceCalculator::calculateEnergy)
        .def("calculateForces", [](ForceCalculator& self, const Geometry& geometry) {
            std::vector<std::array<double, 3>> forces;
            self.calculateForces(geometry, forces);
            return forces;
        })
        .def("clearPotentials", &ForceCalculator::clearPotentials);

    // Integrator plugs
    py::class_<Integrator, std::shared_ptr<Integrator>>(module, "Integrator")
        .def("getName", &Integrator::getName);
    py::class_<VelocityVerletIntegrator, Integrator, std::shared_ptr<VelocityVerletIntegrator>>(
        module, "VelocityVerletIntegrator")
        .def(py::init<>());
    py::class_<LeapfrogIntegrator, Integrator, std::shared_ptr<LeapfrogIntegrator>>(
        module, "LeapfrogIntegrator")
        .def(py::init<>());
    module.def("makeIntegrator", &makeIntegrator, py::arg("name"));

    // Thermostat plugs
    py::class_<Thermostat, std::shared_ptr<Thermostat>>(module, "Thermostat")
        .def("getName", &Thermostat::getName);
    py::class_<VelocityRescaleThermostat, Thermostat, std::shared_ptr<VelocityRescaleThermostat>>(
        module, "VelocityRescaleThermostat")
        .def(py::init<double>(), py::arg("tau") = 0.0)
        .def("getTau", &VelocityRescaleThermostat::getTau);
    py::class_<LangevinThermostat, Thermostat, std::shared_ptr<LangevinThermostat>>(
        module, "LangevinThermostat")
        .def(py::init<double, unsigned int>(), py::arg("friction") = 1.0, py::arg("seed") = 0u)
        .def("getFriction", &LangevinThermostat::getFriction);
    module.def("makeThermostat", &makeThermostat,
               py::arg("name"), py::arg("tau") = 0.0, py::arg("friction") = 1.0, py::arg("seed") = 0u);

    py::class_<MolecularDynamics>(module, "MolecularDynamics")
        .def(py::init<>())
        .def("setGeometry", &MolecularDynamics::setGeometry)
        .def("setForceCalculator", &MolecularDynamics::setForceCalculator)
        .def("setSimulationParameters", &MolecularDynamics::setSimulationParameters)
        .def("setIntegrator", &MolecularDynamics::setIntegrator, py::arg("integrator"))
        .def("setThermostat", &MolecularDynamics::setThermostat, py::arg("thermostat"))
        .def("getIntegrator", &MolecularDynamics::getIntegrator)
        .def("getThermostat", &MolecularDynamics::getThermostat)
        .def("setTrajectoryOutput", &MolecularDynamics::setTrajectoryOutput,
             py::arg("filename"),
             py::arg("format") = TrajectoryFormat::XYZ,
             py::arg("writeFrequency") = 1)
        .def("initialize", &MolecularDynamics::initialize)
        .def("runSimulation", &MolecularDynamics::runSimulation)
        .def("minimizeEnergy", &MolecularDynamics::minimizeEnergy)
        .def("calculatePotentialEnergy", &MolecularDynamics::calculatePotentialEnergy)
        .def("getCurrentEnergy", &MolecularDynamics::getCurrentEnergy)
        .def("getCurrentForces", &MolecularDynamics::getCurrentForces, py::return_value_policy::copy)
        .def("getCurrentGeometry", &MolecularDynamics::getCurrentGeometry, py::return_value_policy::reference_internal);

    py::class_<LJBoxConfig>(module, "LJBoxConfig")
        .def(py::init<>())
        .def_readwrite("numParticles", &LJBoxConfig::numParticles)
        .def_readwrite("density", &LJBoxConfig::density)
        .def_readwrite("temperature", &LJBoxConfig::temperature)
        .def_readwrite("initialization", &LJBoxConfig::initialization)
        .def_readwrite("randomSeed", &LJBoxConfig::randomSeed)
        .def_readwrite("equilibrationSteps", &LJBoxConfig::equilibrationSteps)
        .def_readwrite("productionSteps", &LJBoxConfig::productionSteps);

    py::class_<LJParticleBoxBuilder>(module, "LJParticleBoxBuilder")
        .def_static("createCubicLattice", [](const LJBoxConfig& config) {
            std::array<double, 3> boxSize;
            Geometry geom = LJParticleBoxBuilder::createCubicLattice(config, boxSize);
            // Store box size in geometry
            geom.setBoxDimensions(boxSize);
            return geom;
        });

    py::class_<YAMLParser>(module, "YAMLParser")
        .def(py::init<>())
        .def("parseSimulationConfig", &YAMLParser::parseSimulationConfig)
        .def("parseGeometry", &YAMLParser::parseGeometry)
        .def("parsePotential", &YAMLParser::parsePotential);

    module.def("setYAMLLoader", &YAMLParser::setLoader);
}
