"""Small networks trained on made-up points, for the "how training can go
wrong" figure. Real training runs (PyTorch, Adam), on made-up data.

Training points: 13, evenly spread, from a made-up curve plus a little noise.
Validation points: 12, half-way between them, same curve, same noise level.
Three networks: one y (too simple), four y's (about right), and two steps of
64 numbers each (too flexible), trained for many epochs.

    conda run -n mlip-agent python agent_overfit.py
"""
import os

import numpy as np
import torch

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
rng = np.random.default_rng(3)


def curve(x):
    return x**4 - 1.6 * x**2 + 0.25 * x


xt = np.linspace(-1.5, 1.5, 13)
yt = curve(xt) + rng.normal(0, 0.15, len(xt))
xv = (xt[:-1] + xt[1:]) / 2
yv = curve(xv) + rng.normal(0, 0.15, len(xv))
grid = np.linspace(-1.5, 1.5, 400)


def net_for(m):
    torch.manual_seed(0)
    if m > 8:
        return torch.nn.Sequential(torch.nn.Linear(1, m), torch.nn.SiLU(),
                                   torch.nn.Linear(m, m), torch.nn.SiLU(),
                                   torch.nn.Linear(m, 1))
    return torch.nn.Sequential(torch.nn.Linear(1, m), torch.nn.SiLU(),
                               torch.nn.Linear(m, 1))


def t(a):
    return torch.tensor(np.asarray(a)[:, None], dtype=torch.float32)


curves, hist = {}, []
for name, m, epochs in [("simple", 1, 6000), ("right", 4, 6000), ("flexible", 64, 30000)]:
    net = net_for(m)
    opt = torch.optim.Adam(net.parameters(), lr=3e-3)
    for e in range(epochs):
        opt.zero_grad()
        loss = ((net(t(xt)) - t(yt)) ** 2).mean()
        loss.backward()
        opt.step()
        if name == "flexible" and e % 50 == 0:
            with torch.no_grad():
                hist.append((e, loss.item(), ((net(t(xv)) - t(yv)) ** 2).mean().item()))
    with torch.no_grad():
        curves[name] = net(t(grid)).numpy()[:, 0]
        tr = ((net(t(xt)) - t(yt)) ** 2).mean().item()
        va = ((net(t(xv)) - t(yv)) ** 2).mean().item()
    print(f"{name}: training error {tr:.4f}, validation error {va:.4f}")

np.savetxt(os.path.join(OUT, "agent_overfit_points.csv"),
           np.column_stack([np.r_[xt, xv], np.r_[yt, yv],
                            np.r_[np.zeros(len(xt)), np.ones(len(xv))]]),
           delimiter=",", header="x,y,is_validation", comments="", fmt="%.5f")
np.savetxt(os.path.join(OUT, "agent_overfit_curves.csv"),
           np.column_stack([grid, curves["simple"], curves["right"], curves["flexible"]]),
           delimiter=",", header="x,simple,right,flexible", comments="", fmt="%.5f")
np.savetxt(os.path.join(OUT, "agent_overfit_history.csv"), np.array(hist),
           delimiter=",", header="epoch,training_error,validation_error",
           comments="", fmt=["%d", "%.6f", "%.6f"])
