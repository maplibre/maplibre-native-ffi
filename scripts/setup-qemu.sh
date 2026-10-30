#!/usr/bin/env bash
# Install the pinned user-mode emulators and register them on this host.
set -euo pipefail

image=tonistiigi/binfmt@sha256:400a4873b838d1b89194d982c45e5fb3cda4593fbfd7e08a02e76b03b21166f0
scratch=$(mktemp -d)
container=$(docker create "$image")
trap 'docker rm "$container" >/dev/null; rm -rf "$scratch"' EXIT
for binary in binfmt qemu-riscv64 qemu-loongarch64; do
  docker cp "$container:/usr/bin/$binary" "$scratch/$binary"
done
sudo install -m 755 "$scratch/qemu-riscv64" "$scratch/qemu-loongarch64" /usr/local/bin/

# A Docker daemon can run in a different VM or namespace. Register on the test
# host so its executables and their children use the same emulators.
if ! mountpoint -q /proc/sys/fs/binfmt_misc; then
  sudo mount -t binfmt_misc binfmt_misc /proc/sys/fs/binfmt_misc
fi
sudo env QEMU_BINARY_PATH=/usr/local/bin QEMU_PRESERVE_ARGV0=1 \
  "$scratch/binfmt" --uninstall riscv64,loong64 --install riscv64,loong64
for architecture in riscv64 loongarch64; do
  grep -qx enabled "/proc/sys/fs/binfmt_misc/qemu-$architecture"
done
