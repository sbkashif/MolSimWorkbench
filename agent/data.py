"""Turn rMD17 ethanol into the files MACE reads.

The pool is the official training split (1000 frames) in a fixed shuffled order.
A run with n frames always uses the first n of that order, so adding data only
ever adds frames, and the same n always means the same frames.

The selection frames come from the frames that split leaves out of both its
training and test sets. The official test set (1000 frames) is kept whole for
the final test.
"""
import ase.io
import numpy as np
from ase import Atoms

from . import config

KCAL_TO_EV = 0.0433641


def _load():
    # All 100,000 ethanol frames, and the frame numbers in the
    # official training and test sets of one split.
    d = np.load(config.RMD17_DIR / "npz_data" / f"rmd17_{config.MOLECULE}.npz")
    split = config.RMD17_DIR / "splits"
    train = np.loadtxt(split / f"index_train_{config.SPLIT}.csv", dtype=int)
    test = np.loadtxt(split / f"index_test_{config.SPLIT}.csv", dtype=int)
    return d, train, test


def _selection(d, train, test):
    """A fixed random draw of frames in neither the training nor the test split."""
    # Every frame number, minus the 2000 the split uses.
    all_frames = np.arange(len(d["energies"]))
    unused = np.setdiff1d(all_frames, np.concatenate([train, test]))
    # Seed 0: every run draws the same 500 frames.
    rng = np.random.default_rng(0)
    return rng.choice(unused, config.SELECTION_FRAMES, replace=False)


def _write(path, d, indices):
    # One extended-XYZ file, with energies and forces in eV and eV/Å.
    frames = []
    for i in indices:
        a = Atoms(numbers=d["nuclear_charges"], positions=d["coords"][i])
        a.info["REF_energy"] = float(d["energies"][i] * KCAL_TO_EV)
        a.arrays["REF_forces"] = d["forces"][i] * KCAL_TO_EV
        frames.append(a)
    ase.io.write(path, frames, format="extxyz")


def write_frames(run_dir, n_train):
    """Write the training and selection frames; return their paths."""
    d, train, test = _load()
    # The training pool, shuffled once in a fixed order (seed 0).
    pool = np.random.default_rng(0).permutation(train)
    train_path = run_dir / f"train_{n_train}.xyz"
    selection_path = run_dir / "selection.xyz"
    # Training frames: the first n_train of the pool.
    if not train_path.exists():
        _write(train_path, d, pool[:n_train])
    # Selection frames: written once per run, the same every time.
    if not selection_path.exists():
        _write(selection_path, d, _selection(d, train, test))
    return train_path, selection_path


def write_final_test(run_dir):
    """Write the final test frames: the whole official test split."""
    # Only the harness calls this, once, after the agent stops.
    d, _, test = _load()
    path = run_dir / "final_test.xyz"
    if not path.exists():
        _write(path, d, test)
    return path
