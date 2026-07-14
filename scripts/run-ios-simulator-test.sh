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
    awk -F '[()]' '/ iPhone / && /Booted/ { print $2; exit }'
)
if [[ -z "$device" ]]; then
  echo "No booted iOS simulator device found. Run 'mise run //:ios-simulator:boot' first." >&2
  exit 2
fi

if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi

exec perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" xcrun simctl spawn "$device" "$test_executable"
