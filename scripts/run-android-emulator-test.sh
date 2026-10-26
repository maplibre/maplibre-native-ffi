#!/usr/bin/env bash
# Runs native test executables on ANDROID_SERIAL or on the shared emulator for
# the ABI and API level, which tools/devices/android-emulator boots when it is
# not running yet.
# Every executable runs and reports its own exit status; the first failure
# stops the batch.
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <timeout-seconds> <abi> <native-library> [--api <api>] [--library <shared-library>]... [test-argument ...] -- <test-executable ...>" >&2
  exit 2
fi

timeout_seconds=$1
abi=$2
native_library=$3
shift 3
emulator_api=
# Further shared libraries a test executable loads, such as a plugin, travel
# beside the C API library.
extra_libraries=()
while (($#)); do
  case $1 in
    --api)
      emulator_api=${2:?--api requires an Android API level}
      shift 2
      ;;
    --library)
      extra_libraries+=("${2:?--library requires a shared library}")
      shift 2
      ;;
    *) break ;;
  esac
done
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

timeout_scale=${MLN_TEST_TIMEOUT_SCALE:-3}
# Other sessions may be running tests on the same shared emulator, so each run
# has a directory of its own and removes it when it ends.
remote_dir=/data/local/tmp/maplibre-native-ffi-$$
fixture_dir=${MLN_FFI_TEST_FIXTURE_DIR:-}
# The AVD of the emulator the tests run on, once the device is known; a device
# that is no emulator has none.
avd_name=
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"

# When adb's stdin ends, adb tells the device's shell, and Android 8.0's adbd
# drops the exit status of a shell that exited just before that message came.
# adb then exits 255. A FIFO that this script holds open for reading and writing
# never ends, so every adb call it runs inherits a stdin that adb never reports
# closed.
stdin_dir=$(mktemp -d)
mkfifo "$stdin_dir/stdin"
exec 0<>"$stdin_dir/stdin"
rm -r "$stdin_dir"

for local_file in "$native_library" ${extra_libraries[@]+"${extra_libraries[@]}"} "${test_executables[@]}"; do
  if [[ ! -f "$local_file" ]]; then
    echo "Android test input does not exist: $local_file" >&2
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
  echo "Android fixture directory does not exist: $fixture_dir" >&2
  exit 2
fi

