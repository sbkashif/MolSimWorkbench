# The script from the home page, plus a look at what it built.
# Run it from the top folder of the repository, where data/ is.
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[4]))
import molsimworkbench
from molsimworkbench import System

print("bindings loaded from:", Path(molsimworkbench._molsim.__file__).name)
sim = System.from_yaml("data/lj_play_simulation.yaml",
                       trajectory_output="traj.xyz")
print("type of sim._dynamics:", type(sim._dynamics))
energy = sim.run()
print("final energy:", round(energy, 2))
