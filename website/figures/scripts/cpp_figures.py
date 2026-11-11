"""Figures for the C++ module. One function per figure.

    python cpp_figures.py          # writes every figure into ../
"""
from molsim_figures import FancyArrowPatch, FancyBboxPatch, INK, MUTED, EDGE, MONO, canvas, save

CPP = "#2f6db5"           # C++ module colour, as on the cover
FAIL = "#c0392b"


def file_box(ax, x, y, w, h, name, kind, fc="white", ec=EDGE):
    ax.add_patch(FancyBboxPatch((x - w / 2, y - h / 2), w, h,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc=fc, ec=ec, lw=1.8))
    ax.text(x, y - 12, name, ha="center", va="center", fontsize=15,
            family=MONO, weight="bold", color=INK)
    ax.text(x, y + 16, kind, ha="center", va="center", fontsize=12,
            color=MUTED)


def step_arrow(ax, p, q, label, color=EDGE):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle="-|>", mutation_scale=20,
                                 color=color, lw=2))
    ax.text((p[0] + q[0]) / 2, p[1] - 20, label, ha="center", va="center",
            fontsize=12.5, color=color, family=MONO)


def fig_compile():
    """Source file -> compiler -> executable -> output, and where a type
    error stops the chain."""
    W, H = 1000, 330
    fig, ax = canvas(W, H)
    y = 110
    file_box(ax, 110, y, 180, 80, "pair.cpp", "text you write")
    file_box(ax, 480, y, 250, 80, "pair", "program the computer runs",
             ec=CPP)
    ax.add_patch(FancyBboxPatch((730, y - 32), 240, 64,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc="#f1f3f4", ec="none"))
    ax.text(850, y, "U(1.5) = -0.320337", ha="center", va="center",
            fontsize=14, family=MONO, color=INK)
    ax.text(850, y + 48, "output", ha="center", va="center", fontsize=12,
            color=MUTED)
    step_arrow(ax, (205, y), (350, y), "clang++", color=CPP)
    ax.text(278, y + 22, "compile", ha="center", va="center", fontsize=12,
            color=MUTED)
    step_arrow(ax, (610, y), (725, y), "./pair")
    ax.text(668, y + 22, "run", ha="center", va="center", fontsize=12,
            color=MUTED)

    # A type error stops the chain at the compiler: no program is made.
    ax.add_patch(FancyArrowPatch((278, y + 40), (278, 245), arrowstyle="-|>",
                                 mutation_scale=18, color=FAIL, lw=1.8))
    ax.text(290, 195, "if the code has an error", ha="left", va="center",
            fontsize=12, color=FAIL)
    ax.text(278, 275, "error: assigning to 'double' from incompatible type",
            ha="center", va="center", fontsize=12, family=MONO, color=FAIL)
    ax.text(278, 303, "no program is made", ha="center", va="center",
            fontsize=12, color=MUTED)
    save(fig, "cpp_compile.svg")


def fig_build():
    """The compile arrow opened up: preprocess, compile, link."""
    W, H = 1120, 360
    fig, ax = canvas(W, H)
    y = 120
    stages = [
        (115, "pair.cpp", "10 lines"),
        (395, "pair.ii", "71,811 lines"),
        (675, "pair.o", "machine code, 12 KB"),
        (975, "pair", "executable, 39 KB"),
    ]
    for x, name, note in stages:
        file_box(ax, x, y, 190 if x < 800 else 210, 80, name, note,
                 ec=CPP if name == "pair" else EDGE)
    labels = ["1. preprocess", "2. compile", "3. link"]
    for (x0, *_), (x1, *_), lab in zip(stages, stages[1:], labels):
        ax.add_patch(FancyArrowPatch((x0 + 98, y), (x1 - 100, y),
                                     arrowstyle="-|>", mutation_scale=18,
                                     color=CPP, lw=2))
        ax.text((x0 + x1) / 2, y - 22, lab, ha="center", va="center",
                fontsize=12, color=CPP)
    # Inputs that come from the system, not from you.
    ax.text(255, 255, "<cmath>, <iostream>", ha="center", va="center",
            fontsize=13, family=MONO, color=INK)
    ax.text(255, 280, "header files (text)", ha="center", va="center",
            fontsize=12, color=MUTED)
    ax.add_patch(FancyArrowPatch((255, 238), (255, y + 6), arrowstyle="-|>",
                                 mutation_scale=16, color=EDGE, lw=1.6))
    ax.text(825, 255, "libc++, libSystem", ha="center", va="center",
            fontsize=13, family=MONO, color=INK)
    ax.text(825, 280, "libraries (machine code)", ha="center", va="center",
            fontsize=12, color=MUTED)
    ax.add_patch(FancyArrowPatch((825, 238), (825, y + 6), arrowstyle="-|>",
                                 mutation_scale=16, color=EDGE, lw=1.6))
    ax.text(W / 2, 330, "the header and library files come with the compiler",
            ha="center", va="center", fontsize=12, color=MUTED, style="italic")
    save(fig, "cpp_build.svg")


