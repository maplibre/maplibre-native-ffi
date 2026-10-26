#!/usr/bin/env bash
# Prints the SSH host alias of this checkout's FreeBSD virtual machine. Each
# checkout boots its own, since the machine mounts the checkout it serves.
set -euo pipefail

checksum=$(printf '%s' "$MISE_MONOREPO_ROOT" | cksum | cut -d ' ' -f 1)
echo "mln-ffi-freebsd-$checksum"
