#!/usr/bin/env bash
# Runs a musl C API suite inside Alpine, where the dynamic loader and graphics
# implementation share the ABI that the test executable targets.
set -euo pipefail

preset=${1:?usage: run-musl-test.sh <preset>}
test_binary="$MISE_MONOREPO_ROOT/build/$preset/mln_ffi_c_api_tests"

if [[ ! -x "$test_binary" ]]; then
  echo "The musl C API test executable does not exist: $test_binary" >&2
  exit 2
fi

if command -v docker >/dev/null; then
  container_engine=docker
elif command -v podman >/dev/null; then
  container_engine=podman
else
  echo "Musl tests require Docker or Podman to run Alpine." >&2
  exit 2
fi

case "$preset" in
  *-egl) backend=egl ;;
  *-vulkan) backend=vulkan ;;
  *)
    echo "Unsupported musl test preset: $preset" >&2
    exit 2
    ;;
esac

# The single-quoted program expands its positional arguments inside Alpine.
# shellcheck disable=SC2016
"$container_engine" run --rm \
  --volume "$MISE_MONOREPO_ROOT:$MISE_MONOREPO_ROOT:ro" \
  --workdir "$MISE_MONOREPO_ROOT" \
  alpine:3.22 \
  sh -euc '
    case "$1" in
      egl) apk add --no-cache mesa-dri-gallium mesa-egl mesa-gles ;;
      vulkan) apk add --no-cache mesa-vulkan-swrast vulkan-loader ;;
    esac
    export LIBGL_ALWAYS_SOFTWARE=true
    export MLN_FFI_TEST_FIXTURE_DIR="$2/third_party/maplibre-native/test/fixtures"
    exec "$3"
  ' sh "$backend" "$MISE_MONOREPO_ROOT" "$test_binary"
