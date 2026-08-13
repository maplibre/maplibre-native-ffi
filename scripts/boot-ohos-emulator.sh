#!/usr/bin/env bash
# Boots the OpenHarmony emulator from the Oniro QEMU image and waits until it
# answers hdc. Idempotent: an already-ready emulator, or one this script
# started earlier, is reused.
set -euo pipefail

ohos_sdk_version=${1:?usage: boot-ohos-emulator.sh <ohos-sdk-version>}
connect_key=${MLN_FFI_OHOS_EMULATOR_CONNECT_KEY:-127.0.0.1:55555}
state_dir="$MISE_MONOREPO_ROOT/build/ohos-emulator"
pid_file="$state_dir/qemu.pid"
log_file="$state_dir/qemu.log"

if hdc tconn "$connect_key" >/dev/null 2>&1 &&
  hdc -t "$connect_key" shell echo ready 2>/dev/null | tr -d '\r' | grep -qx ready; then
  echo "OpenHarmony emulator is ready at $connect_key."
  exit 0
fi

mkdir -p "$state_dir"
if [[ -f "$pid_file" ]]; then
  pid=$(<"$pid_file")
  if [[ "$pid" =~ ^[0-9]+$ ]] &&
    kill -0 "$pid" 2>/dev/null &&
    ps -p "$pid" -o args= | grep -q qemu-system-x86_64; then
    echo "Waiting for OpenHarmony emulator process $pid."
  else
    rm -f "$pid_file"
  fi
fi

if [[ ! -f "$pid_file" ]]; then
  emulator_dir="$(mise where "http:oniro-emulator@$ohos_sdk_version")/images"
  launcher=("$MISE_MONOREPO_ROOT/scripts/run-ohos-emulator.sh" "$emulator_dir")
  if command -v setsid >/dev/null 2>&1; then
    nohup setsid "${launcher[@]}" </dev/null >"$log_file" 2>&1 &
  else
    nohup "${launcher[@]}" </dev/null >"$log_file" 2>&1 &
  fi
  pid=$!
  printf '%s\n' "$pid" >"$pid_file"
  echo "Started OpenHarmony emulator process $pid."
fi

pid=$(<"$pid_file")
for ((attempt = 0; attempt < 300; attempt++)); do
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "OpenHarmony emulator exited before it became ready." >&2
    tail -100 "$log_file" >&2
    rm -f "$pid_file"
    exit 1
  fi
  if hdc tconn "$connect_key" >/dev/null 2>&1 &&
    hdc -t "$connect_key" shell echo ready 2>/dev/null | tr -d '\r' | grep -qx ready; then
    echo "OpenHarmony emulator is ready at $connect_key."
    exit 0
  fi
  sleep 1
done

echo "OpenHarmony emulator did not become ready within 300 seconds." >&2
tail -100 "$log_file" >&2
exit 1
