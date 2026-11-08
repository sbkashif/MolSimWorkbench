"""The four tools the agent can call. Each one does real work and returns measured numbers.

The LLM never computes a number itself: it only chooses which tool to call and with
which settings. The harness checks those settings against the limits in config.py
before a tool runs.
"""
import json
import re
import subprocess
import time
import warnings

import numpy as np

from . import config, data


class RunState:
    """What the run has done so far. Tools read and update it; the log records it."""

    def __init__(self, run_dir):
        self.run_dir = run_dir.resolve()
        self.frames = config.START_FRAMES
        self.trainings = 0
        self.model = None          # path of the latest trained model
        self.model_info = None     # its settings, for the log


def train(state, epochs, channels):
    """Train a MACE model on the current training frames."""
    train_path, _ = data.write_frames(state.run_dir, state.frames)
    state.trainings += 1
    name = f"model_{state.trainings}"
    log_path = state.run_dir / f"{name}_train.log"
    cmd = [str(config.PYTHON_BIN / "mace_run_train"),
           "--name", name, "--train_file", str(train_path), "--valid_fraction", "0.1",
           "--energy_key", "REF_energy", "--forces_key", "REF_forces",
           "--E0s", "average", "--model", "MACE", "--hidden_irreps", f"{channels}x0e",
           "--r_max", "4.0", "--batch_size", "10", "--max_num_epochs", str(epochs),
           "--eval_interval", "5", "--device", "cpu", "--default_dtype", "float32",
           "--seed", "1", "--error_table", "PerAtomMAE", "--work_dir", str(state.run_dir)]
    t = time.time()
    with open(log_path, "w") as f:
        done = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=state.run_dir)
    minutes = round((time.time() - t) / 60, 1)
    model_path = state.run_dir / f"{name}.model"
    if done.returncode != 0 or not model_path.exists():
        return {"error": f"training failed, see {log_path.name}"}
    text = (state.run_dir / "logs" / f"{name}_run-1.log").read_text()
    val = re.findall(r"Epoch \d+:.*MAE_F=\s*([\d.]+) meV", text)
    state.model = model_path
    state.model_info = {"frames": state.frames, "epochs": epochs, "channels": channels}
    return {"model": name, "training_frames": state.frames, "epochs": epochs,
            "channels": channels, "minutes": minutes,
            "validation_force_mae_meV_per_A": float(val[-1]) if val else None}


def errors(model_path, frames_path):
    """Force and energy errors of one model on the frames in one file."""
    import ase.io
    from mace.calculators import MACECalculator
    frames = ase.io.read(frames_path, ":")
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        calc = MACECalculator(model_paths=str(model_path), device="cpu",
                              default_dtype="float32")
    f_err, e_pred, e_ref = [], [], []
    for a in frames:
        a.calc = calc
        f_err.append(np.abs(a.get_forces() - a.arrays["REF_forces"]))
        e_pred.append(a.get_potential_energy())
        e_ref.append(a.info["REF_energy"])
    e_diff = np.array(e_pred) - np.array(e_ref)
    n_atoms = len(frames[0])
    return {"model": model_path.stem, "frames": len(frames),
            "force_mae_meV_per_A": round(1000 * float(np.mean(f_err)), 1),
            "energy_mae_meV_per_atom": round(1000 * float(np.mean(np.abs(e_diff))) / n_atoms, 1),
            "energy_mean_shift_eV": round(float(e_diff.mean()), 3),
            "energy_mae_after_shift_meV_per_atom":
                round(1000 * float(np.mean(np.abs(e_diff - e_diff.mean()))) / n_atoms, 1)}


def evaluate(state):
    """Errors of the latest model on the selection frames."""
    if state.model is None:
        return {"error": "no model trained yet"}
    _, selection_path = data.write_frames(state.run_dir, state.frames)
    result = errors(state.model, selection_path)
    result["frames_used"] = "selection"
    result["target_force_mae_meV_per_A"] = config.TARGET_FORCE_MAE
    return result


def final_test(state):
    """Errors of the latest model on the final test frames. The harness calls this
    once, after the agent stops; it is not one of the agent's tools."""
    if state.model is None:
        return {"error": "no model trained"}
    result = errors(state.model, data.write_final_test(state.run_dir))
    result["frames_used"] = "final_test"
    return result


