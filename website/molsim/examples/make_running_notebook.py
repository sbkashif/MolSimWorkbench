"""Write running.ipynb (the notebook version of the Running a simulation page)
and execute it, so the saved notebook shows real outputs.

    python make_running_notebook.py     # needs nbformat, nbclient, ipykernel
"""
import nbformat as nbf
from nbclient import NotebookClient

md, code = nbf.v4.new_markdown_cell, nbf.v4.new_code_cell

nb = nbf.v4.new_notebook()
nb.metadata["kernelspec"] = {"name": "python3", "display_name": "Python 3",
                             "language": "python"}
nb.cells = [
    md("# Running a simulation\n\n"
       "The notebook version of the page *Running a simulation*. It needs the "
       "engine built once (see the repository README); everything below is "
       "Python."),
    md("Find the top folder of the repository, so that `molsimworkbench` and "
       "the `data/` files are found wherever this notebook is opened from."),
    code("""import os, sys
from pathlib import Path

root = Path.cwd()
while not (root / "molsimworkbench.py").exists():
    root = root.parent
os.chdir(root)
sys.path.insert(0, str(root))
sys.path.insert(0, str(root / "website" / "molsim" / "examples"))
print("working in", root.name)"""),
    md("## Run\n\nThe same lines as the page. The engine prints a line every "
       "10 steps. In a notebook those lines reach the cell by a separate route "
       "from Python's own `print`, so the two can mix; keeping the result in a "
       "variable and printing it in the next cell keeps it readable."),
    code("""from molsimworkbench import System

sim = System.from_yaml("data/lj_bulk_simulation.yaml",
                       trajectory_output="traj.xyz")
energy = sim.run()"""),
    code("""print("final energy:", energy)"""),
    md("## Read the trajectory"),
    code("""from read_xyz import read_xyz

x = read_xyz("traj.xyz")
print("frames, atoms, coordinates:", x.shape)"""),
    md("## Mean-square displacement\n\nUndo the jumps by one box length, then "
       "average the squared distance each atom has moved."),
    code("""import numpy as np

L, dt = (125 / 0.8) ** (1 / 3), 0.001
step = np.diff(x, axis=0)
step -= L * np.round(step / L)
unwrapped = np.concatenate([x[:1], x[:1] + np.cumsum(step, axis=0)])

for t in (0.25, 0.5, 1.0):
    k = round(t / dt)
    msd = ((unwrapped[k] - unwrapped[0]) ** 2).sum(axis=1).mean()
    print(f"t = {t:4}: MSD = {msd:.3f}")"""),
]

NotebookClient(nb, timeout=600, kernel_name="python3",
               resources={"metadata": {"path": "."}}).execute()
nbf.write(nb, "running.ipynb")

for i, c in enumerate(nb.cells):
    if c.cell_type != "code":
        continue
    text = "".join(o.get("text", "") for o in c.outputs
                   if o.output_type == "stream").splitlines()
    errors = [o for o in c.outputs if o.output_type == "error"]
    print(f"--- cell {i}: {len(text)} lines, {len(errors)} errors")
    for e in errors:
        print(e["ename"], e["evalue"])
    print("\n".join(text if len(text) <= 6 else text[:3] + ["..."] + text[-3:]))
