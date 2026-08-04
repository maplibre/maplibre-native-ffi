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
# demand and so never noticed. The pool is sized for a whole session instead.
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

echo "linked $preset into $package"
