#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
binding_dir="$(cd "$script_dir/.." && pwd)"
repo_root="$(cd "$binding_dir/../.." && pwd)"
output_dir="$binding_dir/src/Maplibre.Native/Generated"
tmp_output_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_output_dir"' EXIT

case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) rid="osx-arm64" ;;
  Darwin-x86_64) rid="osx-x64" ;;
  Linux-x86_64) rid="linux-x64" ;;
  Linux-aarch64) rid="linux-arm64" ;;
  MINGW*-x86_64 | MSYS*-x86_64 | CYGWIN*-x86_64) rid="win-x64" ;;
  MINGW*-aarch64 | MSYS*-aarch64 | CYGWIN*-aarch64) rid="win-arm64" ;;
  *)
    echo "Unsupported ClangSharp generator host: $(uname -s)-$(uname -m)" >&2
    exit 1
    ;;
esac

clangsharp_version="21.1.8.3" # Keep in sync with dotnet-tools.json.
nuget_packages="${NUGET_PACKAGES:-$HOME/.nuget/packages}"
clangsharp_native_dir="$nuget_packages/clangsharppinvokegenerator.$rid/$clangsharp_version/tools/any/$rid"

clang_include=""
if command -v clang >/dev/null 2>&1; then
  resource_dir="$(clang -print-resource-dir)"
  if [[ -d "$resource_dir/include" ]]; then
    clang_include="$resource_dir/include"
  fi
fi
if [[ -z "$clang_include" ]]; then
  clang_major="${clangsharp_version%%.*}"
  for resource_dir in "/usr/lib/clang/$clang_major" "/usr/lib64/clang/$clang_major"; do
    if [[ -d "$resource_dir/include" ]]; then
      clang_include="$resource_dir/include"
      break
    fi
  done
fi
if [[ -z "$clang_include" ]]; then
  echo "Clang resource headers are unavailable; install the host Clang package" >&2
  exit 1
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

case "$rid" in
  linux-*) native_libraries=(libclang.so libClangSharp.so) ;;
  osx-*) native_libraries=(libclang.dylib libClangSharp.dylib) ;;
  win-*) native_libraries=(libclang.dll libClangSharp.dll) ;;
esac
for native_library in "${native_libraries[@]}"; do
  if [[ ! -f "$clangsharp_native_dir/$native_library" ]]; then
    echo "Missing ClangSharp native library: $clangsharp_native_dir/$native_library" >&2
    exit 1
  fi
done

generator_environment=(env)
case "$rid" in
  linux-*)
    generator_environment+=("LD_LIBRARY_PATH=$clangsharp_native_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}")
    ;;
  osx-*)
    generator_environment+=("DYLD_LIBRARY_PATH=$clangsharp_native_dir${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}")
    ;;
  win-*)
    generator_environment+=("PATH=$clangsharp_native_dir:$PATH")
    ;;
esac

for header in "${headers[@]}"; do
  args=(
    tool run ClangSharpPInvokeGenerator --
    @scripts/generate-clangsharp.rsp
    -f "$repo_root/include/maplibre_native_c/$header.h"
    -t "$repo_root/include/maplibre_native_c/$header.h"
    -o "$tmp_output_dir/$header.g.cs"
  )
  if [[ -n "$clang_include" ]]; then
    args+=(-I "$clang_include")
  fi

  (
    cd "$binding_dir"
    "${generator_environment[@]}" dotnet "${args[@]}"
  )

  if [[ ! -s "$tmp_output_dir/$header.g.cs" ]]; then
    echo "ClangSharp produced no output for $header.h" >&2
    exit 1
  fi
done

rm -f "$output_dir"/*.g.cs
mv "$tmp_output_dir"/*.g.cs "$output_dir"/
