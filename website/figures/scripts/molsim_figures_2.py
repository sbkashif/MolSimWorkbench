"""Figures for "Running the minimal simulation algorithm" and "Reading the engine".
One function per figure; helpers come from molsim_figures.py.

    python molsim_figures_2.py          # writes every figure into ../

Data (in data/): run_energy.csv and run_msd.csv come from the 512-atom run
shown on the home page (the cover recipe, cover/cover.yaml); the four
eps*_sig*.xyz files are the last frames of the runs in eps_sigma_run.yaml;
run_collapse.csv comes from mean_energy in molsim/examples/sweep.py at 512 atoms
and 6000 steps.
"""
import csv
import math
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


def fig_loop_tall():
    """The loop of fig_md_loop, tall and narrow, for the home page's column.
    Beside each step is a small picture of what it does."""
    from matplotlib.patches import Circle, Rectangle
    W, H = 480, 730
    fig, ax = canvas(W, H)
    cx, bw = 180, 256
    ys = {"System": 85, "Force step": 270, "Integrator": 455, "Thermostat": 640}
    lines = {
        "System": ["positions and velocities", "of every atom, in a box"],
        "Force step": ["force on every atom", "from the energy function"],
        "Integrator": ["move every atom by", "one time step"],
        "Thermostat": ["rescale velocities to", "the target temperature"],
    }
    for name, y in ys.items():
        box(ax, cx, y, bw, 116, name, lines[name])
    steps = [("System", "Force step", "positions"),
             ("Force step", "Integrator", "forces"),
             ("Integrator", "Thermostat", "new positions")]
    for a, b, label in steps:
        mf.arrow(ax, (cx, ys[a] + 60), (cx, ys[b] - 60), label=label,
                 label_xy=(cx + 62, (ys[a] + ys[b]) / 2))
    left = cx - bw / 2
    ax.plot([left, 22, 22], [ys["Thermostat"], ys["Thermostat"], ys["System"]],
            color=EDGE, lw=2)
    mf.arrow(ax, (22, ys["System"]), (left - 2, ys["System"]))
    ax.text(12, (ys["System"] + ys["Thermostat"]) / 2, "next step", rotation=90,
            ha="center", va="center", fontsize=12, color=MUTED, style="italic")

    px, pw = 394, 150

    def atom(x, y, r=13, **style):
        ax.add_patch(Circle((x, y), r, **{"fc": PLUG, "ec": "white", "lw": 1.2,
                                          **style}))

    # System: atoms in a box.
    side, sy = 112, ys["System"]
    ax.add_patch(Rectangle((px - side / 2, sy - side / 2), side, side, fc="white",
                           ec=EDGE, lw=1.8))
    for dx, dy in [(-34, -34), (-4, -38), (30, -30), (-38, -4), (-8, -8), (22, 0),
                   (-30, 28), (2, 24), (34, 34)]:
        atom(px + dx, sy + dy)

    # Force step: the energy of two atoms against their distance. Faint curves
    # behind the LJ one show that other energy functions fit the same place.
    py = ys["Force step"]
    ax.plot([cx + bw / 2, px - 66], [py, py], color=PLUG, lw=4)
    x0, x1, y0, y1 = px - 62, px + 62, py - 52, py + 34
    ax.plot([x0, x0, x1], [y0, y1 - 14, y1 - 14], color=EDGE, lw=1.8)
    def curve(energy, start, **style):
        rs = [start + 0.01 * i for i in range(int((2.5 - start) * 100))]
        pts = [(r, energy(r)) for r in rs if energy(r) < 1.7]
        ax.plot([x0 + (r - 0.9) / 1.7 * (x1 - x0) for r, _ in pts],
                [y1 - 50 - u * 26 for _, u in pts], color=PLUG, **style)

    def morse(depth, width, rest):
        return lambda r: depth * ((1 - math.exp(-width * (r - rest))) ** 2 - 1)

    faint = {"lw": 1.6, "ls": (0, (3, 2)), "alpha": 0.55}
    curve(morse(0.55, 2.6, 1.4), 0.95, **faint)
    curve(morse(1.25, 4.5, 1.2), 0.95, **faint)
    curve(lambda r: 4 * (r ** -12 - r ** -6), 0.97, lw=3, solid_capstyle="round")
    ax.text(px, py + 46, "energy function", ha="center", va="center", fontsize=12,
            color=PLUG, weight="bold")
    ax.text(px, py + 66, "LJ here, or your own", ha="center", va="center",
            fontsize=10.5, color=MUTED, style="italic")

    # Integrator: an atom one time step later.
    iy = ys["Integrator"]
    atom(px - 50, iy + 22, r=17, fc="white", ec=PLUG, lw=1.8, ls="--")
    atom(px + 48, iy - 22, r=17)
    mf.arrow(ax, (px - 30, iy + 13), (px + 28, iy - 13))
    ax.text(px - 50, iy + 56, "t", ha="center", va="center", fontsize=12,
            color=MUTED, style="italic")
    ax.text(px + 48, iy + 12, "t + Δt", ha="center", va="center", fontsize=12,
            color=MUTED, style="italic")

    # Thermostat: a thermometer, held at the target.
    ty, hot = ys["Thermostat"], "#d9822b"
    ax.add_patch(FancyBboxPatch((px - 11, ty - 58), 22, 92,
                                boxstyle="round,pad=0,rounding_size=11",
                                fc="white", ec=EDGE, lw=1.8))
    ax.add_patch(Circle((px, ty + 40), 19, fc=hot, ec=EDGE, lw=1.8))
    ax.add_patch(Rectangle((px - 5, ty - 20), 10, 50, fc=hot, ec="none"))
    ax.plot([px + 15, px + 32], [ty - 20, ty - 20], color=INK, lw=2)
    ax.text(px + 38, ty - 20, "target", ha="left", va="center", fontsize=11.5,
            color=MUTED, style="italic")
    save(fig, "loop_tall.svg")


