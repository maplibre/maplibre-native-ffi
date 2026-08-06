#!/usr/bin/env bash
# Stops the OpenHarmony emulator that boot-ohos-emulator.sh started.
set -euo pipefail

pid_file="$MISE_MONOREPO_ROOT/build/ohos-emulator/qemu.pid"
if [[ ! -f "$pid_file" ]]; then
  echo "No mise-managed OpenHarmony emulator is running."
  exit 0
fi

pid=$(<"$pid_file")
if [[ "$pid" =~ ^[0-9]+$ ]] &&
  kill -0 "$pid" 2>/dev/null &&
  ps -p "$pid" -o args= | grep -q qemu-system-x86_64; then
  kill "$pid"
  for ((attempt = 0; attempt < 30; attempt++)); do
    kill -0 "$pid" 2>/dev/null || break
    sleep 1
  done
  if kill -0 "$pid" 2>/dev/null; then
    kill -KILL "$pid"
  fi
fi
rm -f "$pid_file"
hdc tconn 127.0.0.1:55555 -remove >/dev/null 2>&1 || true
