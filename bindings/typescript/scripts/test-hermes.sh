#!/usr/bin/env bash
# Runs the suite on the Hermes test host for a desktop preset whose addon the
# binding's build task built. The first run builds Hermes from source.
#
#   test-hermes.sh <preset>
set -euo pipefail

preset=${1:?usage: test-hermes.sh <preset>}
cd "$(dirname "$0")/.."
build="$PWD/native/build/$preset"
host="$PWD/native/build/hermes-host"

cmake --build "$build" --target maplibre_native_ffi_test_graphics
host_args=(-G Ninja -DCMAKE_BUILD_TYPE=Release)
if [[ "$(uname)" == Darwin ]]; then
  host_args+=(-DCMAKE_OSX_SYSROOT=macosx)
fi
cmake -S test/hermes/host -B "$host" "${host_args[@]}" >/dev/null
cmake --build "$host"

(
  cd test/hermes
  MAPLIBRE_NATIVE_FFI_HERMES_BUNDLE="$host/bundle" pnpm exec vp pack >/dev/null
)
export MAPLIBRE_NATIVE_FFI_ADDON="$build/install/maplibre_native_ffi.node"
export MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS="$build/maplibre_native_ffi_test_graphics.node"
exec "$host/maplibre_native_ffi_hermes_host" "$host/bundle/suite.iife.js"
