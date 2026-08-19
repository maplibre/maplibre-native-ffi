#!/usr/bin/env bash
# Compile a Kotlin/Native-generated Objective-C header. Kotlin tests compile
# against a klib and never see this header.
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <xcode-sdk-name> <header.h>" >&2
  exit 2
fi

sdk_name=$1
header=$2

if [[ ! -f "$header" ]]; then
  echo "error: generated Objective-C header missing: $header" >&2
  exit 1
fi

sdk_path=$(xcrun --sdk "$sdk_name" --show-sdk-path)
echo "clang -fsyntax-only $header (sdk $sdk_name)"
xcrun --sdk "$sdk_name" clang \
  -fsyntax-only \
  -fobjc-arc \
  -x objective-c \
  -isysroot "$sdk_path" \
  "$header"
