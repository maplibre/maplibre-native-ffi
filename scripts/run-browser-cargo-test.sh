#!/usr/bin/env bash
# Cargo target runner for wasm32-unknown-emscripten.
#
# Cargo hands this the emcc module it just linked plus the arguments meant for
# libtest. scripts/run-browser-test.mjs takes it from there; this only reshapes
# the command line and reports which backend the artifact was built for, so the
# runner can select the browser flags that backend needs.
set -euo pipefail

module="$1"
shift

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

module_args=()
for argument in "$@"; do
  module_args+=(--module-arg "$argument")
done

backend_args=()
descriptor="${MAPLIBRE_NATIVE_C_INSTALL_DIR:-}/share/maplibre-native-c/artifact.json"
if [[ -f "$descriptor" ]]; then
  backend=$(sed -n 's/.*"renderBackend": *"\([^"]*\)".*/\1/p' "$descriptor")
  if [[ -n "$backend" ]]; then
    backend_args=(--render-backend "$backend")
  fi
fi

exec node "$repository_root/scripts/run-browser-test.mjs" \
  "$module" \
  --timeout-seconds 300 \
  "${backend_args[@]}" \
  "${module_args[@]}"
