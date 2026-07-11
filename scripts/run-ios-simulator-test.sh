#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <test-executable> [timeout-seconds]" >&2
  exit 2
fi

test_executable=$1
timeout_seconds=${2:-120}
if [[ ! -x "$test_executable" ]]; then
  echo "iOS simulator test executable is not executable: $test_executable" >&2
  exit 2
fi

device=$(
  xcrun simctl list devices available iOS |
    awk -F '[()]' '/ iPhone / && /Shutdown|Booted/ { print $2; exit }'
)
if [[ -z "$device" ]]; then
  echo "No available iOS simulator device found." >&2
  exit 2
fi

state=$(xcrun simctl list devices "$device" | awk -F '[()]' -v id="$device" '$0 ~ id { print $4; exit }')
if [[ "$state" != "Booted" ]]; then
  xcrun simctl boot "$device" >/dev/null 2>&1 || true
  xcrun simctl bootstatus "$device" -b
fi

if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi

exec perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" xcrun simctl spawn "$device" "$test_executable"