def fig_run_collapse():
    """Two pairs of sweeps, each pair falling on one curve."""
    rows = {}
    with open(os.path.join(DATA, "run_collapse.csv")) as f:
        for r in list(csv.DictReader(f)):
            rows.setdefault(r["setting"], []).append((float(r["x"]), float(r["y"])))
    fig, axes = plt.subplots(1, 2, figsize=(8.6, 3.5), sharey=True)
    panels = [("T", "changing T", "eps", "changing ε", r"$k_B T / \varepsilon$"),
              ("rho", "changing density", "sig", "changing σ", r"$\rho \sigma^3$")]
    for ax, (a, la, b, lb, xlabel) in zip(axes, panels):
        x, y = zip(*sorted(rows[a]))
        ax.plot(x, y, "o-", ms=7, lw=2, color=CPP, label=la)
        x, y = zip(*sorted(rows[b]))
        ax.plot(x, y, "s", ms=13, mew=2, mfc="none", color="#c2571a", label=lb)
        ax.set_xlabel(xlabel, color=INK)
        ax.legend(frameon=False, labelcolor=INK)
        ax.grid(color="#e6e8eb", lw=0.8)
        ax.set_axisbelow(True)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)
        ax.spines["left"].set_color(EDGE)
        ax.spines["bottom"].set_color(EDGE)
        ax.tick_params(colors=MUTED)
    axes[0].set_ylabel("potential energy per atom / ε", color=INK)
    fig.tight_layout()
    save(fig, "run_collapse.svg")


def fig_run_layers():
    """The three configuration files and how they reach the engine."""
    W, H = 990, 330
    fig, ax = canvas(W, H)
    box(ax, 180, 90, 330, 110, "lj_bulk_simulation.yaml",
        ["time step, steps, temperature,", "number of atoms, density"])
    box(ax, 180, 250, 330, 90, "potentials/lj_bulk.yaml",
        ["energy function: LJ, ε, σ, cutoff"], ec=PLUG)
    box(ax, 560, 250, 300, 90, "geometry/lj_pair.yaml",
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
    fig_run_collapse()
    fig_loop_tall()
    fig_engine_classes()
