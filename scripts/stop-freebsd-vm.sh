#!/usr/bin/env bash
# Shuts down this checkout's FreeBSD virtual machine and waits until it is off.
set -euo pipefail

host=$("$MISE_MONOREPO_ROOT/scripts/freebsd-vm-host.sh")
if ! ssh -o BatchMode=yes -o ConnectTimeout=10 "$host" true 2>/dev/null; then
  echo "FreeBSD virtual machine is not running."
  exit 0
fi
ssh -o BatchMode=yes "$host" shutdown -p now >/dev/null 2>&1 || true
for ((attempt = 0; attempt < 60; attempt++)); do
  ssh -o BatchMode=yes -o ConnectTimeout=5 "$host" true 2>/dev/null || exit 0
  sleep 1
done
echo "FreeBSD virtual machine at $host did not shut down." >&2
exit 1
