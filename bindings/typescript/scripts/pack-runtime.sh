#!/usr/bin/env bash
# Packs a built Node-API addon as the runtime payload package for its target.
#
# `stage-runtime.sh` writes one payload into the checkout, which is what a
# development build loads. A published payload instead has to say in its name
# which target and render backend it carries, because the facade discovers one
# by importing the packages it knows and a host can only run the payload built
# for it. This produces that package, and the tarball a release uploads.
set -euo pipefail

preset="${1:?usage: pack-runtime.sh <preset>}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
install_dir="$root/build/$preset/install"
version="${MLN_TS_VERSION:-0.0.0}"

# The preset names the target and how the build reaches its renderer. A payload
# is named for the public render backend instead, so the context providers map
# onto the backend they drive.
target="${preset%-*}"
case "${preset##*-}" in
egl | wgl | glx) backend=opengl ;;
*) backend="${preset##*-}" ;;
esac

case "$preset" in
ohos-*)
  # OpenHarmony cross-builds through its own script, which has already staged
  # the payload the device loads.
  source_dir="$root/bindings/typescript/runtime-arkts"
  addon=maplibre-native-ffi.so
  entry=index.ets
  ;;
*)
  source_dir="$root/bindings/typescript/runtime-node"
  addon=maplibre-native-ffi.node
  entry=index.js
  ;;
esac

if [[ ! -f "$source_dir/$addon" ]]; then
  echo "no staged payload at $source_dir/$addon; build the binding for $preset first" >&2
  exit 1
fi

name="@maplibre/native-ffi-runtime-$target-$backend"
out_dir="$root/build/packages/typescript"
package="$out_dir/$target-$backend"

rm -rf "$package"
mkdir -p "$package/lib" "$out_dir/dist"
cp "$source_dir/$addon" "$package/"
cp "$source_dir/$entry" "$source_dir/index.d.ts" "$package/"
cp "$source_dir/runtime.json" "$package/"
if compgen -G "$source_dir/lib/*" >/dev/null; then
  cp "$source_dir"/lib/* "$package/lib/"
fi

# The payload the facade imports has to agree with the one it was built beside,
# so the metadata is taken from what was staged rather than written twice.
python3 - "$package/runtime.json" "$target" "$backend" <<'PYTHON'
import json
import sys

path, target, backend = sys.argv[1:4]
with open(path) as handle:
    runtime = json.load(handle)
if runtime["target"] != target or runtime["backend"] != backend:
    raise SystemExit(
        f"the staged payload reports {runtime['target']}-{runtime['backend']}, "
        f"and this packs {target}-{backend}; build the binding for this preset first"
    )
PYTHON

python3 - "$package/package.json" "$name" "$version" "$entry" "$addon" <<'PYTHON'
import json
import sys

path, name, version, entry, addon = sys.argv[1:6]
extension = addon.rsplit(".", 1)[1]
# ArkTS resolves a module through its own build, so its payload names the ArkTS
# source; every other runtime resolves through node_modules.
exports = (
    {"types": "./index.d.ts", "default": f"./{entry}"}
    if entry.endswith(".ets")
    else {"types": "./index.d.ts", "import": f"./{entry}"}
)
with open(path, "w") as handle:
    json.dump(
        {
            "name": name,
            "version": version,
            "type": "module",
            "description": (
                "Compiled MapLibre Native runtime payload for the TypeScript binding."
            ),
            "license": "BSD-2-Clause",
            "exports": {".": exports, "./runtime.json": "./runtime.json"},
            "files": [entry, "index.d.ts", "runtime.json", f"*.{extension}", "lib"],
        },
        handle,
        indent=2,
    )
PYTHON

# `npm pack` names the tarball after the package and version, and reports the
# name it chose, which is what an upload step needs.
tarball="$(cd "$package" && npm pack --pack-destination "$out_dir/dist" --silent)"
echo "packed $name@$version for $preset into $out_dir/dist/$tarball"
