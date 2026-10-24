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

# Succeeds while an emulator process runs the AVD.
avd_running() {
  pgrep -f -- "-avd $avd_name( |\$)" >/dev/null
}

if ! avd_running; then
  echo "Android emulator $avd_name is not running."
  exit 0
fi

# `emu kill` lets the guest shut down; the signals below are for an emulator
# that no longer answers adb.
serial=$(ANDROID_SERIAL='' "$MISE_MONOREPO_ROOT/scripts/android-device-serial.sh" "$abi" "$api") ||
  serial=
if [[ -n "$serial" ]]; then
  "$adb" -s "$serial" emu kill >/dev/null 2>&1 || true
fi
for ((attempt = 0; attempt < 30; attempt++)); do
  avd_running || exit 0
  sleep 1
done
pkill -f -- "-avd $avd_name( |\$)" || true
sleep 5
if avd_running; then
  pkill -KILL -f -- "-avd $avd_name( |\$)" || true
fi
