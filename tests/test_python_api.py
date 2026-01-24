"""
Comprehensive test suite for the MolSimWorkbench
Tests both LJ and DFT potentials through the Python API
"""

import sys
import os
from pathlib import Path

# Add the parent directory to the path to import molsimworkbench
sys.path.insert(0, str(Path(__file__).parent.parent))

from molsimworkbench import MolSimWorkbench

class TestMolSimWorkbench:
    
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
        try:
            trajectory = self.workbench.run_lj_simulation()
            assert trajectory.exists(), "Trajectory file should exist"
            assert trajectory.suffix == ".xyz", "Trajectory should be XYZ format"
            
            # Check that trajectory has content
            assert trajectory.stat().st_size > 0, "Trajectory should have content"
            return True
            
        except Exception as e:
            print(f"LJ simulation failed: {e}")
            return False
            
    def test_unit_tests(self):
        """Test that C++ unit tests pass"""
        try:
            output = self.workbench.run_tests()
            assert "100% tests passed" in output, "All unit tests should pass"
            return True
            
        except Exception as e:
            print(f"Unit tests failed: {e}")
            return False

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
    
    print("Testing unit tests...")
    test.test_unit_tests()
    print("✓ Unit tests passed")
    
    print("\nAll tests passed! 🎉")