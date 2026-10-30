#!/usr/bin/env bash
# Builds the Android package consumer, and runs the portable binding suite on emulator targets.
set -euo pipefail

preset=${1:?usage: test-dotnet-android.sh <preset> [--build-only]}
case "$preset" in
  android-arm-*) abi=armeabi-v7a; rid=android-arm ;;
  android-arm64-*) abi=arm64-v8a; rid=android-arm64 ;;
  android-x86-*) abi=x86; rid=android-x86 ;;
  android-x64-*) abi=x86_64; rid=android-x64 ;;
  *) echo "The .NET Android suite cannot build $preset." >&2; exit 2 ;;
esac
case "$preset" in
  *-egl) backend=OpenGL ;;
  *-vulkan) backend=Vulkan ;;
esac

dotnet workload install android --skip-manifest-update --verbosity quiet
mise run //:android-sdk-packages
python tools/release/dotnet_release.py prepare-android "$preset"
work="$MISE_MONOREPO_ROOT/build/$preset/dotnet-android-test"
project=bindings/dotnet/tests/Maplibre.NativeFfi.AndroidTest/Maplibre.NativeFfi.AndroidTest.csproj
arguments=(
  "$project" -c Release -r "$rid"
  "-p:MaplibreNativeCInstallDir=$MISE_MONOREPO_ROOT/build/$preset/install"
  "-p:MaplibreAndroidAbi=$abi"
  "-p:MaplibreNativeFfiPackageVersion=0.0.0-android-ci"
  "-p:MaplibreNativeFfiRenderBackend=$backend"
  "-p:BaseOutputPath=$work/app/"
  "-p:RestorePackagesPath=$work/restore"
  "-p:RestoreAdditionalProjectSources=$work/packages"
  "-p:AndroidSdkDirectory=$ANDROID_HOME"
)
if [[ ${2:-} == --build-only ]]; then
  exec dotnet build "${arguments[@]}"
fi

api=$MLN_FFI_ANDROID_EMULATOR_API
if [[ "$preset" == android-x64-egl ]]; then
  api=26
fi
serial=$("$MISE_MONOREPO_ROOT/tools/devices/android-serial" "$abi" --api "$api")
adb="$ANDROID_HOME/platform-tools/adb"
# API 26's logd can reject clearing its buffers. Read only this invocation's
# output using the device's clock instead.
started=$("$adb" -s "$serial" shell "date '+%m-%d %H:%M:%S.000'" | tr -d '\r')
dotnet build "${arguments[@]}" -t:Run "-p:AdbTarget=-s $serial"
# Only the app's test output travels through logcat. A crash or a hang cannot
# count as success: the host must receive the runner's terminal status.
python - "$adb" "$serial" "$started" <<'PY'
import signal
import subprocess
import sys

signal.signal(signal.SIGALRM, lambda *_: sys.exit("Android test app timed out"))
signal.alarm(300)
with subprocess.Popen(
    [sys.argv[1], "-s", sys.argv[2], "logcat", "-T", sys.argv[3], "-v", "raw", "-s", "MaplibreDotnetTests:I"],
    stdout=subprocess.PIPE,
    text=True,
) as log:
    try:
        for line in log.stdout:
            print(line, end="", flush=True)
            if line.startswith("__MLN_DOTNET_TEST_STATUS__="):
                raise SystemExit(int(line.partition("=")[2]))
        raise SystemExit("Android test app returned no status")
    finally:
        log.terminate()
PY
