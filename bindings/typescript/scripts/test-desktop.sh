#!/usr/bin/env bash
# Runs the suite under Node, Bun, and Deno, or under the one runtime named, for
# a desktop preset whose addon the binding's build task built.
#
#   test-desktop.sh <preset> [node|bun|deno]
#
# Each runner limits a case to 300 seconds, so a case that never settles fails
# and names itself instead of holding the job.
set -euo pipefail

preset=${1:?usage: test-desktop.sh <preset> [runtime]}
case "$preset" in
  linux-*-riscv64-* | linux-*-loongarch64-*)
    read -r -a runtimes <<<"${2:-node}"
    if [[ "${runtimes[*]}" != node ]]; then
      echo "error: this architecture runs the suite on the target distribution's Node.js" >&2
      exit 2
    fi
    ;;
  freebsd-*)
    read -r -a runtimes <<<"${2:-node}"
    if [[ "${runtimes[*]}" != node ]]; then
      echo "error: FreeBSD runs the suite on its packaged Node.js" >&2
      exit 2
    fi
    ;;
  linux-musl-*)
    read -r -a runtimes <<<"${2:-node bun}"
    if [[ "${runtimes[0]}" == deno ]]; then
      echo "error: Deno publishes no musl runtime" >&2
      exit 2
    fi
    ;;
  *) read -r -a runtimes <<<"${2:-node bun deno}" ;;
esac
cd "$(dirname "$0")/.."
build="$PWD/native/build/$preset"

cmake --build "$build" --target maplibre_native_ffi_test_graphics
export MAPLIBRE_NATIVE_FFI_ADDON="$build/install/maplibre_native_ffi.node"
export MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS="$build/maplibre_native_ffi_test_graphics.node"

# The loading case installs the packed facade, backend, and addon packages into
# a consumer project, whose TypeScript files must check against the packed
# declarations alone.
mkdir -p build
consumer="$(mktemp -d "$PWD/build/consumer.XXXXXX")"
trap 'rm -rf "$consumer"' EXIT
node scripts/pack.ts "$consumer/npm" "$preset"
cp test/fixtures/consumer/* "$consumer/"
echo '{ "private": true }' > "$consumer/package.json"
# npm applies platform overrides to optional dependencies only; the explicit
# local addon tarball needs --force when the host and target differ.
install_args=(--offline --no-audit --no-fund)
case "$preset" in
  *-riscv64-*) install_args+=(--cpu=riscv64 --force) ;;
  *-loongarch64-*) install_args+=(--cpu=loong64 --force) ;;
esac
if [[ "$preset" == freebsd-* ]]; then
  install_args+=(--os=freebsd --force)
fi
if [[ "$preset" == linux-musl-* ]]; then
  install_args+=(--libc=musl --force)
fi
(cd "$consumer" && npm install "${install_args[@]}" ./npm/*.tgz)
pnpm exec tsc -p "$consumer"
export MAPLIBRE_NATIVE_FFI_TEST_CONSUMER="$consumer"

case "$preset" in
  freebsd-*)
    host=$("$MISE_MONOREPO_ROOT/tools/devices/freebsd-host")
    ssh -o BatchMode=yes "$host" pkg install --yes --quiet node26 >&2
    "$MISE_MONOREPO_ROOT/scripts/run-freebsd-test.sh" "$preset" env \
      MAPLIBRE_NATIVE_FFI_ADDON="$MAPLIBRE_NATIVE_FFI_ADDON" \
      MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS="$MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS" \
      MAPLIBRE_NATIVE_FFI_TEST_CONSUMER="$consumer" \
      node --expose-gc --test --test-isolation=none --test-timeout=300000 \
      "$PWD/test/run/node.ts"
    ;;
  linux-*-riscv64-* | linux-*-loongarch64-*)
    target=${preset#linux-}
    libc=${target%%-*}
    architecture=${target#*-}
    root=$("$MISE_MONOREPO_ROOT/tools/devices/linux-root" "$libc" "${architecture%%-*}")
    # Distribution Node builds can lack the TypeScript stripper. Amaro's
    # portable loader also reaches the suite's workers and child processes.
    strip_loader="$PWD/test/run/strip-types.mjs"
    export NODE_OPTIONS="${NODE_OPTIONS:+$NODE_OPTIONS }--import=$strip_loader"
    "$MISE_MONOREPO_ROOT/scripts/run-qemu-test.sh" "$preset" "$root/usr/bin/node" \
      --expose-gc --test --test-timeout=300000 test/run/node.ts
    ;;
  linux-musl-*)
    # The host packs and type-checks; Alpine runs the addon and consumer with
    # musl runtimes. The official Node image supplies Node on x64 and arm64.
    MLN_FFI_MUSL_TEST_IMAGE="node:$(node --version | sed 's/^v//')-alpine"
    export MLN_FFI_MUSL_TEST_IMAGE
    "$MISE_MONOREPO_ROOT/scripts/run-musl-test.sh" "$preset" env \
      MAPLIBRE_NATIVE_FFI_ADDON="$MAPLIBRE_NATIVE_FFI_ADDON" \
      MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS="$MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS" \
      MAPLIBRE_NATIVE_FFI_TEST_CONSUMER="$consumer" \
      sh "$PWD/scripts/test-musl.sh" "$(bun --version)" "${runtimes[@]}"
    ;;
  *) scripts/run-desktop.sh "${runtimes[@]}" ;;
esac
