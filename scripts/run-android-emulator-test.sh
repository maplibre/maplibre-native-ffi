#!/usr/bin/env bash
# Runs native test executables in the Android emulator, booting it when needed.
# Every executable runs and reports its own exit status; the first failure
# stops the batch.
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <timeout-seconds> <abi> <native-library> [--api <api>] [test-argument ...] -- <test-executable ...>" >&2
  exit 2
fi

timeout_seconds=$1
abi=$2
native_library=$3
shift 3
emulator_api=
if [[ ${1:-} == --api ]]; then
  emulator_api=${2:?--api requires an Android API level}
  shift 2
fi
test_arguments=()
while (($#)) && [[ $1 != -- ]]; do
  test_arguments+=("$1")
  shift
done
if [[ ${1:-} == -- ]]; then
  shift
fi
if (($# == 0)); then
  echo "No test executables given." >&2
  exit 2
fi
test_executables=("$@")

serial=emulator-5554
remote_dir=/data/local/tmp/maplibre-native-ffi
fixture_dir=${MLN_FFI_TEST_FIXTURE_DIR:-}
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"

for local_file in "$native_library" "${test_executables[@]}"; do
  if [[ ! -f "$local_file" ]]; then
    echo "Android emulator test input does not exist: $local_file" >&2
    exit 2
  fi
done
if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi
if [[ -n "$emulator_api" && ! "$emulator_api" =~ ^[0-9]+$ ]]; then
  echo "Invalid Android API level: $emulator_api" >&2
  exit 2
fi
if [[ -n "$fixture_dir" && ! -d "$fixture_dir" ]]; then
  echo "Android emulator fixture directory does not exist: $fixture_dir" >&2
  exit 2
fi

# platform-tools arrives with the first boot, so a missing adb means boot, not
# failure.
if [[ -n "$emulator_api" ]] || [[ ! -x "$adb" ]] ||
  ! "$adb" -s "$serial" shell getprop sys.boot_completed 2>/dev/null |
  tr -d '\r' | grep -qx 1 ||
  ! "$adb" -s "$serial" shell getprop ro.product.cpu.abi 2>/dev/null |
  tr -d '\r' | grep -qx "$abi"; then
  emulator_args=("$abi")
  if [[ -n "$emulator_api" ]]; then
    emulator_args+=(--api "$emulator_api")
  fi
  mise run //:android-emulator:boot "${emulator_args[@]}"
fi

# The shell user may execute what it owns under /data/local/tmp. The Android
# build links the C++ runtime statically, so the C API library pushed beside the
# executable is the only one it loads from here.
"$adb" -s "$serial" shell "rm -rf '$remote_dir' && mkdir -p '$remote_dir/tmp'"
"$adb" -s "$serial" push "$native_library" "$remote_dir/libmaplibre-native-c.so" >/dev/null

fixture_environment=
if [[ -n "$fixture_dir" ]]; then
  "$adb" -s "$serial" push "$fixture_dir" "$remote_dir/fixtures" >/dev/null
  fixture_environment="MLN_FFI_TEST_FIXTURE_DIR='$remote_dir/fixtures' "
fi

for test_executable in "${test_executables[@]}"; do
  echo "Running $(basename "$test_executable") in the Android emulator."
  "$adb" -s "$serial" push "$test_executable" "$remote_dir/test-executable" >/dev/null

  # Android has no /tmp, which is where a runtime library falls back to when
  # TMPDIR is unset, so a test that asks for a temporary directory gets one here.
  remote_command="cd '$remote_dir' && chmod 755 test-executable && ${fixture_environment}TMPDIR='$remote_dir/tmp' LD_LIBRARY_PATH='$remote_dir' ./test-executable"
  for argument in ${test_arguments[@]+"${test_arguments[@]}"}; do
    printf -v quoted_argument '%q' "$argument"
    remote_command+=" $quoted_argument"
  done
  # `adb shell` reports the transport's status rather than the command's on every
  # path, so the exit status travels back in the output instead.
  remote_command+="; test_status=\$?; printf '\n__MLN_FFI_TEST_STATUS__=%s\n' \"\$test_status\""

  output_file=$(mktemp)
  set +e
  perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" \
    "$adb" -s "$serial" shell "$remote_command" | tee "$output_file"
  transport_status=${PIPESTATUS[0]}
  set -e
  test_status=$(sed -n 's/^__MLN_FFI_TEST_STATUS__=//p' "$output_file" | tail -1 | tr -d '\r')
  rm -f "$output_file"
  if ((transport_status != 0)); then
    exit "$transport_status"
  fi
  if [[ ! "$test_status" =~ ^[0-9]+$ ]]; then
    echo "Android emulator test returned no exit status." >&2
    exit 1
  fi
  if ((test_status != 0)); then
    exit "$test_status"
  fi
  "$adb" -s "$serial" shell "rm -f '$remote_dir/test-executable'"
done