def fig_memory():
    """The three variables of pair.cpp in memory while it runs."""
    W, H = 960, 300
    fig, ax = canvas(W, H)
    rows = [("energy", "-0.320337", "0x16d5ce9c0"),
            ("sr6", "0.0877915", "0x16d5ce9c8"),
            ("r", "1.5", "0x16d5ce9d0")]
    x0, cw, y0, rh = 190, 64, 50, 62
    ax.text(x0 + 4 * cw, 26, "8 bytes", ha="center", va="center",
            fontsize=12, color=MUTED)
    for k, (name, value, addr) in enumerate(rows):
        y = y0 + k * rh
        for b in range(8):
            ax.add_patch(plt_rect(x0 + b * cw, y, cw, rh - 10))
        ax.text(x0 + 4 * cw, y + (rh - 10) / 2, value, ha="center",
                va="center", fontsize=14, family=MONO, color=INK,
                weight="bold")
        ax.text(x0 - 20, y + (rh - 10) / 2, name, ha="right", va="center",
                fontsize=15, family=MONO, color=INK, weight="bold")
        ax.text(x0 + 8 * cw + 16, y + (rh - 10) / 2, addr, ha="left",
                va="center", fontsize=12.5, family=MONO, color=MUTED)
    ax.text(x0 + 8 * cw + 16, y0 - 24, "address", ha="left", va="center",
            fontsize=12, color=MUTED)
    ax.text(W / 2, y0 + 3 * rh + 22,
            "each double is 8 bytes; the addresses are 8 apart",
            ha="center", va="center", fontsize=12, color=MUTED, style="italic")
    save(fig, "cpp_memory.svg")


