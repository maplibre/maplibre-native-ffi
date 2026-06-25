#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
binding_dir="$(cd "$script_dir/.." && pwd)"
repo_root="$(cd "$binding_dir/../.." && pwd)"
output_dir="$binding_dir/src/Maplibre.Native/Generated"
tmp_output_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_output_dir"' EXIT

rid=""
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) rid="osx-arm64" ;;
  Darwin-x86_64) rid="osx-x64" ;;
  Linux-x86_64) rid="linux-x64" ;;
  Linux-aarch64) rid="linux-arm64" ;;
  MINGW*|MSYS*|CYGWIN*) rid="win-x64" ;;
  *) rid="" ;;
esac

if [[ -n "$rid" ]]; then
  libclang_dir="$(find "$HOME/.nuget/packages" -path "*/clangsharppinvokegenerator.$rid/*/tools/any/$rid" -type d 2>/dev/null | sort | tail -n 1 || true)"
  if [[ -n "$libclang_dir" ]]; then
    export DYLD_LIBRARY_PATH="$libclang_dir:${DYLD_LIBRARY_PATH:-}"
    export LD_LIBRARY_PATH="$libclang_dir:${LD_LIBRARY_PATH:-}"
    export PATH="$libclang_dir:$PATH"
  fi
fi

clang_include=""
resource_dir="$(clang -print-resource-dir)"
if [[ -d "$resource_dir/include" ]]; then
  clang_include="$resource_dir/include"
fi

headers=(
  android
  base
  diagnostics
  logging
  runtime
  map
  camera
  projection
  query
  render_target
  render_session
  style
  surface
  texture
)

mkdir -p "$output_dir"

(
  cd "$binding_dir"
  dotnet tool restore
)

for header in "${headers[@]}"; do
  args=(
    tool run ClangSharpPInvokeGenerator --
    -f "$repo_root/include/maplibre_native_c/$header.h"
    -t "$repo_root/include/maplibre_native_c/$header.h"
    -I "$repo_root/include/maplibre_native_c"
    -x c
    -std c2x
    -D "__attribute__(x)="
    -n Maplibre.Native.Internal.C
    -m NativeMethods
    -l maplibre-native-c
    -o "$tmp_output_dir/$header.g.cs"
    -c latest-codegen
    -was "*=internal"
  )
  if [[ -n "$clang_include" ]]; then
    args+=(-I "$clang_include")
  fi

  (
    cd "$binding_dir"
    dotnet "${args[@]}"
  )

  if [[ ! -s "$tmp_output_dir/$header.g.cs" ]]; then
    echo "ClangSharp produced no output for $header.h" >&2
    exit 1
  fi
done

rm -f "$output_dir"/*.g.cs
mv "$tmp_output_dir"/*.g.cs "$output_dir"/
