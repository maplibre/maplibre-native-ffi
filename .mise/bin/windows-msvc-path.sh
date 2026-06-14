#!/usr/bin/env bash

# Git Bash puts /usr/bin before MSVC. Initialize the MSVC environment when
# needed and move MSVC's bin ahead of Git so plain link.exe resolves correctly.

append_unique() {
  local -n list="$1"
  local item="${2%/}"
  [[ -n "$item" ]] || return 0
  case ":$list:" in
    *":$item:"*) ;;
    *) list="${list:+$list:}$item" ;;
  esac
}

load_windows_msvc_environment() {
  if [[ -n "${VCTOOLSINSTALLDIR:-${VCToolsInstallDir:-}}" ]]; then
    return 0
  fi
  command -v cmd.exe >/dev/null 2>&1 || return 0

  local original_path="$PATH"
  local vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
  local vswhere_unix="$vswhere"
  if command -v cygpath >/dev/null 2>&1; then
    vswhere_unix="$(cygpath -u "$vswhere")"
  fi
  if [[ ! -x "$vswhere_unix" ]]; then
    echo "vswhere.exe was not found at $vswhere" >&2
    return 1
  fi

  local vs_install
  vs_install="$(
    "$vswhere_unix" -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath \
      | tr -d '\r' \
      | sed -n '1p'
  )"
  if [[ -z "$vs_install" ]]; then
    echo "Visual Studio 2022 C++ tools were not found" >&2
    return 1
  fi

  local vs_dev_cmd="${vs_install}\\Common7\\Tools\\VsDevCmd.bat"
  local vs_dev_loader
  vs_dev_loader="$(mktemp "${TMPDIR:-/tmp}/mln-vsdev.XXXXXX.bat")"
  cat > "$vs_dev_loader" <<EOF
@echo off
call "$vs_dev_cmd" -arch=x64 -host_arch=x64 >nul
set
EOF

  local vs_dev_loader_windows="$vs_dev_loader"
  if command -v cygpath >/dev/null 2>&1; then
    vs_dev_loader_windows="$(cygpath -w "$vs_dev_loader")"
  fi

  local msvc_env_raw
  if ! msvc_env_raw="$(cmd.exe //d //s //c "$vs_dev_loader_windows")"; then
    rm -f "$vs_dev_loader"
    echo "VsDevCmd.bat failed to initialize the Visual Studio environment" >&2
    return 1
  fi
  rm -f "$vs_dev_loader"

  local msvc_env
  msvc_env="$(tr -d '\r' <<< "$msvc_env_raw")"

  local name value
  while IFS='=' read -r name value; do
    [[ "$name" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || continue
    export "$name=$value"
  done <<< "$msvc_env"

  if [[ -z "${VCTOOLSINSTALLDIR:-${VCToolsInstallDir:-}}" ]]; then
    echo "VsDevCmd.bat did not provide VCToolsInstallDir" >&2
    return 1
  fi

  if [[ -n "${Path:-}" ]]; then
    local msvc_path="$Path"
    if command -v cygpath >/dev/null 2>&1; then
      msvc_path="$(cygpath -u -p "$Path")"
    fi
    export PATH="$msvc_path${original_path:+:$original_path}"
  fi
}

normalize_windows_msvc_path() {
  local vc_tools_install_dir="${VCTOOLSINSTALLDIR:-${VCToolsInstallDir:-}}"
  [[ -n "$vc_tools_install_dir" ]] || return 0
  command -v cygpath >/dev/null 2>&1 || return 0

  local host_arch="${VSCMD_ARG_HOST_ARCH:-x64}"
  local target_arch="${VSCMD_ARG_TGT_ARCH:-x64}"
  local msvc_bin
  msvc_bin="$(cygpath -u "${vc_tools_install_dir%\\}\\bin\\Host${host_arch^}\\${target_arch}")"
  [[ -x "$msvc_bin/link.exe" ]] || return 0

  local dependency_bin="" dependency_path="" rest_path="" path_entry
  if [[ -n "${MLN_FFI_DEPS_PREFIX:-}" ]]; then
    dependency_bin="$(cygpath -u "$MLN_FFI_DEPS_PREFIX/bin")"
    dependency_bin="${dependency_bin%/}"
  fi

  IFS=":" read -ra path_entries <<< "$PATH"
  for path_entry in "${path_entries[@]}"; do
    path_entry="${path_entry%/}"
    [[ "$path_entry" != "$msvc_bin" ]] || continue
    if [[ -n "$dependency_bin" && "$path_entry" == "$dependency_bin" ]]; then
      append_unique dependency_path "$path_entry"
    else
      append_unique rest_path "$path_entry"
    fi
  done

  export PATH="${dependency_path:+$dependency_path:}$msvc_bin${rest_path:+:$rest_path}"
}

load_windows_msvc_environment
normalize_windows_msvc_path
