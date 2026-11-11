"""Figures for the pybind11 module. One function per figure.

    python pybind_figures.py          # writes every figure into ../
"""
from molsim_figures import (FancyArrowPatch, FancyBboxPatch, INK, MUTED, EDGE,
                            MONO, canvas, save)
from cpp_figures import CPP

PYB = "#d9822b"           # pybind11 module colour, as on the cover
PY = "#4a5260"            # Python (dark grey; blue is C++)


def box(ax, x, y, w, h, title, lines=(), ec=EDGE, title_color=INK, mono=True):
    ax.add_patch(FancyBboxPatch((x, y), w, h,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc="white", ec=ec, lw=1.8))
    ax.text(x + w / 2, y + 24, title, ha="center", va="center", fontsize=13.5,
            weight="bold", color=title_color)
    for k, line in enumerate(lines):
        ax.text(x + w / 2, y + 56 + k * 26, line, ha="center", va="center",
                fontsize=12, family=MONO if mono else None, color=INK)


def arrow(ax, p, q, color=EDGE, label=None, label_xy=None, rad=0.0):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle="-|>", mutation_scale=18,
                                 color=color, lw=2,
                                 connectionstyle=f"arc3,rad={rad}"))
    if label:
        ax.text(*label_xy, label, ha="center", va="center", fontsize=11.5,
                color=color)


def fig_pybind_call():
    """One call from Python into C++ and back, through pybind11."""
    W, H = 1060, 340
    fig, ax = canvas(W, H)
    box(ax, 20, 40, 300, 120, "Python", ["ljmodule.lj_energy(", "  1.5, 1.0, 1.0)"],
        ec=PY, title_color=PY)
    box(ax, 380, 40, 300, 120, "pybind11",
        ["each float -> double", "(TypeError if it can't)"], ec=PYB,
        title_color=PYB, mono=False)
    box(ax, 740, 40, 300, 140, "C++", ["lj_energy(double r,", "  double epsilon,", "  double sigma)"],
        ec=CPP, title_color=CPP)
    arrow(ax, (322, 100), (378, 100), color=PY)
    arrow(ax, (682, 100), (738, 100), color=PYB)
    # The way back, underneath.
    box(ax, 380, 200, 300, 110, "pybind11", ["double -> float"], ec=PYB,
        title_color=PYB, mono=False)
    ax.text(170, 255, "-0.3203365942785746", ha="center", va="center",
            fontsize=13, family=MONO, color=INK, weight="bold")
    ax.text(170, 282, "a Python float", ha="center", va="center", fontsize=11.5,
            color=MUTED)
    ax.text(890, 255, "-0.32033659427857458", ha="center", va="center",
            fontsize=13, family=MONO, color=INK, weight="bold")
    ax.text(890, 282, "a C++ double: the same number", ha="center",
            va="center", fontsize=11.5, color=MUTED)
    arrow(ax, (890, 182), (890, 237), color=CPP, label="return", label_xy=(925, 208))
    arrow(ax, (775, 255), (682, 255), color=CPP)
    arrow(ax, (378, 255), (292, 255), color=PYB)
    save(fig, "pybind_call.svg")


def fig_pybind_layers():
    """The layers sim.run() passes through, from the script to the engine."""
    W, H = 1000, 360
    fig, ax = canvas(W, H)
    rows = [
        ("your script", "sim = System.from_yaml(...);  sim.run()", PY, "Python"),
        ("molsimworkbench.py", "class System: builds the engine objects, calls them", PY, "Python"),
        ("_molsim.cpython-312-darwin.so", "PYBIND11_MODULE(_molsim, module): py::class_ ...", PYB, "pybind11"),
        ("the engine", "Geometry, LJPotential, ForceCalculator, MolecularDynamics", CPP, "C++"),
    ]
    for k, (name, what, color, lang) in enumerate(rows):
        y = 20 + k * 88
        ax.add_patch(FancyBboxPatch((140, y), 840, 66,
                                    boxstyle="round,pad=0,rounding_size=10",
                                    fc="white", ec=color, lw=1.8))
        ax.text(160, y + 22, name, va="center", fontsize=13.5, family=MONO,
                weight="bold", color=color)
        ax.text(160, y + 47, what, va="center", fontsize=12, color=INK)
        ax.text(120, y + 33, lang, ha="right", va="center", fontsize=12.5,
                weight="bold", color=color)
        if k < len(rows) - 1:
            arrow(ax, (560, y + 67), (560, y + 87), color=EDGE)
    save(fig, "pybind_layers.svg")


def fig_pybind_shared():
    """Two owners of one LJPotential: the Python name and the ForceCalculator.
    Deleting the name leaves the other owner, so the object lives on."""
    W, H = 900, 260
    fig, ax = canvas(W, H)
    ax.add_patch(FancyBboxPatch((360, 90), 220, 80,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc="#fdf3e7", ec=PYB, lw=1.8))
    ax.text(470, 118, "LJPotential", ha="center", va="center", fontsize=14,
            family=MONO, weight="bold", color=INK)
    ax.text(470, 146, "one object, in C++", ha="center", va="center",
            fontsize=11.5, color=MUTED)
    box(ax, 20, 90, 230, 80, "Python's lj", [], ec=PY, title_color=PY)
    ax.text(135, 148, "del lj drops this owner", ha="center", va="center",
            fontsize=11, color=MUTED)
    box(ax, 690, 90, 190, 80, "ForceCalculator", [], ec=CPP, title_color=CPP)
    ax.text(785, 148, "its list of potentials", ha="center", va="center",
            fontsize=11, color=MUTED)
    arrow(ax, (252, 130), (358, 130), color=PY)
    arrow(ax, (688, 130), (582, 130), color=CPP)
    ax.text(305, 112, "shared_ptr", ha="center", va="center", fontsize=11,
            family=MONO, color=PY)
    ax.text(635, 112, "shared_ptr", ha="center", va="center", fontsize=11,
            family=MONO, color=CPP)
    ax.text(W / 2, 222, "the object is deleted only when the last owner lets go",
            ha="center", va="center", fontsize=12.5, color=INK, style="italic")
    save(fig, "pybind_shared.svg")


if __name__ == "__main__":
    fig_pybind_call()
    fig_pybind_layers()
    fig_pybind_shared()
