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
cargo_support_tests() {
  cargo test \
    -p maplibre-native-ffi-sys \
    -p maplibre-native-ffi-core \
    --target wasm32-unknown-emscripten \
    -Zbuild-std=std,panic_abort \
    -- --test-threads=1
}

cargo_binding_test() {
  cargo test \
    -p maplibre-native-ffi \
    --features browser-fixtures \
    --target wasm32-unknown-emscripten \
    -Zbuild-std=std,panic_abort \
    "$@"
}

# The unit tests hold no GPU context, so they share one page.
cargo_binding_test --lib -- --test-threads=1

# Chromium retains GPU and pthread resources longer than their native handles,
# so every integration test gets a page of its own and cannot inherit a context
# budget or worker-pool state from its predecessors. The browser build compiles
# only the suite's browser module, and a test there that names a render backend
# in its cfg compiles only for that backend. The names come from the source:
# the first function after each `#[test]`, kept when its cfg matches.
browser_tests="$MISE_MONOREPO_ROOT/bindings/rust/crates/maplibre-native-ffi/tests/suite/browser/mod.rs"
render_backend=$(sed -n 's/.*"renderBackend": *"\([^"]*\)".*/\1/p' \
  "$native_install_dir/share/maplibre-native-c/artifact.json")
declared=$(grep -c $'^#\[test\]' "$browser_tests")
enumerated=0
test_names=()
while IFS=' ' read -r backend test_name; do
  enumerated=$((enumerated + 1))
  if [[ "$backend" == any || "$backend" == "$render_backend" ]]; then
    test_names+=("$test_name")
  fi
done < <(awk '
  /^#\[cfg\(mln_render_backend = "/ {
    backend = $0
    sub(/^#\[cfg\(mln_render_backend = "/, "", backend)
    sub(/".*/, "", backend)
    next
  }
  /^#\[test\]/ { pending = 1; next }
  pending && /^fn / {
    name = $0
    sub(/^fn /, "", name)
    sub(/\(.*/, "", name)
    print (backend == "" ? "any" : backend), name
    pending = 0
    backend = ""
    next
  }
  !pending { backend = "" }
' "$browser_tests")
if [[ "$enumerated" -ne "$declared" || ${#test_names[@]} -eq 0 ]]; then
  echo "enumerated $enumerated of $declared tests in $browser_tests for $render_backend" >&2
  exit 1
fi
for test_name in "${test_names[@]}"; do
  cargo_binding_test --test suite -- "browser::$test_name" --exact --test-threads=1
done

cargo_support_tests
cargo clippy \
  -p maplibre-native-ffi-sys \
  -p maplibre-native-ffi-core \
  -p maplibre-native-ffi \
  --features maplibre-native-ffi/browser-fixtures \
  --target wasm32-unknown-emscripten \
  -Zbuild-std=std,panic_abort \
  --all-targets -- -D warnings
