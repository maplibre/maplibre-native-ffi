#!/usr/bin/env bash
# Runs the Kotlin binding suite in an Android emulator that matches the preset.
set -euo pipefail

preset=${1:?usage: test-kotlin-android-device.sh <preset>}
case "$preset" in
  android-arm64-egl)
    abi=arm64-v8a
    backend=opengl
    ;;
  android-arm64-vulkan)
    abi=arm64-v8a
    backend=vulkan
    ;;
  android-x64-egl)
    abi=x86_64
    backend=opengl
    ;;
  android-x64-vulkan)
    abi=x86_64
    backend=vulkan
    ;;
  *)
    echo "The Kotlin Android device suite cannot test preset $preset." >&2
    exit 2
    ;;
esac

mise run //:android-sdk-packages
# An explicit serial selects a connected device. Otherwise the shared emulator
# that matches the preset boots, and Gradle tests on it alone rather than on
# every connected device.
api=$MLN_FFI_ANDROID_EMULATOR_API
if [[ "$preset" == android-x64-egl ]]; then
  api=26
fi
if [[ -z "${ANDROID_SERIAL:-}" ]]; then
  mise run //:android-emulator:boot "$abi" --api "$api"
fi
ANDROID_SERIAL=$("$MISE_MONOREPO_ROOT/tools/devices/android-emulator" serial "$abi" --api "$api")
export ANDROID_SERIAL
exec ./gradlew \
  -Pmaplibre.android.backend="$backend" \
  -Pmaplibre.android.abis="$abi" \
  -Pmaplibre.localBuild=build \
  -Pmaplibre.android.testMinify=true \
  :bindings:kotlin:connectedAndroidDeviceTest
