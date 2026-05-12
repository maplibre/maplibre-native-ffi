#!/usr/bin/env bash
set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)
pixi_env="$repo_root/.pixi/envs/default"

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

exec "$@"
