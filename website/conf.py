# Sphinx configuration for the MolSimWorkbench learning website.
# Build locally:  python -m sphinx -E -b html website website/_build/html

project = "MolSimWorkbench"
author = "Salman Bin Kashif"
copyright = "2026, Salman Bin Kashif"

extensions = [
    "myst_parser",
    "sphinx_design",
    "sphinxcontrib.mermaid",
]

source_suffix = {".md": "markdown"}
root_doc = "index"
exclude_patterns = ["_build", "figures/scripts", "LICENSE.md"]

myst_enable_extensions = ["colon_fence", "deflist", "dollarmath"]
myst_heading_anchors = 3
myst_fence_as_directive = ["mermaid"]

numfig = True

html_theme = "sphinx_book_theme"
html_title = "MolSimWorkbench"
html_theme_options = {
    "repository_url": "https://github.com/sbkashif/MolSimWorkbench",
    "repository_branch": "main",
    "path_to_docs": "website",
    "use_repository_button": True,
    "show_toc_level": 2,
    # License and AI-use note on every page (full text: website/LICENSE.md).
    "extra_footer": (
        'Pages, figures and code examples: '
        '<a href="https://creativecommons.org/licenses/by-nc-sa/4.0/">CC BY-NC-SA 4.0</a> '
        '(credit the author, no commercial use, share adaptations alike). '
        'The engine\'s source code: MIT. '
        'Made with the help of AI (Claude) while learning; course design, structure '
        'and presentation by the author. '
        '<a href="https://github.com/sbkashif/MolSimWorkbench/blob/main/website/LICENSE.md">Details</a>.'
    ),
}

# Interactive figures: the page writes a one-line directive such as
# ```{lj-explorer}``` and the script in _static/ draws into the placeholder,
# so the Markdown sources hold no HTML.
html_static_path = ["_static"]
html_js_files = ["lj_explorer.js", "engine_run.js", "sidebar_toggle_fix.js"]
html_css_files = ["lj_explorer.css", "engine_run.css", "detour.css"]

from docutils import nodes as _nodes
from docutils.parsers.rst import Directive as _Directive


class LJExplorer(_Directive):
    """Placeholder for the interactive LJ curve (_static/lj_explorer.js)."""

    def run(self):
        return [_nodes.raw("", '<div class="lj-explorer"></div>', format="html")]


class EngineRun(_Directive):
    """Placeholder for the in-browser engine run (_static/engine_run.js)."""

    def run(self):
        return [_nodes.raw("", '<div class="engine-run"></div>', format="html")]


class EnginePlay(_Directive):
    """Placeholder for the home page's sliders (_static/engine_run.js)."""

    def run(self):
        return [_nodes.raw("", '<div class="engine-play"></div>', format="html")]


class EngineStates(_Directive):
    """Placeholder for the argon state buttons (_static/engine_run.js)."""

    def run(self):
        return [_nodes.raw("", '<div class="engine-play engine-states"></div>', format="html")]


class EnginePython(_Directive):
    """Placeholder for editable Python that runs in the browser (engine_run.js)."""

    def run(self):
        return [_nodes.raw("", '<div class="engine-run engine-python"></div>', format="html")]


def copy_engine_inputs(app, exception):
    """Give the in-browser run the repository's own files, never a stale copy.

    The compiled engine, _static/engine/_molsim.wasm, comes from
    scripts/wasm/build_pyodide.sh.
    """
    if exception is not None or app.builder.format != "html":
        return
    import shutil
    from pathlib import Path

    root = Path(app.srcdir).parent
    target = Path(app.outdir) / "_static" / "engine"
    target.mkdir(parents=True, exist_ok=True)
    for name in ("data/lj_bulk_simulation.yaml", "data/potentials/lj_bulk.yaml",
                 "data/lj_play_simulation.yaml",
                 "molsimworkbench.py", "website/molsim/examples/sweep.py"):
        shutil.copy(root / name, target)


def setup(app):
    app.add_directive("lj-explorer", LJExplorer)
    app.add_directive("engine-run", EngineRun)
    app.add_directive("engine-python", EnginePython)
    app.add_directive("engine-play", EnginePlay)
    app.add_directive("engine-states", EngineStates)
    app.connect("build-finished", copy_engine_inputs)
