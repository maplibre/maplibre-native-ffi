#!/usr/bin/env bash
# Cross-compiles the Go binding tests for an Android or OpenHarmony x64 preset
# and runs them in the emulator, one executable per package.
set -euo pipefail

preset=${1:?usage: test-go-device.sh <android|ohos preset>}

case "$preset" in
  android-x64-* | ohos-x64-*) ;;
  android-* | ohos-*)
    echo "The emulators run x64 guests only; check $preset with //bindings/go:build and run an x64 preset's tests instead." >&2
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
go test -c -o "$test_dir/" ./...
go vet ./...

shopt -s nullglob
test_binaries=("$test_dir"/*.test)
if [[ "$platform" == android ]]; then
  exec "$MISE_MONOREPO_ROOT/scripts/run-android-emulator-test.sh" \
    180 \
    "$native_install_dir/lib/libmaplibre-native-c.so" \
    -test.v -- ${test_binaries[@]+"${test_binaries[@]}"}
fi
exec "$MISE_MONOREPO_ROOT/scripts/run-ohos-emulator-test.sh" \
  180 \
  "$native_install_dir/lib/libmaplibre-native-c.so" \
  "$OHOS_SDK_NATIVE/llvm/lib/$compiler_target/libc++_shared.so" \
  -test.v -- ${test_binaries[@]+"${test_binaries[@]}"}
