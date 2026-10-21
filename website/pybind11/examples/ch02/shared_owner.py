# The ForceCalculator keeps the potential alive after Python lets go of it.
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
from molsimworkbench import _molsim

forces = _molsim.ForceCalculator()
lj = _molsim.LJPotential()
lj.addAtomParameters("LJ", 1.0, 1.0)
lj.setCutoff(2.5)
forces.addPotential(lj)
del lj                                     # Python's name is gone ...

geometry = _molsim.Geometry()
geometry.addAtom("LJ", [0.0, 0.0, 0.0])
geometry.addAtom("LJ", [1.5, 0.0, 0.0])
print("energy:", forces.calculateEnergy(geometry))   # ... the potential is not

try:
    forces.addPotential(5)
except TypeError as error:
    print("TypeError:", error)
