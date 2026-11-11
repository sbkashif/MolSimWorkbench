"""Figures for C++ chapters 3-6. One function per figure.

    python cpp_figures_2.py          # writes every figure into ../
"""
from cpp_figures import CPP, plt_rect
from molsim_figures import (EDGE, INK, MONO, MUTED, PLUG, FancyArrowPatch,
                            FancyBboxPatch, canvas, save)


def box(ax, x, y, w, h, fc="white", ec=EDGE, lw=1.8, ls="-", r=10):
    ax.add_patch(FancyBboxPatch((x, y), w, h,
                                boxstyle=f"round,pad=0,rounding_size={r}",
                                fc=fc, ec=ec, lw=lw, ls=ls))


def arrow(ax, p, q, color=CPP, rad=0.0, lw=1.8, style="-|>"):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle=style, mutation_scale=16,
                                 color=color, lw=lw,
                                 connectionstyle=f"arc3,rad={rad}"))


def text(ax, x, y, s, size=12.5, color=INK, mono=False, bold=False,
         ha="center", va="center"):
    ax.text(x, y, s, fontsize=size, color=color, ha=ha, va=va,
            family=MONO if mono else None, weight="bold" if bold else None)


# ---------------------------------------------------------------- chapter 3

def fig_vector():
    """A vector: a small handle among main's variables, pointing at a row of
    elements stored side by side elsewhere in memory."""
    W, H = 980, 300
    fig, ax = canvas(W, H)
    box(ax, 20, 50, 320, 200)
    text(ax, 320, 75, "main", 14, mono=True, bold=True, ha="right")
    text(ax, 150, 140, "distances", 13, mono=True, ha="right")
    ax.add_patch(plt_rect(165, 115, 150, 50))
    text(ax, 240, 132, "where: 0x...200", 10.5, MUTED, mono=True)
    text(ax, 240, 150, "how many: 4", 10.5, MUTED, mono=True)
    text(ax, 180, 225, "24 bytes, with main's variables", 11, MUTED)

    values = ["1", "1.12246", "1.5", "2.5"]
    x0, cw = 420, 125
    text(ax, x0 + 2 * cw, 75, "the elements, side by side, elsewhere in memory",
         12, MUTED)
    for k, v in enumerate(values):
        x = x0 + k * cw
        ax.add_patch(plt_rect(x, 115, cw, 50))
        text(ax, x + cw / 2, 140, v, 13, mono=True, bold=True)
        text(ax, x + cw / 2, 185, f"[{k}]", 12, CPP, mono=True)
        text(ax, x + cw / 2, 210, f"0x...{0x200 + 8 * k:03x}", 11, MUTED,
             mono=True)
    arrow(ax, (315, 140), (418, 140))
    save(fig, "cpp3_vector.svg")


def fig_positions():
    """std::vector<std::array<double, 3>>: one row of three numbers per atom."""
    W, H = 900, 270
    fig, ax = canvas(W, H)
    rows = [("0.0", "0.0", "0.0"), ("1.5", "0.0", "0.0"), ("0.0", "1.2", "0.0")]
    x0, y0, cw, rh = 230, 60, 110, 46
    for j, lab in enumerate(["x", "y", "z"]):
        text(ax, x0 + j * cw + cw / 2, y0 - 18, f"[{j}]  {lab}", 12, CPP,
             mono=True)
    for i, row in enumerate(rows):
        y = y0 + i * rh
        text(ax, x0 - 20, y + rh / 2 - 4, f"positions[{i}]", 13, mono=True,
             ha="right")
        for j, v in enumerate(row):
            ax.add_patch(plt_rect(x0 + j * cw, y, cw, rh - 8))
            text(ax, x0 + j * cw + cw / 2, y + rh / 2 - 4, v, 13, mono=True,
                 bold=True)
    box(ax, x0 - 6, y0 + rh - 5, 3 * cw + 12, rh + 2, fc="none", ec=PLUG,
        lw=1.5, ls="--", r=6)
    text(ax, x0 + 3 * cw + 20, y0 + 1.5 * rh - 4,
         "one std::array<double, 3>:\nthe position of one atom", 11.5, PLUG,
         ha="left")
    text(ax, W / 2, y0 + 3 * rh + 30,
         "positions[1][0] is atom 1's x: 1.5", 12.5, MUTED, mono=True)
    save(fig, "cpp3_positions.svg")


