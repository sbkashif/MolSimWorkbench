"""Figures for the molecular-simulation pages. One function per figure.

    python molsim_figures.py          # writes every figure into ../
"""
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
INK = "#222222"
MUTED = "#5f6368"
EDGE = "#9aa0a6"
PLUG = "#1a9e9a"          # the energy function: same teal as the atoms
MONO = "DejaVu Sans Mono"


def canvas(w, h):
    fig = plt.figure(figsize=(w / 100, h / 100), dpi=100)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, w)
    ax.set_ylim(h, 0)
    ax.axis("off")
    return fig, ax


def box(ax, x, y, w, h, title, lines=(), fc="white", ec=EDGE, ls="-",
        title_color=INK):
    ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                                boxstyle="round,pad=0,rounding_size=12",
                                fc=fc, ec=ec, lw=1.8, ls=ls))
    n = len(lines)
    ty = y - 12 * n
    ax.text(x, ty, title, ha="center", va="center", fontsize=15,
            weight="bold", color=title_color)
    for i, line in enumerate(lines):
        ax.text(x, ty + 26 * (i + 1), line, ha="center", va="center",
                fontsize=12.5, color=MUTED)


def arrow(ax, p, q, rad=0.0, label=None, label_xy=None):
    ax.add_patch(FancyArrowPatch(p, q, connectionstyle=f"arc3,rad={rad}",
                                 arrowstyle="-|>", mutation_scale=20,
                                 color=EDGE, lw=2))
    if label:
        ax.text(*label_xy, label, ha="center", va="center", fontsize=12,
                color=MUTED, style="italic")


def save(fig, name):
    fig.savefig(os.path.join(OUT, name), format="svg")
    if os.environ.get("PREVIEW"):          # PNG twin for checking by eye
        fig.savefig(os.path.join(os.environ["PREVIEW"], name[:-4] + ".png"))
    plt.close(fig)


def fig_md_loop():
    """The MD loop, with the energy function as a plug in the force step."""
    W, H = 1000, 560
    fig, ax = canvas(W, H)

    sx, sy = 180, 140          # system state
    fx, fy = 560, 140          # force step
    ix, iy = 560, 420          # integrator step
    tx, ty = 180, 420          # thermostat

    box(ax, sx, sy, 300, 120, "System",
        ["positions and velocities", "of every atom, in a box"])
    box(ax, fx, fy, 300, 120, "Force step",
        ["force on every atom", "from the energy function"])
    box(ax, ix, iy, 300, 110, "Integrator",
        ["move every atom by one", "time step (Velocity-Verlet)"])
    box(ax, tx, ty, 300, 110, "Thermostat",
        ["rescale velocities to", "the target temperature"])

    arrow(ax, (sx + 152, sy), (fx - 152, fy), label="positions",
          label_xy=((sx + fx) / 2, sy - 18))
    arrow(ax, (fx, fy + 62), (ix, iy - 57), label="forces",
          label_xy=(fx + 40, (fy + iy) / 2))
    arrow(ax, (ix - 152, iy), (tx + 152, iy), label="new positions",
          label_xy=((ix + tx) / 2, iy - 18))
    arrow(ax, (tx, iy - 57), (sx, sy + 62), label="next step",
          label_xy=(sx - 50, (sy + iy) / 2))

    # The plug: the energy function sits in a socket on the force step's
    # right side. Dashed boxes below it are other energy functions that fit
    # the same socket.
    px, py = 850, fy
    ax.plot([fx + 150, px - 95], [py, py], color=PLUG, lw=4)
    ax.add_patch(FancyBboxPatch((px - 95, py - 26), 190, 52,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc=PLUG, ec="none"))
    ax.text(px, py, "LJ", ha="center", va="center",
            fontsize=16, color="white", weight="bold")
    ax.text(px, py - 50, "Energy function", ha="center", va="center",
            fontsize=13, color=PLUG, weight="bold")
    for k, name in enumerate(["DFT", "machine-learned"]):
        y = py + 75 + k * 62
        ax.add_patch(FancyBboxPatch((px - 95, y - 22), 190, 44,
                                    boxstyle="round,pad=0,rounding_size=10",
                                    fc="white", ec=PLUG, lw=1.8, ls="--"))
        ax.text(px, y, name, ha="center", va="center", fontsize=13.5,
                color=PLUG)
    ax.text(px, py + 75 + 2 * 62 - 10, "or any other", ha="center",
            va="center", fontsize=12, color=MUTED, style="italic")
    save(fig, "md_loop.svg")
    return fig


def fig_periodic_box():
    """A 2D periodic box: the real box, its copies, and a pair that interacts
    across the wall through the nearest copy."""
    import numpy as np
    L = 1.0
    atoms = np.array([[0.12, 0.55], [0.88, 0.48], [0.45, 0.2], [0.6, 0.78],
                      [0.35, 0.62]])
    fig, ax = plt.subplots(figsize=(5.2, 5.2), dpi=100)
    for i in (-1, 0, 1):
        for j in (-1, 0, 1):
            real = i == 0 and j == 0
            ax.add_patch(plt.Rectangle((i * L, j * L), L, L, fill=False,
                                       ec=INK if real else EDGE,
                                       lw=2.2 if real else 1, ls="-" if real else "--"))
            ax.scatter(atoms[:, 0] + i * L, atoms[:, 1] + j * L, s=110,
                       color=PLUG, alpha=1.0 if real else 0.3, zorder=3)
    a, b = atoms[0], atoms[1]
    ax.plot([a[0], b[0] - L], [a[1], b[1]], color=INK, lw=1.8, zorder=2)
    ax.text((a[0] + b[0] - L) / 2, a[1] + 0.09, "nearest copy", ha="center",
            fontsize=10.5, color=INK)
    ax.plot([a[0], b[0]], [a[1], b[1]], color=EDGE, lw=1.2, ls=":", zorder=2)
    ax.text(0.5, 0.42, "far way round", ha="center", fontsize=10, color=MUTED)
    ax.text(0.5, -0.07, "the real box", ha="center", va="top", fontsize=11.5,
            color=INK, weight="bold")
    ax.set_xlim(-1.05, 2.05)
    ax.set_ylim(-1.05, 2.05)
    ax.set_aspect("equal")
    ax.axis("off")
    fig.tight_layout()
    save(fig, "periodic_box.svg")


