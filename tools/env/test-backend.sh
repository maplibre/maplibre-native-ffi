# Sourced by a binding's test task as `source tools/env/test-backend.sh <preset>
# <backend-or-empty> <binding>`, where the binding is its column in
# docs/support-gaps.toml, such as go or kotlin-jvm. Exports MLN_FFI_TEST_BACKEND,
# the render backend whose suites run: the one named, which the preset's install
# must carry, or else the first of its backends that docs/support-gaps.toml lets
# the binding test on the target, in the package's order of preference. The
# suites draw with that backend alone, as a host would.
# shellcheck shell=bash

mln_ffi_test_descriptor="$MISE_MONOREPO_ROOT/build/$1/install/share/maplibre-native-c/artifact.json"
if [[ ! -f "$mln_ffi_test_descriptor" ]]; then
  echo "error: $1 has no native install at ${mln_ffi_test_descriptor%/share/*}; build it first" >&2
  exit 2
fi
MLN_FFI_TEST_BACKEND=${2:-}
if [[ -z "$MLN_FFI_TEST_BACKEND" ]]; then
  MLN_FFI_TEST_BACKEND=$(python3 -c '
import sys
sys.path.insert(0, sys.argv[1])
from tools.ci import gaps, plan
target = plan.target_named(sys.argv[2])
print((gaps.tested_backends(sys.argv[3], target) or target.backends)[0])
' "$MISE_MONOREPO_ROOT" "$1" "${3:?test-backend.sh needs the binding as its third argument}")
elif ! jq -e --arg backend "$MLN_FFI_TEST_BACKEND" '.backends | index($backend)' "$mln_ffi_test_descriptor" >/dev/null; then
  echo "error: $1 carries no $MLN_FFI_TEST_BACKEND backend; it carries $(jq -r '.backends | join(", ")' "$mln_ffi_test_descriptor")" >&2
  exit 2
fi
export MLN_FFI_TEST_BACKEND
unset mln_ffi_test_descriptor
