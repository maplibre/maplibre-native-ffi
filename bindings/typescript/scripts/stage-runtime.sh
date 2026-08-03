#!/usr/bin/env bash
# Stages a built Node-API addon into the runtime payload package.
#
# A runtime package owns one compiled artifact and the metadata naming what it
# was built for. The facade reads that metadata to decide whether an installed
# payload matches the host it is running on.
set -euo pipefail

preset="${1:?usage: stage-runtime.sh <preset>}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
package="$root/bindings/typescript/runtime-node"
install_dir="$root/build/$preset/install"

case "$(uname -s)" in
Darwin) library_name="libmaplibre-native-c.dylib" addon_name="libmln_ts_addon.dylib" ;;
MINGW* | MSYS* | CYGWIN*) library_name="maplibre-native-c.dll" addon_name="mln_ts_addon.dll" ;;
*) library_name="libmaplibre-native-c.so" addon_name="libmln_ts_addon.so" ;;
esac

# The preset names the target and how the build reaches its renderer. A payload
# is identified by the public render backend instead, so the context providers
# map onto the backend they drive.
target="${preset%-*}"
case "${preset##*-}" in
egl | wgl | glx) backend=opengl ;;
*) backend="${preset##*-}" ;;
esac

mkdir -p "$package/lib"
cp "$root/target/debug/$addon_name" "$package/maplibre-native-ffi.node"
if [[ -f "$install_dir/lib/$library_name" ]]; then
  cp "$install_dir/lib/$library_name" "$package/lib/$library_name"
elif [[ -f "$install_dir/bin/$library_name" ]]; then
  cp "$install_dir/bin/$library_name" "$package/lib/$library_name"
fi

# The formatter wraps a long define, so the value can sit on the next line.
fingerprint=$(
  tr '\n' ' ' <"$root/bindings/typescript/host-support/generated/fingerprint.h" |
    sed -n 's/.*MLN_ABI_FINGERPRINT_VALUE[^"]*"\([^"]*\)".*/\1/p'
)

cat >"$package/runtime.json" <<JSON
{
  "transport": "node-api",
  "target": "$target",
  "backend": "$backend",
  "abiFingerprint": "$fingerprint",
  "addon": "./maplibre-native-ffi.node"
}
JSON


echo "staged $preset into $package"
