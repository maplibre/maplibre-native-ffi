#!/usr/bin/env bash
# Boots this checkout's FreeBSD virtual machine, or reuses it, and waits until
# it answers SSH. The machine mounts the checkout over NFS at the path it has on
# the host, so a test executable, the plugins it loads, and the fixtures keep
# their paths. Its disk lives under build/freebsd-vm and keeps the software
# graphics drivers it installs on first boot.
#
# anyvm boots the image that vmactions/freebsd-vm boots, and needs QEMU, UEFI
# firmware, and KVM on a Linux host.
set -euo pipefail

# The release the suites run on. The artifacts require 14.0, the oldest release
# that Zig links against; see cmake/toolchains/zig.cmake.
release=14.4
host=$("$MISE_MONOREPO_ROOT/scripts/freebsd-vm-host.sh")

ready() {
  ssh -o BatchMode=yes -o ConnectTimeout=10 "$host" true 2>/dev/null
}

if ! ready; then
  if [[ ! -r /dev/kvm || ! -w /dev/kvm ]]; then
    echo "The FreeBSD virtual machine needs read and write access to /dev/kvm." >&2
    exit 2
  fi
  anyvm="$(mise where http:anyvm)/anyvm.py"
  python3 "$anyvm" \
    --os freebsd \
    --release "$release" \
    --data-dir "$MISE_MONOREPO_ROOT/build/freebsd-vm" \
    --cache-dir "${XDG_CACHE_HOME:-$HOME/.cache}/anyvm" \
    --ssh-name "$host" \
    --vnc off \
    --remote-vnc no \
    --sync nfs \
    -v "$MISE_MONOREPO_ROOT:$MISE_MONOREPO_ROOT" \
    -d
fi

# The software implementations of EGL, OpenGL ES, and Vulkan that the suites
# draw with. pkg returns at once when they are already installed.
ssh -o BatchMode=yes "$host" \
  pkg install --yes --quiet mesa-dri mesa-libs libglvnd vulkan-loader
echo "FreeBSD virtual machine is ready at $host."