def run_md(state):
    """Run the latest model in OpenMM and report stability and energy conservation."""
    import ase.io
    import openmm as mm
    import openmm.app as app
    import openmm.unit as u
    from ase.neighborlist import natural_cutoffs, neighbor_list
    from openmmml import MLPotential
    if state.model is None:
        return {"error": "no model trained yet"}
    _, selection_path = data.write_frames(state.run_dir, state.frames)
    atoms = ase.io.read(selection_path, 0)
    top = app.Topology()
    res = top.addResidue("MOL", top.addChain())
    for s in atoms.get_chemical_symbols():
        top.addAtom(s, app.Element.getBySymbol(s), res)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        system = MLPotential("mace", modelPath=str(state.model)).createSystem(
            top, precision="single", returnEnergyType="energy")
    sim = app.Simulation(top, system, mm.VerletIntegrator(0.5 * u.femtosecond),
                         mm.Platform.getPlatformByName("CPU"))
    sim.context.setPositions(atoms.positions * u.angstrom)
    sim.context.setVelocitiesToTemperature(config.MD_TEMPERATURE * u.kelvin, 1)
    kj = u.kilojoule_per_mole

    def bonds(x):
        a = atoms.copy()
        a.positions = x
        i, j = neighbor_list("ij", a, [1.2 * c for c in natural_cutoffs(a)])
        return {(p, q) for p, q in zip(i, j) if p < q}

    start_bonds = bonds(atoms.positions)
    totals, closest, t = [], 9.0, time.time()
    for _ in range(config.MD_STEPS // 50):
        sim.step(50)
        st = sim.context.getState(getEnergy=True, getPositions=True)
        totals.append((st.getPotentialEnergy().value_in_unit(kj)
                       + st.getKineticEnergy().value_in_unit(kj)) / 96.485)
        x = st.getPositions(asNumpy=True).value_in_unit(u.angstrom)
        d = np.linalg.norm(x[:, None] - x[None], axis=-1) + np.eye(len(x)) * 9
        closest = min(closest, float(d.min()))
        if not np.all(np.isfinite(x)):
            break
    final_t = st.getKineticEnergy().value_in_unit(kj) / 96.485 * 2 / (3 * len(x) * 8.617e-5)
    drift = float(np.ptp(totals)) if np.all(np.isfinite(totals)) else float("inf")
    return {"model": state.model.stem, "steps": config.MD_STEPS, "timestep_fs": 0.5,
            "start_temperature_K": config.MD_TEMPERATURE,
            "final_temperature_K": round(final_t, 1),
            "energy_range_after_t0_eV": round(drift, 4),
            "closest_pair_A": round(closest, 3),
            "bonds_same_as_start": bonds(x) == start_bonds,
            "wall_seconds": round(time.time() - t, 1),
            "limits": {"energy_range_eV": config.MD_DRIFT_LIMIT,
                       "closest_pair_A": config.MIN_PAIR_LIMIT}}


def add_data(state, frames):
    """Use more of the 1000-frame training pool in the next training."""
    state.frames = min(state.frames + frames, config.MAX_FRAMES)
    return {"training_frames_next_time": state.frames, "pool_size": config.MAX_FRAMES}


def describe():
    """The tool list the LLM sees: names, what they do, and their arguments."""
    return [
        {"type": "function", "function": {
            "name": "train", "description":
                "Train a new MACE model on the current training frames. "
                f"Takes a few minutes. At most {config.MAX_TRAININGS} trainings per run.",
            "parameters": {"type": "object", "required": ["epochs", "channels"], "properties": {
                "epochs": {"type": "integer", "description":
                           f"passes through the training frames, {config.EPOCH_RANGE[0]} to {config.EPOCH_RANGE[1]}"},
                "channels": {"type": "integer", "description":
                             f"model width, one of {list(config.CHANNEL_CHOICES)}"}}}}},
        {"type": "function", "function": {
            "name": "evaluate", "description":
                f"Measure the latest model's force and energy errors on {config.SELECTION_FRAMES} "
                "selection frames, which are never trained on.",
            "parameters": {"type": "object", "properties": {}}}},
        {"type": "function", "function": {
            "name": "run_md", "description":
                "Run 1 ps of MD with the latest model in OpenMM, starting at 500 K, and report "
                "whether it stayed stable and conserved energy.",
            "parameters": {"type": "object", "properties": {}}}},
        {"type": "function", "function": {
            "name": "add_data", "description":
                "Use more frames from the training pool in the next training. "
                f"The pool holds {config.MAX_FRAMES} frames.",
            "parameters": {"type": "object", "required": ["frames"], "properties": {
                "frames": {"type": "integer", "description": "how many frames to add"}}}}},
        {"type": "function", "function": {
            "name": "stop", "description":
                "End the run. Give the reason: target met, or why it cannot be met within the limits.",
            "parameters": {"type": "object", "required": ["reason"], "properties": {
                "reason": {"type": "string"}}}}},
    ]


def as_text(result):
    return json.dumps(result)
