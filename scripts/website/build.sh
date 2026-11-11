#!/usr/bin/env bash
# Builds the website into website/_build/html.
#   scripts/website/build.sh           rewrite every page (fast)
#   scripts/website/build.sh --clean   rebuild everything from scratch
# A clean build deletes the folder a preview server runs from, so restart the
# server afterwards:  cd website/_build/html && python3 -m http.server 8000
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/../../website"

# -a rewrites every page, so each one links the current version of the scripts.
# Without it, a page whose text did not change keeps the old link and the
# browser keeps its cached copy of the script.
FLAGS=(-a)
if [[ "${1:-}" == "--clean" ]]; then
  command rm -rf _build/html _build/doctrees
  FLAGS+=(-E)
fi

conda run --no-capture-output -n molsim-docs python -m sphinx "${FLAGS[@]}" -q -b html . _build/html
echo "Built: website/_build/html/index.html"
