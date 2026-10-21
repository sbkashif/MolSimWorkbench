# Build a two-atom system from Python, piece by piece, with the engine's own
# classes. Run from anywhere: the repo root is put on the import path.
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
from molsimworkbench import _molsim

lj = _molsim.LJPotential()
lj.addAtomParameters("LJ", 1.0, 1.0)      # atom type, epsilon, sigma
lj.setCutoff(2.5)

geometry = _molsim.Geometry()
geometry.addAtom("LJ", [0.0, 0.0, 0.0])
geometry.addAtom("LJ", [1.5, 0.0, 0.0])

forces = _molsim.ForceCalculator()
forces.addPotential(lj)

print("energy:", forces.calculateEnergy(geometry))
print("forces:", forces.calculateForces(geometry))
