"""Figures for the parallel module. One function per figure.

    python parallel_figures.py          # writes every figure into ../

Measured data lives in data/par_*.csv (see data/par_README.txt), so the
figures rebuild without re-running the benchmarks.
"""
import csv
import os

from molsim_figures import (FancyArrowPatch, FancyBboxPatch, INK, MUTED, EDGE,
                            MONO, PLUG, canvas, plt, save)

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
PAR = "#8a4fbf"           # Parallel module colour, as on the cover
THREAD_A = "#2f6db5"
THREAD_B = "#d9822b"
FAIL = "#c0392b"


def rows(name):
    with open(os.path.join(DATA, name)) as f:
        return list(csv.DictReader(f))


def fig_time_vs_n():
    """Whole step (before and after the one-pass fix) and the rest of a step
    against N, log-log, with N and N^2 guide lines."""
    import numpy as np
    d = rows("par_step_split.csv")
    before = rows("par_step_split_before.csv")
    N = np.array([float(r["N"]) for r in d])
    step = np.array([float(r["step_ms"]) for r in d])
    old = np.array([float(r["step_ms"]) for r in before])
    rest = np.array([float(r["rest_ms"]) for r in d])
    fig, ax = plt.subplots(figsize=(7.2, 4.4), dpi=100)
    ax.loglog(N, old, "s--", color=PAR, lw=1.4, alpha=0.55,
              label="whole step, energy as a second pair loop")
    ax.loglog(N, step, "o-", color=PAR, lw=2.2,
              label="whole step, energy from the force pass")
    ax.loglog(N, rest, "o-", color=PLUG, lw=2.2,
              label="everything except the pair loop")
    # Guide lines through the last point: slope 2 (all pairs) and slope 1.
    ax.loglog(N, step[-1] * (N / N[-1]) ** 2, ":", color=MUTED, lw=1.2)
    ax.text(N[1], step[-1] * (N[1] / N[-1]) ** 2 * 0.55, "∝ N²",
            color=MUTED, fontsize=11)
    ax.loglog(N, rest[-1] * (N / N[-1]), ":", color=MUTED, lw=1.2)
    ax.text(N[1], rest[-1] * (N[1] / N[-1]) * 1.6, "∝ N", color=MUTED,
            fontsize=11)
    ax.set_xlabel("number of atoms N", fontsize=12)
    ax.set_ylabel("time per step (ms)", fontsize=12)
    ax.legend(frameon=False, fontsize=10.5, loc="upper left")
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    fig.tight_layout()
    save(fig, "par_time_vs_n.svg")


