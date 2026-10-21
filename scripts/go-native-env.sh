# Sourced by the Go binding's tasks with a native preset as $1. Points cgo at
# the preset's native install and leaves its path in `native_install_dir`.
# shellcheck shell=bash

native_install_dir="$MISE_MONOREPO_ROOT/build/$1/install"
export PKG_CONFIG_PATH="$native_install_dir/share/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
# Go's build cache key omits pkg-config output, so presets that share a C
# compiler would reuse each other's link flags. The key covers CGO_CPPFLAGS,
# so naming the install there keeps each preset's cached packages apart.
export CGO_CPPFLAGS="-I$native_install_dir/include${CGO_CPPFLAGS:+ $CGO_CPPFLAGS}"
