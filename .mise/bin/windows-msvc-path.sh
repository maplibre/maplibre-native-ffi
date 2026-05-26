#!/usr/bin/env bash

# Pixi activates MSVC, but Git Bash puts /usr/bin before MSVC. Keep pixi first
# and move MSVC's bin ahead of Git so plain link.exe resolves to MSVC's linker.

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
  [[ -n "${VCTOOLSINSTALLDIR:-}" ]] || return 0
  command -v cygpath >/dev/null 2>&1 || return 0

  local host_arch="${VSCMD_ARG_HOST_ARCH:-x64}"
  local target_arch="${VSCMD_ARG_TGT_ARCH:-x64}"
  local msvc_bin
  msvc_bin="$(cygpath -u "${VCTOOLSINSTALLDIR%\\}\\bin\\Host${host_arch^}\\${target_arch}")"
  [[ -x "$msvc_bin/link.exe" ]] || return 0

  local conda_prefix="" pixi_path="" rest_path="" path_entry
  if [[ -n "${CONDA_PREFIX:-}" ]]; then
    conda_prefix="$(cygpath -u "$CONDA_PREFIX")"
    conda_prefix="${conda_prefix%/}"
  fi

  IFS=":" read -ra path_entries <<< "$PATH"
  for path_entry in "${path_entries[@]}"; do
    path_entry="${path_entry%/}"
    [[ "$path_entry" != "$msvc_bin" ]] || continue
    if [[ -n "$conda_prefix" && ("$path_entry" == "$conda_prefix" || "$path_entry" == "$conda_prefix"/*) ]]; then
      append_unique pixi_path "$path_entry"
    else
      append_unique rest_path "$path_entry"
    fi
  done

  export PATH="${pixi_path:+$pixi_path:}$msvc_bin${rest_path:+:$rest_path}"
}

normalize_windows_msvc_path
