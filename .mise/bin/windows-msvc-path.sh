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

  local original_path="$PATH"
  original_path="$(cygpath -u -p "$original_path")"
  local vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
  local vswhere_unix
  vswhere_unix="$(cygpath -u "$vswhere")"
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
  : "${MLN_FFI_MSVC_HOST_ARCH:?MLN_FFI_MSVC_HOST_ARCH is required for Windows MSVC setup}"
  : "${MLN_FFI_MSVC_TARGET_ARCH:?MLN_FFI_MSVC_TARGET_ARCH is required for Windows MSVC setup}"
  local vs_dev_loader
  vs_dev_loader="$(mktemp "${TMPDIR:-/tmp}/mln-vsdev.XXXXXX.bat")"
  cat > "$vs_dev_loader" <<EOF
@echo off
set "PATH=%SystemRoot%\\System32;%SystemRoot%;%SystemRoot%\\System32\\Wbem;%SystemRoot%\\System32\\WindowsPowerShell\\v1.0"
call "$vs_dev_cmd" -arch=$MLN_FFI_MSVC_TARGET_ARCH -host_arch=$MLN_FFI_MSVC_HOST_ARCH >nul
set
EOF

  local vs_dev_loader_windows
  vs_dev_loader_windows="$(cygpath -w "$vs_dev_loader")"

  local msvc_env_raw
  if ! msvc_env_raw="$(cmd.exe //d //s //c "$vs_dev_loader_windows")"; then
    rm -f "$vs_dev_loader"
    echo "VsDevCmd.bat failed to initialize the Visual Studio environment" >&2
    return 1
  fi
  rm -f "$vs_dev_loader"

  local msvc_env
  msvc_env="$(tr -d '\r' <<< "$msvc_env_raw")"

  local name value msvc_path_windows=""
  while IFS='=' read -r name value; do
    [[ "$name" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || continue
    case "$name" in
      Path | PATH)
        msvc_path_windows="$value"
        continue
        ;;
    esac
    export "$name=$value"
  done <<< "$msvc_env"

  if [[ -z "${VCTOOLSINSTALLDIR:-${VCToolsInstallDir:-}}" ]]; then
    echo "VsDevCmd.bat did not provide VCToolsInstallDir" >&2
    return 1
  fi

  if [[ -z "$msvc_path_windows" ]]; then
    echo "VsDevCmd.bat did not provide Path" >&2
    return 1
  fi

  local msvc_path
  msvc_path="$(cygpath -u -p "$msvc_path_windows")"
  export PATH="$original_path${msvc_path:+:$msvc_path}"
}

normalize_windows_msvc_path() {
  local vc_tools_install_dir="${VCTOOLSINSTALLDIR:-${VCToolsInstallDir:-}}"
  [[ -n "$vc_tools_install_dir" ]] || {
    echo "VCToolsInstallDir is required after Visual Studio environment setup" >&2
    return 1
  }

  : "${MLN_FFI_MSVC_HOST_ARCH:?MLN_FFI_MSVC_HOST_ARCH is required for Windows MSVC setup}"
  : "${MLN_FFI_MSVC_TARGET_ARCH:?MLN_FFI_MSVC_TARGET_ARCH is required for Windows MSVC setup}"
  local msvc_bin
  msvc_bin="$(cygpath -u "${vc_tools_install_dir%\\}\\bin\\Host${MLN_FFI_MSVC_HOST_ARCH^}\\${MLN_FFI_MSVC_TARGET_ARCH}")"
  if [[ ! -x "$msvc_bin/link.exe" ]]; then
    echo "MSVC link.exe was not found at $msvc_bin/link.exe" >&2
    return 1
  fi

  local dependency_bin="" dependency_path="" rest_path="" path_entry
  if [[ -n "${MLN_FFI_DEPENDENCY_LIBRARY_DIR:-}" ]]; then
    dependency_bin="$(cygpath -u "$(dirname "$MLN_FFI_DEPENDENCY_LIBRARY_DIR")/bin")"
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

setup_windows_msvc_path() {
  if [[ -z "${MLN_FFI_MSVC_HOST_ARCH:-}" && -z "${MLN_FFI_MSVC_TARGET_ARCH:-}" ]]; then
    return 0
  fi

  : "${MLN_FFI_MSVC_HOST_ARCH:?MLN_FFI_MSVC_HOST_ARCH is required for Windows MSVC setup}"
  : "${MLN_FFI_MSVC_TARGET_ARCH:?MLN_FFI_MSVC_TARGET_ARCH is required for Windows MSVC setup}"
  load_windows_msvc_environment && normalize_windows_msvc_path
}

setup_windows_msvc_path
