#!/usr/bin/env bash
# Stops the Android emulator that boot-android-emulator.sh started.
set -euo pipefail

serial=emulator-5554
state_dir="$MISE_MONOREPO_ROOT/build/android-emulator"
pid_file="$state_dir/emulator.pid"
adb="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}/platform-tools/adb"

if [[ ! -f "$pid_file" ]]; then
  echo "No mise-managed Android emulator is running."
  exit 0
fi

# `emu kill` lets the guest shut down; the signals below are for an emulator
# that no longer answers adb.
if [[ -x "$adb" ]]; then
  "$adb" -s "$serial" emu kill >/dev/null 2>&1 || true
fi

pid=$(<"$pid_file")
if [[ "$pid" =~ ^[0-9]+$ ]] && kill -0 "$pid" 2>/dev/null; then
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
