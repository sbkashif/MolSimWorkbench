"""
Comprehensive test suite for the MolSimWorkbench
Tests both LJ and DFT potentials through the Python API
"""

import sys
from pathlib import Path

# Add the parent directory to the path to import molsimworkbench
sys.path.insert(0, str(Path(__file__).parent.parent))

from molsimworkbench import MolSimWorkbench

class TestMolSimWorkbench:
    workbench: MolSimWorkbench
    
    def setup_method(self):
        """Set up test fixtures"""
        self.workbench = MolSimWorkbench()
        
    def test_workbench_initialization(self):
        """Test that the workbench initializes correctly"""
        assert self.workbench.project_root.exists(), "Project root should exist"
        assert self.workbench.build_dir.exists(), "Build directory should exist"
        
    def test_available_potentials(self):
        """Test that expected potentials are available"""
        potentials = self.workbench.get_available_potentials()
        
        # Should have at least LJ and DFT potentials
        assert "lj_only" in potentials, "LJ potential should be available"
        assert "dft" in potentials, "DFT potential should be available"
        
    def test_lj_simulation(self):
        """Test running LJ simulation through Python API"""
        trajectory = self.workbench.run_lj_simulation()
        assert trajectory.exists(), "Trajectory file should exist"
        assert trajectory.suffix == ".xyz", "Trajectory should be XYZ format"
        assert trajectory.stat().st_size > 0, "Trajectory should have content"

    def test_integrator_and_thermostat_plugs(self):
        """Integrator and thermostat are chosen by name or plugged in as objects"""
        import molsimworkbench as mw
        _molsim = mw._molsim

        params = _molsim.SimulationParameters()
        assert params.integrator == "velocity_verlet"
        assert params.thermostat == "velocity_rescale"

        dynamics = _molsim.MolecularDynamics()
        assert dynamics.getIntegrator().getName() == "velocity_verlet"
        assert dynamics.getThermostat().getName() == "velocity_rescale"

        params.integrator = "leapfrog"
        params.thermostat = "berendsen"
        params.thermostatTau = 0.1
        dynamics.setSimulationParameters(params)
        assert dynamics.getIntegrator().getName() == "leapfrog"
        assert dynamics.getThermostat().getName() == "berendsen"

        params.thermostat = "none"
        dynamics.setSimulationParameters(params)
        assert dynamics.getThermostat() is None

        dynamics.setThermostat(_molsim.LangevinThermostat(friction=2.0, seed=1))
        dynamics.setIntegrator(_molsim.VelocityVerletIntegrator())
        dynamics.setSimulationParameters(params)  # explicit plugs are kept
        assert dynamics.getThermostat().getName() == "langevin"
        assert dynamics.getIntegrator().getName() == "velocity_verlet"

        try:
            _molsim.makeIntegrator("euler")
        except ValueError:
            pass
        else:
            raise AssertionError("unknown integrator name should raise")

        system = mw.System.from_yaml(
            self.workbench.project_root / "data" / "lj_langevin_simulation.yaml",
            trajectory_output=self.workbench.build_dir / "plug_test.xyz",
            trajectory_every=1000,
        )
        energy = system.run()
        assert energy == energy, "energy should not be NaN"

if __name__ == "__main__":
    # Run tests directly
    test = TestMolSimWorkbench()
    test.setup_method()
    
    print("Testing workbench initialization...")
    test.test_workbench_initialization()
    print("✓ Workbench initialization test passed")
    
    print("Testing available potentials...")
    test.test_available_potentials()
    print("✓ Available potentials test passed")
    
    print("Testing LJ simulation...")
    test.test_lj_simulation()
    print("✓ LJ simulation test passed")
    
    print("Testing integrator and thermostat plugs...")
    test.test_integrator_and_thermostat_plugs()
    print("✓ Integrator and thermostat plug test passed")
    
    print("\nAll tests passed! 🎉")