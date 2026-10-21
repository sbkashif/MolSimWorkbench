"""Figures for "Running a simulation" and "Reading the engine".
One function per figure; helpers come from molsim_figures.py.

    python molsim_figures_2.py          # writes every figure into ../

Data (in data/): run_energy.csv and run_msd.csv come from the 512-atom run
shown on the home page (the cover recipe, cover/cover.yaml); the four
eps*_sig*.xyz files are the last frames of the runs in eps_sigma_run.yaml.
"""
import csv
import os

import molsim_figures as mf
from molsim_figures import (EDGE, INK, MONO, MUTED, PLUG, FancyArrowPatch,
                            FancyBboxPatch, box, canvas, plt, save)

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
CPP = "#2f6db5"


def read_csv(name):
    with open(os.path.join(DATA, name)) as f:
        rows = list(csv.reader(f))[1:]
    return [[float(v) for v in r] for r in rows]


def fig_run_layers():
    """The three configuration files and how they reach the engine."""
    W, H = 990, 330
    fig, ax = canvas(W, H)
    box(ax, 180, 90, 330, 110, "lj_bulk_simulation.yaml",
        ["time step, steps, temperature,", "number of atoms, density"])
    box(ax, 180, 250, 330, 90, "potentials/lj_bulk.yaml",
        ["energy function: LJ, ε, σ, cutoff"], ec=PLUG)
    box(ax, 560, 250, 300, 90, "geometry/water.yaml",
        ["atom positions, box size"], ls="--")
    ax.text(560, 312, "only without lj_particle_box", ha="center", va="center",
            fontsize=11, color=MUTED, style="italic")
    mf.arrow(ax, (180, 147), (180, 203), label="names", label_xy=(215, 175))
    mf.arrow(ax, (320, 147), (470, 204), label="names", label_xy=(430, 165))
    # The engine reads all of it.
    box(ax, 840, 90, 260, 110, "the engine",
        ["run_simulation (C++), or", "System.from_yaml (Python)"], ec=CPP)
    mf.arrow(ax, (347, 90), (708, 90), label="read by", label_xy=(525, 72))
    save(fig, "run_layers.svg")


def fig_run_energy():
    """Potential energy per atom during the run: a climb, then a plateau."""
    rows = read_csv("run_energy.csv")
    t = [r[0] * 0.001 for r in rows]
    u = [r[1] / 512 for r in rows]
    fig, ax = plt.subplots(figsize=(7.2, 3.3), dpi=100)
    ax.plot(t, u, color=PLUG, lw=1.8)
    late = [v for s, v in zip(t, u) if s >= 1.0]
    mean = sum(late) / len(late)
    ax.axhline(mean, color=MUTED, lw=1, ls="--")
    ax.text(3.95, mean - 0.12, f"average after t = 1: {mean:.2f}", ha="right",
            fontsize=11, color=MUTED)
    ax.axvspan(0, 1.0, color="#f1f3f4", zorder=0)
    ax.text(0.6, 0.9, "settling", transform=ax.get_xaxis_transform(),
            ha="center", fontsize=11, color=MUTED)
    ax.set_xlim(0, 4)
    ax.set_xlabel("time (reduced units)", fontsize=11.5)
    ax.set_ylabel("potential energy\nper atom (ε)", fontsize=11.5)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    fig.tight_layout()
    save(fig, "run_energy.svg")


def fig_run_msd():
    """Mean-square displacement: a straight line, so the atoms keep moving."""
    rows = read_csv("run_msd.csv")
    t = [r[0] for r in rows]
    m = [r[1] for r in rows]
    fig, ax = plt.subplots(figsize=(7.2, 3.3), dpi=100)
    ax.plot(t, m, color=PLUG, lw=2.2, label="this run")
    ax.axhline(1.077 ** 2, color=MUTED, lw=1, ls="--")
    ax.text(0.05, 1.077 ** 2 + 0.05, "(spacing between neighbours)²",
            fontsize=11, color=MUTED)
    ax.set_xlim(0, 4)
    ax.set_ylim(0, 1.9)
    ax.set_xlabel("time (reduced units)", fontsize=11.5)
    ax.set_ylabel("mean-square\ndisplacement (σ²)", fontsize=11.5)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    fig.tight_layout()
    save(fig, "run_msd.svg")


def fig_run_sigma_epsilon():
    """The parked eps/sigma box runs, saved under this page's name."""
    original = mf.save
    mf.save = lambda fig, name: original(fig, "run_" + name)
    try:
        mf.fig_sigma_epsilon_runs()
    finally:
        mf.save = original


def fig_engine_classes():
    """Which object owns which, in the running engine."""
    W, H = 1000, 420
    fig, ax = canvas(W, H)
    box(ax, 170, 90, 320, 100, "MolecularDynamics",
        ["the loop: integrate, thermostat,", "write the trajectory"], ec=CPP)
    box(ax, 170, 300, 320, 100, "Geometry",
        ["atom types and positions,", "box size"])
    box(ax, 540, 90, 280, 100, "ForceCalculator",
        ["a list of potentials;", "adds up their forces"])
    box(ax, 540, 300, 280, 100, "Potential",
        ["the socket: calculateEnergy,", "calculateForces"], ls="--", ec=PLUG)
    box(ax, 850, 240, 220, 70, "LJPotential", [], fc="white", ec=PLUG)
    box(ax, 850, 360, 220, 70, "DFTPotential", [], fc="white", ec=PLUG)
    mf.arrow(ax, (170, 142), (170, 248), label="holds a copy",
             label_xy=(232, 195))
    mf.arrow(ax, (332, 90), (398, 90), label="shares", label_xy=(365, 72))
    mf.arrow(ax, (540, 142), (540, 248), label="list of", label_xy=(585, 195))
    mf.arrow(ax, (738, 240), (682, 285), label="", label_xy=(0, 0))
    mf.arrow(ax, (738, 360), (682, 318), label="", label_xy=(0, 0))
    ax.text(712, 238, "is a", ha="center", va="center", fontsize=11,
            color=MUTED, style="italic")
    ax.text(712, 372, "is a", ha="center", va="center", fontsize=11,
            color=MUTED, style="italic")
    save(fig, "engine_classes.svg")


if __name__ == "__main__":
    fig_run_layers()
    fig_run_energy()
    fig_run_msd()
    fig_run_sigma_epsilon()
    fig_engine_classes()
