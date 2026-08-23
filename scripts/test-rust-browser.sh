#!/usr/bin/env bash
# Runs the Rust binding tests for an Emscripten preset as a page in headless
# Chromium. Proxies synchronous libtest onto a pthread through a C entry-point
# shim and applies the installed core's link options to the final module.
set -euo pipefail

preset=${1:?usage: test-rust-browser.sh <emscripten-preset>}
cd "$MISE_MONOREPO_ROOT"

native_install_dir="$MISE_MONOREPO_ROOT/build/$preset/install"
export MAPLIBRE_NATIVE_C_INSTALL_DIR="$native_install_dir"

link_flags_file="$native_install_dir/share/maplibre-native-c/emscripten-link-flags.txt"
if [[ ! -f "$link_flags_file" ]]; then
  echo "missing $link_flags_file; run 'mise run build $preset' first" >&2
  exit 1
fi

proxy_main_object="$MISE_MONOREPO_ROOT/build/$preset/rust-emscripten-proxy-main.o"
mkdir -p "$(dirname "$proxy_main_object")"
emcc -pthread -fwasm-exceptions -c \
  "$MISE_MONOREPO_ROOT/bindings/rust/emscripten_proxy_main.c" \
  -o "$proxy_main_object"

# Browser test harness options.
harness_flags=(
  -sEXIT_RUNTIME=1
  "-sENVIRONMENT=web,worker"
  -sOFFSCREENCANVAS_SUPPORT=1
  -sEXPORTED_RUNTIME_METHODS="ENV,GL"
  -sPROXY_TO_PTHREAD
  # The fixtures create their own OffscreenCanvas.
  "-sOFFSCREENCANVASES_TO_PTHREAD=''"
  "$proxy_main_object"
)

# Build and link options must reach every crate in the graph.
unit_separator=$'\x1f'
rustflags=("-C" "target-feature=+atomics,+bulk-memory")
while IFS= read -r flag; do
  [[ -n "$flag" ]] || continue
  rustflags+=("-C" "link-arg=$flag")
done <"$link_flags_file"
for flag in "${harness_flags[@]}"; do
  rustflags+=("-C" "link-arg=$flag")
done
encoded=""
for flag in "${rustflags[@]}"; do
  encoded+="${encoded:+$unit_separator}$flag"
done
export CARGO_ENCODED_RUSTFLAGS="$encoded"

# Shared memory requires an atomics-enabled standard library.
export RUSTC_BOOTSTRAP=1
export CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUNNER="$MISE_MONOREPO_ROOT/scripts/run-browser-cargo-test.sh"
cargo_test() {
  cargo test \
    -p maplibre-native-ffi-sys \
    -p maplibre-native-ffi-core \
    -p maplibre-native-ffi \
    --target wasm32-unknown-emscripten \
    -Zbuild-std=std,panic_abort \
    -- "$@" --test-threads=1
}

cargo_binding_test() {
  cargo test \
    -p maplibre-native-ffi \
    --target wasm32-unknown-emscripten \
    -Zbuild-std=std,panic_abort \
    -- "$@" --test-threads=1
}

# A browser process retains GPU and pthread resources longer than their native
# handles. Keep lifecycle-heavy map tests in bounded batches, and give every
# render test a fresh process so Chromium's context budget is not shared across
# otherwise independent fixtures.
cargo_binding_test map::tests::
cargo_binding_test projection::tests::
render_tests=(
  cloned_session_controls_can_be_used_from_another_thread
  cluster_feature_extensions_copy_values_and_feature_collections
  feature_state_and_rendered_queries_copy_native_results
  live_session_blocks_map_close_and_drop_reports_the_leaked_map
  native_pointer_round_trips_address
  opengl_borrowed_texture_session_replaces_its_target
  opengl_context_provider_mask_matches_backend_availability
  opengl_owned_texture_exposes_backend_metadata
  opengl_surface_session_renders_into_the_platform_surface
  owned_texture_session_renders_acquires_resizes_and_reads_back
  sustained_frame_demands_outlast_the_texture_ring_depth
  texture_readback_before_a_frame_reports_invalid_state
  webgpu_borrowed_texture_session_renders_into_a_host_texture
  webgpu_surface_session_renders_into_the_browser_canvas
)
for test_name in "${render_tests[@]}"; do
  cargo_binding_test "render::tests::$test_name" --exact
done
cargo_test --skip map::tests:: --skip projection::tests:: --skip render::tests::
cargo clippy \
  -p maplibre-native-ffi-sys \
  -p maplibre-native-ffi-core \
  -p maplibre-native-ffi \
  --target wasm32-unknown-emscripten \
  -Zbuild-std=std,panic_abort \
  --all-targets -- -D warnings
