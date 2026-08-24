# Sourced by the Rust binding's cross-compilation tasks with a native preset as
# $1. Maps musl, Android, and OpenHarmony presets to their Cargo target. Exports
# the cross-compilation environment (bindgen, CC/CXX, linker) from the Zig and
# Rust toolchains, Android NDK, or OpenHarmony SDK. Leaves `cargo_target` empty
# for host presets, where Cargo picks its own target and toolchain.
# shellcheck shell=bash

cargo_target=

case "$1" in
  android-arm-*) cargo_target=armv7-linux-androideabi ;;
  android-arm64-*) cargo_target=aarch64-linux-android ;;
  android-x64-*) cargo_target=x86_64-linux-android ;;
  linux-musl-arm64-*) cargo_target=aarch64-unknown-linux-musl ;;
  linux-musl-x64-*) cargo_target=x86_64-unknown-linux-musl ;;
  ohos-arm64-*) cargo_target=aarch64-unknown-linux-ohos ;;
  ohos-x64-*) cargo_target=x86_64-unknown-linux-ohos ;;
esac

case "$1" in
  linux-musl-*64-*)
    target_env="${cargo_target//-/_}"
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    compiler="$MISE_MONOREPO_ROOT/build/$1/zig-shim/zig-cc"
    compiler_cxx="$MISE_MONOREPO_ROOT/build/$1/zig-shim/zig-c++"
    if [[ ! -x "$compiler" || ! -x "$compiler_cxx" ]]; then
      echo "The musl Zig compiler wrappers do not exist for $1; run mise run build $1 first." >&2
      return 2
    fi
    rust_sysroot="$(rustc --print sysroot)"
    rust_host="$(rustc --print host-tuple)"
    linker="$rust_sysroot/lib/rustlib/$rust_host/bin/rust-lld"
    if [[ ! -x "$linker" ]]; then
      echo "The Rust linker does not exist: $linker" >&2
      return 2
    fi
    rustflags_variable="CARGO_TARGET_${target_env_upper}_RUSTFLAGS"
    rustflags="${!rustflags_variable:-}"
    rustflags="$rustflags -C linker-flavor=ld.lld"
    if [[ "${MLN_FFI_RUST_MUSL_DYNAMIC:-}" == 1 ]]; then
      dynamic_dir="$MISE_MONOREPO_ROOT/build/$1/zig-shim/rust-dynamic"
      mkdir -p "$dynamic_dir"
      if [[ ! -f "$dynamic_dir/libc.so" ]]; then
        if command -v docker >/dev/null; then
          container_engine=docker
        elif command -v podman >/dev/null; then
          container_engine=podman
        else
          echo "Dynamic musl Rust tests require Docker or Podman to prepare the linker sysroot." >&2
          return 2
        fi
        "$container_engine" run --rm \
          --volume "$dynamic_dir:/out" \
          alpine:3.22 \
          sh -euc 'cp -L /lib/ld-musl-*.so.1 /out/libc.so'
      fi
      ln -sf libc.so "$dynamic_dir/libgcc_s.so"
      case "$cargo_target" in
        aarch64-*) dynamic_loader=/lib/ld-musl-aarch64.so.1 ;;
        x86_64-*) dynamic_loader=/lib/ld-musl-x86_64.so.1 ;;
      esac
      rustflags="$rustflags -C target-feature=-crt-static -C link-self-contained=yes -C link-arg=--dynamic-linker=$dynamic_loader -L native=$dynamic_dir"
    fi
    export "CC_$target_env=$compiler"
    export "CXX_$target_env=$compiler_cxx"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$linker"
    export "CARGO_TARGET_${target_env_upper}_RUNNER=$MISE_MONOREPO_ROOT/scripts/run-musl-test.sh $1"
    export "$rustflags_variable=${rustflags# }"
    ;;
  android-*)
    ndk_prebuilt="$ANDROID_HOME/ndk/$MLN_FFI_ANDROID_NDK_VERSION/toolchains/llvm/prebuilt/linux-x86_64"
    compiler_target="$cargo_target"
    if [[ "$cargo_target" == armv7-linux-androideabi ]]; then
      compiler_target=armv7a-linux-androideabi
    fi
    target_env="${cargo_target//-/_}"
    export "BINDGEN_EXTRA_CLANG_ARGS_$target_env=--target=$cargo_target --sysroot=$ndk_prebuilt/sysroot"
    export "CC_$target_env=$ndk_prebuilt/bin/${compiler_target}24-clang"
    export "CXX_$target_env=$ndk_prebuilt/bin/${compiler_target}24-clang++"
    # tr rather than ${var^^}: macOS tasks can run under Bash 3.2.
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$ndk_prebuilt/bin/${compiler_target}24-clang"
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
