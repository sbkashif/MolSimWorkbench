// Runs the engine in Pyodide, away from the page, so that the page stays
// responsive and Stop can end a long run. See engine_run.js.
const PYODIDE = "https://cdn.jsdelivr.net/pyodide/v0.27.8/full/";
importScripts(PYODIDE + "pyodide.js");

const SIMULATION = "data/lj_bulk_simulation.yaml";
const POTENTIAL = "data/potentials/lj_bulk.yaml";

// Reports a mistake in a file plainly, before the engine sees it.
const CHECK_PY = `
import yaml

def first_problem():
    for path in ("${SIMULATION}", "${POTENTIAL}"):
        try:
            with open(path) as file:
                yaml.safe_load(file)
        except yaml.YAMLError as error:
            return f"{path} is not valid YAML: {error}"

first_problem()
`;

// run.py from the running page, keeping at most about 200 frames for the movie.
const RUN_PY = `
import json
import yaml
from molsimworkbench import System

with open("${SIMULATION}") as file:
    settings = yaml.safe_load(file)["simulation"]
with open("${POTENTIAL}") as file:
    potential = yaml.safe_load(file)["components"][0]

steps = settings.get("total_steps", 1000)
sim = System.from_yaml("${SIMULATION}", trajectory_output="traj.xyz",
                       trajectory_every=max(1, steps // 200))
print("final energy:", sim.run())

box = settings.get("lj_box", {})
json.dumps({
    "side": (box.get("num_particles", 125) / box.get("density", 0.8)) ** (1 / 3),
    "sigma": potential["atom_parameters"]["LJ"]["sigma"],
})
`;

// Runs the reader's Python, showing what it prints but not the engine's lines.
const PYTHON_PY = `
import contextlib
import io
import os
import sys

os.environ["MPLBACKEND"] = "AGG"

class Lines(io.TextIOBase):
    def __init__(self, emit):
        self.emit, self.rest = emit, ""

    def write(self, text):
        self.rest += text
        *lines, self.rest = self.rest.split("\\n")
        for line in lines:
            self.emit(line)
        return len(text)

def run_code(code, emit):
    if "matplotlib.pyplot" in sys.modules:
        sys.modules["matplotlib.pyplot"].close("all")
    for name in os.listdir("."):
        if name.endswith(".png"):
            os.remove(name)
    with contextlib.redirect_stdout(Lines(emit)):
        exec(compile(code, "sweep.py", "exec"), {"__name__": "__main__"})
`;

const say = (type, text) => postMessage({ type, text });
let quiet = false;

async function fetchBytes(name) {
  const response = await fetch("engine/" + name);
  if (!response.ok) throw new Error("could not fetch " + name);
  return new Uint8Array(await response.arrayBuffer());
}

async function start() {
  const pyodide = await loadPyodide({
    indexURL: PYODIDE,
    stdout: (line) => { if (!quiet) say("line", line); },
    stderr: (line) => { if (!quiet) say("line", line); },
  });
  await pyodide.loadPackage("pyyaml", { messageCallback: () => {} });
  pyodide.FS.mkdirTree("/run/data/potentials");
  pyodide.FS.writeFile("/run/molsimworkbench.py", await fetchBytes("molsimworkbench.py"));
  pyodide.FS.writeFile("/run/_molsim.cpython-312-wasm32-emscripten.so", await fetchBytes("_molsim.wasm"));
  pyodide.FS.writeFile("/run/" + SIMULATION, await fetchBytes("lj_bulk_simulation.yaml"));
  pyodide.FS.writeFile("/run/" + POTENTIAL, await fetchBytes("lj_bulk.yaml"));
  pyodide.FS.chdir("/run");
  pyodide.runPython("import sys; sys.path.insert(0, '/run')");
  pyodide.runPython(PYTHON_PY);
  return pyodide;
}

// A trajectory file as one flat array of x, y, z for every atom of every frame.
function readFrames(text) {
  const lines = text.split("\n");
  const atoms = parseInt(lines[0], 10);
  const frames = Math.floor(lines.length / (atoms + 2));
  const positions = new Float32Array(frames * atoms * 3);
  const steps = [];
  for (let f = 0; f < frames; f++) {
    const top = f * (atoms + 2);
    // Comment lines read "Step 10, PE: ..."; the first and last frames have none.
    const step = /^Step (\d+)/.exec(lines[top + 1]);
    steps.push(step ? Number(step[1]) : f ? steps[f - 1] : 0);
    for (let a = 0; a < atoms; a++) {
      const parts = lines[top + 2 + a].trim().split(/\s+/);
      for (let k = 0; k < 3; k++) positions[(f * atoms + a) * 3 + k] = parseFloat(parts[k + 1]);
    }
  }
  return { atoms, steps, positions };
}

async function runFiles(pyodide, data) {
  pyodide.FS.writeFile(SIMULATION, data.simulation);
  pyodide.FS.writeFile(POTENTIAL, data.potential);
  const problem = pyodide.runPython(CHECK_PY);
  if (problem) {
    say("failed", problem);
    return;
  }
  const size = JSON.parse(await pyodide.runPythonAsync(RUN_PY));
  const movie = readFrames(pyodide.FS.readFile("traj.xyz", { encoding: "utf8" }));
  pyodide.FS.unlink("traj.xyz");
  postMessage({ type: "movie", movie: { ...movie, ...size } }, [movie.positions.buffer]);
  say("done");
}

async function runPython(pyodide, data) {
  await pyodide.loadPackagesFromImports(data.code, { messageCallback: () => {} });
  quiet = true;
  try {
    pyodide.globals.get("run_code")(data.code, (line) => say("print", line));
  } finally {
    quiet = false;
  }
  for (const name of pyodide.FS.readdir(".")) {
    if (name.endsWith(".png")) postMessage({ type: "image", bytes: pyodide.FS.readFile(name) });
  }
  say("done");
}

const ready = start();

onmessage = async ({ data }) => {
  let pyodide;
  try {
    pyodide = await ready;
  } catch (error) {
    say("unavailable");
    return;
  }
  try {
    say("started");
    await (data.code === undefined ? runFiles : runPython)(pyodide, data);
  } catch (error) {
    const lines = String(error.message || error).trim().split("\n");
    say("failed", lines[lines.length - 1]);
  }
};
