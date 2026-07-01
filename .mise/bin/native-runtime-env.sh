#!/usr/bin/env bash

mln_host_library_dirs=()

mln_split_host_library_dirs() {
  local separator=":"
  if [[ "${OS:-}" == "Windows_NT" ]]; then
    separator=";"
  fi

  local raw_dirs="${MLN_FFI_HOST_LIBRARY_DIRS:?MLN_FFI_HOST_LIBRARY_DIRS is required}"
  local dirs=()
  IFS="$separator" read -r -a dirs <<< "$raw_dirs"

  mln_host_library_dirs=()
  local dir
  for dir in "${dirs[@]}"; do
    if [[ -n "$dir" ]]; then
      mln_host_library_dirs+=("$dir")
    fi
  done
}

mln_export_rust_runtime_rpaths() {
  case "$MISE_ENV" in
    linux-* | macos-*)
      mln_split_host_library_dirs
      local rustflags="${RUSTFLAGS:-} -C link-arg=-Wl,-rpath,$MLN_FFI_NATIVE_INSTALL_DIR/lib"
      local dir
      for dir in "${mln_host_library_dirs[@]}"; do
        rustflags+=" -C link-arg=-Wl,-rpath,$dir"
      done
      export RUSTFLAGS="${rustflags# }"
      ;;
  esac
}

mln_export_cgo_maplibre_native_flags() {
  export PKG_CONFIG_PATH="$MLN_FFI_NATIVE_INSTALL_DIR/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
  export CGO_CFLAGS="$(pkg-config --cflags maplibre-native-c) ${CGO_CFLAGS:-}"

  local ldflags
  ldflags="$(pkg-config --libs maplibre-native-c)"
  case "$MISE_ENV" in
    linux-* | macos-*)
      mln_split_host_library_dirs
      local dir
      for dir in "${mln_host_library_dirs[@]}"; do
        ldflags+=" -Wl,-rpath,$dir"
      done
      ;;
  esac
  export CGO_LDFLAGS="$ldflags ${CGO_LDFLAGS:-}"
}
