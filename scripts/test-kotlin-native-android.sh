#!/usr/bin/env bash
# Runs Kotlin/Native's common and native binding tests as an Android executable.
set -euo pipefail

preset=${1:?usage: test-kotlin-native-android.sh <preset>}
case "$preset" in
  android-arm64-egl | android-arm64-vulkan) abi=arm64-v8a; target=androidNativeArm64 ;;
  android-x64-egl | android-x64-vulkan) abi=x86_64; target=androidNativeX64 ;;
  *) echo "The Kotlin/Native Android suite cannot test preset $preset." >&2; exit 2 ;;
esac

mise run //:android-sdk-packages --native-only
# Use the project's NDK linker, which also runs natively on Apple Silicon.
# Kotlin's bundled Android toolchain contains an Intel-only macOS linker.
toolchain=$("$MISE_MONOREPO_ROOT/scripts/android-ndk-prebuilt.sh")
./gradlew \
  -PmaplibreNativeCInstallDir="build/$preset/install" \
  -Pmaplibre.android.nativeTestToolchain="$toolchain" \
  ":bindings:kotlin:linkDebugTest${target/androidNative/AndroidNative}"
runner_args=(300 "$abi" "build/$preset/install/lib/libmaplibre-native-c.so")
if [[ "$preset" == android-x64-egl ]]; then
  runner_args+=(--api 26)
fi
runner_args+=(--library "build/$preset/install/lib/libmln_test_graphics.so")
exec "$MISE_MONOREPO_ROOT/scripts/run-android-emulator-test.sh" \
  "${runner_args[@]}" -- "bindings/kotlin/build/bin/$target/debugTest/test.kexe"
