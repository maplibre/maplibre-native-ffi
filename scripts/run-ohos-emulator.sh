#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <image-directory>" >&2
  exit 2
fi

image_dir=$1
connect_key=${MLN_FFI_OHOS_EMULATOR_CONNECT_KEY:-127.0.0.1:55555}
host_address=${connect_key%:*}
host_port=${connect_key##*:}
if [[ -z "$host_address" || ! "$host_port" =~ ^[0-9]+$ ]]; then
  echo "Invalid OpenHarmony emulator connect key: $connect_key" >&2
  exit 2
fi
qemu_bin=${QEMU_BIN:-qemu-system-x86_64}
for image in bzImage ramdisk.img updater.img system.img vendor.img userdata.img; do
  if [[ ! -f "$image_dir/$image" ]]; then
    echo "OpenHarmony emulator image does not exist: $image_dir/$image" >&2
    exit 2
  fi
done
if ! command -v "$qemu_bin" >/dev/null 2>&1; then
  echo "qemu-system-x86_64 is unavailable. Install QEMU through your system package manager and retry." >&2
  exit 2
fi

case "$(uname -s)" in
  Linux)
    if [[ -r /dev/kvm && -w /dev/kvm ]]; then
      acceleration=(-enable-kvm)
      cpu=(-cpu host)
    else
      echo "KVM is inaccessible; using QEMU software emulation." >&2
      acceleration=(-accel "tcg,thread=multi")
      cpu=(-cpu max)
    fi
    ;;
  Darwin)
    # The Oniro image is x86_64, which the hypervisor on Apple Silicon cannot
    # accelerate, so macOS hosts always run it under TCG.
    acceleration=(-accel "tcg,thread=multi")
    cpu=(-cpu max)
    ;;
  *)
    echo "The OpenHarmony emulator supports Linux and macOS hosts." >&2
    exit 2
    ;;
esac

cd "$image_dir"
exec "$qemu_bin" \
  -machine q35 \
  -smp 6 \
  -m 4096M \
  -boot c \
  -nographic \
  -vga none \
  -device virtio-gpu-pci,xres=360,yres=720,max_outputs=1,addr=08.0 \
  -display vnc=127.0.0.1:0 \
  -serial telnet:127.0.0.1:4444,server,nowait \
  -rtc base=utc,clock=host \
  -initrd ramdisk.img \
  -kernel bzImage \
  -drive if=none,file=updater.img,format=raw,id=updater,index=0 \
  -device virtio-blk-pci,drive=updater \
  -drive if=none,file=system.img,format=raw,id=system,index=1 \
  -device virtio-blk-pci,drive=system \
  -drive if=none,file=vendor.img,format=raw,id=vendor,index=2 \
  -device virtio-blk-pci,drive=vendor \
  -drive if=none,file=userdata.img,format=raw,id=userdata,index=3 \
  -device virtio-blk-pci,drive=userdata \
  -append "ip=dhcp loglevel=4 console=ttyS0,115200 init=init root=/dev/ram0 rw ohos.boot.hardware=x86_general ohos.required_mount.system=/dev/block/vdb@/usr@ext4@ro,barrier=1@wait,required ohos.required_mount.vendor=/dev/block/vdc@/vendor@ext4@ro,barrier=1@wait,required ohos.required_mount.misc=/dev/block/vda@/misc@none@none=@wait,required" \
  "${acceleration[@]}" \
  "${cpu[@]}" \
  -netdev "user,id=net0,hostfwd=tcp:$host_address:$host_port-:55555" \
  -device virtio-net-pci,netdev=net0
