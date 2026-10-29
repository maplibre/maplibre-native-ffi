#!/usr/bin/env bash
set -euo pipefail
preset=${1:?preset required}
root=${MISE_MONOREPO_ROOT:?}
work="$root/build/react-native-spike"
case "$preset" in
  android-arm64-egl) abi=arm64-v8a ;;
  android-x64-egl) abi=x86_64 ;;
  ios-simulator-arm64-metal) abi= ;;
  *) echo "Unsupported React Native test preset: $preset" >&2; exit 2 ;;
esac
node scripts/prepare-react-native.ts "$preset"
if [[ -n "$abi" ]]; then
  (
    cd "$root"
    mise exec --no-deps "java@${MLN_RN_JAVA_BUILD_VERSION:?}" -- ./gradlew \
      -Pmaplibre.localBuild="build/$preset/install" \
      -Pmaplibre.android.backend=opengl -Pmaplibre.android.abis="$abi" \
      :bindings:kotlin:runtimes:opengl:bundleAndroidMainAar
  )
  (
    cd "$work/app/android"
    ./gradlew :app:assembleRelease -PreactNativeArchitectures="$abi" \
      -PmaplibreNativeFfiRuntimeAar="$root/bindings/kotlin/runtimes/opengl/build/outputs/aar/opengl.aar"
  )
else
  (
    cd "$work/app/ios"
    pod install
    xcodebuild -workspace HelloWorld.xcworkspace -scheme HelloWorld \
      -configuration Release -sdk iphonesimulator \
      -destination 'generic/platform=iOS Simulator' -derivedDataPath "$work/derived" \
      CODE_SIGNING_ALLOWED=NO ARCHS=arm64 ONLY_ACTIVE_ARCH=YES
  )
fi
