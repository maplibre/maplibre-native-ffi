#!/usr/bin/env bash
# Copies what the npm package carries for iOS into this workspace package: the
# binding's native sources, and an XCFramework of the iOS builds of the C
# library that exist. Build the library with
# `mise run //:build ios-arm64` for devices and
# `mise run //:build ios-simulator-arm64` for the simulator.
set -euo pipefail

package=$(cd "$(dirname "$0")/.." && pwd -P)
root=$(cd "$package/../../.." && pwd -P)
binding="$root/bindings/typescript"

frameworks=()
for preset in ios-arm64 ios-simulator-arm64; do
  framework="$root/build/$preset/install/Frameworks/MaplibreNativeC.framework"
  if [[ -d "$framework" ]]; then
    frameworks+=(-framework "$framework")
  fi
done
if [[ ${#frameworks[@]} -eq 0 ]]; then
  echo "error: no iOS build of the C library. Build one with:" >&2
  echo "  mise run //:build ios-simulator-arm64" >&2
  exit 1
fi

# The same files that scripts/pack.ts puts in the npm package.
rm -rf "$package/native"
mkdir -p "$package/native"
for name in generated_addon.c generated_values.c generated_addon.h runtime.c \
  runtime.h realm.c mutex.h hermes; do
  cp -R "$binding/native/$name" "$package/native/$name"
done
cp -R "$root/include" "$package/native/include"
cp -R "$binding/node_modules/node-api-headers/include" \
  "$package/native/node-api-headers"

rm -rf "$package/ios/MaplibreNativeC.xcframework"
xcodebuild -create-xcframework "${frameworks[@]}" \
  -output "$package/ios/MaplibreNativeC.xcframework"
