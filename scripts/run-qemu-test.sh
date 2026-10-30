#!/usr/bin/env bash
# Runs a Linux test executable for an architecture that the host cannot run,
# under QEMU's user-mode emulator against the root from tools/devices/linux-root.
# The emulator looks up each absolute path in the root first and falls back to
# the host's, so the executable keeps its build-tree paths. The executable gets
# MLN_FFI_DEVICE_TIMEOUT seconds, or 300.
set -euo pipefail

preset=${1:?usage: run-qemu-test.sh <preset> [executable [arguments...]]}
shift
if [[ $# -eq 0 ]]; then
  set -- "$MISE_MONOREPO_ROOT/build/$preset/mln_native_abi_tests"
fi

case "$preset" in
  linux-gnu-*) libc=gnu ;;
  linux-musl-*) libc=musl ;;
  *)
    echo "Unsupported QEMU test preset: $preset" >&2
    exit 2
    ;;
esac
case "$preset" in
  *-riscv64-*) architecture=riscv64 ;;
  *-loongarch64-*) architecture=loongarch64 ;;
  *)
    echo "Unsupported QEMU test preset: $preset" >&2
    exit 2
    ;;
esac

emulator=()
registration="/proc/sys/fs/binfmt_misc/qemu-$architecture"
if [[ ! -r "$registration" ]] || ! grep -qx enabled "$registration"; then
  executable=$(command -v "qemu-$architecture" || command -v "qemu-$architecture-static" || true)
  if [[ -z "$executable" ]]; then
    echo "$preset tests require QEMU's user-mode emulator, qemu-$architecture." >&2
    exit 2
  fi
  emulator=("$executable")
fi
root=$("$MISE_MONOREPO_ROOT/tools/devices/linux-root" "$libc" "$architecture")

# Through the emulator, the loaders find no drivers by listing their manifest
# directories, so each one is named.
vulkan_driver=$(cd "$root" && printf '/%s' usr/share/vulkan/icd.d/lvp_icd*.json)
export VK_DRIVER_FILES="$vulkan_driver"
if [[ -f "$root/usr/share/glvnd/egl_vendor.d/50_mesa.json" ]]; then
  export __EGL_VENDOR_LIBRARY_FILENAMES=/usr/share/glvnd/egl_vendor.d/50_mesa.json
fi
export LIBGL_ALWAYS_SOFTWARE=true
# Emulated software rendering is slow, so waits and the native hang watchdog
# stretch by the timeout scale.
export MLN_TEST_TIMEOUT_SCALE="${MLN_TEST_TIMEOUT_SCALE:-5}"

export QEMU_LD_PREFIX="$root"

exec timeout "${MLN_FFI_DEVICE_TIMEOUT:-300}" "${emulator[@]}" "$@"