# Says whether the emulator at the serial still runs, and if it exited, prints
# the tail of the log that tools/devices/android-emulator started it with, which holds
# the reason the emulator gives. A device that is no emulator reports nothing.
report_emulator_state() {
  local log_dir=${TMPDIR:-/tmp}
  [[ -n "$avd_name" ]] || return 0
  if [[ "$(perl -e 'alarm shift; exec @ARGV' 30 "$adb" -s "$serial" emu avd name 2>/dev/null |
    sed -n 's/\r$//;1p')" == "$avd_name" ]]; then
    echo "The Android emulator at $serial is still running." >&2
  else
    echo "The Android emulator at $serial exited." >&2
    if [[ -f "${log_dir%/}/android-emulator-$avd_name.log" ]]; then
      echo "The end of its log:" >&2
      tail -50 "${log_dir%/}/android-emulator-$avd_name.log" >&2
    fi
  fi
}

# Prints what the device logged before a failure: adbd's and init's lines, which
# show adbd restarting, then the end of the whole log. A device that adb can no
# longer reach says so instead.
report_device_log() {
  local log
  if ! log=$(perl -e 'alarm shift; exec @ARGV' 30 "$adb" -s "$serial" logcat -d -b all 2>&1); then
    echo "adb could not read the device log: $(tail -1 <<<"$log")" >&2
    return
  fi
  echo "adbd and init lines in the device log:" >&2
  { grep -E 'adbd|init *:' <<<"$log" || true; } | tail -20 >&2
  echo "The end of the device log:" >&2
  tail -40 <<<"$log" >&2
}

# Names the adb call that failed and reports the state of the device it ran on.
report_adb_failure() {
  echo "adb exited $2 during $1." >&2
  report_emulator_state
  report_device_log
}

# Runs one adb call against the device, and when it fails, reports it under the
# name that the first argument gives and exits with adb's status.
run_adb() {
  local call=$1 status
  shift
  "$adb" -s "$serial" "$@" && return 0
  status=$?
  report_adb_failure "$call" "$status"
  exit "$status"
}

# An explicit serial selects a connected device. Otherwise the shared emulator
# for the ABI and API level runs the tests, booted first if it is not running.
emulator="$MISE_MONOREPO_ROOT/tools/devices/android-emulator"
if [[ -n "${ANDROID_SERIAL:-}" ]]; then
  serial=$ANDROID_SERIAL
  device_abi=$(run_adb "the ABI query" shell getprop ro.product.cpu.abi | tr -d '\r')
  device_api=$(run_adb "the API level query" shell getprop ro.build.version.sdk | tr -d '\r')
  if [[ "$device_abi" != "$abi" ]] ||
    [[ -n "$emulator_api" && "$device_api" != "$emulator_api" ]]; then
    echo "Android device $serial has ABI $device_abi and API $device_api; expected $abi ${emulator_api:+at API $emulator_api}." >&2
    exit 2
  fi
else
  emulator_api=${emulator_api:-${MLN_FFI_ANDROID_EMULATOR_API:?MLN_FFI_ANDROID_EMULATOR_API must name the default emulator API level}}
  if ! serial=$("$emulator" serial "$abi" --api "$emulator_api"); then
    mise run //:android-emulator:boot "$abi" --api "$emulator_api"
    serial=$("$emulator" serial "$abi" --api "$emulator_api")
  fi
fi
avd_name=$("$adb" -s "$serial" emu avd name 2>/dev/null | sed -n 's/\r$//;1p') || avd_name=

remove_remote_dir() {
  perl -e 'alarm shift; exec @ARGV' 30 \
    "$adb" -s "$serial" shell "rm -rf '$remote_dir'" >/dev/null 2>&1 || true
}
trap remove_remote_dir EXIT

# The shell user may execute what it owns under /data/local/tmp. The Android
# build links the C++ runtime statically, so the C API library pushed beside the
# executable is the only one it loads from here.
run_adb "the shell that prepares $remote_dir" shell "mkdir -p '$remote_dir/tmp'"
run_adb "the push of $(basename "$native_library")" push "$native_library" "$remote_dir/libmaplibre-native-c.so" >/dev/null
for library in ${extra_libraries[@]+"${extra_libraries[@]}"}; do
  run_adb "the push of $(basename "$library")" push "$library" "$remote_dir/$(basename "$library")" >/dev/null
done

fixture_environment=
if [[ -n "$fixture_dir" ]]; then
  run_adb "the push of the fixtures" push "$fixture_dir" "$remote_dir/fixtures" >/dev/null
  fixture_environment="MLN_FFI_TEST_FIXTURE_DIR='$remote_dir/fixtures' "
fi

for test_executable in "${test_executables[@]}"; do
  test_name=$(basename "$test_executable")
  echo "Running $test_name on Android device $serial."
  run_adb "the push of $test_name" push "$test_executable" "$remote_dir/test-executable" >/dev/null

  # Android has no /tmp, which is where a runtime library falls back to when
  # TMPDIR is unset, so a test that asks for a temporary directory gets one here.
  # The emulator renders in software, so waits and the native hang watchdog
  # stretch by the timeout scale.
  remote_command="cd '$remote_dir' && chmod 755 test-executable && ${fixture_environment}MLN_TEST_TIMEOUT_SCALE='$timeout_scale' TMPDIR='$remote_dir/tmp' LD_LIBRARY_PATH='$remote_dir' ./test-executable"
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
  if [[ ! "$test_status" =~ ^[0-9]+$ ]]; then
    echo "$test_name returned no exit status; adb exited $transport_status." >&2
    report_emulator_state
    report_device_log
    exit $((transport_status != 0 ? transport_status : 1))
  fi
  if ((transport_status != 0)); then
    report_adb_failure "the shell that ran $test_name, after it reported exit status $test_status" "$transport_status"
    exit "$transport_status"
  fi
  if ((test_status != 0)); then
    exit "$test_status"
  fi
  run_adb "the cleanup after $test_name" shell "rm -f '$remote_dir/test-executable'"
done
