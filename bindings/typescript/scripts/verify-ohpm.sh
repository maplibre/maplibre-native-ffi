#!/usr/bin/env bash
# Builds a native module using only a HAR's C headers and libraries, for each
# ABI it ships, and checks that it and the addon link the shared C library.
#
#   verify-ohpm.sh <har>
set -euo pipefail

har=${1:?usage: verify-ohpm.sh <har>}
binding="$(cd "$(dirname "$0")/.." && pwd)"
sdk=${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must name the OpenHarmony native SDK}
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
tar -xzf "$har" -C "$work"

for libraries in "$work/package/libs"/*; do
  abi="$(basename "$libraries")"
  cmake -S "$binding/test/ohpm" -B "$work/build/$abi" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$sdk/build/cmake/ohos.toolchain.cmake" \
    -DCMAKE_C_COMPILER_LAUNCHER= \
    -DOHOS_ARCH="$abi" -DMLN_FFI_OHPM_PACKAGE="$work/package"
  cmake --build "$work/build/$abi"
  for module in "$libraries/libmaplibre_native_ffi.so" "$work/build/$abi/libconsumer.so"; do
    "$sdk/llvm/bin/llvm-readelf" --dynamic "$module" |
      grep -F 'Shared library: [libmaplibre-native-c.so]'
  done
done
