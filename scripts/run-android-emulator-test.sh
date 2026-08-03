#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
  echo "usage: $0 <test-executable> <native-library> <timeout-seconds> [test-argument ...]" >&2
  exit 2
fi

test_executable=$1
native_library=$2
timeout_seconds=$3
shift 3
serial=emulator-5554
remote_dir=/data/local/tmp/maplibre-native-ffi
fixture_dir=${MLN_FFI_TEST_FIXTURE_DIR:-}
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"

for local_file in "$test_executable" "$native_library"; do
  if [[ ! -f "$local_file" ]]; then
    echo "Android emulator test input does not exist: $local_file" >&2
    exit 2
  fi
done
if [[ ! -x "$adb" ]]; then
  echo "adb does not exist: $adb" >&2
  exit 2
fi
if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi
if [[ -n "$fixture_dir" && ! -d "$fixture_dir" ]]; then
  echo "Android emulator fixture directory does not exist: $fixture_dir" >&2
  exit 2
fi

if ! "$adb" -s "$serial" shell getprop sys.boot_completed 2>/dev/null |
  tr -d '\r' | grep -qx 1; then
  echo "No Android emulator is ready. Run 'mise run //:android-emulator:boot' first." >&2
  exit 2
fi

# The shell user may execute what it owns under /data/local/tmp. The Android
# build links the C++ runtime statically, so the C API library pushed beside the
# executable is the only one it loads from here.
"$adb" -s "$serial" shell "rm -rf '$remote_dir' && mkdir -p '$remote_dir/tmp'"
"$adb" -s "$serial" push "$test_executable" "$remote_dir/test-executable" >/dev/null
"$adb" -s "$serial" push "$native_library" "$remote_dir/libmaplibre-native-c.so" >/dev/null

fixture_environment=
if [[ -n "$fixture_dir" ]]; then
  "$adb" -s "$serial" push "$fixture_dir" "$remote_dir/fixtures" >/dev/null
  fixture_environment="MLN_FFI_TEST_FIXTURE_DIR='$remote_dir/fixtures' "
fi

# Android has no /tmp, which is where a runtime library falls back to when
# TMPDIR is unset, so a test that asks for a temporary directory gets one here.
remote_command="cd '$remote_dir' && chmod 755 test-executable && ${fixture_environment}TMPDIR='$remote_dir/tmp' LD_LIBRARY_PATH='$remote_dir' ./test-executable"
for argument in "$@"; do
  printf -v quoted_argument '%q' "$argument"
  remote_command+=" $quoted_argument"
done
# `adb shell` reports the transport's status rather than the command's on every
# path, so the exit status travels back in the output instead.
remote_command+="; test_status=\$?; printf '\n__MLN_FFI_TEST_STATUS__=%s\n' \"\$test_status\""

output_file=$(mktemp)
trap 'find "$output_file" -delete' EXIT
set +e
perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" \
  "$adb" -s "$serial" shell "$remote_command" | tee "$output_file"
transport_status=${PIPESTATUS[0]}
set -e
if ((transport_status != 0)); then
  exit "$transport_status"
fi
test_status=$(sed -n 's/^__MLN_FFI_TEST_STATUS__=//p' "$output_file" | tail -1 | tr -d '\r')
if [[ ! "$test_status" =~ ^[0-9]+$ ]]; then
  echo "Android emulator test returned no exit status." >&2
  exit 1
fi
exit "$test_status"
