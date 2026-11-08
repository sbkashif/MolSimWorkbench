#!/usr/bin/env bash
# Build the website, open it in the default browser, and keep it up to date.
#
#   website/preview.sh               # opens the home page
#   website/preview.sh agent/index   # opens one page
#   CLEAN=1 website/preview.sh       # rebuilds every page first
#
# While it runs, saved changes to pages and figures are rebuilt within a few
# seconds: refresh the browser to see them. Press Ctrl-C to stop.
# Set PORT to use another port.
set -euo pipefail

cd "$(dirname "$0")"
page="${1:-index}"
port="${PORT:-8000}"
url="http://localhost:${port}/${page}.html"
stamp=_build/.preview_stamp

build() { conda run -n molsim-docs python -m sphinx -q "$@" -b html . _build/html; }

mkdir -p _build
touch "${stamp}"
if [ -n "${CLEAN:-}" ]; then build -E; else build; fi

watch() {
    while sleep 2; do
        changed=$(find . -path ./_build -prune -o -newer "${stamp}" -type f \
            \( -name '*.md' -o -name '*.svg' -o -name '*.css' -o -name '*.js' \) -print -quit)
        if [ -n "${changed}" ]; then
            touch "${stamp}"
            build && echo "Rebuilt after ${changed}: refresh the browser"
        fi
    done
}
watch &
watcher=$!
trap 'kill "${watcher}" 2>/dev/null' EXIT

echo "Serving ${url}  (Ctrl-C to stop)"
[ -n "${NO_OPEN:-}" ] || (sleep 1 && open "${url}") &
python3 -m http.server "${port}" --bind 127.0.0.1 --directory _build/html
