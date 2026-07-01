#!/usr/bin/env bash

mln_zig_native_args=()

mln_zig_normalize_path_arg() {
  local path="$1"
  if command -v cygpath >/dev/null 2>&1; then
    if cygpath -m "$path" 2>/dev/null; then
      return 0
    fi
  fi
  printf '%s\n' "${path//\\//}"
}

mln_build_zig_native_args() {
  mln_zig_native_args=()

  local raw_args=()
  # MLN_FFI_ZIG_NATIVE_ARGS is assembled from mise config values without spaces.
  # shellcheck disable=SC2206
  raw_args=(${MLN_FFI_ZIG_NATIVE_ARGS:-})

  local arg key value
  for arg in "${raw_args[@]}"; do
    case "$arg" in
      -Dnative-install-dir=* | -Ddependency-include-dir=* | -Ddependency-library-dir=*)
        key="${arg%%=*}"
        value="${arg#*=}"
        mln_zig_native_args+=("$key=$(mln_zig_normalize_path_arg "$value")")
        ;;
      *)
        mln_zig_native_args+=("$arg")
        ;;
    esac
  done
}
