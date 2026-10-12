#!/usr/bin/env bash
# Installs the debug build on the emulator that matches the preset, launches it
# in smoke mode, and waits for the activity to log its first rendered frame.
set -euo pipefail

preset=${1:?usage: smoke.sh <preset>}
case "$preset" in
  android-arm64-egl) abi=arm64-v8a backend=opengl ;;
  android-arm64-vulkan) abi=arm64-v8a backend=vulkan ;;
  android-x64-egl) abi=x86_64 backend=opengl ;;
  android-x64-vulkan) abi=x86_64 backend=vulkan ;;
  *)
    echo "The Android map smoke check cannot run preset $preset." >&2
    exit 2
    ;;
esac

package=org.maplibre.nativeffi.examples.androidmap
rendered="smoke: rendered a frame"
timeout_seconds=120

adb="$ANDROID_HOME/platform-tools/adb"
# An explicit serial selects a connected device. Otherwise this checkout's
# booted emulator runs the smoke, whatever its API level: the app supports
# every level the suites boot, and the x64 EGL target's suites leave API 26 or
# the default level running. Without one, the default emulator boots. adb and
# Gradle both read the exported serial.
serial_script="$MISE_MONOREPO_ROOT/scripts/android-device-serial.sh"
if ! ANDROID_SERIAL=$("$serial_script"); then
  mise run //:android-emulator:boot "$abi"
  ANDROID_SERIAL=$("$serial_script")
fi
export ANDROID_SERIAL
device_abi=$("$adb" shell getprop ro.product.cpu.abi | tr -d '\r')
if [[ "$device_abi" != "$abi" ]]; then
  echo "Android device $ANDROID_SERIAL has ABI $device_abi; preset $preset needs $abi." >&2
  exit 1
fi

./gradlew \
  -Pmaplibre.android.backend="$backend" \
  -Pmaplibre.android.abis="$abi" \
  -Pmaplibre.android.prebuiltBuildRoot=build \
  :examples:android-map:installDebug

"$adb" shell am start -S -W -n "$package/.MainActivity" --ez smoke true
pid=$("$adb" shell pidof "$package" | tr -d '\r')
if [[ -z "$pid" ]]; then
  echo "The Android map exited before it rendered a frame." >&2
  exit 1
fi

# Reading only this launch's process leaves the device log intact and ignores
# other apps. logcat exits at the first line that matches. The watchdog ends it
# when no frame renders in time or the process exits, and a crash of the app
# ends the wait early.
"$adb" logcat --pid="$pid" -v brief -e "$rendered|FATAL EXCEPTION" -m 1 \
  >"${TMPDIR:-/tmp}/android-map-smoke.$$" &
logcat=$!
(
  deadline=$((SECONDS + timeout_seconds))
  while ((SECONDS < deadline)) && kill -0 "$logcat" 2>/dev/null; do
    [[ "$("$adb" shell pidof "$package" | tr -d '\r')" == "$pid" ]] || break
    sleep 1
  done
  kill "$logcat" 2>/dev/null || true
) &
watchdog=$!
wait "$logcat" || true
kill "$watchdog" 2>/dev/null || true
wait "$watchdog" 2>/dev/null || true
line=$(cat "${TMPDIR:-/tmp}/android-map-smoke.$$")
# A process that exits right after its last line can stop the watchdog before
# logcat reads that line, so read what the process logged once more.
if [[ -z "$line" ]]; then
  line=$("$adb" logcat -d --pid="$pid" -v brief -e "$rendered|FATAL EXCEPTION" -m 1 || true)
fi
rm -f "${TMPDIR:-/tmp}/android-map-smoke.$$"
"$adb" shell am force-stop "$package"
if [[ "$line" == *"$rendered"* ]]; then
  echo "$rendered"
  exit 0
fi
echo "The Android map did not render a frame within ${timeout_seconds}s: ${line:-no log line}" >&2
exit 1
