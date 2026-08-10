#!/usr/bin/env bash
# Sourced by Zig binding tasks with a native preset as $1. Android presets add
# the NDK sysroot and the installed libc metadata. Host presets need no extra
# arguments.
# shellcheck shell=bash

zig_cross_args=()

case "$1" in
  android-*)
    native_install_dir="$MISE_MONOREPO_ROOT/build/$1/install"
    descriptor="$native_install_dir/share/maplibre-native-c/artifact.json"
    ndk_prebuilt_root="$ANDROID_HOME/ndk/$MLN_FFI_ANDROID_NDK_VERSION/toolchains/llvm/prebuilt"
    ndk_host_prebuilts=()
    while IFS= read -r ndk_host_prebuilt; do
      ndk_host_prebuilts+=("$ndk_host_prebuilt")
    done < <(find "$ndk_prebuilt_root" -mindepth 1 -maxdepth 1 -type d -print 2>/dev/null | sort)
    if (( ${#ndk_host_prebuilts[@]} != 1 )); then
      echo "The pinned Android NDK must contain one host prebuilt under $ndk_prebuilt_root." >&2
      exit 2
    fi
    ndk_sysroot="${ndk_host_prebuilts[0]}/sysroot"
    if [[ ! -d "$ndk_sysroot" ]]; then
      echo "The pinned Android NDK sysroot does not exist: $ndk_sysroot" >&2
      exit 2
    fi
    if [[ ! -f "$descriptor" ]]; then
      echo "The Android native artifact has no descriptor: $descriptor" >&2
      exit 2
    fi

    zig_target=$(sed -n 's/^[[:space:]]*"zigTarget":[[:space:]]*"\([^"]*\)".*/\1/p' "$descriptor")
    if [[ ! "$zig_target" =~ ^(aarch64|x86_64)-linux-android\.([0-9]+)$ ]]; then
      echo "The Android native artifact has an invalid Zig target: $zig_target" >&2
      exit 2
    fi
    zig_target_triple="${BASH_REMATCH[1]}-linux-android"
    zig_api_level=${BASH_REMATCH[2]}
    zig_include_dir="$ndk_sysroot/usr/include/$zig_target_triple"
    zig_crt_dir="$ndk_sysroot/usr/lib/$zig_target_triple/$zig_api_level"
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
      printf 'msvc_lib_dir=\nkernel32_lib_dir=\ngcc_dir=\n'
    } > "$zig_libc_tmp"
    mv "$zig_libc_tmp" "$zig_libc"
    zig_cross_args=(
      -Dsystem-root="$ndk_sysroot"
      -Ddependency-library-dir="$zig_crt_dir"
      --libc "$zig_libc"
    )
    ;;
esac
