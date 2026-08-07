#!/usr/bin/env bash
# Cross-compiles the Rust binding tests for an Android or OpenHarmony x64
# preset and runs them in the emulator, one executable per crate.
set -euo pipefail

preset=${1:?usage: test-rust-device.sh <android|ohos preset>}

case "$preset" in
  android-x64-* | ohos-x64-*) ;;
  android-* | ohos-*)
    echo "The emulators run x64 guests only; check $preset with //bindings/rust:build and run an x64 preset's tests instead." >&2
    exit 2
    ;;
  *)
    echo "Not an Android or OpenHarmony preset: $preset" >&2
    exit 2
    ;;
esac

cd "$MISE_MONOREPO_ROOT"
native_install_dir="$MISE_MONOREPO_ROOT/build/$preset/install"
export MAPLIBRE_NATIVE_C_INSTALL_DIR="$native_install_dir"
# shellcheck source=scripts/rust-cross-env.sh
source "$MISE_MONOREPO_ROOT/scripts/rust-cross-env.sh" "$preset"

test_manifest=$(mktemp)
trap 'find "$test_manifest" -delete' EXIT
cargo test --no-run \
  -p maplibre-native-ffi-sys \
  -p maplibre-native-ffi-core \
  -p maplibre-native-ffi \
  --target "$cargo_target" \
  --message-format=json |
  "$MISE_MONOREPO_ROOT/scripts/cargo-test-executables.py" \
    >"$test_manifest"
cargo clippy \
  -p maplibre-native-ffi-sys \
  -p maplibre-native-ffi-core \
  -p maplibre-native-ffi \
  --target "$cargo_target" \
  --all-targets -- -D warnings

mapfile -t test_binaries < <(grep . "$test_manifest" || true)
if [[ "$preset" == android-* ]]; then
  exec "$MISE_MONOREPO_ROOT/scripts/run-android-emulator-test.sh" \
    180 \
    "$native_install_dir/lib/libmaplibre-native-c.so" \
    --test-threads=1 -- ${test_binaries[@]+"${test_binaries[@]}"}
fi
exec "$MISE_MONOREPO_ROOT/scripts/run-ohos-emulator-test.sh" \
  180 \
  "$native_install_dir/lib/libmaplibre-native-c.so" \
  "$OHOS_SDK_NATIVE/llvm/lib/$compiler_target/libc++_shared.so" \
  --test-threads=1 -- ${test_binaries[@]+"${test_binaries[@]}"}
