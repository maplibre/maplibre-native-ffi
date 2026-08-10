#!/usr/bin/env bash
# Sourced by Zig binding tasks with a native preset as $1. Android presets add
# the NDK sysroot and the installed libc metadata. Host presets need no extra
# arguments.
# shellcheck shell=bash

zig_cross_args=()

case "$1" in
  android-*)
    native_install_dir="$MISE_MONOREPO_ROOT/build/$1/install"
    ndk_sysroot="$ANDROID_HOME/ndk/$MLN_FFI_ANDROID_NDK_VERSION/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
    zig_libc="$native_install_dir/share/maplibre-native-c/zig-libc"
    if [[ ! -d "$ndk_sysroot" ]]; then
      echo "The pinned Android NDK sysroot does not exist: $ndk_sysroot" >&2
      exit 2
    fi
    if [[ ! -f "$zig_libc" ]]; then
      echo "The Android native artifact has no Zig libc metadata: $zig_libc" >&2
      exit 2
    fi
    zig_crt_dir=$(sed -n 's/^crt_dir=//p' "$zig_libc")
    if [[ ! -d "$zig_crt_dir" ]]; then
      echo "The Android Zig libc CRT directory does not exist: $zig_crt_dir" >&2
      exit 2
    fi
    zig_cross_args=(
      -Dsystem-root="$ndk_sysroot"
      -Ddependency-library-dir="$zig_crt_dir"
      --libc "$zig_libc"
    )
    ;;
esac
