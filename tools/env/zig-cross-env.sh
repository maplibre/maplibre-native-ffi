#!/usr/bin/env bash
# Sourced by Zig binding tasks with a native preset as $1. Android presets add
# the NDK sysroot and the installed libc metadata, and macOS x64 presets run
# their tests under Rosetta. Other host presets need no extra arguments.
# shellcheck shell=bash

zig_cross_args=()

case "$1" in
  android-*)
    native_install_dir="$MISE_MONOREPO_ROOT/build/$1/install"
    descriptor="$native_install_dir/share/maplibre-native-c/artifact.json"
    if ! ndk_prebuilt="$("$(dirname "${BASH_SOURCE[0]}")/android-ndk-prebuilt.sh")"; then
      exit 2
    fi
    ndk_sysroot="$ndk_prebuilt/sysroot"
    if [[ ! -d "$ndk_sysroot" ]]; then
      echo "The pinned Android NDK sysroot does not exist: $ndk_sysroot" >&2
      exit 2
    fi
    if [[ ! -f "$descriptor" ]]; then
      echo "The Android native artifact has no descriptor: $descriptor" >&2
      exit 2
    fi

    zig_target_triple=$(jq -r .platform.zigTarget "$descriptor")
    if [[ ! "$zig_target_triple" =~ ^(aarch64|x86_64|x86|arm)-linux-android$ ]]; then
      echo "The Android native artifact has an invalid Zig target: $zig_target_triple" >&2
      exit 2
    fi
    if ! zig_api_level=$(jq -er .platform.android.api "$descriptor"); then
      echo "The Android native artifact records no API level: $descriptor" >&2
      exit 2
    fi
    ndk_target_triple=$zig_target_triple
    if [[ "$zig_target_triple" == arm-linux-android ]]; then
      ndk_target_triple=arm-linux-androideabi
    elif [[ "$zig_target_triple" == x86-linux-android ]]; then
      ndk_target_triple=i686-linux-android
    fi
    zig_include_dir="$ndk_sysroot/usr/include/$ndk_target_triple"
    zig_crt_dir="$ndk_sysroot/usr/lib/$ndk_target_triple/$zig_api_level"
    if [[ ! -d "$zig_include_dir" ]]; then
      echo "The Android Zig libc include directory does not exist: $zig_include_dir" >&2
      exit 2
    fi
    if [[ ! -d "$zig_crt_dir" ]]; then
      echo "The Android Zig libc CRT directory does not exist: $zig_crt_dir" >&2
      exit 2
    fi

    zig_libc_dir="$MISE_MONOREPO_ROOT/build/$1/zig"
    mkdir -p "$zig_libc_dir"
    zig_libc="$zig_libc_dir/android-libc"
    zig_libc_tmp=$(mktemp "$zig_libc.XXXXXX")
    {
      printf 'include_dir=%s\n' "$zig_include_dir"
      printf 'sys_include_dir=%s\n' "$ndk_sysroot/usr/include"
      printf 'crt_dir=%s\n' "$zig_crt_dir"
      # Zig 0.17 requires every LibCInstallation field and a non-empty cc_dir on
      # Linux ABIs. The NDK CRT directory holds crtbegin_dynamic.o.
      printf 'cc_dir=%s\n' "$zig_crt_dir"
      printf 'msvc_lib_dir=\n'
      printf 'kernel32_lib_dir=\n'
      printf 'darwin_sdk_dir=\n'
    } > "$zig_libc_tmp"
    mv "$zig_libc_tmp" "$zig_libc"
    zig_cross_args=(
      -Dsystem-root="$ndk_sysroot"
      -Dtarget-libc="$zig_libc"
    )
    ;;
  macos-x64-*)
    zig_cross_args=(-frosetta)
    ;;
esac
