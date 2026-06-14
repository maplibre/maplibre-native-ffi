#!/usr/bin/env bash

# Git Bash puts /usr/bin before MSVC. Move MSVC's bin ahead of Git so plain
# link.exe resolves to MSVC's linker.

append_unique() {
  local -n list="$1"
  local item="${2%/}"
  [[ -n "$item" ]] || return 0
  case ":$list:" in
    *":$item:"*) ;;
    *) list="${list:+$list:}$item" ;;
  esac
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

normalize_windows_msvc_path
