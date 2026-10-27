#!/usr/bin/env bash
# Runs the portable Dart binding suites in a Flutter Android application.
set -euo pipefail

preset=${1:?usage: test-dart-android-device.sh <preset>}
case "$preset" in
  android-arm64-egl | android-arm64-vulkan) abi=arm64-v8a; triple=aarch64-linux-android ;;
  android-x64-egl | android-x64-vulkan) abi=x86_64; triple=x86_64-linux-android ;;
  *) echo "The Dart Android device suite cannot test preset $preset." >&2; exit 2 ;;
esac

mise run //:android-sdk-packages
api=$MLN_FFI_ANDROID_EMULATOR_API
if [[ "$preset" == android-x64-egl ]]; then
  api=26
fi
serial=$("$MISE_MONOREPO_ROOT/tools/devices/android-serial" "$abi" --api "$api")

prefix="$MISE_MONOREPO_ROOT/build/$preset/install"
toolchain=$("$MISE_MONOREPO_ROOT/tools/env/android-ndk-prebuilt.sh")
compiler="$toolchain/bin/${triple}26-clang"
app=.dart_tool/flutter_android_test
flutter create --empty --no-pub --org org.maplibre.nativeffi \
  --platforms android --project-name testhost "$app"
cp tool/flutter_android_test/pubspec.yaml "$app/pubspec.yaml"
cp tool/flutter_android_test/MainActivity.kt "$app/android/app/src/main/kotlin/org/maplibre/nativeffi/testhost/MainActivity.kt"
mkdir -p "$app/integration_test" "$app/android/app/src/main/jniLibs/$abi"
cp tool/flutter_android_test/integration_test/binding_test.dart "$app/integration_test/binding_test.dart"
cp "$prefix/lib/libmln_test_graphics.so" "$app/android/app/src/main/jniLibs/$abi/"
"$compiler" -std=c11 -shared -fPIC -Wall -Wextra -Werror \
  -I"$MISE_MONOREPO_ROOT/include" -I"$MISE_MONOREPO_ROOT/third_party/maplibre-native/include" \
  tool/flutter_android_test/android.c -L"$prefix/lib" -lmaplibre-native-c \
  -o "$app/android/app/src/main/jniLibs/$abi/libmaplibre_dart_test.so"
cd "$app"
flutter pub get
flutter test integration_test/binding_test.dart --device-id "$serial" --timeout 10m
