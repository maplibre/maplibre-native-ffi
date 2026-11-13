#!/usr/bin/env bash
# Runs the Kotlin binding suite in an Android emulator that matches the preset,
# with the backend that MLN_FFI_TEST_BACKEND names.
set -euo pipefail

preset=${1:?usage: test-kotlin-android-device.sh <preset>}
case "$preset" in
  android-arm64) abi=arm64-v8a ;;
  android-x86) abi=x86 ;;
  android-x64) abi=x86_64 ;;
  *)
    echo "The Kotlin Android device suite cannot test preset $preset." >&2
    exit 2
    ;;
esac
backend=${MLN_FFI_TEST_BACKEND:?MLN_FFI_TEST_BACKEND must name the backend under test}

mise run //:android-sdk-packages
# Gradle tests on this device alone rather than on every connected device.
ANDROID_SERIAL=$("$MISE_MONOREPO_ROOT/tools/devices/android-serial" "$preset")
export ANDROID_SERIAL
exec ./gradlew \
  -Pmaplibre.android.testBackend="$backend" \
  -Pmaplibre.android.abis="$abi" \
  -Pmaplibre.localBuild=build \
  -Pmaplibre.android.testMinify=true \
  :bindings:kotlin:connectedAndroidDeviceTest
