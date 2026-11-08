"""Pull the numbers for the agent figures out of a finished training run.

    conda run -n mlip-agent python agent_extract.py ~/workspace/mlip-spike

Writes data/agent_val_errors.csv, data/agent_force_parity.csv and
data/agent_one_frame.json. agent_figures.py draws from those files.
"""
import csv
import json
import os
import re
import sys
import warnings

import ase.io
import numpy as np
import torch
from mace import data
from mace.tools import torch_geometric, utils

warnings.filterwarnings("ignore")
RUN = os.path.expanduser(sys.argv[1])
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

pat = re.compile(r"Epoch (\d+):.*MAE_E_per_atom=\s*([\d.]+) meV, MAE_F=\s*([\d.]+) meV")
with open(os.path.join(RUN, "logs/spike_run-1.log")) as f:
    rows = [m.groups() for m in map(pat.search, f) if m]
with open(os.path.join(OUT, "agent_val_errors.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["epoch", "energy_mae_meV_per_atom", "force_mae_meV_per_A"])
    w.writerows(rows)

model = torch.load(os.path.join(RUN, "spike.model"), map_location="cpu",
                   weights_only=False).float().eval()
table = utils.AtomicNumberTable(model.atomic_numbers.tolist())
cut = float(model.r_max)
frames = ase.io.read(os.path.join(RUN, "data/test.xyz"), ":")


def batch(atoms):
    return data.AtomicData.from_config(data.config_from_atoms(atoms),
                                       z_table=table, cutoff=cut)


loader = torch_geometric.dataloader.DataLoader([batch(a) for a in frames],
                                               batch_size=20)
pred = np.concatenate([model(b.to_dict(), compute_force=True)["forces"]
                       .detach().numpy() for b in loader])
ref = np.concatenate([a.arrays["REF_forces"] for a in frames])
print("force MAE meV/A", 1000 * np.abs(pred - ref).mean(), "n", pred.size)
with open(os.path.join(OUT, "agent_force_parity.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["dft_eV_per_A", "mlip_eV_per_A"])
    w.writerows(zip(ref.ravel().round(4), pred.ravel().round(4)))

loader = torch_geometric.dataloader.DataLoader([batch(a) for a in frames],
                                               batch_size=20)
u_pred = np.concatenate([model(b.to_dict(), compute_force=True)["energy"]
                         .detach().numpy() for b in loader])
u_dft = np.array([a.info["REF_energy"] for a in frames])
with open(os.path.join(OUT, "agent_energy_parity.csv"), "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["dft_eV", "mlip_eV"])
    w.writerows(zip(u_dft.round(4), u_pred.round(4)))

a = frames[0]
b = next(iter(torch_geometric.dataloader.DataLoader([batch(a)], batch_size=1)))
o = model(b.to_dict(), compute_force=True)
json.dump({
    "symbols": a.get_chemical_symbols(),
    "positions_A": a.positions.round(4).tolist(),
    "r_max_A": cut,
    "parameters": sum(p.numel() for p in model.parameters()),
    "h_atom0": o["node_feats"][0].detach().numpy().round(4).tolist(),
    "E_atoms_eV": o["node_energy"].detach().numpy().round(3).tolist(),
    "U_eV": round(float(o["energy"]), 3),
    "U_dft_eV": round(a.info["REF_energy"], 3),
}, open(os.path.join(OUT, "agent_one_frame.json"), "w"), indent=1)
