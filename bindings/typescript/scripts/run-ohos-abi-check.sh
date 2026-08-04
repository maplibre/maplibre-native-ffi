#!/usr/bin/env bash
# Builds the normalized ABI check for OpenHarmony and runs it on the emulator.
#
# ArkTS runs the same addon Node does, and proving that needs an ArkTS
# application. This proves the layer underneath it — the generated dispatch, the
# layouts, and the diagnostics — against the library as cross-built, on the
# device itself.
set -euo pipefail

preset="${1:?usage: run-ohos-abi-check.sh <preset>}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
sdk="${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must point at the OpenHarmony native SDK}"
install_dir="$root/build/$preset/install"
connect_key=127.0.0.1:55555
remote_dir=/data/local/tmp/mln-ts-abi

case "$preset" in
ohos-arm64-*) compiler_target=aarch64-linux-ohos ;;
ohos-x64-*) compiler_target=x86_64-linux-ohos ;;
*)
  echo "run-ohos-abi-check.sh takes an ohos preset, not $preset" >&2
  exit 1
  ;;
esac

binary="$root/build/$preset/mln-ts-abi-smoke"
# The SDK's Clang is older than the host's and knows C23 by its working name.
"$sdk/llvm/bin/clang" \
  --target="$compiler_target" --sysroot="$sdk/sysroot" -std=c2x -O1 \
  -I"$root/bindings/typescript/host-support/include" \
  -I"$root/bindings/typescript/host-support/generated" \
  -I"$install_dir/include" \
  "$root/bindings/typescript/host-support/src/mln_abi.c" \
  "$root/bindings/typescript/host-support/tests/abi_smoke.c" \
  -o "$binary" \
  -L"$install_dir/lib" -lmaplibre-native-c

mise run //:ohos-emulator:boot

hdc -t "$connect_key" shell "rm -rf '$remote_dir' && mkdir -p '$remote_dir'"
hdc -t "$connect_key" file send "$binary" "$remote_dir/abi-smoke"
hdc -t "$connect_key" file send \
  "$install_dir/lib/libmaplibre-native-c.so" "$remote_dir/libmaplibre-native-c.so"
hdc -t "$connect_key" file send \
  "$sdk/llvm/lib/$compiler_target/libc++_shared.so" "$remote_dir/libc++_shared.so"

output=$(
  hdc -t "$connect_key" shell \
    "cd '$remote_dir' && chmod 755 abi-smoke && LD_LIBRARY_PATH='$remote_dir' ./abi-smoke; echo EXIT=\$?"
)
printf '%s\n' "$output"
# hdc reports the device shell's success rather than the command's, so the
# marker the command prints is what decides this.
printf '%s' "$output" | tr -d '\r' | grep -qx 'EXIT=0'
