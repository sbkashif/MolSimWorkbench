"""Read an XYZ trajectory written by MolSimWorkbench into a NumPy array."""
import sys

import numpy as np


def read_xyz(path):
    """Return positions with shape (frames, atoms, 3)."""
    frames = []
    with open(path) as f:
        lines = f.read().splitlines()
    i = 0
    while i < len(lines):
        n = int(lines[i])                      # line 1: number of atoms
        rows = lines[i + 2:i + 2 + n]          # line 2 is a comment
        frames.append([[float(c) for c in row.split()[1:4]] for row in rows])
        i += 2 + n
    return np.array(frames)


if __name__ == "__main__":
    x = read_xyz(sys.argv[1])
    print("frames, atoms, coordinates:", x.shape)
    print("atom 0, first frame:", x[0, 0])
    print("atom 0, last frame: ", x[-1, 0])
