#!/usr/bin/env bash
# Stops the Android emulator that boot-android-emulator.sh started in this
# checkout. Other emulators keep running.
set -euo pipefail

state_root="$MISE_MONOREPO_ROOT/build/android-emulator"
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"

shopt -s nullglob
pid_files=("$state_root"/emulator.pid "$state_root"/*/emulator.pid)
shopt -u nullglob
existing_pid_files=()
for pid_file in "${pid_files[@]}"; do
  [[ -f "$pid_file" ]] && existing_pid_files+=("$pid_file")
done
if ((${#existing_pid_files[@]} == 0)); then
  echo "No mise-managed Android emulator is running."
  exit 0
fi

# Act on a recorded PID only while it still belongs to a mise-managed AVD. An
# exited emulator leaves its files behind, and the OS may reuse its PID.
for pid_file in "${existing_pid_files[@]}"; do
  state_dir=${pid_file%/*}
  serial_file="$state_dir/serial"
  pid=$(<"$pid_file")
  if [[ "$pid" =~ ^[0-9]+$ ]] &&
    kill -0 "$pid" 2>/dev/null &&
    ps -p "$pid" -o args= | grep -q 'mln-ffi-'; then
    # `emu kill` lets the guest shut down; the signals below are for an
    # emulator that no longer answers adb. The kill goes to the recorded serial
    # only while the emulator there runs the AVD that names the state
    # directory, so an emulator that this checkout did not start keeps running.
    if [[ -x "$adb" && -f "$serial_file" ]]; then
      serial=$(<"$serial_file")
      running_avd=$("$adb" -s "$serial" emu avd name 2>/dev/null | sed -n '1s/\r$//p') ||
        running_avd=
      if [[ "$running_avd" == "${state_dir##*/}" ]]; then
        "$adb" -s "$serial" emu kill >/dev/null 2>&1 || true
      fi
    fi
    for ((attempt = 0; attempt < 30; attempt++)); do
      kill -0 "$pid" 2>/dev/null || break
      sleep 1
    done
    if kill -0 "$pid" 2>/dev/null; then
      kill "$pid"
      sleep 5
    fi
    if kill -0 "$pid" 2>/dev/null; then
      kill -KILL "$pid"
    fi
  fi
  rm -f "$pid_file" "$serial_file"
done
