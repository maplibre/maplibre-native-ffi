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
    --target wasm32-unknown-emscripten \
    -Zbuild-std=std,panic_abort \
    -- "$@" --test-threads=1
}

# Chromium retains GPU and pthread resources longer than their native handles.
# Give every integration test a fresh process so otherwise independent tests
# cannot inherit a context budget or worker-pool state from their predecessors.
binding_source="$MISE_MONOREPO_ROOT/bindings/rust/crates/maplibre-native-ffi/src"

# Each entry pairs a source file with the module path its tests are compiled
# under. Tests live in indented `mod tests` blocks, so the names are read from
# the first function declaration after each `#[test]` attribute at any depth.
test_sources=(
  "completion.rs completion::tests::"
  "custom_geometry.rs custom_geometry::tests::"
  "custom_mvt_vector.rs custom_mvt_vector::tests::"
  "events.rs events::tests::"
  "handle.rs handle::tests::"
  "lib.rs tests::"
  "logging.rs logging::tests::"
  "map/tests.rs map::tests::"
  "projection.rs projection::tests::"
  "render/tests.rs render::tests::"
  "resource.rs resource::tests::"
  "runtime.rs runtime::tests::"
)

list_test_names() {
  awk '
    /#\[test\]/ { pending = 1; next }
    pending && /^[[:space:]]*(pub[[:space:]]+)?(async[[:space:]]+)?fn[[:space:]]/ {
      sub(/^[[:space:]]*(pub[[:space:]]+)?(async[[:space:]]+)?fn[[:space:]]+/, "")
      sub(/[(<].*/, "")
      print
      pending = 0
    }
  ' "$1"
}

# A source file that grows tests without an entry here would otherwise be
# skipped silently, and so would a file whose module path stopped matching.
listed_files=()
for entry in "${test_sources[@]}"; do
  listed_files+=("${entry%% *}")
done
while IFS= read -r source; do
  relative="${source#"$binding_source/"}"
  for listed in "${listed_files[@]}"; do
    if [[ "$listed" == "$relative" ]]; then
      continue 2
    fi
  done
  echo "$relative declares tests but is missing from test_sources in ${BASH_SOURCE[0]}" >&2
  exit 1
done < <(grep -rl $'#\[test\]' "$binding_source" | sort)

for entry in "${test_sources[@]}"; do
  relative="${entry%% *}"
  prefix="${entry#* }"
  source="$binding_source/$relative"
  declared=$(grep -c $'#\[test\]' "$source")
  enumerated=0
  test_names=()
  while IFS= read -r test_name; do
    enumerated=$((enumerated + 1))
    # A pair of `cfg` alternatives shares one name, and only one of them is
    # compiled for this target.
    case " ${test_names[*]:-} " in
      *" $test_name "*) continue ;;
    esac
    test_names+=("$test_name")
  done < <(list_test_names "$source")
  if [[ "$enumerated" -ne "$declared" ]]; then
    echo "enumerated $enumerated of $declared tests in $relative" >&2
    exit 1
  fi
  for test_name in "${test_names[@]}"; do
    cargo_binding_test "$prefix$test_name" --exact
  done
done

cargo_support_tests
cargo clippy \
  -p maplibre-native-ffi-sys \
  -p maplibre-native-ffi-core \
  -p maplibre-native-ffi \
  --target wasm32-unknown-emscripten \
  -Zbuild-std=std,panic_abort \
  --all-targets -- -D warnings
