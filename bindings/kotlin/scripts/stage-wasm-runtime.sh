#!/usr/bin/env bash
# Stages a web preset's browser package with the runtime's module object
# (runtimes/common/src/wasmJsMain/resources) for the Kotlin/Wasm suite, and
# prints the directory. test-runtime.mjs stands in for the runtime's index.mjs:
# it loads the package from the directory that Karma serves, so that webpack
# leaves the package, whose tile worker script and shaper module load by URLs
# relative to its own, as it is.
#
# Usage: stage-wasm-runtime.sh <web preset>, from the repository root.
set -euo pipefail

preset="$1"
package="build/$preset/install/share/maplibre-native-c/web"
staging="$PWD/build/kotlin-wasm/$preset"
if [[ ! -f "$package/maplibre_native_c.js" ]]; then
  echo "error: $package holds no browser package; run mise run //:build $preset" >&2
  exit 1
fi
rm -rf "$staging"
mkdir -p "$staging"
cp -R "$package/." "$staging/"
cp bindings/kotlin/runtimes/common/src/wasmJsMain/resources/runtime.mjs "$staging/"
cat > "$staging/test-runtime.mjs" <<SCRIPT
import { instantiateFrom } from "./runtime.mjs";

export default async function instantiate(options) {
  // Karma serves files outside its base path under /absolute.
  const bindings = await import(/* webpackIgnore: true */ "/absolute$staging/maplibre_native_c.js");
  return instantiateFrom(bindings, options);
}
SCRIPT
printf '%s\n' "$staging"
