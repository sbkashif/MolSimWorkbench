"""A real slice of the trained model's energy surface, for the PES figure.

Starts from the first test frame of the page-1 run, then changes two of
ethanol's coordinates and holds the rest fixed: the C-O bond length (O and its
H move along the bond) and the H-O-C-C twist (only the hydroxyl H turns). It also writes a
relaxed twist scan, where every other atom is free to settle. Both
ranges stay inside what the training frames cover: C-O from the 1st to the 99th
percentile of the training frames, the twist over its full turn.

    conda run -n mlip-agent python agent_pes_slice.py ~/workspace/mlip-spike
"""
import os
import sys
import warnings

import ase.io
import numpy as np

RUN = os.path.expanduser(sys.argv[1])
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
C1, C2, O, HO = 0, 1, 2, 8          # atom order in the rMD17/MD17 ethanol frames

train = ase.io.read(os.path.join(RUN, "data/train.xyz"), ":")
co = np.array([a.get_distance(C1, O) for a in train])
lo, hi = np.percentile(co, [1, 99])

from mace.calculators import MACECalculator
with warnings.catch_warnings():
    warnings.simplefilter("ignore")
    calc = MACECalculator(model_paths=os.path.join(RUN, "spike.model"), device="cpu",
                          default_dtype="float32")

start = ase.io.read(os.path.join(RUN, "data/test.xyz"), 0)
lengths = np.linspace(lo, hi, 23)
twists = np.linspace(0, 360, 37)
rows = []
for d in lengths:
    for t in twists:
        a = start.copy()
        a.set_distance(C1, O, d, fix=0, mask=[i in (O, HO) for i in range(len(a))])
        # dihedral C2-C1-O-HO is the same angle as HO-O-C1-C2; naming HO last
        # makes ASE move HO itself, so the angle set is the angle measured
        a.set_dihedral(C2, C1, O, HO, t, indices=[HO])
        assert abs((a.get_dihedral(HO, O, C1, C2) - t + 180) % 360 - 180) < 1e-6
        a.calc = calc
        rows.append((round(float(d), 4), float(t), a.get_potential_energy()))
rows = np.array(rows)
rows[:, 2] -= rows[:, 2].min()
np.savetxt(os.path.join(OUT, "agent_pes_slice.csv"), rows, delimiter=",",
           header="CO_length_A,HOCC_twist_deg,energy_above_min_eV", comments="",
           fmt=["%.4f", "%.1f", "%.5f"])
print(f"C-O range {lo:.3f}-{hi:.3f} A; energy range {rows[:, 2].max():.3f} eV")

# Relaxed twist: hold only the twist, let every other atom settle (BFGS)
from ase.constraints import FixInternals
from ase.optimize import BFGS
relaxed = []
for t in np.arange(0, 360, 10.0):
    a = start.copy()
    a.set_dihedral(C2, C1, O, HO, t, indices=[HO])
    a.calc = calc
    a.set_constraint(FixInternals(dihedrals_deg=[[a.get_dihedral(HO, O, C1, C2),
                                                  [HO, O, C1, C2]]]))
    BFGS(a, logfile=None).run(fmax=0.01, steps=500)
    relaxed.append((float(t), round(a.get_dihedral(HO, O, C1, C2), 2),
                    round(a.get_distance(C1, O), 4), a.get_potential_energy()))
relaxed = np.array(relaxed)
relaxed[:, 3] -= relaxed[:, 3].min()
np.savetxt(os.path.join(OUT, "agent_twist_relaxed.csv"), relaxed, delimiter=",",
           header="twist_set_deg,twist_after_deg,CO_length_A,energy_above_min_eV",
           comments="", fmt=["%.1f", "%.2f", "%.4f", "%.5f"])
