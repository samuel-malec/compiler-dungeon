#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if ! command -v emcmake >/dev/null 2>&1; then
    if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
        # shellcheck disable=SC1091
        source "$HOME/emsdk/emsdk_env.sh"
    else
        echo "error: emcmake not found and ~/emsdk is missing." >&2
        echo "Install/activate the Emscripten SDK first, e.g.:" >&2
        echo "  git clone https://github.com/emscripten-core/emsdk.git ~/emsdk" >&2
        echo "  ~/emsdk/emsdk install latest && ~/emsdk/emsdk activate latest" >&2
        exit 1
    fi
fi

emcmake cmake -B build-wasm
cmake --build build-wasm -j
