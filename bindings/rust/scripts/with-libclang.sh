#!/usr/bin/env bash
set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)
pixi_env="$repo_root/.pixi/envs/default"
pixi_lib_dir="$pixi_env/lib"
pixi_bin_dir="$pixi_env/Library/bin"

if [[ -d "$pixi_lib_dir" ]]; then
  case "$(uname -s)" in
    Darwin) ;;
    *) export LD_LIBRARY_PATH="$pixi_lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ;;
  esac
fi
if [[ -d "$pixi_bin_dir" ]]; then
  export PATH="$pixi_bin_dir${PATH:+:$PATH}"
fi

if [[ -z "${LIBCLANG_PATH:-}" ]]; then
  for libclang_candidate in \
    "$pixi_env/lib/libclang.dylib" \
    "$pixi_env/lib/libclang."*.dylib \
    "$pixi_env/lib/libclang.so" \
    "$pixi_env/lib/libclang.so."* \
    "$pixi_env/Library/bin/libclang.dll"; do
    if [[ -e "$libclang_candidate" ]]; then
      libclang_link_dir="$repo_root/target/libclang"
      mkdir -p "$libclang_link_dir"
      case "$libclang_candidate" in
        *.dylib) ln -sfn "$libclang_candidate" "$libclang_link_dir/libclang.dylib" ;;
        *.dll) ln -sfn "$libclang_candidate" "$libclang_link_dir/libclang.dll" ;;
        *) ln -sfn "$libclang_candidate" "$libclang_link_dir/libclang.so" ;;
      esac
      export LIBCLANG_PATH="$libclang_link_dir"
      break
    fi
  done
fi

if [[ -z "${LIBCLANG_PATH:-}" ]]; then
  echo "could not find pixi-provided libclang under $pixi_env" >&2
  exit 1
fi

native_library_dir="${MLN_FFI_BUILD_DIR:-$repo_root/build/host}"
if [[ -n "${MLN_FFI_CMAKE_BUILD_CONFIG:-}" && -d "$native_library_dir/$MLN_FFI_CMAKE_BUILD_CONFIG" ]]; then
  native_library_dir="$native_library_dir/$MLN_FFI_CMAKE_BUILD_CONFIG"
fi
export DYLD_LIBRARY_PATH="$native_library_dir${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="$native_library_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$@"
