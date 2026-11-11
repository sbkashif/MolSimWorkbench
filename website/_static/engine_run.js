// In-page runs of the engine. {engine-run} shows the settings files and a movie
// of the atoms; {engine-python} shows Python code and the images it saves.
// Nothing downloads until the reader presses Run; the engine then runs on the
// reader's own computer, in engine_worker.js.
(function () {
  const base = new URL(".", document.currentScript.src).href;
  const MAX_LINES = 500;

  const escape = (text) =>
    text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  const paint = (kind, text) => `<span class="engine-${kind}">${escape(text)}</span>`;

  // Colours text by replacing each match of pattern; plain text is escaped.
  function colour(text, pattern, replace) {
    let html = "";
    let last = 0;
    for (const match of text.matchAll(pattern)) {
      html += escape(text.slice(last, match.index)) + replace(match);
      last = match.index + match[0].length;
    }
    return html + escape(text.slice(last));
  }

  const YAML = /(#.*$)|("[^"\n]*"|'[^'\n]*')(\s*:)?|([A-Za-z_]\w*)(\s*:)|\b(true|false|null)\b|(-?\d+\.?\d*(?:e-?\d+)?)/gm;
  const yaml = (text) =>
    colour(text, YAML, ([all, comment, quoted, quotedColon, key, keyColon, word, number]) => {
      if (comment) return paint("comment", comment);
      if (quoted) return paint(quotedColon ? "key" : "string", quoted) + escape(quotedColon || "");
      if (key) return paint("key", key) + escape(keyColon);
      if (word) return paint("word", word);
      if (number) return paint("number", number);
      return escape(all);
    });

  const PYTHON = /(#.*$)|("""[\s\S]*?"""|f?"[^"\n]*"|'[^'\n]*')|\b(def|return|import|from|as|for|in|with|if|else|not|and|or|True|False|None)\b|(\b\d+\.?\d*\b)/gm;
  const python = (text) =>
    colour(text, PYTHON, ([all, comment, string, word, number]) => {
      if (comment) return paint("comment", comment);
      if (string) return paint("string", string);
      if (word) return paint("word", word);
      if (number) return paint("number", number);
      return escape(all);
    });

  const OUTPUT = /^(Error.*)$|^(final energy:)|([A-Z][a-z]+(?: [A-Z][a-z]+)?(?=:))|(^Step\b)|(-?\b\d+\.?\d*(?:e[-+]?\d+)?\b)/gm;
  const output = (text) =>
    colour(text, OUTPUT, ([all, error, final, name, step, number]) => {
      if (error) return paint("error", error);
      if (final) return paint("string", final);
      if (name) return paint("key", name);
      if (step) return paint("word", step);
      if (number) return paint("number", number);
      return escape(all);
    });

  // A text box cannot colour its own text, so a coloured copy sits behind it.
  function box(parent, title, rows, highlight) {
    const label = document.createElement("label");
    const name = document.createElement("span");
    name.textContent = title;
    const frame = document.createElement("div");
    frame.className = "engine-code";
    const behind = document.createElement("pre");
    behind.setAttribute("aria-hidden", "true");
    const area = document.createElement("textarea");
    area.spellcheck = false;
    area.rows = rows;
    const follow = () => {
      behind.scrollTop = area.scrollTop;
      behind.scrollLeft = area.scrollLeft;
    };
    area.refresh = () => {
      behind.innerHTML = highlight(area.value) + "\n\n";
      follow();
    };
    area.addEventListener("input", area.refresh);
    area.addEventListener("scroll", follow);
    frame.append(behind, area);
    label.append(name, frame);
    parent.append(label);
    return area;
  }

  function button(parent, text) {
    const b = document.createElement("button");
    b.type = "button";
    b.textContent = text;
    parent.append(b);
    return b;
  }

  // The atoms in their box, turned a little so that the box reads as a cube.
  function makeMovie(parent) {
    const SIZE = 360;
    const YAW = 0.5;
    const PITCH = 0.35;
    const holder = document.createElement("div");
    holder.className = "engine-run-movie";
    holder.hidden = true;
    const canvas = document.createElement("canvas");
    canvas.setAttribute("role", "img");
    canvas.setAttribute("aria-label", "Movie of the atoms in the run, from the engine's trajectory.");
    const controls = document.createElement("div");
    controls.className = "engine-run-movie-controls";
    const play = button(controls, "Pause");
    const slider = document.createElement("input");
    slider.type = "range";
    slider.min = 0;
    slider.setAttribute("aria-label", "Frame of the movie");
    const label = document.createElement("output");
    controls.append(slider, label);
    holder.append(canvas, controls);
    parent.append(holder);

    const scale = window.devicePixelRatio || 1;
    canvas.width = canvas.height = SIZE * scale;
    canvas.style.width = SIZE + "px";
    const ctx = canvas.getContext("2d");
    ctx.scale(scale, scale);

    let movie = null;
    let frame = 0;
    let timer = null;

    function project(x, y, z, side) {
      const zoom = SIZE / (side * 1.95);
      const cx = x - side / 2, cy = y - side / 2, cz = z - side / 2;
      const rx = cx * Math.cos(YAW) + cz * Math.sin(YAW);
      const rz = -cx * Math.sin(YAW) + cz * Math.cos(YAW);
      const ry = cy * Math.cos(PITCH) - rz * Math.sin(PITCH);
      const depth = cy * Math.sin(PITCH) + rz * Math.cos(PITCH);
      return [SIZE / 2 + rx * zoom, SIZE / 2 - ry * zoom, depth, zoom];
    }

    function draw() {
      const style = getComputedStyle(document.documentElement);
      const line = style.getPropertyValue("--pst-color-border").trim() || "#dadce0";
      const { atoms, side, sigma, positions, steps } = movie;
      ctx.clearRect(0, 0, SIZE, SIZE);

      ctx.strokeStyle = line;
      ctx.lineWidth = 1;
      for (let a = 0; a < 8; a++) {
        for (let k = 0; k < 3; k++) {
          const b = a ^ (1 << k);
          if (b < a) continue;
          const corner = (c) => project((c & 1) * side, ((c >> 1) & 1) * side, ((c >> 2) & 1) * side, side);
          const [x1, y1] = corner(a), [x2, y2] = corner(b);
          ctx.beginPath();
          ctx.moveTo(x1, y1);
          ctx.lineTo(x2, y2);
          ctx.stroke();
        }
      }

      const wrap = (v) => ((v % side) + side) % side;
      const points = [];
      for (let a = 0; a < atoms; a++) {
        const i = (frame * atoms + a) * 3;
        points.push(project(wrap(positions[i]), wrap(positions[i + 1]), wrap(positions[i + 2]), side));
      }
      points.sort((p, q) => p[2] - q[2]);
      // Spheres lit from the upper left, and darker towards the back of the box.
      const near = side * 0.9;
      for (const [x, y, depth, zoom] of points) {
        const radius = (sigma / 2) * zoom;
        const light = 30 + 22 * (0.5 + depth / (2 * near));
        const shine = ctx.createRadialGradient(
          x - radius * 0.35, y - radius * 0.35, radius * 0.1, x, y, radius
        );
        shine.addColorStop(0, `hsl(178 75% ${light + 34}%)`);
        shine.addColorStop(0.55, `hsl(178 72% ${light}%)`);
        shine.addColorStop(1, `hsl(182 70% ${light - 18}%)`);
        ctx.fillStyle = shine;
        ctx.beginPath();
        ctx.arc(x, y, radius, 0, 2 * Math.PI);
        ctx.fill();
      }

      slider.value = frame;
      label.textContent = "step " + steps[frame];
    }

    const pause = () => {
      clearInterval(timer);
      timer = null;
      play.textContent = "Play";
    };
    const resume = () => {
      play.textContent = "Pause";
      timer = setInterval(() => {
        frame = (frame + 1) % movie.steps.length;
        draw();
      }, 60);
    };

    play.onclick = () => (timer ? pause() : resume());
    slider.oninput = () => {
      pause();
      frame = Number(slider.value);
      draw();
    };

    return {
      show(newMovie) {
        pause();
        movie = newMovie;
        frame = 0;
        slider.max = movie.steps.length - 1;
        holder.hidden = false;
        controls.hidden = movie.steps.length < 2;
        draw();
        if (!controls.hidden && !matchMedia("(prefers-reduced-motion: reduce)").matches) resume();
      },
      hide() {
        pause();
        holder.hidden = true;
      },
    };
  }

  async function build(root) {
    const isPython = root.classList.contains("engine-python");
    const rows = 14;
    const files = document.createElement("div");
    files.className = "engine-run-files";
    const areas = isPython
      ? [[box(files, "sweep.py", 22, python), "sweep.py"]]
      : [
          [box(files, "data/lj_bulk_simulation.yaml", rows, yaml), "lj_bulk_simulation.yaml"],
          [box(files, "data/potentials/lj_bulk.yaml", rows, yaml), "lj_bulk.yaml"],
        ];

    const bar = document.createElement("div");
    bar.className = "engine-run-bar";
    const run = button(bar, "Run");
    const stop = button(bar, "Stop");
    const reset = button(bar, isPython ? "Reset code" : "Reset files");
    const status = document.createElement("span");
    status.className = "engine-run-status";
    status.setAttribute("role", "status");
    bar.append(status);

    const results = document.createElement("div");
    results.className = "engine-run-results";
    const screen = document.createElement("pre");
    screen.className = "engine-run-output";
    screen.hidden = true;
    root.append(files, bar, results);
    results.append(screen);
    const movie = isPython ? null : makeMovie(results);
    const image = document.createElement("img");
    image.alt = "The plot saved by the code above.";
    image.hidden = true;
    results.append(image);

    const originals = await Promise.all(
      areas.map(([, name]) => fetch(base + "engine/" + name).then((r) => r.text()))
    );
    const fill = () =>
      areas.forEach(([area], i) => {
        area.value = originals[i];
        area.refresh();
      });
    fill();

    let worker = null;
    let lines = [];
    let drawing = false;
    let started = 0;

    const draw = () => {
      drawing = false;
      screen.innerHTML = output(lines.join("\n"));
      screen.scrollTop = screen.scrollHeight;
    };
    const add = (line) => {
      screen.hidden = false;
      lines.push(line);
      if (lines.length > MAX_LINES) lines = lines.slice(-MAX_LINES);
      if (!drawing) {
        drawing = true;
        requestAnimationFrame(draw);
      }
    };
    const running = (yes) => {
      run.disabled = yes;
      stop.disabled = !yes;
    };
    const finish = (text) => {
      running(false);
      status.textContent = text;
    };

    function makeWorker() {
      const unavailable = () => {
        worker.terminate();
        worker = null;
        finish("Could not load the engine. Check your connection and press Run again.");
      };
      worker = new Worker(base + "engine_worker.js");
      worker.onmessage = ({ data }) => {
        if (data.type === "line" || data.type === "print") add(data.text);
        else if (data.type === "started") {
          started = performance.now();
          status.textContent = "Running on your computer…";
        } else if (data.type === "movie") movie.show(data.movie);
        else if (data.type === "image") {
          image.src = URL.createObjectURL(new Blob([data.bytes], { type: "image/png" }));
          image.hidden = false;
        } else if (data.type === "done") {
          const seconds = ((performance.now() - started) / 1000).toFixed(1);
          finish("Finished in " + seconds + " s");
        } else if (data.type === "failed") {
          add("Error: " + data.text);
          finish("The run failed. Check the text above.");
        } else if (data.type === "unavailable") unavailable();
      };
      worker.onerror = unavailable;
    }

    // Start loading once the reader reaches for the controls, so that Run is
    // quick, without making every visitor download Python.
    const warm = () => {
      if (!worker) makeWorker();
    };
    bar.addEventListener("pointerenter", warm, { once: true });
    root.addEventListener("focusin", warm, { once: true });

    run.onclick = () => {
      status.textContent = "Loading (once per visit)…";
      warm();
      lines = [];
      screen.textContent = "";
      screen.hidden = isPython;
      image.hidden = true;
      if (movie) movie.hide();
      running(true);
      worker.postMessage(
        isPython
          ? { code: areas[0][0].value }
          : { simulation: areas[0][0].value, potential: areas[1][0].value }
      );
    };
    stop.onclick = () => {
      worker.terminate();
      worker = null;
      finish("Stopped");
    };
    reset.onclick = fill;
    running(false);
  }

  // The home page: plain-language sliders and a Run button on the real engine.
  // Every run starts from a cubic lattice, which is what the page shows first.
  const SLIDERS = [
    {
      key: "epsilon", name: "How sticky", symbol: "ε", unit: "K", decimals: 0, perUnit: 120, min: 36, max: 360, step: 12, value: 120,
      note: "Stickier atoms take more heat to pull apart, so the material boils at a higher temperature.",
    },
    {
      key: "sigma", name: "How big", symbol: "σ", unit: "Å", decimals: 2, perUnit: 3.4, min: 2.38, max: 4.76, step: 0.17, value: 3.4,
      note: "Bigger atoms fill more of the box, so the same number of them turns a thin gas into a crowded fluid.",
    },
    {
      key: "temperature", name: "How hot", symbol: "T", unit: "K", decimals: 0, perUnit: 120, min: 36, max: 360, step: 12, value: 120,
      note: "Heat makes atoms move faster, until they pull free of each other and a liquid boils.",
    },
    {
      key: "density", name: "How crowded", symbol: "ρ", unit: "g/cm³", decimals: 2, perUnit: 1.688, min: 0.08, max: 1.52, step: 0.01, value: 0.51,
      note: "Squeezing the same atoms into a smaller box pushes a gas towards a liquid.",
    },
  ];

  // The sliders are in argon's real units; perUnit converts them to the engine's
  // reduced units (ε/k = 120 K, σ = 3.4 Å, and 1.688 g/cm³ for one atom per σ³).
  const reading = (slider, value) =>
    slider.symbol + " = " + value.toFixed(slider.decimals) + " " + slider.unit;

  // Rahman's 1964 run (Phys. Rev. 136, A405), in reduced units: 94.4 K, the
  // density of his box of side 10.229σ for 864 atoms, a cutoff of 2.25σ and
  // steps of 10⁻¹⁴ s. The number of atoms stays that of every other run.
  const RAHMAN = {
    timestep: 0.00463,
    temperature: 94.4 / 120,
    density: 864 / 10.229 ** 3,
    cutoff: 2.25,
    shown: { temperature: "T = 94.4 K", density: "ρ = 1.374 g/cm³" },
  };

  // The buttons of {engine-states}: the same box, read as argon.
  const STATES = [
    { name: "Liquid argon at 94.4 K", argon: "liquid argon at 94.4 K and 1.374 g/cm³, with 125 atoms", temperature: 96, epsilon: 120, sigma: 3.4, density: 1.36, exact: RAHMAN },
    { name: "Compressed gas", argon: "compressed argon gas at 27 °C", temperature: 300, epsilon: 120, sigma: 3.4, density: 0.17 },
  ];

  // Two properties measured from a run's frames: the distance to the nearest
  // neighbours (Å) and the diffusion constant (cm²/s). The engine's units of
  // length and time are argon's: 3.4 Å and 2.16 ps.
  const SETTLE = 500; // steps left out, while the starting grid settles
  function measure({ atoms, side, sigma, steps, positions }, timestep, from, to) {
    const every = steps[2] - steps[1];
    const first = Math.ceil(from / every);
    const end = Math.min(Math.round(to / every) + 1, steps.length - 1); // the last frame repeats a step number
    const frames = end - first;
    const wrap = (d) => d - side * Math.round(d / side);

    // Follow each atom across the box edges, without any drift of the whole box.
    const paths = new Float64Array(frames * atoms * 3);
    for (let f = 0; f < frames; f++) {
      const drift = [0, 0, 0];
      for (let a = 0; a < atoms; a++) {
        for (let k = 0; k < 3; k++) {
          const at = (f * atoms + a) * 3 + k;
          const now = ((first + f) * atoms + a) * 3 + k;
          paths[at] = f ? paths[at - atoms * 3] + wrap(positions[now] - positions[now - atoms * 3]) : positions[now];
          drift[k] += paths[at] / atoms;
        }
      }
      for (let a = 0; a < atoms; a++) for (let k = 0; k < 3; k++) paths[(f * atoms + a) * 3 + k] -= drift[k];
    }
    const spread = (lag) => {
      let total = 0;
      let count = 0;
      for (let f = 0; f + lag < frames; f += 2, count++) {
        for (let i = 0; i < atoms * 3; i++) {
          total += (paths[(f + lag) * atoms * 3 + i] - paths[f * atoms * 3 + i]) ** 2;
        }
      }
      return total / count / atoms;
    };
    // Rahman's slope, from his straight stretch between 1.5 and 3 ps.
    const picoseconds = every * timestep * 2.16;
    const early = Math.round(1.5 / picoseconds);
    const late = Math.round(3 / picoseconds);
    const slope = (spread(late) - spread(early)) / (6 * (late - early) * picoseconds);
    const diffusion = slope * 3.4 ** 2 * 1e-4;

    // The first peak of the pair distribution.
    const width = 0.02;
    const counts = new Float64Array(40); // from 0.9σ to 1.7σ
    const start = 0.9 * sigma;
    const bins = width * sigma;
    for (let f = first; f < end; f += 2) {
      for (let a = 0; a < atoms; a++) {
        for (let b = a + 1; b < atoms; b++) {
          let squared = 0;
          for (let k = 0; k < 3; k++) {
            squared += wrap(positions[(f * atoms + a) * 3 + k] - positions[(f * atoms + b) * 3 + k]) ** 2;
          }
          const bin = Math.floor((Math.sqrt(squared) - start) / bins);
          if (bin >= 0 && bin < counts.length) counts[bin] += 1;
        }
      }
    }
    const middle = (i) => start + (i + 0.5) * bins;
    const shell = (i) => middle(i) ** 2;
    let peak = 0;
    counts.forEach((count, i) => {
      if (count / shell(i) > counts[peak] / shell(peak)) peak = i;
    });
    return { neighbour: middle(peak) * 3.4, diffusion };
  }

  const BANNER = String.raw`
 __  __     _ ___ _
|  \/  |___| / __(_)_ __
| |\/| / _ \ \__ \ | '  \
|_|  |_\___/_|___/_|_|_|_|
__      __       _   _                 _
\ \    / /__ _ _| |_| |__  ___ _ _  __| |_
 \ \/\/ / _ \ '_| / / '_ \/ -_) ' \/ _| ' \
  \_/\_/\___/_| |_\_\_.__/\___|_||_\__|_||_|
`.slice(1);

  async function buildPlay(root) {
    const states = root.classList.contains("engine-states");
    const stage = document.createElement("div");
    stage.className = "engine-play-stage";
    const panel = (title) => {
      const holder = document.createElement("div");
      holder.className = "engine-play-panel";
      const heading = document.createElement("div");
      heading.className = "engine-play-heading";
      heading.textContent = title;
      holder.append(heading);
      stage.append(holder);
      return holder;
    };
    const simulator = panel("Simulator");
    const screen = document.createElement("pre");
    screen.className = "engine-run-output";
    simulator.append(screen);
    const movie = makeMovie(panel("Visualization"));
    root.append(stage);
    const controls = document.createElement("div");
    controls.className = "engine-play-controls";
    const inputs = {};
    for (const slider of SLIDERS) {
      const label = document.createElement("label");
      const name = document.createElement("span");
      name.textContent = slider.name;
      const input = document.createElement("input");
      input.type = "range";
      input.min = slider.min;
      input.max = slider.max;
      input.step = slider.step;
      input.value = slider.value;
      const value = document.createElement("output");
      const show = () => (value.textContent = reading(slider, Number(input.value)));
      input.addEventListener("input", show);
      show();
      label.append(name, input, value);
      const note = document.createElement("p");
      note.textContent = slider.note;
      controls.append(label, note);
      inputs[slider.key] = { input, show, value, perUnit: slider.perUnit };
    }
    const bar = document.createElement("div");
    bar.className = "engine-run-bar";
    const run = button(bar, states ? "Run" : "Run custom");
    const reset = button(bar, "Reset");
    const status = document.createElement("span");
    status.className = "engine-run-status";
    status.setAttribute("role", "status");
    const row = states ? bar : document.createElement("div");
    if (!states) {
      row.className = "engine-run-bar engine-play-presets";
      const name = document.createElement("span");
      name.textContent = "Start with";
      row.append(name);
    }
    const offered = states ? STATES : STATES.filter((state) => state.exact);
    const choices = offered.map((state) => button(row, state.name));
    if (!states) {
      const custom = document.createElement("p");
      custom.className = "engine-play-custom";
      custom.textContent = "Or make your own choices:";
      controls.prepend(row, custom);
    }
    bar.append(status);
    if (states) {
      run.hidden = reset.hidden = true;
      root.append(bar);
    } else {
      root.append(controls, bar);
    }
    const busy = (yes) => [run, ...choices].forEach((b) => (b.disabled = yes));

    const get = (name) => fetch(base + "engine/" + name).then((r) => r.text());
    const [simulation, potential] = await Promise.all([get("lj_play_simulation.yaml"), get("lj_bulk.yaml")]);

    const atoms = Number(/num_particles: (\d+)/.exec(simulation)[1]);
    const perSide = Math.round(atoms ** (1 / 3));
    const number = (key) => Number(inputs[key].input.value) / inputs[key].perUnit;
    const grid = (side) => {
      const lattice = new Float32Array(atoms * 3);
      for (let a = 0; a < atoms; a++) {
        const cell = [a % perSide, Math.floor(a / perSide) % perSide, Math.floor(a / perSide ** 2)];
        cell.forEach((c, k) => (lattice[a * 3 + k] = ((c + 0.5) * side) / perSide));
      }
      return lattice;
    };
    const settings = () =>
      SLIDERS.map((s) => reading(s, Number(inputs[s.key].input.value))).join(", ");

    // The terminal: the settings, then what the engine prints.
    let lines = [];
    let measured = ""; // what the page measured from the finished run
    let drawing = false;
    const draw = () => {
      drawing = false;
      screen.innerHTML = paint("comment", "# " + ran + "\n") + output(lines.join("\n")) +
        measured.split(/(?<=\n)/).map((line) => paint(line.startsWith("#") ? "comment" : "string", line)).join("");
      screen.scrollTop = screen.scrollHeight;
    };
    const showStart = () => {
      const side = (atoms / number("density")) ** (1 / 3);
      movie.show({ atoms, side, sigma: number("sigma"), steps: [0], positions: grid(side) });
      status.textContent = "Every run starts from this grid.";
      const offers = offered.map((state) => state.name).concat(states ? [] : [run.textContent]);
      screen.innerHTML = paint("banner", BANNER) + paint("comment", "\nPress ") +
        offers.map((name) => paint("press", " " + name + " ")).join(paint("comment", " or ")) +
        (states ? "" : paint("comment", "\n\n# " + settings()));
      screen.scrollTop = 0;
    };

    let worker = null;
    let started = 0;
    let ran = "";
    let named = ""; // the argon state chosen, if a button started this run
    let stale = false; // the sliders moved after this run began
    let chosen = null; // fixed settings of the button just pressed
    let exact = null; // fixed settings of the run in progress
    showStart();

    const backToStart = () => {
      stale = true;
      Object.values(inputs).forEach(({ show }) => show());
      showStart();
    };
    Object.values(inputs).forEach(({ input }) => input.addEventListener("input", backToStart));

    const unavailable = () => {
      if (worker) worker.terminate();
      worker = null;
      busy(false);
      status.textContent = "Could not load the engine. Check your connection and press Run again.";
    };

    // Starts loading once the reader reaches for the controls, so that the
    // engine is ready by the time they press Run.
    function warm() {
      if (worker) return;
      worker = new Worker(base + "engine_worker.js");
      worker.onmessage = ({ data }) => {
        if (data.type === "line") {
          // The code shown on the home page does not print the final energy.
          if (stale || data.text.startsWith("final energy:")) return;
          lines.push(data.text);
          if (lines.length > MAX_LINES) lines = lines.slice(-MAX_LINES);
          if (!drawing) {
            drawing = true;
            requestAnimationFrame(draw);
          }
        } else if (data.type === "started") {
          started = performance.now();
          if (!stale) status.textContent = "Running on your computer…";
        } else if (data.type === "movie") {
          if (!stale) {
            movie.show(data.movie);
            const timestep = exact ? exact.timestep : Number(/^  timestep: (.*)$/m.exec(simulation)[1]);
            const row = (from, to) => {
              const { neighbour, diffusion } = measure(data.movie, timestep, from, to);
              return "\n" + String(to).padStart(5) + neighbour.toFixed(1).padStart(10) + " Å" +
                (diffusion * 1e5).toFixed(1).padStart(10) + " × 10⁻⁵ cm²/s";
            };
            const total = Number(/^  total_steps: (\d+)$/m.exec(simulation)[1]);
            measured = "\n\n# Measured from traj.xyz. The first row is steps 0 to " + SETTLE +
              ", while the grid settles." +
              "\n# Later rows run from step " + SETTLE + " to the step shown." +
              "\n# Step  Neighbours        Diffusion constant" + row(0, SETTLE);
            for (let to = 2 * SETTLE; to <= total; to += SETTLE / 2) measured += row(SETTLE, to);
            measured += "\n# Neighbours: first peak of the pair distribution." +
              "\n# Diffusion: slope of mean-square displacement, 1.5 to 3 ps." +
              (exact ? "\n# Rahman 1964, 864 atoms: 3.7 Å and 2.43 × 10⁻⁵ cm²/s" : "");
            draw();
          }
        } else if (data.type === "done") {
          busy(false);
          const seconds = ((performance.now() - started) / 1000).toFixed(1);
          if (!stale) status.textContent = "Ran " + ran + " in " + seconds + " s.";
        } else if (data.type === "failed") {
          busy(false);
          if (!stale) status.textContent = "That run failed: " + data.text;
        } else if (data.type === "unavailable") unavailable();
      };
      worker.onerror = unavailable;
    }
    controls.addEventListener("pointerenter", warm, { once: true });
    bar.addEventListener("pointerenter", warm, { once: true });
    root.addEventListener("focusin", warm, { once: true });

    run.onclick = () => {
      warm();
      busy(true);
      stale = false;
      ran = named || settings();
      named = "";
      exact = chosen;
      chosen = null;
      measured = "";
      lines = [];
      draw();
      status.textContent = "Loading (once per visit)…";
      const files = {
        simulation: simulation
          .replace(/^  temperature: .*$/m, "  temperature: " + (exact ? exact.temperature : number("temperature")))
          .replace(/^    density: .*$/m, "    density: " + (exact ? exact.density : number("density"))),
        potential: potential
          .replace(/epsilon: [\d.]+/, "epsilon: " + number("epsilon"))
          .replace(/sigma: [\d.]+/, "sigma: " + number("sigma"))
          .replace(/cutoff: [\d.]+/, "cutoff: " + (exact ? exact.cutoff : 2.5 * number("sigma"))),
      };
      if (exact) {
        files.simulation = files.simulation
          .replace(/^  timestep: .*$/m, "  timestep: " + exact.timestep);
      }
      worker.postMessage(files);
    };
    screen.addEventListener("click", ({ target }) => {
      const pressed = target.closest(".engine-press");
      if (!pressed || run.disabled) return;
      const i = offered.findIndex((state) => state.name === pressed.textContent.trim());
      (i < 0 ? run : choices[i]).click();
    });
    choices.forEach((choice, i) => {
      choice.onclick = () => {
        for (const slider of SLIDERS) {
          inputs[slider.key].input.value = offered[i][slider.key];
          inputs[slider.key].show();
        }
        named = offered[i].argon;
        chosen = offered[i].exact || null;
        if (chosen) {
          for (const key in chosen.shown) inputs[key].value.textContent = chosen.shown[key];
        }
        run.click();
      };
    });
    reset.onclick = () => {
      for (const slider of SLIDERS) {
        inputs[slider.key].input.value = slider.value;
        inputs[slider.key].show();
      }
      backToStart();
    };
  }

  document.addEventListener("DOMContentLoaded", () => {
    document.querySelectorAll(".engine-run").forEach(build);
    document.querySelectorAll(".engine-play").forEach(buildPlay);
  });
})();
