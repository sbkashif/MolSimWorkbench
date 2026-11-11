"""Python API for MolSimWorkbench backed by native `_molsim` bindings."""

import sys
from pathlib import Path


def _load_bindings():
    try:
        import _molsim
        return _molsim
    except ImportError:
        project_root = Path(__file__).resolve().parent
        build_dir = project_root / "build"
        candidates = [
            build_dir,
            build_dir / "Debug",
            build_dir / "Release",
            build_dir / "RelWithDebInfo",
            build_dir / "MinSizeRel",
        ]
        for candidate in candidates:
            if candidate.exists():
                candidate_path = str(candidate)
                if candidate_path not in sys.path:
                    sys.path.insert(0, candidate_path)
        import _molsim
        return _molsim


_molsim = _load_bindings()

# The version is set in CMakeLists.txt; a build older than 0.7.0 has none.
__version__ = getattr(_molsim, "__version__", "unknown")

# In a web browser the engine cannot start a separate Python process to read
# YAML, so it reads through the Python it is already running in.
if sys.platform == "emscripten":
    import json
    import yaml

    def _load_yaml(path):
        with open(path) as file:
            return json.dumps(yaml.safe_load(file))

    _molsim.setYAMLLoader(_load_yaml)


class System:
    """Configured simulation system ready to run."""

    def __init__(self, dynamics):
        self._dynamics = dynamics

    @classmethod
    def from_yaml(cls, config_path, trajectory_output="water_trajectory.xyz", trajectory_every=1,
                  integrator=None, thermostat=None):
        """Build a system from a YAML config.

        `integrator` ("velocity_verlet", "leapfrog") and `thermostat`
        ("velocity_rescale", "berendsen", "langevin", "none") override the
        names in the YAML. Either may also be a plug object, such as
        `_molsim.LangevinThermostat(friction=2.0, seed=1)`.
        """
        parser = _molsim.YAMLParser()
        config = parser.parseSimulationConfig(str(config_path))
        if isinstance(integrator, str):
            config.simulationParams.integrator = integrator
        if isinstance(thermostat, str):
            config.simulationParams.thermostat = thermostat

        # Handle geometry based on system mode
        if config.systemMode == "lj_particle_box":
            # LJ bulk box: build geometry using LJParticleBoxBuilder
            geometry = _molsim.LJParticleBoxBuilder.createCubicLattice(config.ljBoxConfig)
            # Update simulation box dimensions
            box_dims = geometry.getBoxDimensions()
            config.simulationParams.box = _molsim.SimulationBox(box_dims[0], box_dims[1], box_dims[2])
            config.simulationParams.usePBC = True
        else:
            # Explicit geometry file mode
            geometry = parser.parseGeometry(config.geometryFile)
        
        force_calculator = _molsim.ForceCalculator()
        for potential_file in config.potentialConfigFiles:
            force_calculator.addPotential(parser.parsePotential(potential_file))

        dynamics = _molsim.MolecularDynamics()
        dynamics.setGeometry(geometry)
        dynamics.setForceCalculator(force_calculator)
        dynamics.setSimulationParameters(config.simulationParams)
        if integrator is not None and not isinstance(integrator, str):
            dynamics.setIntegrator(integrator)
        if thermostat is not None and not isinstance(thermostat, str):
            dynamics.setThermostat(thermostat)
        dynamics.setTrajectoryOutput(str(trajectory_output), _molsim.TrajectoryFormat.XYZ, trajectory_every)

        return cls(dynamics)

    def run(self):
        self._dynamics.initialize()
        self._dynamics.runSimulation()
        return self._dynamics.getCurrentEnergy()


class Workbench:
    """Convenience API for one-line config-driven runs."""

    @staticmethod
    def run(config_path, trajectory_output="water_trajectory.xyz"):
        system = System.from_yaml(config_path, trajectory_output)
        return system.run()


class MolSimWorkbench:
    """Backward-compatible facade around the new binding-backed API."""

    def __init__(self):
        self.project_root = Path(__file__).resolve().parent
        self.build_dir = self.project_root / "build"
        self.examples_dir = self.project_root / "examples"

    def run_lj_simulation(self, output_dir=None):
        output_base = Path(output_dir) if output_dir else (self.examples_dir / "lj" / "output")
        output_base.mkdir(parents=True, exist_ok=True)
        trajectory = output_base / "water_trajectory.xyz"
        Workbench.run("data/simulation_config.yaml", trajectory)
        return trajectory

    def get_available_potentials(self):
        potentials_dir = self.project_root / "data" / "potentials"
        return sorted(path.stem for path in potentials_dir.glob("*.yaml"))


def create_workbench():
    return MolSimWorkbench()