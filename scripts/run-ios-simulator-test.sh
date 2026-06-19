#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <test-executable>" >&2
  exit 2
fi

test_executable=$1
if [[ ! -x "$test_executable" ]]; then
  echo "iOS simulator test executable is not executable: $test_executable" >&2
  exit 2
fi

device=${MLN_FFI_IOS_SIMULATOR_DEVICE:-}
if [[ -z "$device" ]]; then
  # iPhone simulators are present on the CI images and cover the app runtime surface used by these tests.
  device=$(xcrun simctl list devices available iOS |
    awk -F '[()]' '/ iPhone / && /Shutdown|Booted/ { print $2; exit }')
fi

if [[ -z "$device" ]]; then
  echo "No available iOS simulator device found. Set MLN_FFI_IOS_SIMULATOR_DEVICE to a simulator UDID." >&2
  exit 2
fi

state=$(xcrun simctl list devices "$device" | awk -F '[()]' -v id="$device" '$0 ~ id { print $4; exit }')
if [[ "$state" != "Booted" ]]; then
  xcrun simctl boot "$device" >/dev/null 2>&1 || true
  xcrun simctl bootstatus "$device" -b
fi

if [[ -n "${MLN_FFI_TEST_TIMEOUT:-}" ]]; then
  if [[ ! "$MLN_FFI_TEST_TIMEOUT" =~ ^[0-9]+s?$ ]]; then
    echo "Invalid MLN_FFI_TEST_TIMEOUT: $MLN_FFI_TEST_TIMEOUT" >&2
    exit 2
  fi
  seconds=${MLN_FFI_TEST_TIMEOUT%s}
  exec perl -e 'alarm shift; exec @ARGV' "$seconds" xcrun simctl spawn "$device" "$test_executable"
fi

exec xcrun simctl spawn "$device" "$test_executable"
