# Sourced by the Rust binding's cross-compilation tasks with a native preset as
# $1. Maps musl, FreeBSD, Android, iOS, tvOS, visionOS, Mac Catalyst, macOS
# x64, and OpenHarmony presets to their Cargo target. Exports the
# cross-compilation environment (CC/CXX, linker) from the Zig and Rust
# toolchains, Android NDK, or OpenHarmony SDK, and for targets that run off the
# host, tools/devices/run as Cargo's runner, which it also names in
# `cargo_runner`. Leaves `cargo_target` and `cargo_runner` empty for host
# presets, where Cargo picks its own target and toolchain.
# shellcheck shell=bash

cargo_target=
cargo_runner=

case "$1" in
  android-arm-*) cargo_target=armv7-linux-androideabi ;;
  android-arm64-*) cargo_target=aarch64-linux-android ;;
  android-x86-*) cargo_target=i686-linux-android ;;
  android-x64-*) cargo_target=x86_64-linux-android ;;
  tvos-arm64-*) cargo_target=aarch64-apple-tvos ;;
  tvos-simulator-arm64-*) cargo_target=aarch64-apple-tvos-sim ;;
  visionos-arm64-*) cargo_target=aarch64-apple-visionos ;;
  visionos-simulator-arm64-*) cargo_target=aarch64-apple-visionos-sim ;;
  ios-arm64-*) cargo_target=aarch64-apple-ios ;;
  ios-maccatalyst-arm64-*) cargo_target=aarch64-apple-ios-macabi ;;
  ios-simulator-arm64-*) cargo_target=aarch64-apple-ios-sim ;;
  macos-x64-*) cargo_target=x86_64-apple-darwin ;;
  linux-musl-arm64-*) cargo_target=aarch64-unknown-linux-musl ;;
  linux-musl-x64-*) cargo_target=x86_64-unknown-linux-musl ;;
  freebsd-x64-*) cargo_target=x86_64-unknown-freebsd ;;
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
    # Zig instruments unoptimized C with UBSan, whose runtime rust-lld does not
    # link, and a debug test build compiles its C dependencies unoptimized.
    export "CFLAGS_$target_env=-fno-sanitize=undefined"
    export "CXXFLAGS_$target_env=-fno-sanitize=undefined"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$linker"
    export "$rustflags_variable=${rustflags# }"
    ;;
  freebsd-x64-*)
    # The native build's Zig wrappers compile and link against FreeBSD's libc.
    target_env="${cargo_target//-/_}"
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    compiler="$MISE_MONOREPO_ROOT/build/$1/zig-shim/zig-cc"
    compiler_cxx="$MISE_MONOREPO_ROOT/build/$1/zig-shim/zig-c++"
    if [[ ! -x "$compiler" || ! -x "$compiler_cxx" ]]; then
      echo "The FreeBSD Zig compiler wrappers do not exist for $1; run mise run build $1 first." >&2
      return 2
    fi
    # The libc crate names system libraries that Zig's FreeBSD libc lacks.
    # Nothing calls into them, so empty libraries stand in at link time.
    stub_dir="$MISE_MONOREPO_ROOT/build/$1/zig-shim/rust-stubs"
    mkdir -p "$stub_dir"
    for library in kvm memstat procstat devstat; do
      if [[ ! -f "$stub_dir/lib$library.so" ]]; then
        "$compiler" -shared -o "$stub_dir/lib$library.so" -x c /dev/null
      fi
    done
    rustflags_variable="CARGO_TARGET_${target_env_upper}_RUSTFLAGS"
    rustflags="${!rustflags_variable:-} -L native=$stub_dir"
    export "$rustflags_variable=${rustflags# }"
    export "CC_$target_env=$compiler"
    export "CXX_$target_env=$compiler_cxx"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$compiler"
    ;;
  android-*)
    if ! ndk_prebuilt="$("$(dirname "${BASH_SOURCE[0]}")/android-ndk-prebuilt.sh")"; then
      return 2
    fi
    descriptor="$MISE_MONOREPO_ROOT/build/$1/install/share/maplibre-native-c/artifact.json"
    if ! android_api=$(jq -er .platform.android.api "$descriptor"); then
      echo "The Android native artifact records no API level: $descriptor" >&2
      return 2
    fi
    compiler_target="$cargo_target"
    if [[ "$cargo_target" == armv7-linux-androideabi ]]; then
      compiler_target=armv7a-linux-androideabi
    fi
    target_env="${cargo_target//-/_}"
    export "CC_$target_env=$ndk_prebuilt/bin/${compiler_target}${android_api}-clang"
    export "CXX_$target_env=$ndk_prebuilt/bin/${compiler_target}${android_api}-clang++"
    # tr rather than ${var^^}: macOS tasks can run under Bash 3.2.
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$ndk_prebuilt/bin/${compiler_target}${android_api}-clang"
    ;;
  visionos-*)
    export XROS_DEPLOYMENT_TARGET=1.0
    ;;
  ios-arm64-* | ios-simulator-arm64-*)
    # rustc and the cc crate find the SDK through xcrun themselves. Both target
    # the native archive's floor, which cmake/platform/apple.cmake sets, so the
    # linker sees one iOS version.
    export IPHONEOS_DEPLOYMENT_TARGET=15.0
    ;;
  tvos-arm64-* | tvos-simulator-arm64-*)
    export TVOS_DEPLOYMENT_TARGET=15.0
    ;;
  ohos-*64-*)
    # The OHOS SDK clang target drops the `-unknown` vendor from the cargo target.
    compiler_target="${cargo_target/-unknown/}"
    sysroot="$OHOS_SDK_NATIVE/sysroot"
    target_flags="--target=$compiler_target --sysroot=$sysroot"
    target_env="${cargo_target//-/_}"
    export "CC_$target_env=$OHOS_SDK_NATIVE/llvm/bin/clang $target_flags"
    export "CXX_$target_env=$OHOS_SDK_NATIVE/llvm/bin/clang++ $target_flags"
    target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
    export "CARGO_TARGET_${target_env_upper}_LINKER=$OHOS_SDK_NATIVE/llvm/bin/clang"
    export RUSTFLAGS="-C link-arg=--target=$compiler_target -C link-arg=--sysroot=$sysroot -C link-arg=-fuse-ld=lld"
    ;;
esac

# Cargo hands each executable it runs for these targets to tools/devices/run,
# which finds the device from the preset's install.
case "$1" in
  android-* | ohos-*64-* | ios-simulator-arm64-* | tvos-simulator-arm64-* | visionos-simulator-arm64-* | linux-musl-*64-* | freebsd-x64-*)
    cargo_runner="$MISE_MONOREPO_ROOT/tools/devices/run"
    target_env_upper="$(printf '%s' "${cargo_target//-/_}" | tr '[:lower:]' '[:upper:]')"
    export "CARGO_TARGET_${target_env_upper}_RUNNER=$cargo_runner"
    export MLN_FFI_DEVICE_INSTALL="$MISE_MONOREPO_ROOT/build/$1/install"
    ;;
esac
