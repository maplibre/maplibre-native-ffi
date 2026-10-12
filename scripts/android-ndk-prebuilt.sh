#!/usr/bin/env bash
# Prints the host prebuilt directory of the pinned Android NDK, which holds the
# cross toolchain under bin/ and the target headers under sysroot/. The host
# name in the path varies by machine and an NDK carries exactly one prebuilt,
# so the directory is discovered rather than named.
set -euo pipefail

prebuilt_root="${ANDROID_HOME:?ANDROID_HOME is unset; run 'mise -E android install' or point it at an SDK}/ndk/${MLN_FFI_ANDROID_NDK_VERSION:?MLN_FFI_ANDROID_NDK_VERSION is unset; run this through a mise task}/toolchains/llvm/prebuilt"
prebuilt=
for candidate in "$prebuilt_root"/*; do
  if [[ -d "$candidate/bin" ]]; then
    if [[ -n "$prebuilt" ]]; then
      echo "The pinned Android NDK has multiple host prebuilts under $prebuilt_root." >&2
      exit 2
    fi
    prebuilt="$candidate"
  fi
done
if [[ -z "$prebuilt" ]]; then
  echo "The pinned Android NDK has no host prebuilt under $prebuilt_root." >&2
  exit 2
fi
echo "$prebuilt"