def fig_sigma_epsilon_runs():
    """Four runs of the engine, same 512 atoms in the same box at T = 1:
    a thin slice seen from above, each atom drawn to scale (diameter sigma)."""
    import numpy as np
    from matplotlib.patches import Circle
    L = (512 / 0.3) ** (1 / 3)
    data = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
    panels = [
        ("eps0.5_sig1.0", "ε = 0.5", "weak attraction: spread out"),
        ("eps2.0_sig1.0", "ε = 2", "strong attraction: pulled together"),
        ("eps0.5_sig0.8", "σ = 0.8", "small atoms: mostly empty"),
        ("eps0.5_sig1.4", "σ = 1.4", "large atoms: crowded"),
    ]
    fig, axes = plt.subplots(1, 4, figsize=(12, 3.6), dpi=100)
    for ax, (name, title, note) in zip(axes, panels):
        sig = float(name.split("sig")[1])
        rows = open(os.path.join(data, name + ".xyz")).read().splitlines()[2:]
        X = np.array([[float(c) for c in r.split()[1:4]] for r in rows])
        # A slice 3 sigma thick, centred where the atoms are densest.
        counts, edges = np.histogram(X[:, 2], bins=24, range=(-L / 2, L / 2))
        zc = (edges[np.argmax(counts)] + edges[np.argmax(counts) + 1]) / 2
        dz = (X[:, 2] - zc + L / 2) % L - L / 2
        slab = np.abs(dz) < 1.5
        for x, y in X[slab, :2]:
            ax.add_patch(Circle((x, y), sig / 2, fc=PLUG, ec="#127572",
                                lw=0.8, alpha=0.85))
        ax.add_patch(plt.Rectangle((-L / 2, -L / 2), L, L, fill=False,
                                   ec=MUTED, lw=1.2))
        ax.set_xlim(-L / 2 - 0.3, L / 2 + 0.3)
        ax.set_ylim(-L / 2 - 0.3, L / 2 + 0.3)
        ax.set_aspect("equal")
        ax.axis("off")
        ax.set_title(title, fontsize=14, weight="bold", color=INK)
        ax.text(0, -L / 2 - 1.0, note, ha="center", va="top", fontsize=12,
                color=MUTED)
    fig.tight_layout()
    save(fig, "sigma_epsilon_runs.svg")


def fig_engine_loop():
    """Reading the engine: one pass of runSimulation() as a circle of four steps,
    each labelled with the engine function that does it."""
    import numpy as np
    W, H = 960, 560
    fig, ax = canvas(W, H)
    cx, cy, Rx, Ry = W / 2, H / 2 + 5, 320, 185
    steps = [  # angle (deg, clockwise from top), title, function
        (0, "move every atom", "integrator->step()"),
        (90, "forces at the\nnew positions", "computeForces()"),
        (180, "finish the velocities", "integrator->step()"),
        (270, "hold the temperature", "thermostat->afterStep()"),
    ]
    pos = []
    for ang, title, fn in steps:
        a = np.radians(ang)
        x, y = cx + Rx * np.sin(a), cy - Ry * np.cos(a)
        pos.append((x, y))
        ax.add_patch(FancyBboxPatch((x - 135, y - 38), 270, 76,
                                    boxstyle="round,pad=0,rounding_size=12",
                                    fc="white", ec=EDGE, lw=1.8, zorder=3))
        ax.text(x, y - 11, title, ha="center", va="center", fontsize=13.5,
                weight="bold", color=INK, zorder=4, linespacing=1.15)
        ax.text(x, y + 22, fn, ha="center", va="center", fontsize=11,
                family=MONO, color="#127572", zorder=4)
    # Clockwise arrows along the circle between neighbouring steps.
    for k in range(4):
        a0 = np.radians(steps[k][0] + 33)
        a1 = np.radians(steps[k][0] + 57)
        p = (cx + Rx * np.sin(a0), cy - Ry * np.cos(a0))
        q = (cx + Rx * np.sin(a1), cy - Ry * np.cos(a1))
        ax.add_patch(FancyArrowPatch(p, q, connectionstyle="arc3,rad=-0.25",
                                     arrowstyle="-|>", mutation_scale=20,
                                     color=EDGE, lw=2.2))
    ax.text(cx, cy - 14, "one pass of", ha="center", va="center",
            fontsize=12.5, color=MUTED)
    ax.text(cx, cy + 10, "runSimulation()", ha="center", va="center",
            fontsize=13, family=MONO, color=INK, weight="bold")
    ax.text(cx, cy + 34, "= one time step", ha="center", va="center",
            fontsize=12.5, color=MUTED)
    save(fig, "engine_loop.svg")


if __name__ == "__main__":
    fig_engine_loop()
    fig_md_loop()
    fig_periodic_box()
    # fig_sigma_epsilon_runs() is kept for a later page on running
    # simulations; it is not on any page yet.
