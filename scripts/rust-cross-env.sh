# Sourced by the Rust binding's cross-compilation tasks with a native preset as
# $1. Maps android-*/ohos-* presets to their cargo target and exports the
# cross-compilation environment (bindgen, CC/CXX, linker) from the Android NDK
# or OpenHarmony SDK. Leaves `cargo_target` empty for host presets, where cargo
# picks its own target and toolchain.
# shellcheck shell=bash

cargo_target=

case "$1" in
  android-arm64-*) cargo_target=aarch64-linux-android ;;
  android-x64-*) cargo_target=x86_64-linux-android ;;
  ohos-arm64-*) cargo_target=aarch64-unknown-linux-ohos ;;
  ohos-x64-*) cargo_target=x86_64-unknown-linux-ohos ;;
esac

case "$1" in
  android-*64-*)
    ndk_prebuilt_root="$ANDROID_HOME/ndk/$MLN_FFI_ANDROID_NDK_VERSION/toolchains/llvm/prebuilt"
    ndk_prebuilt=
    for host_prebuilt in "$ndk_prebuilt_root"/*; do
      if [[ -d "$host_prebuilt/bin" ]]; then
        if [[ -n "$ndk_prebuilt" ]]; then
          echo "The pinned Android NDK has multiple host prebuilts under $ndk_prebuilt_root." >&2
          return 2
        fi
        ndk_prebuilt="$host_prebuilt"
      fi
    done
    if [[ -z "$ndk_prebuilt" ]]; then
      echo "The pinned Android NDK has no host prebuilt under $ndk_prebuilt_root." >&2
      return 2
    fi
    target_env="${cargo_target//-/_}"
    export "BINDGEN_EXTRA_CLANG_ARGS_$target_env=--target=$cargo_target --sysroot=$ndk_prebuilt/sysroot"
    export "CC_$target_env=$ndk_prebuilt/bin/${cargo_target}24-clang"
    export "CXX_$target_env=$ndk_prebuilt/bin/${cargo_target}24-clang++"
    # tr rather than ${var^^}: macOS tasks can run under Bash 3.2.
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$ndk_prebuilt/bin/${cargo_target}24-clang"
    ;;
  ohos-*64-*)
    # The OHOS SDK clang target drops the `-unknown` vendor from the cargo target.
    compiler_target="${cargo_target/-unknown/}"
    sysroot="$OHOS_SDK_NATIVE/sysroot"
    target_flags="--target=$compiler_target --sysroot=$sysroot"
    target_env="${cargo_target//-/_}"
    export "BINDGEN_EXTRA_CLANG_ARGS_$target_env=$target_flags -I$sysroot/usr/include/$compiler_target"
    export "CC_$target_env=$OHOS_SDK_NATIVE/llvm/bin/clang $target_flags"
    export "CXX_$target_env=$OHOS_SDK_NATIVE/llvm/bin/clang++ $target_flags"
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$OHOS_SDK_NATIVE/llvm/bin/clang"
    export RUSTFLAGS="-C link-arg=--target=$compiler_target -C link-arg=--sysroot=$sysroot -C link-arg=-fuse-ld=lld"
    ;;
esac
