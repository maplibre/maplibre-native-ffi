#!/usr/bin/env bash
# Runs the portable Dart binding suites in a Flutter Android application on an
# emulator that matches the preset, with the backend that MLN_FFI_TEST_BACKEND
# names.
set -euo pipefail

preset=${1:?usage: test-dart-android-device.sh <preset>}
case "$preset" in
  android-arm64) abi=arm64-v8a ;;
  android-x64) abi=x86_64 ;;
  *) echo "The Dart Android device suite cannot test preset $preset." >&2; exit 2 ;;
esac
backend=${MLN_FFI_TEST_BACKEND:?MLN_FFI_TEST_BACKEND must name the backend under test}

mise run //:android-sdk-packages
serial=$("$MISE_MONOREPO_ROOT/tools/devices/android-serial" "$preset")

prefix="$MISE_MONOREPO_ROOT/build/$preset/install"
app=.dart_tool/flutter_android_test
flutter create --empty --no-pub --org org.maplibre.nativeffi \
  --platforms android --project-name testhost "$app"
cp tool/flutter_android_test/pubspec.yaml "$app/pubspec.yaml"
mkdir -p "$app/integration_test" "$app/android/app/src/main/jniLibs/$abi"
cp tool/flutter_android_test/integration_test/binding_test.dart "$app/integration_test/binding_test.dart"
cp "$prefix/lib/libmln_test_graphics.so" "$app/android/app/src/main/jniLibs/$abi/"
cd "$app"
flutter pub get
# The app runs without this environment, so the backend under test is a define.
flutter test integration_test/binding_test.dart --device-id "$serial" --timeout 10m \
  --dart-define=MLN_FFI_TEST_BACKEND="$backend"
