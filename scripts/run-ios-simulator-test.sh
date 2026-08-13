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

runtime=${MLN_FFI_SIMULATOR_RUNTIME:-iOS}
case "$runtime" in
  iOS)
    device_filter=' iPhone '
    boot_task=ios-simulator:boot
    ;;
  tvOS)
    device_filter='Apple TV'
    boot_task=tvos-simulator:boot
    ;;
  *)
    echo "Unknown simulator runtime: $runtime" >&2
    exit 2
    ;;
esac

device=$(
  xcrun simctl list devices available "$runtime" |
    awk -v filter="$device_filter" '
      index($0, filter) && /Booted/ {
        if (match($0, /[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}/)) {
          print substr($0, RSTART, RLENGTH)
          exit
        }
      }
    '
)
if [[ -z "$device" ]]; then
  echo "No booted $runtime simulator device found. Run 'mise run //:$boot_task' first." >&2
  exit 2
fi

if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi

# simctl hands the spawned process the variables named SIMCTL_CHILD_<NAME>, with
# the prefix removed, so the fixture directory travels under that spelling. The
# binding suites share this runner and read no fixtures, so the variable travels
# when the caller set one and the C API suite reports its own absence.
if [[ -n "${MLN_FFI_TEST_FIXTURE_DIR:-}" ]]; then
  export SIMCTL_CHILD_MLN_FFI_TEST_FIXTURE_DIR="$MLN_FFI_TEST_FIXTURE_DIR"
fi

exec perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" xcrun simctl spawn "$device" "$test_executable"
