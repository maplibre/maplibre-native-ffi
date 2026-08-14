#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <test-executable> [timeout-seconds]" >&2
  exit 2
fi

test_executable=$1
timeout_seconds=${2:-120}
if [[ ! -x "$test_executable" ]]; then
  echo "Simulator test executable is not executable: $test_executable" >&2
  exit 2
fi

if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi

script_dir=$(cd "$(dirname "$0")" && pwd)
runtime=${MLN_FFI_SIMULATOR_RUNTIME:-iOS}
device=$("$script_dir/apple-simulator.sh" find-booted "$runtime")

# simctl hands the spawned process the variables named SIMCTL_CHILD_<NAME>, with
# the prefix removed, so the fixture directory travels under that spelling. The
# binding suites share this runner and read no fixtures, so the variable travels
# when the caller set one and the C API suite reports its own absence.
if [[ -n "${MLN_FFI_TEST_FIXTURE_DIR:-}" ]]; then
  export SIMCTL_CHILD_MLN_FFI_TEST_FIXTURE_DIR="$MLN_FFI_TEST_FIXTURE_DIR"
fi

exec perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" xcrun simctl spawn "$device" "$test_executable"
