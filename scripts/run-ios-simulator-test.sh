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

timeout_seconds() {
  local timeout=$1
  case "$timeout" in
    *ms)
      local milliseconds=${timeout%ms}
      [[ "$milliseconds" =~ ^[0-9]+$ ]] || return 1
      echo $(((milliseconds + 999) / 1000))
      ;;
    *s)
      local seconds=${timeout%s}
      [[ "$seconds" =~ ^[0-9]+$ ]] || return 1
      echo "$seconds"
      ;;
    *m)
      local minutes=${timeout%m}
      [[ "$minutes" =~ ^[0-9]+$ ]] || return 1
      echo $((minutes * 60))
      ;;
    *h)
      local hours=${timeout%h}
      [[ "$hours" =~ ^[0-9]+$ ]] || return 1
      echo $((hours * 3600))
      ;;
    *)
      [[ "$timeout" =~ ^[0-9]+$ ]] || return 1
      echo "$timeout"
      ;;
  esac
}

if [[ -n "${MLN_FFI_TEST_TIMEOUT:-}" ]]; then
  if ! seconds=$(timeout_seconds "$MLN_FFI_TEST_TIMEOUT"); then
    echo "Invalid MLN_FFI_TEST_TIMEOUT: $MLN_FFI_TEST_TIMEOUT" >&2
    exit 2
  fi
  exec perl -e 'alarm shift; exec @ARGV' "$seconds" xcrun simctl spawn "$device" "$test_executable"
fi

exec xcrun simctl spawn "$device" "$test_executable"
