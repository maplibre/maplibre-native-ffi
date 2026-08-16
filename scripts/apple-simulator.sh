#!/usr/bin/env bash
# Resolves Apple simulator devices by UUID. Device names can contain
# parentheses, so field-splitting on '(' is not a stable id.
set -euo pipefail

uuid_re='[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}'

usage() {
  echo "usage: $0 boot <iOS|tvOS>" >&2
  echo "       $0 find-booted <iOS|tvOS>" >&2
  exit 2
}

device_filter() {
  case "$1" in
    iOS) printf '%s\n' ' iPhone ' ;;
    tvOS) printf '%s\n' 'Apple TV' ;;
    *)
      echo "Unknown simulator runtime: $1" >&2
      usage
      ;;
  esac
}

find_device() {
  local runtime=$1
  local state_re=$2
  local filter
  filter=$(device_filter "$runtime")
  xcrun simctl list devices available "$runtime" |
    awk -v filter="$filter" -v state_re="$state_re" -v uuid_re="$uuid_re" '
      index($0, filter) && $0 ~ state_re {
        if (match($0, uuid_re)) {
          print substr($0, RSTART, RLENGTH)
          exit
        }
      }
    '
}

device_state() {
  xcrun simctl list devices "$1" |
    awk -v id="$1" '
      index($0, id) {
        if ($0 ~ /Booted/) { print "Booted"; exit }
        if ($0 ~ /Shutdown/) { print "Shutdown"; exit }
      }
    '
}

boot_runtime() {
  local runtime=$1
  local device
  device=$(find_device "$runtime" 'Shutdown|Booted')
  if [[ -z "$device" ]]; then
    echo "No available $runtime simulator device found." >&2
    exit 2
  fi
  if [[ "$(device_state "$device")" != "Booted" ]]; then
    xcrun simctl boot "$device"
    xcrun simctl bootstatus "$device" -b
  fi
}

find_booted() {
  local runtime=$1
  local device
  device=$(find_device "$runtime" 'Booted')
  if [[ -z "$device" ]]; then
    echo "No booted $runtime simulator device found. Run 'mise run //:apple-simulator:boot $runtime' first." >&2
    exit 2
  fi
  printf '%s\n' "$device"
}

if [[ $# -ne 2 ]]; then
  usage
fi

case "$1" in
  boot) boot_runtime "$2" ;;
  find-booted) find_booted "$2" ;;
  *) usage ;;
esac
