#!/usr/bin/env bash
# Runs the prepared desktop suite under the runtimes named by the caller.
set -euo pipefail
cd "$(dirname "$0")/.."
for runtime in "$@"; do
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
