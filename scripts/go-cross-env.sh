# Sourced by the Go binding's cross-compilation tasks with a native preset as
# $1. Maps android-*/ohos-* presets to GOOS/GOARCH and exports the cross C
# toolchain from the Android NDK or OpenHarmony SDK. Leaves `goos` empty for
# host presets. OpenHarmony has no Go port, so its presets build GOOS=linux
# binaries against the OHOS sysroot; the Oniro emulator runs them.
# shellcheck shell=bash

goos=

case "$1" in
  android-arm-* | android-arm64-* | android-x64-*)
    goos=android
    case "$1" in
      android-arm-*)
        goarch=arm
        compiler_prefix=armv7a-linux-androideabi
        export GOARM=7
        ;;
      android-arm64-*)
        goarch=arm64
        compiler_prefix=aarch64-linux-android
        ;;
      android-x64-*)
        goarch=amd64
        compiler_prefix=x86_64-linux-android
        ;;
    esac
    ndk_bin="$ANDROID_HOME/ndk/$MLN_FFI_ANDROID_NDK_VERSION/toolchains/llvm/prebuilt/linux-x86_64/bin"
    export GOOS="$goos" GOARCH="$goarch" CGO_ENABLED=1
    export CC="$ndk_bin/${compiler_prefix}24-clang"
    export CXX="$ndk_bin/${compiler_prefix}24-clang++"
    ;;
  ohos-arm64-* | ohos-x64-*)
    goos=linux
    if [[ "$1" == ohos-arm64-* ]]; then
      goarch=arm64
      compiler_target=aarch64-linux-ohos
    else
      goarch=amd64
      compiler_target=x86_64-linux-ohos
    fi
    sysroot="$OHOS_SDK_NATIVE/sysroot"
    target_flags="--target=$compiler_target --sysroot=$sysroot"
    export GOOS="$goos" GOARCH="$goarch" CGO_ENABLED=1
    export CC="$OHOS_SDK_NATIVE/llvm/bin/clang $target_flags"
    export CXX="$OHOS_SDK_NATIVE/llvm/bin/clang++ $target_flags"
    ;;
esac
