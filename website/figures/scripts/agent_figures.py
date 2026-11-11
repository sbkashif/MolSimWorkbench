"""Figures for the agent section. One function per figure.

    python agent_figures.py          # writes every figure into ../

Data comes from agent_extract.py, which reads the real training run.
"""
import csv
import json
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Circle, Rectangle

from molsim_figures import (EDGE, INK, MUTED, PLUG, arrow, box, canvas, save)

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
LLM = "#6a5acd"
COLORS = {"C": "#555555", "O": "#c0392b", "H": "#d5d8dc"}


def rows(name):
    with open(os.path.join(DATA, name)) as f:
        return [[float(v) for v in r] for r in list(csv.reader(f))[1:]]


def frame():
    with open(os.path.join(DATA, "agent_one_frame.json")) as f:
        return json.load(f)


def fig_mlip_atom():
    """One atom's neighbours in, 64 numbers, one energy share, total energy."""
    d = frame()
    W, H = 1000, 400
    fig, ax = canvas(W, H)

    p = np.array(d["positions_A"])
    p = p - p[0]
    _, _, v = np.linalg.svd(p - p.mean(0))
    xy = p @ v[:2].T                       # flattest view of the molecule
    s = 29
    cx, cy = 150, 185
    pts = [(cx + s * x, cy + s * y) for x, y in xy]
    ax.add_patch(Circle((cx, cy), s * d["r_max_A"], fc="none", ec=PLUG, lw=1.6, ls="--"))
    dist = np.linalg.norm(p[:, None] - p[None], axis=-1)
    for i in range(9):
        for j in range(i):
            heavy = d["symbols"][i] != "H" or d["symbols"][j] != "H"
            if dist[i, j] < 1.7 and heavy:
                ax.plot(*zip(pts[i], pts[j]), color=EDGE, lw=2, zorder=1)
    for i, (sym, q) in enumerate(zip(d["symbols"], pts)):
        ax.add_patch(Circle(q, 15 if sym != "H" else 10, fc=COLORS[sym],
                            ec=PLUG if i == 0 else INK,
                            lw=3 if i == 0 else 0.8, zorder=3))
    ax.text(cx, 335, "atom i (a carbon) and its", ha="center", fontsize=12.5,
            color=MUTED)
    ax.text(cx, 357, f"neighbours within {d['r_max_A']:.0f} Å", ha="center",
            fontsize=12.5, color=MUTED)

    box(ax, 410, 185, 130, 92, "Stages 1–3", ["and round 2"], ec=PLUG)
    arrow(ax, (275, 185), (343, 185))

    h = np.array(d["h_atom0"])
    x0, y0, c = 520, 120, 16
    lim = np.abs(h).max()
    cmap = plt.get_cmap("BrBG")
    for k, val in enumerate(h):
        ax.add_patch(Rectangle((x0 + (k % 8) * c, y0 + (k // 8) * c), c, c,
                               fc=cmap(0.5 + 0.5 * val / lim), ec="white",
                               lw=0.6))
    ax.text(x0 + 64, y0 - 18, "$\\mathbf{h}_i$: 64 numbers", ha="center",
            fontsize=13, color=INK)
    arrow(ax, (477, 185), (514, 185))

    box(ax, 760, 185, 130, 92, "$E_i$", [f"{d['E_atoms_eV'][0]:.3f} eV"])
    arrow(ax, (658, 185), (693, 185))
    ax.text(676, 152, "read", ha="center", fontsize=10.5, color=MUTED)
    ax.text(676, 168, "out", ha="center", fontsize=10.5, color=MUTED)
    box(ax, 920, 185, 120, 92, "$U$", [f"{d['U_eV']:.2f} eV"], ec=PLUG)
    arrow(ax, (827, 185), (858, 185))
    ax.text(842, 268, "add up all 9 atoms", ha="center", fontsize=12.5,
            color=MUTED)
    save(fig, "agent_mlip_atom.svg")


def fig_fit_loop():
    """One training step: predict, compare with DFT, adjust the parameters."""
    W, H = 1000, 300
    fig, ax = canvas(W, H)
    box(ax, 140, 90, 220, 96, "Batch", ["10 ethanol frames"])
    box(ax, 500, 90, 240, 96, "Predict", ["energy and forces", "using θ"],
        ec=PLUG)
    box(ax, 860, 90, 220, 96, "Compare", ["with DFT: the loss $L$"])
    box(ax, 500, 240, 240, 84, "Adjust θ", ["to lower $L$"], ec=PLUG)
    arrow(ax, (252, 90), (378, 90))
    arrow(ax, (622, 90), (748, 90))
    arrow(ax, (860, 140), (624, 240), rad=-0.12)
    arrow(ax, (500, 196), (500, 140))
    ax.text(140, 240, "1800 rounds in the test run", ha="center",
            fontsize=12.5, color=MUTED)
    save(fig, "agent_fit_loop.svg")


def fig_val_curve():
    """Validation force error against epoch, from the training log."""
    r = np.array(rows("agent_val_errors.csv"))
    fig, ax = plt.subplots(figsize=(6.4, 3.2), dpi=100)
    ax.plot(r[:, 0], r[:, 2], color=PLUG, lw=2, marker="o", ms=6)
    ax.annotate(f"{r[0, 2]:.0f}", (r[0, 0], r[0, 2]), xytext=(8, 0),
                textcoords="offset points", color=INK, fontsize=11)
    ax.annotate(f"{r[-1, 2]:.0f}", (r[-1, 0], r[-1, 2]), xytext=(0, 9),
                textcoords="offset points", color=INK, fontsize=11,
                ha="center")
    ax.set_xlabel("Epoch", color=MUTED)
    ax.set_ylabel("Force error (meV/Å)", color=MUTED)
    ax.set_ylim(0, 50 * np.ceil(r[:, 2].max() / 50 + 0.2))
    ax.grid(axis="y", color="#e8eaed", lw=0.8)
    ax.set_axisbelow(True)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(EDGE)
    ax.tick_params(colors=MUTED)
    fig.tight_layout()
    save(fig, "agent_val_curve.svg")


def fig_force_parity():
    """Predicted against DFT force components on the 200 test frames."""
    r = np.array(rows("agent_force_parity.csv"))
    fig, ax = plt.subplots(figsize=(4.2, 4.2), dpi=100)
    lim = np.abs(r).max() * 1.05
    ax.plot([-lim, lim], [-lim, lim], color=EDGE, lw=1, zorder=1)
    ax.scatter(r[:, 0], r[:, 1], s=5, color=PLUG, alpha=0.35, lw=0,
               rasterized=True, zorder=2)
    ax.set_xlim(-lim, lim)
    ax.set_ylim(-lim, lim)
    ax.set_aspect("equal")
    ax.set_xlabel("DFT force (eV/Å)", color=MUTED)
    ax.set_ylabel("MLIP force (eV/Å)", color=MUTED)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(EDGE)
    ax.tick_params(colors=MUTED)
    fig.tight_layout()
    save(fig, "agent_force_parity.svg")


def fig_energy_offset():
    """Predicted against DFT energy: a parallel line below the perfect one."""
    r = np.array(rows("agent_energy_parity.csv"))
    r = r - r[:, 0].mean()
    fig, ax = plt.subplots(figsize=(4.2, 4.2), dpi=100)
    lo, hi = r.min() - 0.08, r.max() + 0.08
    ax.plot([lo, hi], [lo, hi], color=EDGE, lw=1, zorder=1)
    ax.scatter(r[:, 0], r[:, 1], s=14, color=PLUG, alpha=0.6, lw=0, zorder=2)
    ax.set_xlim(lo, hi)
    ax.set_ylim(lo, hi)
    ax.set_aspect("equal")
    ax.set_xlabel("DFT energy (eV, from the mean)", color=MUTED)
    ax.set_ylabel("MLIP energy (eV, from the same zero)", color=MUTED)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(EDGE)
    ax.tick_params(colors=MUTED)
    fig.tight_layout()
    save(fig, "agent_energy_offset.svg")


def fig_lj_vs_mlip():
    """The energy calculation in LJ and in an MLIP, stage by stage."""
    W, H = 1000, 470
    fig, ax = canvas(W, H)
    lj, ml = 90, 310

    def atom(x, y, sym, r=13, ec=INK, lw=0.8):
        ax.add_patch(Circle((x, y), r, fc=COLORS[sym], ec=ec, lw=lw, zorder=3))

    def stage(x, y, w, h, title, lines, ec=EDGE):
        box(ax, x, y, w, h, title, lines, ec=ec)

    ax.text(40, 28, "LJ", fontsize=15, weight="bold", color=INK)
    ax.text(40, 205, "MLIP", fontsize=15, weight="bold", color=PLUG)

    # LJ: one pair in, a formula, a sum over pairs
    ax.plot([75, 175], [lj - 10, lj - 10], color=EDGE, lw=2, zorder=1)
    atom(75, lj - 10, "C")
    atom(175, lj - 10, "C")
    ax.text(125, lj - 22, "$r_{ij}$", ha="center", fontsize=14, color=INK)
    ax.text(125, lj + 34, "one pair $i,j$", ha="center", fontsize=12.5,
            color=MUTED)
    stage(490, lj, 440, 90, "Fixed formula",
          [r"$u_{ij} = 4\varepsilon\,[(\sigma/r_{ij})^{12}"
           r" - (\sigma/r_{ij})^{6}]$"])
    stage(865, lj, 190, 90, "All pairs", [r"$U = \Sigma_{i<j}\;u_{ij}$"])
    arrow(ax, (222, lj), (266, lj))
    arrow(ax, (712, lj), (767, lj))
    ax.text(490, lj + 68, r"parameters: $\varepsilon$ and $\sigma$, 2 numbers",
            ha="center", fontsize=12.5, color=MUTED)

    # MLIP: an atom's surroundings in, two fitted stages, a sum over atoms
    cx, cy = 125, ml - 10
    ax.add_patch(Circle((cx, cy), 78, fc="none", ec=PLUG, lw=1.6, ls="--"))
    for ang, dist, sym in [(20, 52, "C"), (150, 56, "O"), (255, 50, "H"),
                           (95, 46, "H")]:
        x = cx + dist * np.cos(np.radians(ang))
        y = cy - dist * np.sin(np.radians(ang))
        ax.plot([cx, x], [cy, y], color=EDGE, lw=1.6, zorder=1)
        atom(x, y, sym, 12 if sym != "H" else 9)
    atom(cx, cy, "C", 14, ec=PLUG, lw=2.6)
    ax.text(12, ml + 96, "atom $i$ and its neighbours:", ha="left",
            fontsize=12.5, color=MUTED)
    ax.text(12, ml + 118, r"positions, atomic numbers: $\mathcal{N}_i$",
            ha="left", fontsize=12.5, color=MUTED)
    stage(490, ml, 440, 90, "MLIP model (a neural network)",
          [r"$E_i = \mathrm{MLIP}_\theta(\mathcal{N}_i)$"], ec=PLUG)
    stage(865, ml, 190, 90, "All atoms", [r"$U = \Sigma_i\;E_i$"])
    arrow(ax, (222, ml), (266, ml))
    arrow(ax, (712, ml), (767, ml))
    ax.text(490, ml + 68, r"parameters: $\theta$, 67,472 numbers", ha="center", fontsize=12.5, color=MUTED)
    save(fig, "agent_lj_vs_mlip.svg")


def fig_nn_basics():
    """A small neural network: 3 numbers, 4, then 1, with the weight counts."""
    W, H = 760, 400
    fig, ax = canvas(W, H)
    cols = [(170, [118, 193, 268]), (380, [80, 155, 230, 305]), (590, [193])]
    names = [[r"$x_1$", r"$x_2$", r"$x_3$"],
             [r"$y_1$", r"$y_2$", r"$y_3$", r"$y_4$"], ["out"]]
    for (x0, ys0), (x1, ys1) in zip(cols, cols[1:]):
        for a in ys0:
            for b in ys1:
                ax.plot([x0, x1], [a, b], color=EDGE, lw=1.1, zorder=1)
    for k, (x, ys) in enumerate(cols):
        for y, nm in zip(ys, names[k]):
            ax.add_patch(Circle((x, y), 21, fc="white",
                                ec=PLUG if k else INK, lw=1.8, zorder=3))
            ax.text(x, y, nm, ha="center", va="center", fontsize=12.5,
                    color=INK, zorder=4)
    for x, top in [(170, "in"), (380, "after the first step"),
                   (590, "after the second step")]:
        ax.text(x, 22, top, ha="center", fontsize=13, color=INK)
    for x, sub in [(170, "3 numbers, $x$"), (380, "$m = 4$ numbers, $y$"),
                   (590, "1 number, out")]:
        ax.text(x, 44, sub, ha="center", fontsize=12.5, color=MUTED)
    for x, line1, line2 in [(275, "weights $W$", "4 × 3 = 12"),
                            (485, "weights $V$", "1 × 4 = 4")]:
        ax.text(x, 355, line1, ha="center", fontsize=12.5, color=INK)
        ax.text(x, 377, line2, ha="center", fontsize=12.5, color=MUTED)
    save(fig, "agent_nn_basics.svg")


def fig_phi():
    """How phi turns a straight line into one bend: the line W x + b, the same
    line through phi, and what changing b and W does to the bend."""
    def phi(u):  # SiLU, the smooth phi MACE uses
        return u / (1 + np.exp(-u))

    x = np.linspace(-2, 2, 400)
    W, b = 2.0, 0.0
    x0 = -b / W
    line = W * x + b
    fig, axes = plt.subplots(1, 3, figsize=(10.2, 3.1), dpi=100, sharey=True)

    ax = axes[0]
    ax.plot(x, line, color=EDGE, lw=2.2)
    ax.set_title(r"(a) before $\phi$: $Wx + b$", fontsize=12, color=INK)

    ax = axes[1]
    ax.plot(x, line, color=EDGE, lw=1.4, ls="--", label="before $\\phi$")
    ax.plot(x, phi(line), color=PLUG, lw=2.4, label="after $\\phi$")
    for xa in (-1.7, -1.3, -0.9):
        ax.annotate("", (xa, phi(W * xa + b)), (xa, W * xa + b),
                    arrowprops=dict(arrowstyle="->", color=PLUG, lw=1.1))
    ax.set_title(r"(b) after $\phi$: one bend", fontsize=12, color=INK)
    ax.legend(loc="upper left", fontsize=9.5, frameon=False, labelcolor=MUTED)

    ax = axes[2]
    ax.plot(x, phi(line), color=PLUG, lw=2.2, label="$W = 2,\\ b = 0$")
    ax.plot(x, phi(W * x + b - 1.5), color=PLUG, lw=2, ls="--",
            label="$b$ changed to $-1.5$")
    ax.plot(x, phi(-W * x + b), color=PLUG, lw=2, ls=":",
            label="$W$ flipped to $-2$")
    ax.set_title(r"(c) changing $b$ and $W$", fontsize=12, color=INK)
    ax.legend(loc="lower right", fontsize=9.5, frameon=False, labelcolor=MUTED)
    for ax_, xs in [(axes[0], [x0]), (axes[1], [x0]),
                    (axes[2], [x0, (1.5 - b) / W])]:
        for xb in xs:
            ax_.plot([xb], [0], marker="v", ms=8, color=LLM, zorder=4)
    for ax in axes:
        ax.set_xticks([])
        ax.set_yticks([])
        ax.axhline(0, color=EDGE, lw=0.8, zorder=0)
        ax.set_xlabel("$x$", color=MUTED)
        ax.set_ylim(-4.0, 4.6)
        for sp in ("top", "right"):
            ax.spines[sp].set_visible(False)
        for sp in ("left", "bottom"):
            ax.spines[sp].set_color(EDGE)
    fig.tight_layout()
    save(fig, "agent_phi.svg")


def fig_lj_family_vs_pes():
    """LJ can only rescale and shift one curve; the trained MLIP gives a surface
    of its own shape. Panels b and c are real outputs of the page-1 model, from
    agent_pes_slice.py: a rigid slice, and a relaxed twist scan."""
    from mpl_toolkits.mplot3d import Axes3D  # noqa: F401
    fig = plt.figure(figsize=(13.2, 3.9), dpi=100)
    ax = fig.add_subplot(1, 3, 1)
    r = np.linspace(0.85, 3.0, 400)

    def lj(r, eps, sig):
        return 4 * eps * ((sig / r) ** 12 - (sig / r) ** 6)

    for eps, sig, ls, label in [(1, 1, "-", r"$\varepsilon = 1,\ \sigma = 1$"),
                                (2, 1, "--", r"$\varepsilon = 2$: deeper"),
                                (1, 1.3, ":", r"$\sigma = 1.3$: further out")]:
        ax.plot(r, lj(r, eps, sig), color=PLUG, lw=2, ls=ls, label=label)
    ax.axhline(0, color=EDGE, lw=0.8)
    ax.set_ylim(-2.4, 1.5)
    ax.set_xlabel("distance between two atoms, $r$", color=MUTED)
    ax.set_ylabel("energy", color=MUTED)
    ax.set_title("(a) LJ: one curve shape, rescaled", fontsize=12, color=INK)
    ax.legend(loc="upper right", fontsize=9.5, frameon=False, labelcolor=MUTED)
    ax.set_xticks([])
    ax.set_yticks([])

    d = np.loadtxt(os.path.join(DATA, "agent_pes_slice.csv"), delimiter=",", skiprows=1)
    L = np.unique(d[:, 0])
    T = np.unique(d[:, 1])
    E = d[:, 2].reshape(len(L), len(T))
    TT, LL = np.meshgrid(T, L)
    ax3 = fig.add_subplot(1, 3, 2, projection="3d")
    ax3.plot_surface(TT, LL, E, cmap="GnBu_r", linewidth=0, antialiased=True)
    ax3.set_xlabel("H–O–C–C twist (°)", color=MUTED, labelpad=4)
    ax3.set_ylabel("C–O length (Å)", color=MUTED, labelpad=6)
    ax3.set_zlabel("energy (eV)", color=MUTED, labelpad=4)
    ax3.set_xticks([0, 120, 240, 360])
    ax3.view_init(elev=28, azim=-60)
    ax3.set_title("(b) model output, other atoms held", fontsize=12, color=INK)

    rx = np.loadtxt(os.path.join(DATA, "agent_twist_relaxed.csv"), delimiter=",",
                    skiprows=1)
    tw = rx[:, 1] % 360
    order = np.argsort(tw)
    tw, en = np.append(tw[order], 360.0), np.append(rx[order, 3], rx[order, 3][0])
    ax = fig.add_subplot(1, 3, 3)
    ax.plot(tw, 1000 * en, color=PLUG, lw=2.2, marker="o", ms=3)
    for t0, name in [(60, "gauche"), (180, "trans"), (300, "gauche")]:
        e0 = 1000 * en[np.argmin(np.abs(tw - t0))]
        ax.plot([t0], [e0], marker="v", ms=9, color=LLM, ls="none")
        ax.text(t0, e0 - 5, name, ha="center", va="top", fontsize=10, color=LLM)
    ax.set_xlim(0, 360)
    ax.set_ylim(-14, 10 * np.ceil(100 * en.max()) + 6)
    ax.set_xticks([0, 60, 120, 180, 240, 300, 360])
    ax.set_xlabel("H–O–C–C twist (°)", color=MUTED)
    ax.set_ylabel("energy (meV)", color=MUTED)
    ax.set_title("(c) model output, other atoms relaxed", fontsize=12, color=INK)
    for a_ in fig.axes:
        if a_ is ax3:
            continue
        for sp in ("top", "right"):
            a_.spines[sp].set_visible(False)
        for sp in ("left", "bottom"):
            a_.spines[sp].set_color(EDGE)
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.2)
    fig.text(0.66, 0.02, "b and c: the model's energy along two directions chosen "
             "to draw it, not parameters of the model", ha="center", va="bottom",
             fontsize=9.5, color=MUTED, style="italic")
    save(fig, "agent_lj_family_vs_pes.svg")


def fig_train_wrong():
    """How training can go wrong, from real training runs on made-up points
    (agent_overfit.py): too simple, about right, too flexible, and the
    training and validation error of the flexible one over the epochs."""
    pts = np.loadtxt(os.path.join(DATA, "agent_overfit_points.csv"), delimiter=",",
                     skiprows=1)
    cv = np.loadtxt(os.path.join(DATA, "agent_overfit_curves.csv"), delimiter=",",
                    skiprows=1)
    h = np.loadtxt(os.path.join(DATA, "agent_overfit_history.csv"), delimiter=",",
                   skiprows=1)
    tr, va = pts[pts[:, 2] == 0], pts[pts[:, 2] == 1]
    fig, axes = plt.subplots(1, 4, figsize=(13.0, 3.2), dpi=100)
    titles = ["(a) too simple", "(b) about right", "(c) too flexible"]
    for k, (ax, title) in enumerate(zip(axes[:3], titles)):
        ax.plot(cv[:, 0], cv[:, k + 1], color=PLUG, lw=2, label="model")
        ax.scatter(tr[:, 0], tr[:, 1], s=18, color=INK, zorder=3,
                   label="training points")
        ax.scatter(va[:, 0], va[:, 1], s=26, facecolors="white", edgecolors=LLM,
                   linewidths=1.6, zorder=3, label="validation points")
        ax.set_ylim(pts[:, 1].min() - 0.6, pts[:, 1].max() + 0.6)
        ax.set_title(title, fontsize=12, color=INK)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_xlabel("in, $x$", color=MUTED)
    axes[0].set_ylabel("out", color=MUTED)
    axes[0].legend(loc="upper center", fontsize=8.5, frameon=False, labelcolor=MUTED)
    ax = axes[3]
    ax.plot(h[1:, 0], h[1:, 1], color=INK, lw=1.8, label="training points")
    ax.plot(h[1:, 0], h[1:, 2], color=LLM, lw=1.8, label="validation points")
    i = np.argmin(h[:, 2])
    ax.axvline(h[i, 0], color=EDGE, lw=1, ls="--")
    ax.text(h[i, 0] * 1.3, 0.25, "validation error\nturns up here",
            fontsize=9, color=MUTED)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_ylim(1e-3, 0.6)
    ax.text(400, 9e-3, "training", fontsize=9.5, color=INK, ha="center", va="top")
    ax.text(4e3, 8e-2, "validation", fontsize=9.5, color=LLM, ha="center")
    ax.set_xlabel("epoch", color=MUTED)
    ax.set_ylabel("error", color=MUTED)
    ax.set_title("(d) the error of (c) as it trains", fontsize=12, color=INK)
    for a_ in axes:
        for sp in ("top", "right"):
            a_.spines[sp].set_visible(False)
        for sp in ("left", "bottom"):
            a_.spines[sp].set_color(EDGE)
    fig.tight_layout()
    save(fig, "agent_train_wrong.svg")


def fig_line_vs_bends():
    """Made-up points fitted by the 1 -> m -> 1 network of the basics:
    no phi (a straight line), m = 1 (one bend), m = 4 (four bends).
    Each y_k = phi(W_k x + b_k); out = sum_k V_k y_k + c."""
    import itertools
    x = np.linspace(-1.5, 1.5, 25)
    y = x**4 - 1.6 * x**2 + 0.25 * x
    grid = np.linspace(-1.5, 1.5, 300)

    def phi(t):  # smooth: near zero for t < 0, near t for t > 0
        return t / (1 + np.exp(-t))

    def basis(t, units):  # units: (W, b) pairs
        return np.column_stack([np.ones_like(t)] + [phi(w * t + b) for w, b in units])

    def best(m):
        # try bends at a grid of places, facing either way; keep the best fit
        places = np.linspace(-1.3, 1.3, 14)
        best_err, best_units = np.inf, None
        for ks in itertools.combinations(places, m):
            for signs in itertools.product((1, -1), repeat=m):
                units = [(6 * s_, -6 * s_ * k) for s_, k in zip(signs, ks)]
                A = basis(x, units)
                coef = np.linalg.lstsq(A, y, rcond=None)[0]
                err = np.sum((A @ coef - y) ** 2)
                if err < best_err:
                    best_err, best_units, best_coef = err, units, coef
        return (basis(grid, best_units) @ best_coef, [-b / w for w, b in best_units],
                [best_coef[i + 1] * phi(w * grid + b) for i, (w, b) in enumerate(best_units)])

    line = np.polyval(np.polyfit(x, y, 1), grid)
    one, one_k, _ = best(1)
    four, four_k, _ = best(4)

    fig, axes = plt.subplots(1, 3, figsize=(9.6, 3.1), dpi=100, sharey=True)
    titles = [r"(a) no $\phi$: a straight line", r"(b) $m = 1$: one bend",
              r"(c) $m = 4$: four bends"]
    for i, (slope, shift) in enumerate([(0.9, 0.0), (-0.6, 0.0), (0.25, 0.6),
                                        (0.25, -0.6)]):
        axes[0].plot(grid, np.polyval(np.polyfit(x, y, 1), grid) + (slope - 0.25) * grid
                     + shift, color=EDGE, lw=1.2, ls="--", zorder=1,
                     label="other $V$, $W$, $b$" if i == 0 else None)
    for ax, curve, title in zip(axes, [line, one, four], titles):
        ax.plot(grid, curve, color=PLUG, lw=2, zorder=2, label="out")
        ax.scatter(x, y, s=14, color=INK, zorder=3)
        ax.set_title(title, fontsize=12, color=INK)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_xlabel("in, $x$", color=MUTED)
        for sp in ("top", "right"):
            ax.spines[sp].set_visible(False)
        for sp in ("left", "bottom"):
            ax.spines[sp].set_color(EDGE)
    for ax, ks, curve in [(axes[1], one_k, one), (axes[2], four_k, four)]:
        for k in ks:
            ax.plot([k], [np.interp(k, grid, curve)], marker="v", ms=8, ls="none",
                    color=LLM, zorder=4, label="a bend" if k == ks[0] else None)
    for ax in axes:
        ax.set_ylim(y.min() - 0.3, y.max() + 1.1)
        ax.legend(loc="upper center", fontsize=9, frameon=False, labelcolor=MUTED,
                  ncol=1)
    axes[0].set_ylabel("out", color=MUTED)
    fig.tight_layout()
    save(fig, "agent_line_vs_bends.svg")


def fig_ethanol_frame():
    """One ethanol frame: 9 atoms, 27 numbers, one DFT energy; 21 matter."""
    d = frame()
    W, H = 1000, 470
    fig, ax = canvas(W, H)
    sym = d["symbols"]
    p = np.array(d["positions_A"])
    q = p - p.mean(0)
    rng = np.random.default_rng(0)
    best = None
    for _ in range(4000):                  # the view that spreads atoms most
        m, _ = np.linalg.qr(rng.normal(size=(3, 3)))
        t = q @ m[:, :2]
        dd = np.linalg.norm(t[:, None] - t[None], axis=-1) + np.eye(9) * 9
        if best is None or dd.min() > best[0]:
            best = (dd.min(), t)
    xy = best[1]
    s, cx, cy = 58, 165, 205
    pts = [(cx + s * x, cy + s * y) for x, y in xy]
    dist = np.linalg.norm(p[:, None] - p[None], axis=-1)
    heavy = [k for k in range(9) if sym[k] != "H"]
    for i in range(9):
        for j in range(i):
            if sym[i] != "H" and sym[j] != "H" and dist[i, j] < 1.7:
                ax.plot(*zip(pts[i], pts[j]), color=EDGE, lw=2.5, zorder=1)
        if sym[i] == "H":
            j = min(heavy, key=lambda k: dist[i, k])
            ax.plot(*zip(pts[i], pts[j]), color=EDGE, lw=2.5, zorder=1)
    for k, (sy, (x, y)) in enumerate(zip(sym, pts)):
        r = 20 if sy != "H" else 14
        ax.add_patch(Circle((x, y), r, fc=COLORS[sy], ec=INK, lw=0.8,
                            zorder=3))
        ax.text(x, y, str(k + 1), ha="center", va="center", fontsize=11,
                color="white" if sy != "H" else INK, zorder=4)
    ax.text(cx, 30, "ethanol: 9 atoms", ha="center", fontsize=13.5,
            color=INK, weight="bold")

    tx, ty, rh = 420, 70, 25
    ax.text(tx + 115, 30, "one frame: 9 × 3 = 27 numbers", ha="center",
            fontsize=13.5, color=INK, weight="bold")
    for c, lab in zip([tx, tx + 80, tx + 160, tx + 240], ["atom", "x", "y", "z"]):
        ax.text(c, ty, lab, ha="center", va="center", fontsize=12.5,
                color=MUTED, style="italic" if lab != "atom" else "normal")
    ax.plot([tx - 30, tx + 270], [ty + 13, ty + 13], color=EDGE, lw=1)
    for k in range(9):
        y = ty + rh * (k + 1)
        ax.text(tx, y, f"{k + 1} {sym[k]}", ha="center", va="center",
                fontsize=12, color=INK)
        for c, val in zip([tx + 80, tx + 160, tx + 240], p[k]):
            ax.text(c, y, f"{val:.2f}".replace("-", "−"), ha="center",
                    va="center", fontsize=12, color=INK)
    ax.text(tx + 115, ty + rh * 10 + 4, "coordinates in Å", ha="center",
            fontsize=11.5, color=MUTED)

    arrow(ax, (718, 190), (782, 190))
    ax.text(750, 172, "DFT", ha="center", fontsize=12, color=MUTED)
    box(ax, 885, 190, 190, 92, "Energy",
        [f"{d['U_dft_eV']:.3f} eV".replace("-", "−")], ec=PLUG)
    ax.text(885, 262, "one point on the", ha="center", fontsize=12,
            color=MUTED)
    ax.text(885, 282, "energy surface", ha="center", fontsize=12,
            color=MUTED)

    ax.plot([40, 960], [370, 370], color=EDGE, lw=1)
    ax.text(40, 400, "Moving the whole molecule (3 directions) or turning it "
            "(3 axes) changes all 27 numbers, but not the energy.",
            ha="left", va="center", fontsize=12.5, color=MUTED)
    ax.text(40, 430, "So the energy depends on 27 − 6 = 21 numbers.",
            ha="left", va="center", fontsize=13, color=INK, weight="bold")
    save(fig, "agent_ethanol_frame.svg")


def fig_mace_steps():
    """The stages of the trained MACE model, with its real weight counts."""
    W, H = 1060, 860
    fig, ax = canvas(W, H)
    bx, bw, bh = 175, 280, 84
    nx = 345
    steps = [
        (100, "Stage 1: Describe", ["each neighbour as numbers"], "96",
         ["distance → 8 numbers, direction → 16 numbers:",
          "    fixed formulas, no weights",
          "atomic number → a list of 32: 3 lists (H, C, O) × 32 = 96"], False),
        (240, "Stage 2: Gather", ["sum over neighbours"], "34,304",
         ["chain 8 → 64 → 64 → 64 → 128, distance in, weights out:",
          "    8×64 + 64×64 + 64×64 + 64×128 = 16,896",
          "mixing the 32 numbers: 1,024 + 4,096 + 12,288 = 17,408"], True),
        (380, "Stage 3: Combine", ["multiply, up to 3 at a time"], "3,712",
         ["28 kinds of product, each weighted separately",
          "    for the 32 numbers and 3 atomic numbers: 28×32×3 = 2,688",
          "one more mix of the 32 numbers: 32×32 = 1,024"], False),
        (530, "Round 2", ["stages 2 and 3 again"], "28,800",
         ["gather: the same chain, 16,896",
          "    plus mixing, 1,024 + 4,096 + 3,072 = 8,192",
          "combine: 3,712 as before"], True),
        (680, "Stage 4: Read out", ["64 numbers → $E_i$"], "560",
         ["round 1's 32 numbers → 1: 32 weights",
          "round 2's 32 numbers → 16 → 1, with φ:",
          "    32×16 + 16×1 = 528"], True),
    ]
    ax.text(bx, 20, r"$\mathcal{N}_i$: positions and atomic numbers",
            ha="center", va="center", fontsize=13, color=INK)
    arrow(ax, (bx, 34), (bx, 100 - bh / 2 - 2))
    for k, (y, title, lines, n, notes, chain) in enumerate(steps):
        box(ax, bx, y, bw, bh, title, lines, ec=PLUG)
        if chain:
            ax.text(bx + bw / 2 - 8, y + bh / 2 - 12, "φ(Wx)", ha="right",
                    va="center", fontsize=11.5, color=LLM, weight="bold")
        for i, line in enumerate(notes):
            ax.text(nx, y - 24 + 24 * i, line, ha="left", va="center",
                    fontsize=12, color=MUTED)
        ax.text(1040, y - 11, n, ha="right", va="center", fontsize=13.5,
                weight="bold", color=INK)
        ax.text(1040, y + 13, "weights", ha="right", va="center",
                fontsize=11.5, color=MUTED)
        if k + 1 < len(steps):
            nxt = steps[k + 1][0]
            arrow(ax, (bx, y + bh / 2 + 2), (bx, nxt - bh / 2 - 2))
    for y, label in [(455, "32 numbers"), (605, "32 more: 64 in all, $\\mathbf{h}_i$")]:
        ax.text(bx + 12, y, label, ha="left", va="center", fontsize=12,
                color=INK, style="italic")
    arrow(ax, (bx, 680 + bh / 2 + 2), (bx, 770))
    ax.text(bx, 792, "$E_i$, the atom's share of $U$", ha="center",
            va="center", fontsize=13, color=INK)
    ax.plot([nx, 1040], [762, 762], color=EDGE, lw=1)
    ax.text(1040, 792, "96 + 34,304 + 3,712 + 28,800 + 560 = 67,472",
            ha="right", va="center", fontsize=13, weight="bold", color=INK)
    ax.text(nx, 830, "φ(Wx): this stage contains a chain of basic steps",
            ha="left", va="center", fontsize=12, color=LLM)
    save(fig, "agent_mace_steps.svg")


def fig_agent_parts():
    """LLM, harness, tools: who talks to whom."""
    W, H = 1000, 380
    fig, ax = canvas(W, H)
    y = 175
    box(ax, 150, y, 220, 120, "LLM", ["qwen3:8b", "writes text only"],
        ec=LLM, title_color=LLM)
    box(ax, 500, y, 240, 120, "Harness", ["Python program", "keeps the history",
                                          "enforces the limits"], ec=INK)
    box(ax, 850, y, 220, 120, "Tools", ["train, evaluate,", "run_md, add_data"],
        ec=PLUG)
    arrow(ax, (378, y - 30), (262, y - 30))
    ax.text(320, y - 52, "all text so far", ha="center", fontsize=11.5,
            color=MUTED)
    arrow(ax, (262, y + 30), (378, y + 30))
    ax.text(320, y + 60, "a tool call", ha="center", fontsize=12, color=MUTED)
    ax.text(320, y + 80, "or stop", ha="center", fontsize=12, color=MUTED)
    arrow(ax, (622, y - 30), (738, y - 30))
    ax.text(680, y - 52, "runs the tool", ha="center", fontsize=12,
            color=MUTED)
    arrow(ax, (738, y + 30), (622, y + 30))
    ax.text(680, y + 60, "measured", ha="center", fontsize=12, color=MUTED)
    ax.text(680, y + 80, "result", ha="center", fontsize=12, color=MUTED)
    box(ax, 500, 40, 240, 50, "Goal", [], ec=EDGE)
    arrow(ax, (500, 66), (500, 112))
    ax.text(150, 330, "chooses", ha="center", fontsize=13, color=LLM,
            weight="bold")
    ax.text(500, 330, "enforces and records", ha="center", fontsize=13,
            color=INK, weight="bold")
    ax.text(850, 330, "does the work", ha="center", fontsize=13, color=PLUG,
            weight="bold")
    save(fig, "agent_parts.svg")


def fig_agent_history():
    """The harness resends the whole history on every pass."""
    W, H = 1000, 340
    fig, ax = canvas(W, H)
    items = [("Goal", EDGE), ("Tool call: train", LLM),
             ("Result: 35.5 meV/Å", PLUG), ("Tool call: add_data", LLM),
             ("Result: 400 frames", PLUG)]
    for k, x in enumerate([170, 500, 830]):
        n = 1 + 2 * k
        ax.text(x, 22, f"pass {k + 1} sends", ha="center", fontsize=13.5,
                color=INK, weight="bold")
        for i in range(n):
            label, ec = items[i]
            box(ax, x, 62 + 44 * i, 250, 36, "", [], ec=ec)
            ax.text(x, 62 + 44 * i, label, ha="center", va="center",
                    fontsize=12.5, color=INK)
        ax.text(x, 62 + 44 * n + 6, "→ the LLM writes the next", ha="center",
                fontsize=12, color=MUTED)
        ax.text(x, 62 + 44 * n + 26, "tool call", ha="center", fontsize=12,
                color=MUTED)
    save(fig, "agent_history.svg")


def fig_run_timeline():
    """The Qwen3 run, step by step, from its log."""
    with open(os.path.join(DATA, "agent_qwen3_log.jsonl")) as f:
        log = [json.loads(line) for line in f]
    tools_ = [e for e in log if e["kind"] == "tool"]
    end = [e for e in log if e["kind"] == "end"][0]

    def said(r, name):
        if "refused_by_harness" in r:
            return ("refused by check, not run", r["refused_by_harness"])
        if name == "train":
            return (f"validation force error {r['validation_force_mae_meV_per_A']:.1f} meV/Å",
                    f"{r['training_frames']} frames, {r['minutes']} min")
        if name == "add_data":
            return (f"{r['training_frames_next_time']} frames next time", "")
        if name == "evaluate":
            return (f"selection force error {r['force_mae_meV_per_A']} meV/Å",
                    f"energy error {r['energy_mae_meV_per_atom']} meV/atom")
        if name == "run_md":
            return (f"energy range {r['energy_range_after_t0_eV']} eV, "
                    f"closest pair {r['closest_pair_A']:.2f} Å",
                    "bonds the same as at the start" if r["bonds_same_as_start"]
                    else "bonds changed")

    def chose(e):
        a = e["args"]
        if e["name"] == "train":
            return f"train: {a['channels']} channels, {a['epochs']} epochs"
        if e["name"] == "add_data":
            return f"add_data: {a['frames']} frames"
        return e["name"]

    rows = [(e["t_min"], chose(e), *said(e["result"], e["name"])) for e in tools_]
    rows.append((end["t_min"], "stop", "its reason: \"Target met\"", ""))
    W, H = 1000, 90 + 70 * len(rows)
    fig, ax = canvas(W, H)
    for x, lab, c in [(70, "done at, min", MUTED), (330, "the LLM chose", LLM),
                      (720, "the tool measured", PLUG)]:
        ax.text(x, 30, lab, ha="center", fontsize=13, color=c, weight="bold")
    for k, (t, c, r1, r2) in enumerate(rows):
        y = 85 + 70 * k
        ax.text(70, y, f"{t:.1f}", ha="center", va="center", fontsize=13,
                color=INK)
        box(ax, 330, y, 330, 50, "", [], ec=LLM)
        ax.text(330, y, c, ha="center", va="center", fontsize=12.5, color=INK)
        if r1:
            box(ax, 720, y, 400, 56, "", [], ec=PLUG if k < len(rows) - 1 else LLM)
            ax.text(720, y - (10 if r2 else 0), r1, ha="center", va="center",
                    fontsize=12.5, color=INK)
            if r2:
                ax.text(720, y + 12, r2, ha="center", va="center",
                        fontsize=11.5, color=MUTED)
            arrow(ax, (497, y), (518, y))
        if k + 1 < len(rows):
            arrow(ax, (330, y + 26), (330, y + 44))
    save(fig, "agent_run_timeline.svg")


def fig_pipeline():
    """The loop the agent runs."""
    W, H = 1000, 330
    fig, ax = canvas(W, H)
    top, low = 70, 260
    box(ax, 140, top, 230, 90, "Public data", ["ethanol, DFT labels"])
    box(ax, 500, top, 230, 90, "Train MACE", ["fit θ"], ec=PLUG)
    box(ax, 860, top, 230, 90, "Held-out check", ["errors on unseen frames"],
        ec=PLUG)
    box(ax, 860, low, 230, 90, "OpenMM check", ["stable, energy conserved"],
        ec=PLUG)
    box(ax, 500, low, 230, 90, "LLM decides", ["next step"], ec=LLM,
        title_color=LLM)
    box(ax, 140, low, 230, 90, "Stop", ["saved log"])
    arrow(ax, (257, top), (383, top))
    arrow(ax, (617, top), (743, top))
    arrow(ax, (860, 117), (860, 213))
    arrow(ax, (743, low), (617, low))
    arrow(ax, (383, low), (257, low))
    arrow(ax, (420, 213), (170, 117))
    ax.text(420, 150, "more data or a changed setting", ha="left",
            fontsize=12.5, color=MUTED, style="italic")
    save(fig, "agent_pipeline.svg")


def fig_llm_io():
    """The big picture of an LLM: text in, text out, and its three limits."""
    W, H = 1000, 270
    fig, ax = canvas(W, H)
    y = 120
    box(ax, 170, y, 300, 150, "Text sent to it",
        ["\u201cThe validation force error", "is 35.5 meV/\u00c5. What",
         "should I change?\u201d"], ec=EDGE)
    box(ax, 500, y, 200, 150, "LLM", ["learned from a", "very large amount", "of text"],
        ec=LLM, title_color=LLM)
    box(ax, 830, y, 300, 150, "Text back",
        ["\u201cAdd more training frames,", "and train a wider", "model.\u201d"],
        ec=EDGE)
    arrow(ax, (322, y), (398, y))
    arrow(ax, (602, y), (678, y))
    for x, label in [(170, "all it sees: it keeps"), (500, "writes the reply that"),
                     (830, "only words: nothing")]:
        ax.text(x, 228, label, ha="center", fontsize=13, color=INK, weight="bold")
    for x, label in [(170, "nothing between questions"), (500, "fits best, unchecked"),
                     (830, "is trained or run")]:
        ax.text(x, 252, label, ha="center", fontsize=13, color=INK, weight="bold")
    save(fig, "agent_llm_io.svg")


def fig_llm_nn():
    """The LLM drawn like the small network: text in, a score per token out.
    Sizes are Qwen3 8B's (its config.json); the numbers in the lists and the
    scores are made up to show the idea."""
    W, H = 1000, 480
    fig, ax = canvas(W, H)
    toks = ["validation", "error", "is", "high", "\u22ee", "change", "?"]
    ids = ["10519", "1465", "374", "1550", None, "2297", "30"]  # Qwen3 tokenizer
    ys = [120, 165, 210, 255, 300, 345, 390]
    vecs = ["0.12, \u22120.40, \u2026", "\u22120.81, 0.05, \u2026", "0.33, 0.27, \u2026",
            "0.70, \u22120.22, \u2026", None,
            "\u22120.09, 0.66, \u2026", "0.51, \u22120.18, \u2026"]
    # text pieces, then the list of numbers each becomes
    for y, t, v, n in zip(ys, toks, vecs, ids):
        if v is None:
            ax.text(75, y, t, ha="center", va="center", fontsize=16, color=MUTED)
            continue
        w = 10 * len(t) + 16
        ax.add_patch(Rectangle((75 - w / 2, y - 14), w, 28, fc="#f1f3f4",
                               ec=EDGE, lw=1.4))
        ax.text(75, y, t, ha="center", va="center", fontsize=12.5, color=INK)
        arrow(ax, (75 + w / 2 + 4, y), (190, y))
        ax.text(186, y - 13, "#" + n, ha="right", va="center", fontsize=10.5,
                color=MUTED)
        ax.text(197, y, "[" + v + "]", ha="left", va="center", fontsize=11,
                color=MUTED, family="DejaVu Sans Mono")
    # the network: columns of circles joined by lines, many steps in between
    cols = [390, 480, 640, 730]
    cy = [150, 210, 270, 330, 390]
    for x0, x1 in [(cols[0], cols[1]), (cols[2], cols[3])]:
        for a in cy:
            for b in cy:
                ax.plot([x0, x1], [a, b], color=EDGE, lw=0.8, zorder=1)
    for x in cols:
        for y in cy:
            ax.add_patch(Circle((x, y), 13, fc="white", ec=LLM, lw=1.6, zorder=3))
    ax.text(560, 270, "\u2026", ha="center", va="center", fontsize=28, color=LLM)
    ax.add_patch(Rectangle((360, 115), 400, 310, fc="none", ec=LLM, lw=1.2,
                           ls=(0, (4, 3))))
    # scores for the next piece
    arrow(ax, (765, 270), (800, 270))
    for i, (t, sc) in enumerate([("Add", 0.62), ("Train", 0.21), ("Use", 0.09),
                                 ("the", 0.03)]):
        y = 190 + 40 * i
        ax.text(812, y, t, ha="left", va="center", fontsize=12.5,
                color=LLM if i == 0 else INK, weight="bold" if i == 0 else None)
        ax.add_patch(Rectangle((870, y - 8), 110 * sc, 16,
                               fc=LLM if i == 0 else EDGE, ec="none"))
    ax.text(812, 350, "\u22ee", ha="left", va="center", fontsize=18, color=MUTED)
    # headings, as in the small network
    for x, top, sub in [(75, "in", "text, in pieces"),
                        (255, "its row", "4,096 numbers"),
                        (560, "36 steps", "8.2 billion weights"),
                        (890, "out", "151,936 scores")]:
        ax.text(x, 40, top, ha="center", fontsize=13, color=INK, weight="bold")
        ax.text(x, 64, sub, ha="center", fontsize=12.5, color=MUTED)
    ax.text(500, 462, "Small network of the last page: 3 numbers in, 16 weights, "
            "1 number out.", ha="center", fontsize=12, color=MUTED, style="italic")
    save(fig, "agent_llm_nn.svg")


def fig_llm_lookup():
    """How one token becomes numbers: its place in the list picks a row of the
    table. Place numbers are from Qwen3's tokenizer; row values are made up."""
    W, H = 1000, 340
    fig, ax = canvas(W, H)
    # 1. the token
    ax.text(90, 40, "1. token", ha="center", fontsize=13, color=INK, weight="bold")
    ax.add_patch(Rectangle((25, 213), 130, 34, fc="#f1f3f4", ec=EDGE, lw=1.4))
    ax.text(90, 230, "\u2423validation", ha="center", va="center", fontsize=13,
            color=INK)
    arrow(ax, (160, 230), (215, 230))
    # 2. its place in the list
    ax.text(317, 40, "2. place in the list", ha="center", fontsize=13, color=INK,
            weight="bold")
    rows = [("0", "!"), ("\u22ee", ""), ("1465", "\u2423error"), ("\u22ee", ""),
            ("10518", "opts"), ("10519", "\u2423validation"),
            ("10520", "\u2423concerns"), ("\u22ee", "")]
    for i, (n, t) in enumerate(rows):
        y = 80 + 30 * i
        hit = n == "10519"
        if n == "\u22ee":
            ax.text(317, y, "\u22ee", ha="center", va="center", fontsize=15,
                    color=MUTED)
            continue
        ax.add_patch(Rectangle((220, y - 13), 195, 26, fc="white",
                               ec=LLM if hit else EDGE, lw=1.6 if hit else 1))
        ax.text(228, y, n, ha="left", va="center", fontsize=12,
                color=LLM if hit else MUTED, weight="bold" if hit else None)
        ax.text(290, y, t, ha="left", va="center", fontsize=12,
                color=LLM if hit else INK)
    arrow(ax, (420, 230), (458, 212))
    # 3. the table: one row of 4,096 numbers per place
    ax.text(640, 40, "3. row 10,519 of the table", ha="center", fontsize=13,
            color=INK, weight="bold")
    for i, (n, vals) in enumerate([("1465", "\u22120.81   0.05   0.47  \u2026"),
                                   ("\u22ee", None),
                                   ("10519", "0.12  \u22120.40   0.93  \u2026"),
                                   ("\u22ee", None)]):
        y = 130 + 40 * i
        if vals is None:
            ax.text(640, y, "\u22ee", ha="center", va="center", fontsize=15,
                    color=MUTED)
            continue
        hit = n == "10519"
        ax.add_patch(Rectangle((465, y - 15), 350, 30, fc="white",
                               ec=LLM if hit else EDGE, lw=1.6 if hit else 1))
        ax.text(475, y, n, ha="left", va="center", fontsize=12,
                color=LLM if hit else MUTED, weight="bold" if hit else None)
        ax.text(545, y, vals, ha="left", va="center", fontsize=12,
                color=LLM if hit else INK, family="DejaVu Sans Mono")
    ax.text(640, 75, "151,936 rows \u00d7 4,096 numbers, all weights",
            ha="center", fontsize=12, color=MUTED)
    arrow(ax, (820, 210), (870, 210))
    ax.text(930, 40, "4. into the", ha="center", fontsize=13, color=INK,
            weight="bold")
    ax.text(930, 62, "network", ha="center", fontsize=13, color=INK,
            weight="bold")
    ax.text(935, 210, "4,096\nnumbers", ha="center", va="center", fontsize=12.5,
            color=LLM)
    ax.text(500, 325, "The place numbers are Qwen3's; the numbers in the rows are "
            "made up.", ha="center", fontsize=12, color=MUTED, style="italic")
    save(fig, "agent_llm_lookup.svg")


def fig_llm_steps():
    """Inside the LLM: the reply is written one token at a time."""
    W, H = 1000, 330
    fig, ax = canvas(W, H)
    # Inside the LLM: the reply grows one token at a time.
    ax.add_patch(Rectangle((60, 20), 880, 295, fc="white", ec=LLM, lw=1.2,
                           ls=(0, (4, 3))))
    ax.text(500, 48, "Inside the LLM: the reply is written one piece (token) "
            "at a time", ha="center", fontsize=13, color=LLM, weight="bold")
    ax.text(500, 72, "each step reads the question (grey) and the reply so far, "
            "and adds the piece that fits best (purple)", ha="center",
            fontsize=12, color=MUTED)
    reply = ["Add", "more", "training", "frames", "."]
    why = ["answers to \u201cwhat should I change?\u201d often start with \u201cAdd\u201d",
           "\u201cAdd\u201d is often followed by \u201cmore\u201d",
           "the question is about a validation error, so \u201cmore training\u201d fits",
           "\u201cmore training\u201d is often followed by \u201cdata\u201d or \u201cframes\u201d"]
    for k in range(4):
        yy = 108 + 56 * k
        ax.text(130, yy, f"step {k + 1}", ha="left", va="center", fontsize=12,
                color=MUTED)
        ax.add_patch(Rectangle((230, yy - 12), 250, 24, fc="#f1f3f4",
                               ec=EDGE, lw=1.4))
        ax.text(355, yy, "The validation \u2026 change?", ha="center",
                va="center", fontsize=12, color=MUTED, style="italic")
        x = 490
        for i, tok in enumerate(reply[:k + 1]):
            new = i == k
            w = 11 * len(tok) + 16
            ax.add_patch(Rectangle((x, yy - 12), w, 24, fc="white",
                                   ec=LLM if new else EDGE, lw=1.4))
            ax.text(x + w / 2, yy, tok, ha="center", va="center", fontsize=12,
                    color=LLM if new else INK)
            x += w + 5
        ax.text(230, yy + 23, "why: " + why[k], ha="left", va="center",
                fontsize=11.5, color=MUTED, style="italic")
    save(fig, "agent_llm_steps.svg")


if __name__ == "__main__":
    fig_lj_vs_mlip()
    fig_nn_basics()
    fig_phi()
    fig_lj_family_vs_pes()
    fig_line_vs_bends()
    fig_train_wrong()
    fig_ethanol_frame()
    fig_mace_steps()
    fig_mlip_atom()
    fig_fit_loop()
    fig_val_curve()
    fig_force_parity()
    fig_energy_offset()
    fig_llm_lookup()
    fig_llm_nn()
    fig_llm_io()
    fig_llm_steps()
    fig_pipeline()
    fig_agent_parts()
    fig_agent_history()
    fig_run_timeline()
