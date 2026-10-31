#!/usr/bin/env bash
# Runs the portable Dart binding suites in a Flutter Android application.
set -euo pipefail

preset=${1:?usage: test-dart-android-device.sh <preset>}
case "$preset" in
  android-arm64-egl | android-arm64-vulkan) abi=arm64-v8a ;;
  android-x64-egl | android-x64-vulkan) abi=x86_64 ;;
  *) echo "The Dart Android device suite cannot test preset $preset." >&2; exit 2 ;;
esac

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
flutter test integration_test/binding_test.dart --device-id "$serial" --timeout 10m
