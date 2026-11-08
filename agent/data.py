"""Turn rMD17 ethanol into the files MACE reads.

The pool is the official training split (1000 frames) in a fixed shuffled order.
A run with n frames always uses the first n of that order, so adding data only
ever adds frames, and the same n always means the same frames.
"""
import ase.io
import numpy as np
from ase import Atoms

from . import config

KCAL_TO_EV = 0.0433641


def _load():
    d = np.load(config.RMD17_DIR / "npz_data" / f"rmd17_{config.MOLECULE}.npz")
    split = config.RMD17_DIR / "splits"
    train = np.loadtxt(split / f"index_train_{config.SPLIT}.csv", dtype=int)
    test = np.loadtxt(split / f"index_test_{config.SPLIT}.csv", dtype=int)
    return d, train, test


def _write(path, d, indices):
    frames = []
    for i in indices:
        a = Atoms(numbers=d["nuclear_charges"], positions=d["coords"][i])
        a.info["REF_energy"] = float(d["energies"][i] * KCAL_TO_EV)
        a.arrays["REF_forces"] = d["forces"][i] * KCAL_TO_EV
        frames.append(a)
    ase.io.write(path, frames, format="extxyz")


def write_frames(run_dir, n_train):
    """Write train.xyz (first n_train of the pool) and selection.xyz; return their paths.

    The selection frames are the first half of the official test split. The agent
    sees their errors and decides on them, so they are not a test set.
    """
    d, train, test = _load()
    pool = np.random.default_rng(0).permutation(train)
    train_path = run_dir / f"train_{n_train}.xyz"
    selection_path = run_dir / "selection.xyz"
    if not train_path.exists():
        _write(train_path, d, pool[:n_train])
    if not selection_path.exists():
        _write(selection_path, d, test[:config.SELECTION_FRAMES])
    return train_path, selection_path


def write_final_test(run_dir):
    """Write final_test.xyz: the second half of the official test split.

    Only the harness reads it, once, after the agent stops.
    """
    d, _, test = _load()
    start = config.SELECTION_FRAMES
    path = run_dir / "final_test.xyz"
    if not path.exists():
        _write(path, d, test[start:start + config.FINAL_TEST_FRAMES])
    return path
