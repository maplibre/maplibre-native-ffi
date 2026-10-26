#!/usr/bin/env bash
# Cross-compiles the Rust binding tests for an Android, OpenHarmony x64, or Apple
# simulator preset and runs them in the matching emulator or simulator, one
# executable per crate.
set -euo pipefail

preset=${1:?usage: test-rust-device.sh <android|ohos|ios-simulator|visionos-simulator preset>}

case "$preset" in
  android-arm64-*)
    abi=arm64-v8a
    ;;
  android-x64-*)
    abi=x86_64
    ;;
  android-*)
    echo "The Android emulator runs arm64 and x64 guests; check $preset with //bindings/rust:build and test an android-arm64 or android-x64 preset instead." >&2
    exit 2
    ;;
  ohos-x64-egl) ;;
  ohos-x64-*)
    echo "The OpenHarmony emulator runs EGL only; check $preset with //bindings/rust:build and test ohos-x64-egl instead." >&2
    exit 2
    ;;
  ohos-*)
    echo "The OpenHarmony emulator runs an x64 guest; check $preset with //bindings/rust:build and test ohos-x64-egl instead." >&2
    exit 2
    ;;
  ios-simulator-* | visionos-simulator-*) ;;
  *)
    echo "Not an Android, OpenHarmony, or Apple simulator preset: $preset" >&2
    exit 2
    ;;
esac

cd "$MISE_MONOREPO_ROOT"
native_install_dir="$MISE_MONOREPO_ROOT/build/$preset/install"
export MAPLIBRE_NATIVE_FFI_LOCAL_BUILD="$native_install_dir"
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

# Each test binary gets ten minutes on the guest, where the suite renders in
# software and the whole binding suite runs single-threaded.
timeout_seconds=600
# The render tests load their GPU contexts from tests/graphics, which travels
# beside the C API library.
graphics_library="$native_install_dir/lib/libmln_test_graphics.so"

# A while loop rather than mapfile: macOS tasks can run under Bash 3.2.
test_binaries=()
while IFS= read -r test_binary || [[ -n "$test_binary" ]]; do
  if [[ -n "$test_binary" ]]; then
    test_binaries+=("$test_binary")
  fi
done <"$test_manifest"
if [[ "$preset" == ios-simulator-* || "$preset" == visionos-simulator-* ]]; then
  # The simulator reads the host's file system, so the test binaries locate the
  # C API library in the install as the host suite does. simctl passes each
  # SIMCTL_CHILD_ variable to the process without the prefix.
  export SIMCTL_CHILD_DYLD_LIBRARY_PATH="$native_install_dir/lib"
  if [[ "$preset" == visionos-* ]]; then
    export MLN_FFI_SIMULATOR_RUNTIME=visionOS
    mise run //:visionos-simulator:boot
  else
    mise run //:ios-simulator:boot
  fi
  for test_binary in ${test_binaries[@]+"${test_binaries[@]}"}; do
    "$MISE_MONOREPO_ROOT/scripts/run-ios-simulator-test.sh" \
      "$test_binary" "$timeout_seconds"
  done
  exit 0
fi
if [[ "$preset" == android-* ]]; then
  emulator_args=()
  if [[ "$preset" == android-x64-egl ]]; then
    emulator_args+=(--api 26)
  fi
  exec "$MISE_MONOREPO_ROOT/scripts/run-android-emulator-test.sh" \
    "$timeout_seconds" \
    "$abi" \
    "$native_install_dir/lib/libmaplibre-native-c.so" \
    ${emulator_args[@]+"${emulator_args[@]}"} \
    --library "$graphics_library" \
    --test-threads=1 -- ${test_binaries[@]+"${test_binaries[@]}"}
fi
exec "$MISE_MONOREPO_ROOT/scripts/run-ohos-emulator-test.sh" \
  "$timeout_seconds" \
  "$native_install_dir/lib/libmaplibre-native-c.so" \
  "$OHOS_SDK_NATIVE/llvm/lib/$compiler_target/libc++_shared.so" \
  --library "$graphics_library" \
  --test-threads=1 -- ${test_binaries[@]+"${test_binaries[@]}"}
