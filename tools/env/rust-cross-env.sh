# Sourced with a MinGW native preset as $1. Exports Cargo's settings for
# cross-compiling to the preset's target, and sets `cargo_target`.
# shellcheck shell=bash

case "$1" in
  mingw-x64) cargo_target=x86_64-pc-windows-gnu ;;
  mingw-arm64) cargo_target=aarch64-pc-windows-gnullvm ;;
  *)
    echo "No Cargo cross-compilation settings serve $1." >&2
    return 2
    ;;
esac

# Cargo builds its host-side dependencies with the MSVC Rust toolchain.
source "$MISE_MONOREPO_ROOT/tools/env/windows-msvc-env.sh"
target_env="${cargo_target//-/_}"
target_env_upper="$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')"
export "CC_$target_env=$MLN_FFI_MSYS2_ROOT/bin/clang.exe"
export "CXX_$target_env=$MLN_FFI_MSYS2_ROOT/bin/clang++.exe"
export "AR_$target_env=$MLN_FFI_MSYS2_ROOT/bin/llvm-ar.exe"
export "CARGO_TARGET_${target_env_upper}_LINKER=$MLN_FFI_MSYS2_ROOT/bin/clang.exe"
rustflags_variable="CARGO_TARGET_${target_env_upper}_RUSTFLAGS"
rustflags=${!rustflags_variable:-}
export "$rustflags_variable=${rustflags:+$rustflags }-C link-arg=-fuse-ld=lld"
