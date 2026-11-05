#!/bin/sh
# Builds the library against this system's GLib, gobject-introspection, Vala,
# and GJS, then runs the package and suite programs against it, for a target
# whose system runs its tests: Alpine for musl, and the virtual machine for
# FreeBSD, which has installed them. The repository may mount read-only, so the
# library installs into a scratch copy of the native install.
#
#   test-from-source.sh <native-install>
set -eu
native=${1:?usage: test-from-source.sh <native-install>}
case "$(uname -s)" in
  Linux) apk add --no-cache --quiet bash build-base gjs glib-dev gobject-introspection-dev meson vala ;;
esac
cd "$(dirname "$0")/.."
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
prefix="$scratch/install"
cp -R "$native" "$prefix"
export PKG_CONFIG_PATH="$prefix/share/pkgconfig"
meson setup "$scratch/build" . --prefix="$prefix" --libdir=lib --buildtype=release
meson install -C "$scratch/build" --quiet
export MLN_FFI_GOBJECT_PREFIX="$prefix"
bash .mise/tasks/package
bash .mise/tasks/suite
