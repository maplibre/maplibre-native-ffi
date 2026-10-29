#!/usr/bin/env bash
set -euo pipefail
preset=${1:?preset required}
root=${MISE_MONOREPO_ROOT:?}
work="$root/build/react-native-spike"
case "$preset" in
  android-arm-egl | android-arm-vulkan) abi=armeabi-v7a ;;
  android-arm64-egl | android-arm64-vulkan) abi=arm64-v8a ;;
  android-x86-egl | android-x86-vulkan) abi=x86 ;;
  android-x64-egl | android-x64-vulkan) abi=x86_64 ;;
  ios-arm64-metal | ios-simulator-arm64-metal) abi= ;;
  *) echo "Unsupported React Native test preset: $preset" >&2; exit 2 ;;
esac
node scripts/prepare-react-native.ts "$preset"
if [[ -n "$abi" ]]; then
  backend=${preset##*-}
  if [[ "$backend" == egl ]]; then backend=opengl; fi
  (
    cd "$root"
    mise exec --no-deps "java@${MLN_RN_JAVA_BUILD_VERSION:?}" -- ./gradlew \
      -Pmaplibre.localBuild="build/$preset/install" \
      -Pmaplibre.android.backend="$backend" -Pmaplibre.android.abis="$abi" \
      ":bindings:kotlin:runtimes:$backend:bundleAndroidMainAar"
  )
  (
    cd "$work/app/android"
    ./gradlew :app:assembleRelease -PreactNativeArchitectures="$abi" \
      -Pmaplibre.android.backend="$backend" \
      -PmaplibreNativeFfiRuntimeAar="$root/bindings/kotlin/runtimes/$backend/build/outputs/aar/$backend.aar"
  )
else
  sdk=iphoneos
  destination='generic/platform=iOS'
  if [[ $preset == ios-simulator-* ]]; then
    sdk=iphonesimulator
    destination='generic/platform=iOS Simulator'
  fi
  (
    cd "$work/app/ios"
    build_ios() {
      xcodebuild "$@" -scheme HelloWorld \
        -configuration Release -sdk "$sdk" \
        -destination "$destination" -derivedDataPath "$work/derived" \
        CODE_SIGNING_ALLOWED=NO ARCHS=arm64 ONLY_ACTIVE_ARCH=YES
    }
    # Prove the packed podspec links, then run the suite through SwiftPM.
    pod install
    build_ios -workspace HelloWorld.xcworkspace
    node ../node_modules/react-native/scripts/setup-apple-spm.js add --deintegrate --yes
    build_ios -project HelloWorld.xcodeproj
  )
fi
