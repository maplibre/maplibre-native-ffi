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
  android-x86-egl)
    abi=x86
    backend=opengl
    ;;
  android-x86-vulkan)
    abi=x86
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
# Gradle tests on this device alone rather than on every connected device.
ANDROID_SERIAL=$("$MISE_MONOREPO_ROOT/tools/devices/android-serial" "$preset")
export ANDROID_SERIAL
exec ./gradlew \
  -Pmaplibre.android.backend="$backend" \
  -Pmaplibre.android.abis="$abi" \
  -Pmaplibre.localBuild=build \
  -Pmaplibre.android.testMinify=true \
  :bindings:kotlin:connectedAndroidDeviceTest
