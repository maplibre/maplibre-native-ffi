# Sourced by the go-map tasks with a native preset as $1 and the SDL 3 version
# as $2. Points cgo at the preset's native install, SDL 3, and the Khronos
# headers.
# shellcheck shell=bash

# shellcheck source=scripts/go-native-env.sh
source "$MISE_MONOREPO_ROOT/scripts/go-native-env.sh" "$1"
sdl_prefix="$(mise where "conda:sdl3@$2")"
export PKG_CONFIG_PATH="$sdl_prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
export CGO_CPPFLAGS="$CGO_CPPFLAGS -I$MISE_MONOREPO_ROOT/third_party/khronos/include"
export CGO_LDFLAGS="-Wl,-rpath,$sdl_prefix/lib${CGO_LDFLAGS:+ $CGO_LDFLAGS}"
