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
runtimes=${2:-node bun deno}
cd "$(dirname "$0")/.."
build="$PWD/native/build/$preset"

cmake --build "$build" --target maplibre_native_ffi_test_graphics
export MAPLIBRE_NATIVE_FFI_ADDON="$build/install/maplibre_native_ffi.node"
export MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS="$build/maplibre_native_ffi_test_graphics.node"

# The loading case installs the packed facade, backend, and addon packages into
# a consumer project, whose TypeScript files must check against the packed
# declarations alone.
consumer="$(mktemp -d)"
trap 'rm -rf "$consumer"' EXIT
node scripts/pack.ts "$consumer/npm" "$preset"
cp test/fixtures/consumer/* "$consumer/"
echo '{ "private": true }' > "$consumer/package.json"
(cd "$consumer" && npm install --offline --no-audit --no-fund ./npm/*.tgz)
pnpm exec tsc -p "$consumer"
export MAPLIBRE_NATIVE_FFI_TEST_CONSUMER="$consumer"

for runtime in $runtimes; do
  case "$runtime" in
    node)
      node --expose-gc --test --test-isolation=none --test-timeout=300000 \
        test/run/node.ts
      ;;
    bun)
      bun test --timeout 300000 test/run/bun.test.ts
      ;;
    deno)
      # vp check type-checks the suite against the Node types, as Node and Bun
      # run it unchecked. Children get only what loading the addon needs. Deno
      # fails a case as soon as its event loop empties.
      deno test --no-check --allow-ffi --allow-read --allow-env --allow-net \
        --allow-run --allow-write --v8-flags=--expose-gc test/run/deno.test.ts
      ;;
  esac
done
