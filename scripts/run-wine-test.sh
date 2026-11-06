#!/usr/bin/env bash
# Runs a Windows test executable under Wine on a Linux host. Wine implements WGL
# over the host's OpenGL and Vulkan over the host's Vulkan driver, so Windows CI
# stays the authority on how the Windows targets behave. The executable gets
# MLN_FFI_DEVICE_TIMEOUT seconds, or 300.
#
# By default it runs under Steam's Proton Experimental, whose launcher sets up
# the prefix: fonts, ICU, DXVK, and vkd3d-proton, which .NET and WPF need. Set
# WINE to use another Wine. When it's a Proton's files/bin/wine, that Proton's
# launcher still sets up the prefix. Any other Wine gets a bare prefix.
set -euo pipefail

if [[ $# -eq 0 ]]; then
  echo "usage: run-wine-test.sh <executable> [arguments...]" >&2
  exit 2
fi
if [[ ! -f "$1" ]]; then
  echo "The Windows test executable does not exist: $1" >&2
  exit 2
fi

if [[ -n "${WINE:-}" ]]; then
  proton="$(dirname "$WINE")/../../proton"
else
  proton=
  for steam in "$HOME/.local/share/Steam" "$HOME/.steam/steam" \
    "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam"; do
    if [[ -f "$steam/steamapps/common/Proton - Experimental/proton" ]]; then
      proton="$steam/steamapps/common/Proton - Experimental/proton"
      break
    fi
  done
  if [[ -z "$proton" ]]; then
    echo "Install Proton Experimental from Steam, or set WINE to a Wine executable." >&2
    exit 2
  fi
fi

export WINEDEBUG="${WINEDEBUG:--all}"
export DXVK_LOG_LEVEL="${DXVK_LOG_LEVEL:-none}"
export VKD3D_DEBUG="${VKD3D_DEBUG:-none}"
export MLN_FFI_TEST_FIXTURE_DIR="$MISE_MONOREPO_ROOT/tests/native/fixtures"
timeout_seconds="${MLN_FFI_DEVICE_TIMEOUT:-300}"

if [[ ! -f "$proton" ]]; then
  # Each checkout keeps its own prefix, which Wine creates on first use.
  export WINEPREFIX="${WINEPREFIX:-$MISE_MONOREPO_ROOT/build/wine-prefix}"
  exec timeout "$timeout_seconds" "$WINE" "$@"
fi

# Each checkout keeps its own prefix under build/proton, in compat data's pfx.
export STEAM_COMPAT_DATA_PATH="${STEAM_COMPAT_DATA_PATH:-$MISE_MONOREPO_ROOT/build/proton}"
mkdir -p "$STEAM_COMPAT_DATA_PATH"
# Prefix setup reads the Steam client's directory, which can be empty. Proton's
# own drives would map it into the prefix, and no test needs it.
if [[ -z "${STEAM_COMPAT_CLIENT_INSTALL_PATH:-}" ]]; then
  export STEAM_COMPAT_CLIENT_INSTALL_PATH="$STEAM_COMPAT_DATA_PATH/steam"
  mkdir -p "$STEAM_COMPAT_CLIENT_INSTALL_PATH"
fi
# Every verb but runinprefix creates or updates the prefix, under Proton's lock.
# getcompatpath only adds a path lookup. The run and waitforexitandrun verbs
# start the executable through Proton's steam.exe, which needs a Steam client,
# and they drop its output and exit status.
python3 "$proton" getcompatpath / >/dev/null
exec timeout "$timeout_seconds" python3 "$proton" runinprefix "$@"
