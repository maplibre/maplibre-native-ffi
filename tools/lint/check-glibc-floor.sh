#!/usr/bin/env bash
# Verifies that a Linux shared object requires no glibc newer than the floor.
#
# Usage: check-glibc-floor.sh <floor> <library>
#
# The floor is a glibc version such as 2.17. The zig toolchain links the Linux
# natives against that glibc, so a newer requirement means the link bound a
# symbol of the build machine's glibc instead.
set -euo pipefail

floor="$1"
library="$2"

if [[ ! -f "$library" ]]; then
  echo "error: $library not found; build it first" >&2
  exit 1
fi

required="$(readelf --version-info "$library" |
  grep -oE 'GLIBC_[0-9]+\.[0-9]+' |
  sort -uV |
  tail -1)"

if [[ -z "$required" ]]; then
  echo "error: $library declares no glibc version requirement" >&2
  exit 1
fi

highest="$(printf '%s\nGLIBC_%s\n' "$required" "$floor" | sort -V | tail -1)"
if [[ "$highest" != "GLIBC_$floor" ]]; then
  echo "error: $library requires $required, above the GLIBC_$floor floor" >&2
  echo "The build host's glibc sets this floor; build on an older image or" >&2
  echo "raise glibc_floor in .config/mise.linux.toml to accept the new requirement." >&2
  exit 1
fi

echo "$library requires $required (floor GLIBC_$floor)"
