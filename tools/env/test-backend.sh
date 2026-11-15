# Sourced by a binding's test task as `source tools/env/test-backend.sh <preset>
# [backend]`. Exports MLN_FFI_TEST_BACKEND, the render backend whose suites run:
# the one named, which the preset's install must carry, or else the platform's
# default backend, or for a browser package that lacks it, the one it carries.
# The suites draw with that backend alone, as a host would.
# shellcheck shell=bash

mln_ffi_test_descriptor="$MISE_MONOREPO_ROOT/build/$1/install/share/maplibre-native-c/artifact.json"
if [[ ! -f "$mln_ffi_test_descriptor" ]]; then
  echo "error: $1 has no native install at ${mln_ffi_test_descriptor%/share/*}; build it first" >&2
  exit 2
fi
MLN_FFI_TEST_BACKEND=${2:-}
if [[ -z "$MLN_FFI_TEST_BACKEND" ]]; then
  MLN_FFI_TEST_BACKEND=$(jq -r '.platform.defaultBackend as $default | if (.backends | index($default)) then $default else .backends[0] end' "$mln_ffi_test_descriptor")
elif ! jq -e --arg backend "$MLN_FFI_TEST_BACKEND" '.backends | index($backend)' "$mln_ffi_test_descriptor" >/dev/null; then
  echo "error: $1 carries no $MLN_FFI_TEST_BACKEND backend; it carries $(jq -r '.backends | join(", ")' "$mln_ffi_test_descriptor")" >&2
  exit 2
fi
export MLN_FFI_TEST_BACKEND
unset mln_ffi_test_descriptor
