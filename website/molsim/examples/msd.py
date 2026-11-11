"""Mean-square displacement of an MolSimWorkbench trajectory.

    python msd.py traj.xyz <box length> <time step>
"""
import sys

import numpy as np

from read_xyz import read_xyz

x = read_xyz(sys.argv[1])
L, dt = float(sys.argv[2]), float(sys.argv[3])

# The engine wraps atoms back into the box, so an atom that crosses a face
# jumps by one box length. Undo the jumps: take each frame-to-frame step by
# the minimum image, then add the steps up.
step = np.diff(x, axis=0)
step -= L * np.round(step / L)
unwrapped = np.concatenate([x[:1], x[:1] + np.cumsum(step, axis=0)])

for t in (0.25, 0.5, 1.0):
    k = round(t / dt)
    msd = ((unwrapped[k] - unwrapped[0]) ** 2).sum(axis=1).mean()
    print(f"t = {t:4}: MSD = {msd:.3f}")
