"""Run the engine once per value of one setting, and plot what changes."""
import tempfile
from pathlib import Path

import matplotlib.pyplot as plt
import yaml

from molsimworkbench import System

SETTING = "temperature"  # or "epsilon", "sigma", "density"
VALUES = [0.8, 1.0, 1.25, 1.5, 2.0, 2.5]

ATOMS = 125
STEPS = 2000


def mean_energy(temperature=1.0, epsilon=1.0, sigma=1.0, density=0.8,
                atoms=ATOMS, steps=STEPS):
    """Potential energy per atom, averaged over the second half of one run."""
    with open("data/lj_bulk_simulation.yaml") as file:
        simulation = yaml.safe_load(file)
    with open("data/potentials/lj_bulk.yaml") as file:
        potential = yaml.safe_load(file)

    simulation["simulation"]["temperature"] = temperature
    simulation["simulation"]["total_steps"] = steps
    simulation["simulation"]["lj_box"]["num_particles"] = atoms
    simulation["simulation"]["lj_box"]["density"] = density
    lj = potential["components"][0]
    lj["atom_parameters"]["LJ"] = {"epsilon": epsilon, "sigma": sigma}
    lj["cutoff"] = 2.5 * sigma  # the cutoff stays 2.5σ as σ changes

    # The engine looks for potential files inside data/.
    potential_file = Path("data/potentials/sweep.yaml")
    potential_file.write_text(yaml.safe_dump(potential))
    simulation["potential_config_files"] = ["potentials/sweep.yaml"]

    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        (folder / "run.yaml").write_text(yaml.safe_dump(simulation))
        trajectory = folder / "traj.xyz"

        sim = System.from_yaml(str(folder / "run.yaml"),
                               trajectory_output=trajectory,
                               trajectory_every=10)
        potential_file.unlink()
        sim.run()

        # Each frame's comment line reads "Step 10, PE: -608.3, KE: 187.5".
        energies = [float(line.split("PE:")[1].split(",")[0])
                    for line in trajectory.read_text().splitlines()
                    if line.startswith("Step")]

    second_half = energies[len(energies) // 2:]
    return sum(second_half) / len(second_half) / atoms


def main():
    results = [mean_energy(**{SETTING: value}) for value in VALUES]

    for value, energy in zip(VALUES, results):
        print(f"{SETTING} = {value:<6} U per atom = {energy:.3f}")

    plt.figure(figsize=(4.5, 3.2), layout="constrained")
    plt.plot(VALUES, results, "o-")
    plt.xlabel(SETTING)
    plt.ylabel("potential energy per atom")
    plt.savefig("sweep.png", dpi=150)


if __name__ == "__main__":
    main()
