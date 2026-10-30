# Sourced by Swift tasks after native-env.sh and swift-libxml2-env.sh.
# shellcheck shell=bash

case "$usage_preset" in
  android-arm-*) swift_android_arch=armv7 ndk_arch=arm-linux-androideabi ;;
  android-arm64-*) swift_android_arch=aarch64 ndk_arch=aarch64-linux-android ;;
  android-x64-*) swift_android_arch=x86_64 ndk_arch=x86_64-linux-android ;;
  *)
    echo "The Swift Android SDK supports android-arm, android-arm64, and android-x64 presets." >&2
    exit 2
    ;;
esac

mise run --no-deps //bindings/swift:android-sdk
export ANDROID_NDK_HOME="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/ndk/${MLN_FFI_ANDROID_NDK_VERSION}"
ndk_prebuilt=$("$MISE_MONOREPO_ROOT/tools/env/android-ndk-prebuilt.sh")
swift_android_triple="$swift_android_arch-unknown-linux-android26"
swift_android_configuration=$(swift sdk configure "$MLN_FFI_SWIFT_ANDROID_SDK" "$swift_android_triple" --show-configuration)
swift_android_sysroot=$(sed -n 's/^sdkRootPath: //p' <<<"$swift_android_configuration")
swift_android_resources=$(sed -n 's/^swiftResourcesPath: //p' <<<"$swift_android_configuration")
# The SDK's setup script connects its sysroot and Clang headers to the local
# NDK. Reconnect only after the NDK changes; the script replaces the sysroot.
if [[ "$(readlink "$swift_android_sysroot/usr/include" || true)" != "$ndk_prebuilt/sysroot/usr/include" ]]; then
  bash "$(dirname "$swift_android_sysroot")/scripts/setup-android-sdk.sh"
fi
swift_android_cxx="$ndk_prebuilt/sysroot/usr/lib/$ndk_arch/libc++_shared.so"
swift_android_args=(
  --build-system native
  --swift-sdk "$MLN_FFI_SWIFT_ANDROID_SDK"
  --triple "$swift_android_triple"
  --scratch-path ".build/$usage_preset"
  --pkg-config-path "$native_install_dir/share/pkgconfig"
)