# ---------------------------------------------------------------- chapter 4

def fig_objects():
    """One class, two objects: each object holds its own data members; the
    member functions are written once, in the class."""
    W, H = 980, 345
    fig, ax = canvas(W, H)
    box(ax, 30, 40, 330, 250, fc="#f6f8fb", ec=CPP)
    text(ax, 195, 65, "class LJPotential", 14, CPP, mono=True, bold=True)
    text(ax, 50, 105, "public:", 12.5, MUTED, mono=True, ha="left")
    text(ax, 70, 132, "LJPotential(eps, sig, cut)", 12, mono=True, ha="left")
    text(ax, 70, 158, "energy(r) const", 12, mono=True, ha="left")
    text(ax, 50, 195, "private:", 12.5, MUTED, mono=True, ha="left")
    text(ax, 70, 222, "epsilon_  sigma_  cutoff_", 12, mono=True, ha="left")
    text(ax, 195, 268, "the blueprint: written once", 11, MUTED)

    for k, (name, vals) in enumerate([("argon", ("1.0", "1.0", "2.5")),
                                      ("sticky", ("2.0", "1.0", "2.5"))]):
        y = 50 + k * 130
        box(ax, 560, y, 380, 110)
        text(ax, 925, y + 22, name, 14, mono=True, bold=True, ha="right")
        for j, (m, v) in enumerate(zip(["epsilon_", "sigma_", "cutoff_"], vals)):
            x = 580 + j * 120
            text(ax, x + 55, y + 52, m, 11.5, MUTED, mono=True)
            ax.add_patch(plt_rect(x + 10, y + 64, 90, 30))
            text(ax, x + 55, y + 79, v, 13, mono=True, bold=True)
        arrow(ax, (362, 120 + k * 60), (556, y + 55), rad=0.05 - 0.1 * k)
    text(ax, 490, 322, "LJPotential argon(1.0, 1.0, 2.5); makes an object",
         11.5, MUTED, mono=True)
    save(fig, "cpp4_objects.svg")


def fig_reference():
    """Passing by const reference: the parameter is a second name for the
    caller's object, so nothing is copied."""
    W, H = 900, 250
    fig, ax = canvas(W, H)
    box(ax, 40, 40, 330, 170)
    text(ax, 350, 64, "main", 14, mono=True, bold=True, ha="right")
    text(ax, 180, 120, "geometry", 13, mono=True, ha="right")
    ax.add_patch(plt_rect(195, 95, 150, 50))
    text(ax, 270, 120, "4 positions", 12, mono=True, bold=True)
    text(ax, 205, 185, "at 0x...880", 11, MUTED, mono=True)

    box(ax, 530, 40, 340, 170)
    text(ax, 850, 64, "total_energy", 14, mono=True, bold=True, ha="right")
    text(ax, 560, 120, "geometry", 13, mono=True, ha="left")
    text(ax, 700, 185, "a second name, no copy", 11, MUTED)
    arrow(ax, (552, 120), (348, 120), rad=0.0)
    text(ax, 450, 102, "const Geometry&", 12, CPP, mono=True)
    save(fig, "cpp4_reference.svg")


# ---------------------------------------------------------------- chapter 5

