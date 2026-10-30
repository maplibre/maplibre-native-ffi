#!/usr/bin/env bash
# Install the pinned user-mode emulators and register them on this host.
set -euo pipefail

image=tonistiigi/binfmt@sha256:400a4873b838d1b89194d982c45e5fb3cda4593fbfd7e08a02e76b03b21166f0
if (($# == 0)); then
  set -- riscv64 loongarch64
fi
scratch=$(mktemp -d)
container=$(docker create "$image")
trap 'docker rm "$container" >/dev/null; rm -rf "$scratch"' EXIT
for binary in binfmt "${@/#/qemu-}"; do
  docker cp "$container:/usr/bin/$binary" "$scratch/$binary"
done
sudo install -m 755 "$scratch"/qemu-* /usr/local/bin/

# A Docker daemon can run in a different VM or namespace. Register on the test
# host so its executables and their children use the same emulators.
if ! mountpoint -q /proc/sys/fs/binfmt_misc; then
  sudo mount -t binfmt_misc binfmt_misc /proc/sys/fs/binfmt_misc
fi
targets=()
for architecture; do
  case "$architecture" in
    x86_64) targets+=(amd64) ;;
    loongarch64) targets+=(loong64) ;;
    *) targets+=("$architecture") ;;
  esac
done
target_list=$(IFS=,; echo "${targets[*]}")
sudo env QEMU_BINARY_PATH=/usr/local/bin QEMU_PRESERVE_ARGV0=1 \
  "$scratch/binfmt" --uninstall "$target_list" --install "$target_list"
for architecture; do
  grep -qx enabled "/proc/sys/fs/binfmt_misc/qemu-$architecture"
done
