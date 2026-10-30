# Sourced by Go tasks with a native preset as $1. Exports cross-compilation
# settings and leaves `goos` nonempty when tests need tools/devices/run.
# shellcheck shell=bash

goos=

case "$1" in
  android-arm-* | android-arm64-* | android-x86-* | android-x64-*)
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
      android-x86-*)
        goarch=386
        compiler_prefix=i686-linux-android
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
  linux-musl-arm64-* | linux-musl-x64-* | freebsd-x64-* | linux-*-riscv64-* | linux-*-loongarch64-*)
    if [[ "$1" == freebsd-* ]]; then
      goos=freebsd
    else
      goos=linux
    fi
    case "$1" in
      *-arm64-*) goarch=arm64 ;;
      *-riscv64-*) goarch=riscv64 ;;
      *-loongarch64-*) goarch=loong64 ;;
      *) goarch=amd64 ;;
    esac
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
  windows-x64-* | windows-arm64-* | mingw-x64-*)
    if [[ "$1" == windows-arm64-* ]]; then
      goarch=arm64
    else
      goarch=amd64
    fi
    # cgo takes GNU-style compiler flags, including when it links an MSVC DLL.
    export GOOS=windows GOARCH="$goarch" CGO_ENABLED=1
    export CC="$MLN_FFI_MSYS2_ROOT/bin/clang.exe"
    export CXX="$MLN_FFI_MSYS2_ROOT/bin/clang++.exe"
    export PKG_CONFIG="$MLN_FFI_MSYS2_ROOT/bin/pkgconf.exe"
    msys2_bin_dir="$(cygpath -u "$MLN_FFI_MSYS2_ROOT/bin")"
    export PATH="$msys2_bin_dir${PATH:+:$PATH}"
    ;;
  macos-x64-*)
    export GOARCH=amd64 CGO_ENABLED=1
    export CC="clang -arch x86_64" CXX="clang++ -arch x86_64"
    ;;
  ohos-arm64-* | ohos-x64-*)
    # Go has no OpenHarmony port, so use its Linux port with the OHOS sysroot.
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
