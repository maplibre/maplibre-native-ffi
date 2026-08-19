#!/usr/bin/env bash
# Compile a Kotlin/Native-generated Objective-C framework header the way an
# Xcode consumer does. Kotlin tests compile against a klib and never see this
# header, so a name that collides with a C macro fails here.
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <xcode-sdk-name> <framework-bundle> <stub.m>" >&2
  exit 2
fi

sdk_name=$1
framework_dir=$2
stub=$3

if [[ ! -d "$framework_dir" ]]; then
  echo "error: framework bundle missing: $framework_dir" >&2
  exit 1
fi

base=$(basename "$framework_dir" .framework)
header=$framework_dir/Headers/$base.h
if [[ ! -f "$header" ]]; then
  echo "error: generated Objective-C header missing: $header" >&2
  exit 1
fi

sdk_path=$(xcrun --sdk "$sdk_name" --show-sdk-path)
framework_search=$(dirname "$framework_dir")

echo "clang -fsyntax-only $header (sdk $sdk_name)"
xcrun --sdk "$sdk_name" clang \
  -fsyntax-only \
  -fobjc-arc \
  -isysroot "$sdk_path" \
  -F "$framework_search" \
  "$stub"
