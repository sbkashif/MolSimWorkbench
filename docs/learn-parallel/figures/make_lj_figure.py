#!/usr/bin/env python3
"""Generate the Lennard-Jones figure used in docs/learn-parallel/00_starting_point.md.

    python3 docs/learn-parallel/figures/make_lj_figure.py
"""
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Reduced units: epsilon = sigma = 1, as this repo uses.
r = np.linspace(0.90, 3.0, 2000)
rep = 4.0 * r**-12.0            # the r^-12 repulsive term alone
att = -4.0 * r**-6.0            # the r^-6 attractive term alone
U = rep + att
F = (24.0 / r) * (2.0 * r**-12.0 - r**-6.0)

R_MIN = 2.0 ** (1.0 / 6.0)      # 1.12246... : bottom of the well
CUTOFF = 2.5

INK, GRID = "#1a1a1a", "#d8d8d8"
C_TOTAL, C_REP, C_ATT = "#1f4e79", "#c0392b", "#1e8449"
C_MARK = "#8e44ad"

fig, (ax1, ax2) = plt.subplots(
    2, 1, figsize=(7.4, 8.2), sharex=True,
    gridspec_kw={"height_ratios": [1.5, 1.0], "hspace": 0.10},
)
fig.patch.set_facecolor("white")

# ---------------------------------------------------------------- energy panel
ax1.set_facecolor("white")
ax1.plot(r, rep, "--", color=C_REP, lw=1.3, alpha=0.75,
         label=r"repulsion  $+4\varepsilon(\sigma/r)^{12}$")
ax1.plot(r, att, "--", color=C_ATT, lw=1.3, alpha=0.75,
         label=r"attraction  $-4\varepsilon(\sigma/r)^{6}$")
ax1.plot(r, U, color=C_TOTAL, lw=2.6, label=r"total  $U(r)$", zorder=5)
ax1.axhline(0, color=INK, lw=0.9)

# sigma: where U crosses zero
ax1.plot([1.0], [0.0], "o", color=C_MARK, ms=8, zorder=7)
ax1.annotate(r"$\sigma$ — where $U=0$" "\n" "atoms are 'just touching'",
             xy=(1.0, 0.0), xytext=(1.34, 1.35), fontsize=10.5, color=C_MARK,
             arrowprops=dict(arrowstyle="->", color=C_MARK, lw=1.3))

# epsilon: depth of the well
ax1.plot([R_MIN], [-1.0], "o", color=C_MARK, ms=8, zorder=7)
ax1.annotate("", xy=(R_MIN, 0.0), xytext=(R_MIN, -1.0),
             arrowprops=dict(arrowstyle="<->", color=C_MARK, lw=1.5))
ax1.text(R_MIN + 0.07, -0.40, r"depth $=\varepsilon$", fontsize=10.5, color=C_MARK,
         zorder=12, bbox=dict(facecolor="white", edgecolor="none", alpha=0.94, pad=1.8))
ax1.annotate(r"$r_{\min}=2^{1/6}\sigma \approx 1.122\,\sigma$" "\n" "the distance atoms prefer",
             xy=(R_MIN, -1.0), xytext=(1.44, -1.78), fontsize=10.5, color=C_MARK,
             arrowprops=dict(arrowstyle="->", color=C_MARK, lw=1.3),
             zorder=12, bbox=dict(facecolor="white", edgecolor="none", alpha=0.94, pad=1.8))

# cutoff
for ax in (ax1, ax2):
    ax.axvline(CUTOFF, color=INK, ls=":", lw=1.4, alpha=0.8)
ax2.text(CUTOFF - 0.06, 8.4, "cutoff\n2.5$\\sigma$", fontsize=10.5,
         ha="right", va="top", color=INK, fontweight="bold")
ax1.annotate(r"$U=-0.016\,\varepsilon$ here" "\n" "(1.6% of the well depth)",
             xy=(CUTOFF, -0.0163), xytext=(1.99, -0.86), fontsize=9.5, color=INK,
             arrowprops=dict(arrowstyle="->", color=INK, lw=1.0),
             zorder=12, bbox=dict(facecolor="white", edgecolor="none", alpha=0.94, pad=1.8))

ax1.set_ylim(-2.1, 2.1)
ax1.set_ylabel(r"energy  $U(r)\ /\ \varepsilon$", fontsize=12)
ax1.legend(loc="upper right", fontsize=10, framealpha=0.95)
ax1.grid(True, color=GRID, lw=0.6)
ax1.set_title("The Lennard-Jones pair potential (reduced units, "
              r"$\varepsilon=\sigma=1$)", fontsize=13, pad=12)

# ----------------------------------------------------------------- force panel
ax2.set_facecolor("white")
ax2.plot(r, F, color=C_TOTAL, lw=2.6)
ax2.axhline(0, color=INK, lw=0.9)
ax2.fill_between(r, F, 0, where=(F > 0), color=C_REP, alpha=0.16)
ax2.fill_between(r, F, 0, where=(F < 0), color=C_ATT, alpha=0.16)

ax2.plot([R_MIN], [0.0], "o", color=C_MARK, ms=8, zorder=7)
ax2.annotate(r"$F=0$ at $r_{\min}$ — nothing pushes",
             xy=(R_MIN, 0.0), xytext=(1.38, 3.4), fontsize=10.5, color=C_MARK,
             arrowprops=dict(arrowstyle="->", color=C_MARK, lw=1.3))
ax2.text(0.955, 7.4, "F > 0\nPUSH APART\n(spring compressed)",
         fontsize=10, color=C_REP, va="top", fontweight="bold")
ax2.text(1.72, -3.9, "F < 0   PULL TOGETHER   (spring stretched)",
         fontsize=10, color=C_ATT, fontweight="bold")
ax2.annotate("at $r=1.2\\sigma$:  $F=-2.21$",
             xy=(1.2, -2.2117), xytext=(1.34, -1.20), fontsize=9.5, color=INK,
             arrowprops=dict(arrowstyle="->", color=INK, lw=1.0),
             zorder=12, bbox=dict(facecolor="white", edgecolor="none", alpha=0.94, pad=1.8))

ax2.set_ylim(-5.5, 10.0)
ax2.set_xlim(0.90, 3.0)
ax2.set_xlabel(r"separation  $r\ /\ \sigma$", fontsize=12)
ax2.set_ylabel(r"force  $F(r)\ /\ (\varepsilon/\sigma)$", fontsize=12)
ax2.grid(True, color=GRID, lw=0.6)

for ax in (ax1, ax2):
    for spine in ax.spines.values():
        spine.set_color("#999999")

out = __file__.replace("make_lj_figure.py", "lj_potential.svg")
fig.savefig(out, format="svg", bbox_inches="tight", facecolor="white")
print("wrote", out)

import os
if os.environ.get("LJ_FIG_PNG"):
    png = os.environ["LJ_FIG_PNG"]
    fig.savefig(png, format="png", dpi=110, bbox_inches="tight", facecolor="white")
    print("wrote", png)