def fig_computer():
    """The parts of a computer this chapter uses: disk, memory, processor
    (registers to hold values, an execution unit to run instructions),
    screen; the operating system moves things between them."""
    W, H = 1080, 420
    fig, ax = canvas(W, H)

    def part(x, y, w, h, title, note):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0,rounding_size=12",
                                    fc="white", ec=EDGE, lw=1.8))
        ax.text(x + w / 2, y + 28, title, ha="center", va="center",
                fontsize=15, weight="bold", color=INK)
        if note:
            ax.text(x + w / 2, y + h - 30, note, ha="center", va="center",
                    fontsize=11.5, color=MUTED, linespacing=1.4)

    def sub(x, y, w, h, title):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0,rounding_size=8",
                                    fc="#f6f8fb", ec=CPP, lw=1.2))
        ax.text(x + w / 2, y + 18, title, ha="center", va="center",
                fontsize=11.5, color=CPP, weight="bold")

    # Disk: files that stay.
    part(20, 60, 170, 270, "Disk", "kept when\npower is off")
    for k, f in enumerate(["pair.cpp", "pair"]):
        ax.text(105, 135 + k * 40, f, ha="center", va="center", fontsize=13,
                family=MONO, color=INK)

    # Memory: a row of numbered bytes.
    part(240, 60, 320, 270, "Memory", "working storage,\nlost when power is off")
    for k, (name, addr) in enumerate([("r", "0x...9d0"), ("sr6", "0x...9c8"),
                                      ("energy", "0x...9c0")]):
        y = 120 + k * 42
        ax.add_patch(plt_rect(320, y, 140, 32))
        ax.text(310, y + 16, name, ha="right", va="center", fontsize=12.5,
                family=MONO, color=INK)
        ax.text(470, y + 16, addr, ha="left", va="center", fontsize=11,
                family=MONO, color=MUTED)

    # Processor: registers (storage) and an execution unit (does the work).
    part(610, 60, 330, 270, "Processor", "")
    sub(625, 108, 140, 200, "registers")
    for k, (reg, val) in enumerate([("d0", "1.5"), ("d1", ""), ("d2", "")]):
        y = 143 + k * 50
        ax.add_patch(FancyBboxPatch((640, y), 110, 34,
                                    boxstyle="round,pad=0,rounding_size=6",
                                    fc="white", ec=CPP, lw=1.0))
        ax.text(660, y + 17, reg, ha="center", va="center", fontsize=12,
                family=MONO, color=CPP)
        ax.text(715, y + 17, val, ha="center", va="center", fontsize=12.5,
                family=MONO, color=INK, weight="bold")
    ax.text(695, 295, "hold values", ha="center", va="center", fontsize=10.5,
            color=MUTED)
    sub(780, 108, 145, 200, "execution unit")
    ax.text(852, 163, "running:", ha="center", va="center", fontsize=10.5,
            color=MUTED)
    ax.text(852, 188, "fmov d0, #1.5", ha="center", va="center",
            fontsize=11.5, family=MONO, color=INK, weight="bold")
    ax.text(852, 295, "runs instructions", ha="center", va="center",
            fontsize=10.5, color=MUTED)
    ax.add_patch(FancyArrowPatch((790, 188), (752, 160), arrowstyle="-|>",
                                 mutation_scale=14, color=CPP, lw=1.5,
                                 connectionstyle="arc3,rad=0.2"))

    # Screen.
    part(980, 60, 90, 270, "Screen", "")
    ax.text(1025, 195, "U(1.5) =\n-0.32", ha="center", va="center",
            fontsize=10, family=MONO, color=INK)

    def link(x0, x1, y, label, both=False):
        ax.add_patch(FancyArrowPatch((x0, y), (x1, y),
                                     arrowstyle="<|-|>" if both else "-|>",
                                     mutation_scale=16, color=EDGE, lw=1.8))
        ax.text((x0 + x1) / 2, y - 14, label, ha="center", va="center",
                fontsize=11, color=MUTED)
    link(192, 238, 195, "load")
    link(562, 608, 195, "copy", both=True)
    link(942, 978, 195, "")

    # The operating system manages all of it.
    ax.add_patch(FancyBboxPatch((20, 355), 1050, 40,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc="#f1f3f4", ec="none"))
    ax.text(545, 375, "operating system: starts programs, gives them memory, "
            "passes their output to the screen",
            ha="center", va="center", fontsize=12, color=INK)
    save(fig, "cpp_computer.svg")


