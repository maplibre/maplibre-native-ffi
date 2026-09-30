#!/usr/bin/env bash
# Cross-compiles the Go binding tests for an Android preset or OpenHarmony x64
# preset and runs them in the matching emulator, one executable per package.
set -euo pipefail

preset=${1:?usage: test-go-device.sh <android|ohos preset>}

case "$preset" in
  android-arm64-*)
    abi=arm64-v8a
    ;;
  android-x64-*)
    abi=x86_64
    ;;
  android-*)
    echo "The Android emulator runs arm64 and x64 guests; check $preset with //bindings/go:build and test an android-arm64 or android-x64 preset instead." >&2
    exit 2
    ;;
  ohos-x64-egl) ;;
  ohos-x64-*)
    echo "The OpenHarmony emulator runs EGL only; check $preset with //bindings/go:build and test ohos-x64-egl instead." >&2
    exit 2
    ;;
  ohos-*)
    echo "The OpenHarmony emulator runs an x64 guest; check $preset with //bindings/go:build and test ohos-x64-egl instead." >&2
    exit 2
    ;;
  *)
    echo "Not an Android or OpenHarmony preset: $preset" >&2
    exit 2
    ;;
esac

platform=${preset%%-*}
cd "$MISE_MONOREPO_ROOT/bindings/go"
native_install_dir="$MISE_MONOREPO_ROOT/build/$preset/install"
test_dir="$MISE_MONOREPO_ROOT/build/$platform-emulator/$preset/go"
mkdir -p "$test_dir"
find "$test_dir" -maxdepth 1 -type f -name '*.test' -delete

export PKG_CONFIG_PATH="$native_install_dir/share/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
# shellcheck source=scripts/go-cross-env.sh
source "$MISE_MONOREPO_ROOT/scripts/go-cross-env.sh" "$preset"
# shellcheck source=bindings/go/test-tags.sh
source "$MISE_MONOREPO_ROOT/bindings/go/test-tags.sh" "$preset"
go test -c -tags "$go_test_tags" -o "$test_dir/" ./...
go vet -tags "$go_test_tags" ./...
# The render tests load tests/graphics, which travels beside the C API library.
graphics_library="$native_install_dir/lib/libmln_test_graphics.so"

shopt -s nullglob
test_binaries=("$test_dir"/*.test)
if [[ "$platform" == android ]]; then
  emulator_args=()
  if [[ "$preset" == android-x64-egl ]]; then
    emulator_args+=(--api 26)
  fi
  exec "$MISE_MONOREPO_ROOT/scripts/run-android-emulator-test.sh" \
    180 \
    "$abi" \
    "$native_install_dir/lib/libmaplibre-native-c.so" \
    ${emulator_args[@]+"${emulator_args[@]}"} \
    --library "$graphics_library" \
    -test.v -- ${test_binaries[@]+"${test_binaries[@]}"}
fi
exec "$MISE_MONOREPO_ROOT/scripts/run-ohos-emulator-test.sh" \
  180 \
  "$native_install_dir/lib/libmaplibre-native-c.so" \
  "$OHOS_SDK_NATIVE/llvm/lib/$compiler_target/libc++_shared.so" \
  --library "$graphics_library" \
  -test.v -- ${test_binaries[@]+"${test_binaries[@]}"}
