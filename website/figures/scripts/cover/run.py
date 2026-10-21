# The script shown on the cover. Run it with the repo root on PYTHONPATH and
# data/lj_box.yaml (a copy of cover.yaml) plus data/potentials/lj_bulk.yaml
# in the working directory.
from molsimworkbench import System

sim = System.from_yaml("data/lj_box.yaml",
                       trajectory_output="traj.xyz")
sim.run()