def fig_function_parts():
    """The parts of a function definition, labelled on the real code."""
    W, H = 900, 320
    fig, ax = canvas(W, H)
    x0, y0, lh = 60, 120, 34
    lines = [
        "double lj_energy(double r) {",
        "    double sr6 = std::pow(1.0 / r, 6);",
        "    return 4.0 * (sr6 * sr6 - sr6);",
        "}",
    ]
    r = fig.canvas.get_renderer()
    ax.add_patch(FancyBboxPatch((x0 - 20, y0 - 26), 620, len(lines) * lh + 18,
                                boxstyle="round,pad=0,rounding_size=10",
                                fc="#f6f8fb", ec="none"))
    tx = []
    for k, line in enumerate(lines):
        tx.append(ax.text(x0, y0 + k * lh, line, va="center", fontsize=15,
                          family=MONO, color=INK))

    # Monospace: every character has the same width, so columns map to x.
    probe = ax.text(x0, 0, "M" * 40, fontsize=15, family=MONO)
    cw = probe.get_window_extent(r).width * (W / fig.bbox.width) / 40
    probe.remove()

    def span(k, start, text):
        """Pixel x-range of `text` at column `start` of line k."""
        return x0 + start * cw, x0 + (start + len(text)) * cw

    def mark(k, start, text, label, up=True):
        a, b = span(k, start, text)
        y = y0 + k * lh + (-16 if up else 16)
        ly = y + (-34 if up else 34)
        ax.plot([a, a, b, b], [y + (6 if up else -6), y, y, y + (6 if up else -6)],
                color=CPP, lw=1.6)
        ax.plot([(a + b) / 2, (a + b) / 2], [y, ly + (10 if up else -10)],
                color=CPP, lw=1.2)
        ax.text((a + b) / 2, ly, label, ha="center", va="center",
                fontsize=12.5, color=CPP, weight="bold")

    mark(0, 0, "double", "return type")
    mark(0, 7, "lj_energy", "name")
    mark(0, 17, "double r", "parameter")
    # Body: a bracket on the right of lines 1-2, and the return statement.
    bx = x0 + 600
    ax.plot([bx - 10, bx, bx, bx - 10], [y0 + lh - 14, y0 + lh - 14,
                                         y0 + 2 * lh + 14, y0 + 2 * lh + 14],
            color=CPP, lw=1.6)
    ax.text(bx + 12, y0 + 1.5 * lh, "body: runs on\nevery call", va="center",
            fontsize=12.5, color=CPP, weight="bold")
    a, b = span(2, 4, "return")
    yb = y0 + 3 * lh + 22
    ax.plot([a, a, b, b], [y0 + 2 * lh + 10, y0 + 2 * lh + 16,
                           y0 + 2 * lh + 16, y0 + 2 * lh + 10], color=CPP, lw=1.6)
    ax.plot([b, b + 40], [y0 + 2 * lh + 16, yb], color=CPP, lw=1.2)
    ax.text(b + 46, yb, "return: hands the result back to the caller",
            va="center", fontsize=12.5, color=CPP, weight="bold")
    save(fig, "cpp_function_parts.svg")


def fig_call():
    """One call: the argument is copied into the function's own variable,
    the result is copied back, and the function's variables then go away."""
    W, H = 980, 330
    fig, ax = canvas(W, H)

    def frame(x, y, w, h, title, rows, addr_note):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0,rounding_size=10",
                                    fc="white", ec=EDGE, lw=1.8))
        ax.text(x + w - 16, y + 24, title, ha="right", va="center",
                fontsize=14, family=MONO, weight="bold", color=INK)
        for k, (name, value) in enumerate(rows):
            yy = y + 62 + k * 40
            ax.text(x + 110, yy, name, ha="right", va="center", fontsize=13,
                    family=MONO, color=INK)
            ax.add_patch(plt_rect(x + 122, yy - 15, 120, 30))
            ax.text(x + 182, yy, value, ha="center", va="center", fontsize=13,
                    family=MONO, color=INK, weight="bold")
        ax.text(x + w / 2, y + h - 18, addr_note, ha="center", va="center",
                fontsize=11, family=MONO, color=MUTED)

    frame(40, 60, 300, 210, "main", [("r", "1.5"), ("energy", "-0.320")],
          "r at 0x...900")
    frame(620, 60, 320, 250, "lj_energy",
          [("r", "1.5"), ("epsilon", "1.0"), ("sigma", "1.0"), ("sr6", "0.0878")],
          "r at 0x...8b8: a different place")
    ax.add_patch(FancyArrowPatch((285, 122), (790, 104), arrowstyle="-|>",
                                 mutation_scale=18, color=CPP, lw=2,
                                 connectionstyle="arc3,rad=-0.15"))
    ax.text(510, 62, "call: the arguments are copied in", ha="center",
            va="center", fontsize=12.5, color=CPP)
    ax.add_patch(FancyArrowPatch((620, 250), (285, 168), arrowstyle="-|>",
                                 mutation_scale=18, color=CPP, lw=2,
                                 connectionstyle="arc3,rad=-0.15"))
    ax.text(470, 262, "return: the result is copied back", ha="center",
            va="center", fontsize=12.5, color=CPP)
    save(fig, "cpp_call.svg")


def plt_rect(x, y, w, h):
    import matplotlib.pyplot as plt
    return plt.Rectangle((x, y), w, h, fc="#f1f3f4", ec="#c9ccd1", lw=1)


if __name__ == "__main__":
    fig_compile()
    fig_build()
    fig_memory()
    fig_computer()
    fig_function_parts()
    fig_call()