def fig_cores_threads():
    """One program, several threads, each running on a core; one shared memory."""
    W, H = 900, 340
    fig, ax = canvas(W, H)
    ax.add_patch(FancyBboxPatch((30, 40), 520, 190,
                                boxstyle="round,pad=0,rounding_size=12",
                                fc="white", ec=EDGE, lw=1.8))
    ax.text(290, 62, "Processor: 10 cores", ha="center", va="center",
            fontsize=14, weight="bold", color=INK)
    for k in range(10):
        x = 50 + (k % 5) * 98
        y = 155 - (k // 5) * 70          # first row drawn is the bottom one
        perf = k < 4
        ax.add_patch(FancyBboxPatch((x, y), 86, 56,
                                    boxstyle="round,pad=0,rounding_size=8",
                                    fc="#f3ecfa" if perf else "#f6f8fb",
                                    ec=PAR if perf else EDGE, lw=1.4))
        ax.text(x + 43, y + 18, "core", ha="center", va="center", fontsize=11,
                color=INK)
        ax.text(x + 43, y + 38, "fast" if perf else "efficient", ha="center",
                va="center", fontsize=10, color=PAR if perf else MUTED)
    # Threads of one program, each placed on a core by the operating system.
    for k, col in enumerate([THREAD_A, THREAD_B, "#1a9e9a", "#7f8c8d"]):
        x = 50 + k * 98 + 43
        ax.plot([x, x], [255, 213], color=col, lw=2)
        ax.text(x, 268, f"thread {k}", ha="center", va="center", fontsize=11,
                color=col, weight="bold")
    ax.text(290, 300, "one program, four threads; the operating system\n"
            "runs each thread on a core", ha="center", va="center",
            fontsize=11.5, color=MUTED, linespacing=1.3)
    ax.add_patch(FancyBboxPatch((620, 40), 250, 190,
                                boxstyle="round,pad=0,rounding_size=12",
                                fc="white", ec=EDGE, lw=1.8))
    ax.text(745, 62, "Memory: shared", ha="center", va="center", fontsize=14,
            weight="bold", color=INK)
    for k, name in enumerate(["x[ ], y[ ], z[ ]", "fx[ ], fy[ ], fz[ ]"]):
        ax.add_patch(FancyBboxPatch((645, 95 + k * 60), 200, 40,
                                    boxstyle="round,pad=0,rounding_size=6",
                                    fc="#f1f3f4", ec="none"))
        ax.text(745, 115 + k * 60, name, ha="center", va="center",
                fontsize=12, family=MONO, color=INK)
    ax.add_patch(FancyArrowPatch((552, 135), (618, 135), arrowstyle="<|-|>",
                                 mutation_scale=16, color=EDGE, lw=1.8))
    ax.text(745, 300, "every thread can read\nand write all of it",
            ha="center", va="center", fontsize=11.5, color=MUTED,
            linespacing=1.3)
    save(fig, "par_cores_threads.svg")


def fig_scatter_gather():
    """Which force slots each thread writes: scatter (i<j) vs gather."""
    W, H = 960, 310
    fig, ax = canvas(W, H)
    n = 8
    for panel, (x0, title) in enumerate([(30, "scatter: pair (i, j) writes f[i] AND f[j]"),
                                         (500, "gather: atom i writes only f[i]")]):
        ax.text(x0, 30, title, va="center", fontsize=13, weight="bold",
                color=INK)
        for k in range(n):
            x = x0 + 10 + k * 52
            col = THREAD_A if k < n // 2 else THREAD_B
            ax.add_patch(FancyBboxPatch((x, 150), 44, 44,
                                        boxstyle="round,pad=0,rounding_size=6",
                                        fc="white", ec=col, lw=1.6))
            ax.text(x + 22, 172, f"f[{k}]", ha="center", va="center",
                    fontsize=11, family=MONO, color=INK)
        ax.text(x0 + 10 + 1.5 * 52 + 22, 120, "thread A owns atoms 0-3",
                ha="center", fontsize=11, color=THREAD_A)
        ax.text(x0 + 10 + 5.5 * 52 + 22, 120, "thread B owns atoms 4-7",
                ha="center", fontsize=11, color=THREAD_B)
    # Scatter: thread A's pair (1, 6) writes f[6], which thread B also writes.
    xa = 30 + 10 + 1 * 52 + 22
    xb = 30 + 10 + 6 * 52 + 22
    ax.add_patch(FancyArrowPatch((xa, 200), (xb, 200), arrowstyle="-|>",
                                 mutation_scale=16, color=THREAD_A, lw=2,
                                 connectionstyle="arc3,rad=0.35"))
    ax.add_patch(FancyArrowPatch((30 + 10 + 4 * 52 + 22, 200), (xb - 6, 200),
                                 arrowstyle="-|>", mutation_scale=16,
                                 color=THREAD_B, lw=2,
                                 connectionstyle="arc3,rad=0.45"))
    ax.text(xb, 288, "f[6]: written by A and by B", ha="center",
            fontsize=11.5, color=FAIL, weight="bold")
    # Gather: each thread writes only its own slots.
    for k in range(n):
        x = 500 + 10 + k * 52 + 22
        col = THREAD_A if k < n // 2 else THREAD_B
        ax.add_patch(FancyArrowPatch((x, 240), (x, 198), arrowstyle="-|>",
                                     mutation_scale=12, color=col, lw=1.5))
    ax.text(500 + 10 + 3.5 * 52 + 22, 288, "every slot has exactly one writer",
            ha="center", fontsize=11.5, color=PLUG, weight="bold")
    save(fig, "par_scatter_gather.svg")


def fig_race():
    """Two threads add to f[6] at the same time; one addition is lost."""
    W, H = 900, 330
    fig, ax = canvas(W, H)
    steps = [
        (THREAD_A, "A reads f[6]", "2.0"),
        (THREAD_B, "B reads f[6]", "2.0"),
        (THREAD_A, "A adds 0.5, writes", "2.5"),
        (THREAD_B, "B adds 1.0, writes", "3.0"),
    ]
    ax.text(30, 30, "time →", fontsize=12, color=MUTED)
    for k, (col, what, val) in enumerate(steps):
        x = 40 + k * 205
        row = 80 if col == THREAD_A else 150
        ax.add_patch(FancyBboxPatch((x, row), 185, 50,
                                    boxstyle="round,pad=0,rounding_size=8",
                                    fc="white", ec=col, lw=1.8))
        ax.text(x + 92, row + 25, what, ha="center", va="center", fontsize=12,
                color=col, weight="bold")
        ax.add_patch(FancyBboxPatch((x + 50, 235), 85, 40,
                                    boxstyle="round,pad=0,rounding_size=6",
                                    fc="#f1f3f4", ec="none"))
        ax.text(x + 92, 255, val, ha="center", va="center", fontsize=13,
                family=MONO, color=INK, weight="bold")
    ax.text(20, 255, "f[6]", ha="left", va="center", fontsize=12,
            family=MONO, color=INK)
    ax.text(W / 2, 305, "should be 2.0 + 0.5 + 1.0 = 3.5; A's 0.5 is lost",
            ha="center", va="center", fontsize=12.5, color=FAIL, weight="bold")
    save(fig, "par_race.svg")


def _speedups(d, N, backend):
    base = next(float(r["ms_best_of_2_runs"]) for r in d
                if r["N"] == N and r["backend"] == "serial-triangular")
    return [(int(r["threads"]), base / float(r["ms_best_of_2_runs"]))
            for r in d if r["N"] == N and r["backend"] == backend]


def _speedup_axes(ax, ymax):
    ax.axhline(1, color=MUTED, lw=1, ls="--")
    ax.text(10.2, 1.04, "serial speed", color=MUTED, fontsize=10.5,
            va="bottom", ha="right")
    ax.axvspan(4.5, 10.5, color="#f6f8fb", zorder=0)
    ax.text(7.5, ymax * 0.95, "threads 5-10 also use efficiency cores",
            ha="center", fontsize=10.5, color=MUTED)
    ax.set_xlim(0.5, 10.5)
    ax.set_ylim(0, ymax)
    ax.set_xticks([1, 2, 4, 6, 8, 10])
    ax.set_xlabel("threads", fontsize=12)
    ax.set_ylabel("speedup over serial-triangular", fontsize=12)
    ax.legend(frameon=False, fontsize=10.5, loc="upper left")
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def fig_backends():
    """The three parallel backends at N = 4096, against the best serial."""
    d = rows("par_thread_sweep.csv")
    fig, ax = plt.subplots(figsize=(7.2, 4.4), dpi=100)
    for backend, col, style in [("openmp-private", PAR, "o-"),
                                ("openmp-gather", THREAD_A, "s-"),
                                ("std::thread-gather", THREAD_B, "^-")]:
        ax.plot(*zip(*_speedups(d, "4096", backend)), style, color=col, lw=2,
                label=backend)
    _speedup_axes(ax, 5.2)
    fig.tight_layout()
    save(fig, "par_backends.svg")


def fig_speedup():
    """Speedup of the private-buffer OpenMP kernel over the fastest serial
    kernel, for each system size."""
    d = rows("par_thread_sweep.csv")
    fig, ax = plt.subplots(figsize=(7.2, 4.4), dpi=100)
    colors = {"125": "#b0b4ba", "512": "#9b7cc4", "1000": "#7a52b3",
              "2197": "#5b2f9e", "4096": "#3d1680"}
    for N, col in colors.items():
        ax.plot(*zip(*_speedups(d, N, "openmp-private")), "o-", color=col,
                lw=2, label=f"N = {N}")
    _speedup_axes(ax, 5.2)
    fig.tight_layout()
    save(fig, "par_speedup.svg")


if __name__ == "__main__":
    fig_time_vs_n()
    fig_cores_threads()
    fig_scatter_gather()
    fig_race()
    fig_backends()
    fig_speedup()
