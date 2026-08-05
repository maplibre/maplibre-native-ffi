#!/usr/bin/env bash
# Links the WebAssembly runtime payload.
#
# The native build installs a static library, so the payload is linked here with
# the shared host-support shim on top of it.
#
# The module instantiates in a browser, in Node, in Bun, and in Deno, and the
# whole binding API works in all four. Its *default* resource loading does not:
# MapLibre's Emscripten HTTP source is Emscripten Fetch, which is XHR, so a
# non-browser host serves resources through a resource provider of its own until
# a host-injected network adapter exists. The WebGL and OffscreenCanvas options
# below are likewise for the browser; a non-browser host uses the non-rendering
# API domains.
set -euo pipefail

preset="${1:?usage: build-wasm-runtime.sh <preset>}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
install_dir="$root/build/$preset/install"
# The browser build installs headers but keeps its archives in the build tree,
# and MapLibre Native's core and vendored libraries are archives of their own,
# so the link takes the C API first and then everything it draws from.
# The Rust platform library is built by cargo rather than by CMake, so it sits
# in the cargo target directory instead of the preset's build tree.
rust_archive="$root/target/wasm32-unknown-emscripten/release/libmln_ffi_platform.a"
mapfile -t archives < <(
  {
    printf '%s\n' "$root/build/$preset/libmaplibre-native-c.a"
    find "$root/build/$preset" -name '*.a' ! -name 'libmaplibre-native-c.a' -print
    printf '%s\n' "$rust_archive"
  }
)
support="$root/bindings/typescript/host-support"
package="$root/bindings/typescript/runtime-wasm"

emcc="${EMSDK:?EMSDK must point at the Emscripten SDK}/upstream/emscripten/emcc"

# Every symbol the transport reaches, plus the allocator it takes slabs from.
# Emscripten drops anything a link does not name.
exported_functions='[
  "_malloc",
  "_free",
  "_mln_abi_fingerprint",
  "_mln_abi_header_digest",
  "_mln_abi_entrypoint_count",
  "_mln_abi_entrypoint_name",
  "_mln_abi_call",
  "_mln_abi_symbol",
  "_mln_abi_log_listener_address",
  "_mln_abi_resource_request_listener_address",
  "_mln_abi_custom_geometry_fetch_listener_address",
  "_mln_abi_custom_geometry_cancel_listener_address",
  "_mln_abi_record_destroy",
  "_mln_abi_queue_drain",
  "_mln_abi_queue_depth",
  "_mln_abi_transfer_issue",
  "_mln_abi_transfer_claim",
  "_mln_abi_transfer_discard"
]'

mkdir -p "$package"
# On the pool size below: a browser cannot grow it. Creating a worker needs the
# event loop, and the thread that would wait for one is the thread that runs it,
# so an exhausted pool wedges the page rather than failing. Node grows it on
# demand and so never noticed.
#
# One runtime with a map takes twelve to sixteen workers, measured by walking
# the pool size down until the conformance suite stopped passing, so this is
# roughly four times what a page with a map needs. It is not sized for a whole
# session: a closed runtime gives its workers back, but only once the thread
# that closed it yields to the event loop, because the worker returns through a
# message. Code that opens and closes runtimes without ever reaching a task
# boundary therefore holds every worker it has taken, and no pool that fits in a
# browser is large enough for that. `loadBrowser()` says so where a consumer
# meets it.
"$emcc" \
  -std=c23 \
  -O2 \
  -pthread \
  -fwasm-exceptions \
  -I"$support/include" \
  -I"$support/generated" \
  -I"$install_dir/include" \
  "$support/src/mln_abi.c" \
  "$support/generated/layout_assert.c" \
  "${archives[@]}" \
  -o "$package/maplibre-native-ffi.mjs" \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sEXPORT_NAME=createMaplibreRuntime \
  -sALLOW_MEMORY_GROWTH=1 \
  -sINITIAL_MEMORY=512MB \
  -sSTACK_SIZE=1MB \
  -sPTHREAD_POOL_SIZE=64 \
  -sFETCH=1 \
  -sUSE_ZLIB=1 \
  -sFULL_ES3=1 \
  -sMIN_WEBGL_VERSION=2 \
  -sMAX_WEBGL_VERSION=2 \
  -sOFFSCREENCANVAS_SUPPORT=1 \
  -sWASM_BIGINT=1 \
  -sEXPORTED_FUNCTIONS="$exported_functions" \
  -sEXPORTED_RUNTIME_METHODS='["UTF8ToString","HEAPU8","FS","GL"]' \
  "${@:2}"

