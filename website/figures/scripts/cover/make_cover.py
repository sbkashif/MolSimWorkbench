"""Cover image: a short Python script, the settings it reads, and the run it
produced. (What ``sim.run()`` goes through in C++ is text on the home page,
with links, not part of the image.)

Recipe, run in an empty working directory:

1. Simulate (512 atoms, density 0.8, T = 1.0, 4000 steps from a cubic
   lattice) with the very script shown on the cover. Copy cover.yaml to
   data/lj_box.yaml, data/potentials/lj_bulk.yaml next to it, then run
       PYTHONPATH=<repo> python run.py
2. python make_cover.py split      -> split/s*.xyz (every 40th step, wrapped)
3. For each frame index, VMD writes a Tachyon scene and tachyon renders it
   (commands in render_frames.tcl)                      -> frames/f*.tga
4. python make_cover.py            -> cover.webp, cover.png (last frame)
"""
import glob
import io

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
from PIL import Image, ImageChops

W, H = 1200, 640
SIM = 460                       # animation size in px
SIM_XY = (715, 40)              # its top-left corner

MODULES = {
    "Molecular simulation": "#1a9e9a",
    "C++": "#2f6db5",
    "pybind11": "#d9822b",
    "Parallel": "#8a4fbf",
    "Testing": "#3b9b3b",
}
INK = "#222222"
MUTED = "#5f6368"
MONO = "DejaVu Sans Mono"

# Files on the cover, as (text, colour) runs per line.
KW, STR, FN, PLAIN = "#c678dd", "#98c379", "#61afef", "#e6e6e6"
KEY, NUM, DIM = "#e06c75", "#d19a66", "#7f848e"
SCRIPT = [
    [("from", KW), (" molsimworkbench ", PLAIN), ("import", KW), (" System", PLAIN)],
    [],
    [("sim = System.", PLAIN), ("from_yaml", FN), ("(", PLAIN),
     ('"data/lj_box.yaml"', STR), (",", PLAIN)],
    [("        trajectory_output=", PLAIN), ('"traj.xyz"', STR), (")", PLAIN)],
    [("sim.", PLAIN), ("run", FN), ("()", PLAIN)],
]
# An excerpt of data/lj_box.yaml: the keys that set the state point.
CONFIG = [
    [("simulation:", KEY)],
    [("  temperature: ", KEY), ("1.0", NUM), ("     # held by the thermostat", DIM)],
    [("  total_steps: ", KEY), ("4000", NUM)],
    [("  lj_box:", KEY)],
    [("    num_particles: ", KEY), ("512", NUM)],
    [("    density: ", KEY), ("0.8", NUM)],
    [("    initialization: ", KEY), ('"cubic_lattice"', STR)],
]

def code_card(ax, x, y, w, title, lines, row_h=36):
    h = 80 + (len(lines) - 1) * row_h + 34
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0,rounding_size=14",
                                fc="#282c34", ec="none"))
    ax.add_patch(FancyBboxPatch((x, y), w, 44, boxstyle="round,pad=0,rounding_size=14",
                                fc="#21252b", ec="none"))
    ax.add_patch(plt.Rectangle((x, y + 30), w, 14, fc="#21252b", ec="none"))
    for i, c in enumerate(["#ff5f56", "#ffbd2e", "#27c93f"]):
        ax.add_patch(plt.Circle((x + 24 + i * 22, y + 22), 7, fc=c, ec="none"))
    ax.text(x + w / 2, y + 22, title, ha="center", va="center",
            fontsize=14, color="#9da5b4", family=MONO)
    r = ax.figure.canvas.get_renderer()
    for row, runs in enumerate(lines):
        cx = x + 26
        cy = y + 80 + row * row_h
        for text, colour in runs:
            t = ax.text(cx, cy, text, va="center", fontsize=15,
                        family=MONO, color=colour)
            cx = t.get_window_extent(r).x1 * (W / ax.figure.bbox.width)
    return y + h


def overlay():
    fig = plt.figure(figsize=(W / 100, H / 100), dpi=100)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, W)
    ax.set_ylim(H, 0)
    ax.axis("off")
    fig.patch.set_facecolor("white")

    # Top: the script, the settings it reads, and the run they produced.
    bottom = code_card(ax, 30, 30, 640, "run.py", SCRIPT)
    code_card(ax, 30, bottom + 24, 640, "data/lj_box.yaml (excerpt)", CONFIG,
              row_h=34)
    ax.add_patch(FancyArrowPatch((672, 300), (712, 300), arrowstyle="-|>",
                                 mutation_scale=22, color="#9aa0a6", lw=2.5))
    cx = SIM_XY[0] + SIM / 2
    ax.text(cx, SIM_XY[1] + SIM + 22, "traj.xyz: the atoms start on a grid",
            ha="center", va="center", fontsize=13.5, color=MUTED)
    ax.text(cx, SIM_XY[1] + SIM + 46, "and settle into an equilibrium liquid",
            ha="center", va="center", fontsize=13.5, color=MUTED)

    buf = io.BytesIO()
    fig.savefig(buf, format="png", dpi=100)
    plt.close(fig)
    buf.seek(0)
    return Image.open(buf).convert("RGB")


def sim_frames():
    paths = sorted(glob.glob("frames/f*.tga"))
    frames = [Image.open(p).convert("RGB") for p in paths]
    bg = Image.new("RGB", frames[0].size, "white")
    box = None
    for f in frames:
        b = ImageChops.difference(f, bg).getbbox()
        box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]),
                                     max(box[2], b[2]), max(box[3], b[3]))
    side = max(box[2] - box[0], box[3] - box[1])
    cx, cy = (box[0] + box[2]) / 2, (box[1] + box[3]) / 2
    crop = (int(cx - side / 2), int(cy - side / 2),
            int(cx + side / 2), int(cy + side / 2))
    return [f.crop(crop).resize((SIM, SIM), Image.LANCZOS) for f in frames]


def main():
    base = overlay()
    out = []
    for f in sim_frames():
        im = base.copy()
        im.paste(f, SIM_XY)
        out.append(im)
    out[-1].save("cover.png")
    # Hold the lattice and the final liquid a little longer.
    durations = [900] + [70] * (len(out) - 2) + [1500]
    out[0].save("cover.webp", save_all=True, append_images=out[1:],
                duration=durations, loop=0, quality=80, method=6)


def split(stride=40):
    """Write every stride-th frame of traj.xyz to split/sNNN.xyz, wrapped into
    [-L/2, L/2). The engine writes the first frame centred on the origin and
    later frames in [0, L), so without wrapping the box would jump."""
    import os
    L = (512 / 0.8) ** (1 / 3)
    os.makedirs("split", exist_ok=True)
    lines = open("traj.xyz").read().splitlines()
    i = step = k = 0
    while i < len(lines):
        n = int(lines[i])
        if step % stride == 0:
            with open(f"split/s{k:03d}.xyz", "w") as f:
                f.write(f"{n}\nstep {step}\n")
                for row in lines[i + 2:i + 2 + n]:
                    x, y, z = ((float(c) + L / 2) % L - L / 2
                               for c in row.split()[1:4])
                    f.write(f"LJ {x:.5f} {y:.5f} {z:.5f}\n")
            k += 1
        i += 2 + n
        step += 1


if __name__ == "__main__":
    import sys
    split() if sys.argv[1:] == ["split"] else main()
