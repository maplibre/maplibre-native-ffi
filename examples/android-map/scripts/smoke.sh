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

mise run //:android-emulator:boot "$abi"
export ANDROID_SERIAL=${ANDROID_SERIAL:-emulator-5554}
adb="$ANDROID_HOME/platform-tools/adb"

./gradlew \
  -Pmaplibre.android.backend="$backend" \
  -Pmaplibre.android.abis="$abi" \
  -Pmaplibre.android.prebuiltBuildRoot=build \
  :examples:android-map:installDebug

"$adb" logcat -c
"$adb" shell am start -S -W -n "$package/.MainActivity" --ez smoke true

# logcat exits at the first line that matches. The watchdog ends it when no
# frame renders in time, and a crash of the app ends the wait early.
"$adb" logcat -v brief -e "$rendered|FATAL EXCEPTION" -m 1 >"${TMPDIR:-/tmp}/android-map-smoke.$$" &
logcat=$!
(
  deadline=$((SECONDS + timeout_seconds))
  while ((SECONDS < deadline)) && kill -0 "$logcat" 2>/dev/null; do
    sleep 1
  done
  kill "$logcat" 2>/dev/null || true
) &
watchdog=$!
wait "$logcat" || true
kill "$watchdog" 2>/dev/null || true
wait "$watchdog" 2>/dev/null || true
line=$(cat "${TMPDIR:-/tmp}/android-map-smoke.$$")
rm -f "${TMPDIR:-/tmp}/android-map-smoke.$$"
"$adb" shell am force-stop "$package"
if [[ "$line" == *"$rendered"* ]]; then
  echo "$rendered"
  exit 0
fi
echo "The Android map did not render a frame within ${timeout_seconds}s: ${line:-no log line}" >&2
exit 1
