#!/usr/bin/env bash
# Prints the adb serial that Android device tasks use: ANDROID_SERIAL when it is
# set, otherwise the serial of the shared emulator for the ABI and API level,
# the one that runs the AVD api-<api>-<abi>, once it has booted. The API level
# defaults to MLN_FFI_ANDROID_EMULATOR_API. Prints nothing and exits 1 when
# neither exists, so a caller can boot the emulator and ask again.
set -euo pipefail

if [[ -n "${ANDROID_SERIAL:-}" ]]; then
  printf '%s\n' "$ANDROID_SERIAL"
  exit 0
fi

usage="usage: android-device-serial.sh <abi> [<api>]"
abi=${1:?$usage}
api=${2:-${MLN_FFI_ANDROID_EMULATOR_API:?$usage}}
avd_name="api-$api-$abi"
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"
[[ -x "$adb" ]] || exit 1

# Whoever started the emulator, its console names the AVD it runs.
while read -r serial _; do
  [[ "$serial" == emulator-* ]] || continue
  if [[ "$("$adb" -s "$serial" emu avd name </dev/null 2>/dev/null | sed -n 's/\r$//;1p')" == "$avd_name" ]] &&
    "$adb" -s "$serial" shell getprop sys.boot_completed </dev/null 2>/dev/null |
    tr -d '\r' | grep -qx 1; then
    printf '%s\n' "$serial"
    exit 0
  fi
done < <("$adb" devices | tr -d '\r')
exit 1
