# Sourced by the Go binding's tasks with a native preset as $1. Points cgo at
# the preset's native install, as tools/env/native-env.sh does, and leaves its
# path in `native_install_dir`.
# shellcheck shell=bash

# shellcheck source=tools/env/native-env.sh
source "$(dirname "${BASH_SOURCE[0]}")/native-env.sh" "$1"
# Go's build cache key omits pkg-config output, so presets that share a C
# compiler would reuse each other's link flags. The key covers CGO_CPPFLAGS,
# so naming the install there keeps each preset's cached packages apart. The
# key also omits the headers outside the package, so a C struct that changes
# size would keep packages compiled against its old layout; a checksum of the
# installed headers in CGO_CPPFLAGS rebuilds them.
headers_cksum="$(
  find "$native_install_dir/include" -type f -name '*.h' | LC_ALL=C sort |
    while IFS= read -r header; do
      cat "$header"
    done | cksum
)"
export CGO_CPPFLAGS="-I$native_install_dir/include -DMLN_HEADERS_CKSUM=${headers_cksum%% *}${CGO_CPPFLAGS:+ $CGO_CPPFLAGS}"