def fig_dispatch():
    """Virtual dispatch: the force calculator holds pointers to plugs of
    different classes; each object carries a hidden pointer to its class's
    table of functions, and p->energy(r) follows it."""
    W, H = 1060, 440
    fig, ax = canvas(W, H)
    # The calculator and its vector of pointers.
    box(ax, 20, 40, 250, 340)
    text(ax, 250, 64, "calculator", 13.5, mono=True, bold=True, ha="right")
    text(ax, 145, 100, "potentials_", 12, MUTED, mono=True)
    for k in range(2):
        y = 125 + k * 130
        ax.add_patch(plt_rect(70, y, 150, 50))
        text(ax, 145, y + 25, f"shared_ptr [{k}]", 11.5, mono=True)
    text(ax, 145, 350, "p->energy(r) for each", 11, CPP, mono=True)

    # The two objects.
    objs = [("an LJPotential", ["table", "epsilon_ 1.0", "sigma_ 1.0",
                                "cutoff_ 2.5"], 60),
            ("a SoftRepulsion", ["table", "strength_ 10", "range_ 1.0"], 255)]
    for k, (title, cells, y) in enumerate(objs):
        box(ax, 360, y, 330, 40 + 30 * len(cells) + 10)
        text(ax, 675, y + 20, title, 12.5, mono=True, bold=True, ha="right")
        for j, c in enumerate(cells):
            cy = y + 40 + 30 * j
            hidden = c == "table"
            ax.add_patch(plt_rect(380, cy, 230, 26))
            text(ax, 390, cy + 13, "hidden: table pointer" if hidden else c,
                 11.5, PLUG if hidden else INK, mono=True, ha="left")
        arrow(ax, (222, 150 + k * 130), (378, y + 53), rad=0.0)

    # The two tables of functions.
    tables = [("LJPotential's table", ["energy -> LJ formula",
                                       "name -> \"Lennard-Jones\""], 60),
              ("SoftRepulsion's table", ["energy -> A(1 - r/rc)^2",
                                         "name -> \"soft repulsion\""], 255)]
    for k, (title, rows, y) in enumerate(tables):
        box(ax, 770, y, 270, 110, fc="#f6f8fb", ec=PLUG)
        text(ax, 905, y + 20, title, 12, PLUG, bold=True)
        for j, rr in enumerate(rows):
            text(ax, 790, y + 52 + 28 * j, rr, 11, mono=True, ha="left")
        arrow(ax, (612, y + 53), (768, y + 53), color=PLUG)
    text(ax, W / 2, 420, "one table per class, written by the compiler; "
         "every object of the class points to it", 11.5, MUTED)
    save(fig, "cpp5_dispatch.svg")


# ---------------------------------------------------------------- chapter 6

def fig_files():
    """Two source files share one header; each compiles to its own object
    file; the linker matches what main.o needs with what lj.o has."""
    W, H = 1060, 380
    fig, ax = canvas(W, H)

    def file_box(x, y, w, name, lines, ec=EDGE, fc="white"):
        h = 40 + 24 * len(lines)
        box(ax, x, y, w, h, fc=fc, ec=ec)
        text(ax, x + w / 2, y + 20, name, 13.5, mono=True, bold=True)
        for k, line in enumerate(lines):
            text(ax, x + w / 2, y + 46 + 24 * k, line, 11, MUTED)
        return h

    file_box(20, 150, 190, "lj.h", ["declarations:", "what exists"],
             ec=PLUG, fc="#f2faf9")
    file_box(300, 40, 210, "lj.cpp", ["#include \"lj.h\"", "the code of LJPotential"])
    file_box(300, 250, 210, "main.cpp", ["#include \"lj.h\"", "uses LJPotential"])
    arrow(ax, (212, 175), (298, 80), color=PLUG, rad=-0.1)
    arrow(ax, (212, 205), (298, 285), color=PLUG, rad=0.1)
    text(ax, 150, 128, "pasted in", 11, PLUG)

    file_box(600, 40, 220, "lj.o", ["has: LJPotential::energy",
                                    "has: the constructor"])
    file_box(600, 250, 220, "main.o", ["needs: LJPotential::energy",
                                       "needs: the constructor"])
    arrow(ax, (512, 80), (598, 80))
    arrow(ax, (512, 290), (598, 290))
    text(ax, 555, 62, "-c", 11.5, CPP, mono=True)
    text(ax, 555, 272, "-c", 11.5, CPP, mono=True)

    file_box(900, 150, 140, "pair", ["executable"], ec=CPP)
    arrow(ax, (822, 80), (918, 150), rad=0.1)
    arrow(ax, (822, 290), (918, 216), rad=-0.1)
    text(ax, 880, 112, "link", 11.5, CPP)
    text(ax, 700, 200, "each need is matched to a \"has\"", 11, MUTED)
    save(fig, "cpp6_files.svg")


if __name__ == "__main__":
    fig_vector()
    fig_positions()
    fig_objects()
    fig_reference()
    fig_dispatch()
    fig_files()
