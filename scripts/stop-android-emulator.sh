#!/usr/bin/env bash
# Stops the shared Android emulator for an API level and ABI, whoever started
# it. Every session on the machine may be using that emulator, so no task stops
# it on its own; this is for a person who knows that none is. Other emulators
# keep running.
set -euo pipefail

usage="usage: stop-android-emulator.sh <api> <abi>"
api=${1:?$usage}
abi=${2:?$usage}
avd_name="api-$api-$abi"
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"
# The emulator takes the AVD as `-avd <name>` or `@<name>`.
avd_pattern="(-avd |@)$avd_name( |\$)"

# The emulator's console names the AVD it runs, booted or not.
serial=
if [[ -x "$adb" ]]; then
  while read -r device _; do
    [[ "$device" == emulator-* ]] || continue
    if [[ "$("$adb" -s "$device" emu avd name </dev/null 2>/dev/null | sed -n 's/\r$//;1p')" == "$avd_name" ]]; then
      serial=$device
      break
    fi
  done < <("$adb" devices | tr -d '\r')
fi
if [[ -z "$serial" ]] && ! pgrep -f -- "$avd_pattern" >/dev/null; then
  echo "Android emulator $avd_name is not running."
  exit 0
fi

# `emu kill` lets the guest shut down; the signals below are for an emulator
# that no longer answers adb.
if [[ -n "$serial" ]]; then
  "$adb" -s "$serial" emu kill >/dev/null 2>&1 || true
fi
for ((attempt = 0; attempt < 30; attempt++)); do
  pgrep -f -- "$avd_pattern" >/dev/null || exit 0
  sleep 1
done
pkill -f -- "$avd_pattern" || true
sleep 5
if pgrep -f -- "$avd_pattern" >/dev/null; then
  pkill -KILL -f -- "$avd_pattern" || true
fi
