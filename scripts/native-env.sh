# Sourced by tasks that build against a native install, with a native preset as
# $1. Points pkg-config at the preset's install, puts its DLL directory on PATH
# on Windows, and leaves the install's path in `native_install_dir`.
# shellcheck shell=bash

native_install_dir="$MISE_MONOREPO_ROOT/build/$1/install"
export PKG_CONFIG_PATH="$native_install_dir/share/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
if command -v cygpath >/dev/null 2>&1; then
  native_bin_dir="$(cygpath -u "$native_install_dir/bin")"
  export PATH="$native_bin_dir${PATH:+:${PATH}}"
fi
