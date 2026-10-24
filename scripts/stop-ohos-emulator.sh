#!/usr/bin/env bash
# Stops the shared OpenHarmony emulator, whoever started it. Every session on
# the machine may be using that emulator, so no task stops it on its own; this
# is for a person who knows that none is.
set -euo pipefail

if ! process=$("$MISE_MONOREPO_ROOT/scripts/ohos-emulator-process.sh"); then
  echo "OpenHarmony emulator is not running."
  exit 0
fi
read -r pid connect_key <<<"$process"
# Disconnect hdc before QEMU closes the forwarded socket. This keeps the
# listener port reusable when another suite starts the emulator immediately.
hdc tconn "$connect_key" -remove >/dev/null 2>&1 || true

kill "$pid" 2>/dev/null || exit 0
for ((attempt = 0; attempt < 30; attempt++)); do
  kill -0 "$pid" 2>/dev/null || exit 0
  sleep 1
done
kill -KILL "$pid" 2>/dev/null || true
