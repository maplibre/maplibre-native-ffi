#!/usr/bin/env bash
# Runs a Windows test executable under Wine on a Linux host. Wine implements WGL
# over the host's OpenGL and Vulkan over the host's Vulkan driver, so Windows CI
# stays the authority on how the Windows targets behave.
set -euo pipefail

if [[ $# -eq 0 ]]; then
  echo "usage: run-wine-test.sh <executable> [arguments...]" >&2
  exit 2
fi
if [[ -z "${WINE:-}" ]]; then
  echo "Set WINE to a Wine executable, such as Proton's files/bin/wine." >&2
  exit 2
fi
if [[ ! -f "$1" ]]; then
  echo "The Windows test executable does not exist: $1" >&2
  exit 2
fi

# Each checkout keeps its own prefix, which Wine creates on first use.
export WINEPREFIX="${WINEPREFIX:-$MISE_MONOREPO_ROOT/build/wine-prefix}"
export WINEDEBUG="${WINEDEBUG:--all}"
# Proton's icu.dll forwards to the ICU release that Proton ships beside Wine,
# which only Proton's own launcher links into a prefix.
proton_icu="$(dirname "$WINE")/../lib/wine/icu/x86_64-windows"
if [[ -d "$proton_icu" ]]; then
  export WINEPATH="Z:$(cd "$proton_icu" && pwd)${WINEPATH:+;$WINEPATH}"
fi
export MLN_FFI_TEST_FIXTURE_DIR="$MISE_MONOREPO_ROOT/tests/native/fixtures"
exec timeout 300 "$WINE" "$@"
