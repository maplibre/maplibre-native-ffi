#!/usr/bin/env bash
# Prints the process ID and hdc connect key of the shared OpenHarmony emulator:
# the QEMU process that runs the Oniro image in the image directory, which
# defaults to the one mise installed, whoever started it. Prints nothing and
# exits 1 when no QEMU runs the image.
set -euo pipefail

image_dir=${1:-$(mise where http:oniro-emulator)/images}
image_dir=$(cd -P "$image_dir" && pwd)

# run-ohos-emulator.sh names each image by its absolute path, and the host side
# of its port forward is the connect key. The path travels in the environment so
# that awk's own command line does not match it. awk reads every line, since
# ps would fail on a pipe that closes early.
ps -Aww -o pid=,args= |
  userdata="file=$image_dir/userdata.img," awk '
    !found && $2 ~ /qemu-system/ && index($0, ENVIRON["userdata"]) &&
      match($0, /hostfwd=tcp:[^ -]+-/) {
      print $1, substr($0, RSTART + 12, RLENGTH - 13)
      found = 1
    }
    END { exit !found }
  '
