#!/usr/bin/env bash
# Stops the OpenHarmony emulator that boot-ohos-emulator.sh started.
set -euo pipefail

pid_file="$MISE_MONOREPO_ROOT/build/ohos-emulator/qemu.pid"
connect_key=${MLN_FFI_OHOS_EMULATOR_CONNECT_KEY:-127.0.0.1:55555}
if [[ ! -f "$pid_file" ]]; then
  echo "No mise-managed OpenHarmony emulator is running."
  exit 0
fi
# Disconnect hdc before QEMU closes the forwarded socket. This keeps the
# listener port reusable when another suite starts the emulator immediately.
hdc tconn "$connect_key" -remove >/dev/null 2>&1 || true

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
