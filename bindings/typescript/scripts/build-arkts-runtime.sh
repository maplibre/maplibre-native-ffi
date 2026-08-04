#!/usr/bin/env bash
# Cross-builds the Node-API addon for OpenHarmony and stages the ArkTS payload.
#
# ArkTS implements Node-API, so the same addon and the same TypeScript serve it;
# only the toolchain and the payload's target metadata differ. The OpenHarmony
# SDK supplies the compiler, linker, and sysroot, exactly as it does for the
# Rust binding's cross builds.
set -euo pipefail

preset="${1:?usage: build-arkts-runtime.sh <preset>}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
sdk="${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must point at the OpenHarmony native SDK}"
install_dir="$root/build/$preset/install"
package="$root/bindings/typescript/runtime-arkts"

case "$preset" in
ohos-arm64-*) cargo_target=aarch64-unknown-linux-ohos compiler_target=aarch64-linux-ohos ;;
ohos-x64-*) cargo_target=x86_64-unknown-linux-ohos compiler_target=x86_64-linux-ohos ;;
*)
  echo "build-arkts-runtime.sh takes an ohos preset, not $preset" >&2
  exit 1
  ;;
esac

sysroot="$sdk/sysroot"
target_flags="--target=$compiler_target --sysroot=$sysroot"
target_env="${cargo_target//-/_}"
target_env_upper=$(printf '%s' "$target_env" | tr '[:lower:]' '[:upper:]')

export MAPLIBRE_NATIVE_C_INSTALL_DIR="$install_dir"
export "BINDGEN_EXTRA_CLANG_ARGS_$target_env=$target_flags -I$sysroot/usr/include/$compiler_target"
export "CC_$target_env=$sdk/llvm/bin/clang $target_flags"
export "CXX_$target_env=$sdk/llvm/bin/clang++ $target_flags"
export "CARGO_TARGET_${target_env_upper}_LINKER=$sdk/llvm/bin/clang"
export RUSTFLAGS="-C link-arg=--target=$compiler_target -C link-arg=--sysroot=$sysroot -C link-arg=-fuse-ld=lld"

cargo build -p mln-ts-addon --target "$cargo_target"

target_name="${preset%-*}"
case "${preset##*-}" in
egl | wgl | glx) backend=opengl ;;
*) backend="${preset##*-}" ;;
esac

fingerprint=$(
  tr '\n' ' ' <"$root/bindings/typescript/host-support/generated/fingerprint.h" |
    sed -n 's/.*MLN_ABI_FINGERPRINT_VALUE[^"]*"\([^"]*\)".*/\1/p'
)

mkdir -p "$package/lib"
cp "$root/target/$cargo_target/debug/libmln_ts_addon.so" \
  "$package/maplibre-native-ffi.so"
cp "$install_dir/lib/libmaplibre-native-c.so" "$package/lib/"
# ArkTS loads a module by its library name, and the C++ runtime the native
# library was built against is not on the device by default.
cp "$sdk/llvm/lib/$compiler_target/libc++_shared.so" "$package/lib/" 2>/dev/null || true

cat >"$package/runtime.json" <<JSON
{
  "transport": "node-api",
  "target": "$target_name",
  "backend": "$backend",
  "abiFingerprint": "$fingerprint",
  "addon": "./maplibre-native-ffi.so"
}
JSON

echo "staged $preset into $package"
