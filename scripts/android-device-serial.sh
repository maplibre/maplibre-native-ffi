#!/usr/bin/env bash
# Prints the adb serial that Android device tasks use: ANDROID_SERIAL when it is
# set, otherwise the serial of the emulator that boot-android-emulator.sh
# started in this checkout, once that emulator has booted. Prints nothing and
# exits 1 when neither exists, so a caller can boot the emulator and ask again.
set -euo pipefail

if [[ -n "${ANDROID_SERIAL:-}" ]]; then
  printf '%s\n' "$ANDROID_SERIAL"
  exit 0
fi

# Outside a mise task the checkout is the one that holds this script.
repository_root="${MISE_MONOREPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
state_root="$repository_root/build/android-emulator"
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"
[[ -x "$adb" ]] || exit 1

# Each AVD's state directory carries its name. A recorded serial counts only
# while the emulator there still runs that AVD, because another emulator may
# take the port after this one exits.
shopt -s nullglob
for serial_file in "$state_root"/*/serial; do
  state_dir=${serial_file%/serial}
  avd_name=${state_dir##*/}
  serial=$(<"$serial_file")
  if [[ "$("$adb" -s "$serial" emu avd name 2>/dev/null | sed -n '1s/\r$//p')" == "$avd_name" ]] &&
    "$adb" -s "$serial" shell getprop sys.boot_completed 2>/dev/null |
    tr -d '\r' | grep -qx 1; then
    printf '%s\n' "$serial"
    exit 0
  fi
done
exit 1