target="${preset%-*}"
case "${preset##*-}" in
webgl) backend=opengl ;;
*) backend="${preset##*-}" ;;
esac

fingerprint=$(
  tr '\n' ' ' <"$support/generated/fingerprint.h" |
    sed -n 's/.*MLN_ABI_FINGERPRINT_VALUE[^"]*"\([^"]*\)".*/\1/p'
)

cat >"$package/runtime.json" <<JSON
{
  "transport": "wasm",
  "target": "$target",
  "backend": "$backend",
  "abiFingerprint": "$fingerprint",
  "module": "./maplibre-native-ffi.mjs"
}
JSON

# The rest of the payload package. `pack-runtime.sh` packs these; the whole
# directory is build output, so they are written here rather than checked in.
#
# The entry names the factory rather than passing the module file through as a
# default export, so a payload looks the same to the facade whichever transport
# it carries. The metadata is inlined rather than imported, because a JSON
# import attribute is not something every browser has. The extension is `.mjs`
# rather than `.js` because this directory carries no `package.json` of its own
# until it is packed, and a `.js` file here would be read as CommonJS.
cat >"$package/index.mjs" <<JS
// Loads this payload's compiled module. The payload defines no MapLibre API of
// its own; the facade owns every public name.
export { default as createRuntime } from "./maplibre-native-ffi.mjs";

export const runtime = {
  transport: "wasm",
  target: "$target",
  backend: "$backend",
  abiFingerprint: "$fingerprint",
  module: "./maplibre-native-ffi.mjs",
};
JS

cat >"$package/index.d.ts" <<'TS'
/** Metadata describing what this payload was built for. */
export interface RuntimeMetadata {
  readonly transport: "wasm";
  readonly target: string;
  readonly backend: string;
  readonly abiFingerprint: string;
  /** The module file, relative to this package. */
  readonly module: string;
}

export declare const runtime: RuntimeMetadata;

/**
 * Instantiates the compiled WebAssembly module.
 *
 * What it resolves to is the facade's `WasmModule`, which is declared there
 * because the facade owns every public type.
 */
export declare function createRuntime(
  options?: Record<string, unknown>,
): Promise<unknown>;
TS

cat >"$package/README.md" <<'MARKDOWN'
# MapLibre Native WebAssembly runtime payload

This package carries one compiled MapLibre Native artifact and the metadata
describing it. It defines no MapLibre API of its own: the facade,
[`@maplibre/native-ffi`](https://www.npmjs.com/package/@maplibre/native-ffi),
owns every public name.

```bash
npm install @maplibre/native-ffi @maplibre/native-ffi-runtime-emscripten-wasm32-opengl
```

```ts
import { loadBrowser } from "@maplibre/native-ffi/browser";

const maplibre = await loadBrowser();
```

## The page has to be cross-origin isolated

The module is threaded, and a browser withholds `SharedArrayBuffer` from a page
that is not cross-origin isolated, so it cannot start its threads at all without
these response headers on the document:

```http
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

## Where the files go

`loadBrowser()` resolves this package by name, which works wherever the host
resolves packages at runtime. A bundled application names the payload itself:
either import this package and pass what it gave back as `module`, or copy
`maplibre-native-ffi.mjs` and `maplibre-native-ffi.wasm` into what the
application serves and pass the `.mjs` URL as `moduleUrl`. The module finds the
`.wasm` beside the URL it was loaded from, and starts its workers from the same
place.
MARKDOWN

echo "linked $preset into $package"
