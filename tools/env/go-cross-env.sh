# Sourced by the Go binding's cross-compilation tasks with a native preset as
# $1. Maps musl, FreeBSD, Android, macOS x64, and OpenHarmony presets to
# GOOS/GOARCH and exports the cross C toolchain from the native build's Zig
# wrappers, the Android NDK, Apple Clang, or the OpenHarmony SDK. Leaves `goos`
# empty for presets that run on the host, which macOS x64 does under Rosetta. OpenHarmony has no
# Go port, so its presets build GOOS=linux binaries against the OHOS sysroot;
# the Oniro emulator runs them.
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
    if ! ndk_prebuilt="$("$(dirname "${BASH_SOURCE[0]}")/android-ndk-prebuilt.sh")"; then
      return 2
    fi
    ndk_bin="$ndk_prebuilt/bin"
    export GOOS="$goos" GOARCH="$goarch" CGO_ENABLED=1
    export CC="$ndk_bin/${compiler_prefix}24-clang"
    export CXX="$ndk_bin/${compiler_prefix}24-clang++"
    ;;
  linux-musl-arm64-* | linux-musl-x64-* | freebsd-x64-*)
    if [[ "$1" == freebsd-* ]]; then
      goos=freebsd
    else
      goos=linux
    fi
    if [[ "$1" == linux-musl-arm64-* ]]; then
      goarch=arm64
    else
      goarch=amd64
    fi
    compiler="$MISE_MONOREPO_ROOT/build/$1/zig-shim/zig-cc"
    compiler_cxx="$MISE_MONOREPO_ROOT/build/$1/zig-shim/zig-c++"
    if [[ ! -x "$compiler" || ! -x "$compiler_cxx" ]]; then
      echo "The Zig compiler wrappers do not exist for $1; run mise run build $1 first." >&2
      return 2
    fi
    # The binding links the C API's shared library, which makes Zig link musl
    # dynamically, so the test binary can load Alpine's graphics drivers.
    export GOOS="$goos" GOARCH="$goarch" CGO_ENABLED=1
    export CC="$compiler" CXX="$compiler_cxx"
    ;;
  macos-x64-*)
    export GOARCH=amd64 CGO_ENABLED=1
    export CC="clang -arch x86_64" CXX="clang++ -arch x86_64"
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
    # The OHOS loader leaves Go's internal function-pointer relocations
    # unresolved in PIE executables. Link device binaries as ET_EXEC instead.
    export CGO_LDFLAGS="${CGO_LDFLAGS:-} -no-pie"
    ;;
esac
