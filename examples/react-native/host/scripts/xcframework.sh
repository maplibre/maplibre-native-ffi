#!/usr/bin/env bash
# Combines the iOS builds of the C library that exist into the XCFramework that
# the podspec vendors. Build them with `mise run //:build ios-arm64-metal` for
# devices and `mise run //:build ios-simulator-arm64-metal` for the simulator.
set -euo pipefail

host=$(cd "$(dirname "$0")/.." && pwd -P)
root=$(cd "$host/../../.." && pwd -P)
output="$host/build/MaplibreNativeC.xcframework"

frameworks=()
for preset in ios-arm64-metal ios-simulator-arm64-metal; do
  framework="$root/build/$preset/install/Frameworks/MaplibreNativeC.framework"
  if [[ -d "$framework" ]]; then
    frameworks+=(-framework "$framework")
  fi
done
if [[ ${#frameworks[@]} -eq 0 ]]; then
  echo "error: no iOS build of the C library. Build one with:" >&2
  echo "  mise run //:build ios-simulator-arm64-metal" >&2
  exit 1
fi

rm -rf "$output"
xcodebuild -create-xcframework "${frameworks[@]}" -output "$output"
