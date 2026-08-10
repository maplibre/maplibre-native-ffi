#!/usr/bin/env bash
# Stops the Android emulator that boot-android-emulator.sh started.
set -euo pipefail

serial=emulator-5554
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

# `emu kill` lets the guest shut down; the signals below are for an emulator
# that no longer answers adb.
if [[ -x "$adb" ]]; then
  "$adb" -s "$serial" emu kill >/dev/null 2>&1 || true
fi

# Signal a recorded PID only while it still belongs to a mise-managed AVD. An
# exited emulator leaves the file behind, and the OS may reuse its PID.
for pid_file in "${existing_pid_files[@]}"; do
  pid=$(<"$pid_file")
  if [[ "$pid" =~ ^[0-9]+$ ]] &&
    kill -0 "$pid" 2>/dev/null &&
    ps -p "$pid" -o args= | grep -q 'mln-ffi-'; then
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
  rm -f "$pid_file"
done
