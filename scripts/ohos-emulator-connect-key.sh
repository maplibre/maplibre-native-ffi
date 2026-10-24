#!/usr/bin/env bash
# Prints the hdc connect key of the shared OpenHarmony emulator once it answers
# hdc, whoever started it. Prints nothing and exits 1 when it is not running or
# not ready yet, so a caller can boot it and ask again.
set -euo pipefail

process=$("$(dirname "$0")/ohos-emulator-process.sh") || exit 1
read -r _ connect_key <<<"$process"
if hdc tconn "$connect_key" >/dev/null 2>&1 &&
  hdc -t "$connect_key" shell echo ready 2>/dev/null | tr -d '\r' | grep -qx ready; then
  printf '%s\n' "$connect_key"
  exit 0
fi
exit 1
