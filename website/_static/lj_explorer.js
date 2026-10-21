// Interactive Lennard-Jones curve: sliders for epsilon and sigma.
// Fills every <div class="lj-explorer"></div> on the page (made by the
// {lj-explorer} directive in conf.py).
(function () {
  "use strict";

  var R_MIN = 0.6, R_MAX = 4.0;     // plotted distance range
  var U_MIN = -2.2, U_MAX = 1.5;    // plotted energy range
  var CUTOFF = 2.5;                 // in units of sigma, as in the simulation

  function lj(r, eps, sig) {
    var s6 = Math.pow(sig / r, 6);
    return 4 * eps * (s6 * s6 - s6);
  }

  function colors() {
    var css = getComputedStyle(document.documentElement);
    var get = function (name, fallback) {
      var v = css.getPropertyValue(name).trim();
      return v || fallback;
    };
    return {
      text: get("--pst-color-text-base", "#222222"),
      muted: get("--pst-color-text-muted", "#5f6368"),
      grid: get("--pst-color-border", "#dadce0"),
      curve: "#1a9e9a",
      background: get("--pst-color-background", "#ffffff"),
    };
  }

  function slider(label, min, max, step, value) {
    var wrap = document.createElement("label");
    wrap.className = "lj-explorer-slider";
    var name = document.createElement("span");
    name.innerHTML = label;
    var input = document.createElement("input");
    input.type = "range";
    input.min = min;
    input.max = max;
    input.step = step;
    input.value = value;
    var out = document.createElement("output");
    wrap.append(name, input, out);
    return { wrap: wrap, input: input, out: out };
  }

  function build(root) {
    if (root.dataset.built) return;
    root.dataset.built = "1";
    var eps = slider("&epsilon; (well depth)", 0.2, 2.0, 0.05, 1.0);
    var sig = slider("&sigma; (atom size)", 0.6, 1.6, 0.01, 1.0);
    var tem = slider("k<sub>B</sub>T (thermal energy)", 0.1, 2.5, 0.05, 0.5);
    var controls = document.createElement("div");
    controls.className = "lj-explorer-controls";
    controls.append(eps.wrap, sig.wrap, tem.wrap);

    var canvas = document.createElement("canvas");
    var readout = document.createElement("p");
    readout.className = "lj-explorer-readout";
    var reset = document.createElement("button");
    reset.type = "button";
    reset.textContent = "Reset";
    root.append(controls, canvas, readout, reset);

    var hoverR = null;              // distance under the pointer, if any
    var phase = 0;                  // where the ball is in its motion
    var still = window.matchMedia &&
      window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    // Test hook: <div class="lj-explorer" data-phase="1.2"> freezes the ball
    // at that moment of its motion. The site itself never sets it.
    if (root.dataset.phase) { phase = parseFloat(root.dataset.phase); }
    var frozen = !!root.dataset.phase;

    // Distance where the curve reaches energy `level`, between a and b
    // (bisection; the curve is monotonic on each side of the minimum).
    function crossing(level, a, b, e, s) {
      for (var k = 0; k < 60; k++) {
        var m = (a + b) / 2;
        if ((lj(a, e, s) - level) * (lj(m, e, s) - level) <= 0) b = m; else a = m;
      }
      return (a + b) / 2;
    }

    function draw() {
      var e = parseFloat(eps.input.value);
      var s = parseFloat(sig.input.value);
      var t = parseFloat(tem.input.value);
      eps.out.textContent = e.toFixed(2);
      sig.out.textContent = s.toFixed(2);
      tem.out.textContent = t.toFixed(2);

      var dpr = window.devicePixelRatio || 1;
      var STRIP = 80;                 // room above the plot for the two atoms
      var st = getComputedStyle(root);      // fit inside the box's padding
      var W = root.clientWidth - parseFloat(st.paddingLeft) - parseFloat(st.paddingRight);
      var H = STRIP + Math.round(Math.min(360, W * 0.6));
      canvas.width = W * dpr;
      canvas.height = H * dpr;
      canvas.style.width = W + "px";
      canvas.style.height = H + "px";
      var ctx = canvas.getContext("2d");
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, W, H);
      var c = colors();

      var pad = { l: 48, r: 12, t: STRIP + 12, b: 38 };
      var x = function (r) { return pad.l + (r - R_MIN) / (R_MAX - R_MIN) * (W - pad.l - pad.r); };
      var y = function (u) { return pad.t + (U_MAX - u) / (U_MAX - U_MIN) * (H - pad.t - pad.b); };

      // Axes and grid.
      ctx.font = "12px sans-serif";
      ctx.lineWidth = 1;
      ctx.strokeStyle = c.grid;
      ctx.fillStyle = c.muted;
      ctx.textAlign = "center";
      for (var r = 1; r <= R_MAX; r += 1) {
        ctx.beginPath(); ctx.moveTo(x(r), pad.t); ctx.lineTo(x(r), H - pad.b); ctx.stroke();
        ctx.fillText(r.toFixed(0), x(r), H - pad.b + 16);
      }
      ctx.textAlign = "right";
      for (var u = -2; u <= 1; u += 1) {
        ctx.beginPath(); ctx.moveTo(pad.l, y(u)); ctx.lineTo(W - pad.r, y(u)); ctx.stroke();
        ctx.fillText(u.toFixed(0), pad.l - 6, y(u) + 4);
      }
      ctx.textAlign = "center";
      ctx.fillText("distance r", (pad.l + W - pad.r) / 2, H - 6);
      ctx.save();
      ctx.translate(13, (pad.t + H - pad.b) / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.fillText("energy U", 0, 0);
      ctx.restore();
      ctx.strokeStyle = c.muted;
      ctx.beginPath(); ctx.moveTo(pad.l, y(0)); ctx.lineTo(W - pad.r, y(0)); ctx.stroke();

      function curve(e, s, color, width, dash) {
        ctx.save();
        ctx.beginPath();
        ctx.rect(pad.l, pad.t, W - pad.l - pad.r, H - pad.t - pad.b);
        ctx.clip();
        ctx.strokeStyle = color;
        ctx.lineWidth = width;
        ctx.setLineDash(dash || []);
        ctx.beginPath();
        var started = false;
        for (var i = 0; i <= 600; i++) {
          var r = R_MIN + (R_MAX - R_MIN) * i / 600;
          // Keep off-scale values just outside the plot; the clip hides them.
          var u = Math.min(Math.max(lj(r, e, s), U_MIN - 1), U_MAX + 1);
          if (!started) { ctx.moveTo(x(r), y(u)); started = true; }
          else ctx.lineTo(x(r), y(u));
        }
        ctx.stroke();
        ctx.restore();
      }

      curve(1, 1, c.grid, 2, [5, 4]);           // reference: epsilon = sigma = 1
      curve(e, s, c.curve, 3);

      function label(text, lx, ly, color, align) {
        ctx.font = "13px sans-serif";
        ctx.textAlign = align || "left";
        ctx.strokeStyle = c.background;       // halo keeps labels readable
        ctx.lineWidth = 4;
        ctx.strokeText(text, lx, ly);
        ctx.fillStyle = color;
        ctx.fillText(text, lx, ly);
      }
      var bottom = H - pad.b, right = W - pad.r;

      // Cutoff.
      var rc = CUTOFF * s;
      if (rc < R_MAX) {
        ctx.strokeStyle = c.muted;
        ctx.lineWidth = 1;
        ctx.setLineDash([3, 3]);
        ctx.beginPath(); ctx.moveTo(x(rc), pad.t); ctx.lineTo(x(rc), bottom); ctx.stroke();
        ctx.setLineDash([]);
        label("cutoff 2.5\u03c3: U = 0 beyond", x(rc) + 5, pad.t + 14, c.muted);
      }

      // Thermal energy, measured up from the bottom of the well.
      var rmin = Math.pow(2, 1 / 6) * s;
      var level = -e + t;
      var stays = level < 0;
      var tc = stays ? "#127572" : "#c0392b";
      if (level < U_MAX) {
        ctx.strokeStyle = tc;
        ctx.lineWidth = 2;
        ctx.setLineDash([6, 4]);
        ctx.beginPath(); ctx.moveTo(x(rmin), y(level));
        ctx.lineTo(x(rmin + 0.9 * s), y(level)); ctx.stroke();
        ctx.setLineDash([]);
        label(stays ? "thermal energy below \u03b5: the pair stays together"
                    : "thermal energy above \u03b5: the pair breaks apart",
              x(rmin + 0.9 * s) + 6, y(level) + 4, tc);
      }

      // The ball: a pair with this much energy, moving along the curve. Its
      // height is the potential energy; the gap up to the dashed line is the
      // energy of motion. Below the barrier it turns back at both walls.
      var r1 = crossing(level, 0.85 * s, rmin, e, s), rb;
      if (stays) {
        var r2 = crossing(level, rmin, 6 * s, e, s);
        rb = r1 + (r2 - r1) * (1 - Math.cos(phase)) / 2;
      } else {
        var f = (phase / (2 * Math.PI)) % 1;   // roll out, then start again
        rb = r1 + (R_MAX + 0.2 - r1) * f * f;
      }
      if (still) rb = rmin;
      if (hoverR !== null) rb = Math.max(hoverR, 0.5 * s);
      ctx.fillStyle = tc;
      ctx.beginPath(); ctx.arc(x(rb), y(lj(rb, e, s)), 7, 0, 2 * Math.PI); ctx.fill();

      // The pair itself, above the plot: diameter sigma, centres rb apart,
      // on one scale (smaller than the plot's, so that the atoms fit).
      var k = 48;                                 // pixels per unit length
      var ay = STRIP / 2 + 2, a0 = pad.l + 0.5 * 1.6 * k;
      var a1 = Math.min(a0 + rb * k, W + 2 * k);
      ctx.save();
      ctx.beginPath(); ctx.rect(0, 0, W, STRIP); ctx.clip();
      [a0, a1].forEach(function (ac) {
        ctx.fillStyle = c.curve;
        ctx.globalAlpha = 0.85;
        ctx.beginPath(); ctx.arc(ac, ay, 0.5 * s * k, 0, 2 * Math.PI); ctx.fill();
        ctx.globalAlpha = 1;
        ctx.fillStyle = c.background;
        ctx.beginPath(); ctx.arc(ac, ay, 2.5, 0, 2 * Math.PI); ctx.fill();
      });
      ctx.strokeStyle = c.background;
      ctx.lineWidth = 1.5;
      ctx.beginPath(); ctx.moveTo(a0, ay); ctx.lineTo(a1, ay); ctx.stroke();
      ctx.restore();
      label("r = " + rb.toFixed(2) + (Math.abs(rb - s) < 0.03 ? "  (touching)" : ""),
            Math.min(a1 + 0.5 * s * k + 8, W - 120), ay + 4, c.text);

      // Depth of the well, epsilon.
      var ax = x(rmin + 0.3 * s);
      ctx.strokeStyle = c.text;
      ctx.lineWidth = 1.3;
      ctx.beginPath(); ctx.moveTo(ax, y(0)); ctx.lineTo(ax, y(-e)); ctx.stroke();
      [[y(0), 1], [y(-e), -1]].forEach(function (p) {
        ctx.beginPath(); ctx.moveTo(ax - 4, p[0] + 6 * p[1]); ctx.lineTo(ax, p[0]);
        ctx.lineTo(ax + 4, p[0] + 6 * p[1]); ctx.stroke();
      });
      // Put the label on whichever half of the arrow the thermal line avoids.
      var ey = Math.abs(level + 0.25 * e) < Math.abs(level + 0.75 * e) ? -0.75 * e : -0.25 * e;
      label("\u03b5", ax + 6, y(ey) + 4, c.text);

      // Lowest point and zero crossing.
      ctx.fillStyle = c.curve;
      ctx.beginPath(); ctx.arc(x(rmin), y(-e), 5, 0, 2 * Math.PI); ctx.fill();
      var low = "lowest: U = \u2212\u03b5 at r = 1.12\u03c3";
      if (y(-e) + 22 < bottom) label(low, x(rmin), y(-e) + 22, c.curve, "center");
      else label(low, x(rmin) + 10, y(-e) - 10, c.curve);
      ctx.fillStyle = c.text;
      ctx.beginPath(); ctx.arc(x(s), y(0), 4, 0, 2 * Math.PI); ctx.fill();
      label("U = 0 at r = \u03c3", x(s) + 8, y(0) - 8, c.text);

      // Value under the pointer.
      if (hoverR !== null && hoverR > R_MIN && hoverR < R_MAX) {
        var hu = lj(hoverR, e, s);
        if (hu > U_MIN && hu < U_MAX) {
          ctx.strokeStyle = c.muted;
          ctx.setLineDash([2, 3]);
          ctx.beginPath(); ctx.moveTo(x(hoverR), pad.t); ctx.lineTo(x(hoverR), H - pad.b); ctx.stroke();
          ctx.setLineDash([]);
          ctx.fillStyle = c.curve;
          ctx.beginPath(); ctx.arc(x(hoverR), y(hu), 4, 0, 2 * Math.PI); ctx.fill();
          ctx.fillStyle = c.text;
          ctx.textAlign = x(hoverR) > W - 150 ? "right" : "left";
          ctx.font = "13px sans-serif";
          var label = "U(" + hoverR.toFixed(2) + ") = " + hu.toFixed(3);
          var lx = x(hoverR) + (ctx.textAlign === "left" ? 8 : -8), ly = y(hu) - 10;
          ctx.strokeStyle = c.background;     // halo keeps the label readable
          ctx.lineWidth = 4;
          ctx.strokeText(label, lx, ly);
          ctx.fillText(label, lx, ly);
        }
      }

      readout.textContent =
        "σ = " + s.toFixed(2) + ": U = 0 at r = " + s.toFixed(2) +
        ", lowest point at r = " + rmin.toFixed(2) +
        ", cutoff at r = " + rc.toFixed(2) +
        ".  ε = " + e.toFixed(2) + ", kBT = " + t.toFixed(2) +
        ": kBT/ε = " + (t / e).toFixed(2) + ".";
    }

    function pointer(ev) {
      var box = canvas.getBoundingClientRect();
      var W = box.width, padL = 48, padR = 12;
      hoverR = R_MIN + (ev.clientX - box.left - padL) / (W - padL - padR) * (R_MAX - R_MIN);
      draw();
    }
    canvas.addEventListener("pointermove", pointer);
    canvas.addEventListener("pointerdown", pointer);
    canvas.addEventListener("pointerleave", function () { hoverR = null; draw(); });
    eps.input.addEventListener("input", draw);
    sig.input.addEventListener("input", draw);
    tem.input.addEventListener("input", draw);
    reset.addEventListener("click", function () {
      eps.input.value = 1; sig.input.value = 1; tem.input.value = 0.5; draw();
    });
    window.addEventListener("resize", draw);
    // Redraw when the theme switches between light and dark.
    new MutationObserver(draw).observe(document.documentElement,
      { attributes: true, attributeFilter: ["data-theme"] });
    function tick() {
      phase += 0.035;
      draw();
      window.requestAnimationFrame(tick);
    }
    if (still || frozen) draw(); else window.requestAnimationFrame(tick);
  }

  document.addEventListener("DOMContentLoaded", function () {
    document.querySelectorAll(".lj-explorer").forEach(build);
  });
})();
