#!/usr/bin/env bash
# Runs a FreeBSD test executable in this checkout's FreeBSD virtual machine,
# where the dynamic loader and graphics implementation share the ABI that the
# executable targets. See tools/devices/freebsd-host.
set -euo pipefail

preset=${1:?usage: run-freebsd-test.sh <preset> [executable [arguments...]]}
shift
if [[ $# -eq 0 ]]; then
  set -- "$MISE_MONOREPO_ROOT/build/$preset/mln_native_abi_tests"
fi

if [[ "$1" == */* && ! -x "$1" ]]; then
  echo "The FreeBSD test executable does not exist: $1" >&2
  exit 2
fi
# The machine's working directory is the repository root, not the caller's.
if [[ "$1" == */* && "$1" != /* ]]; then
  set -- "$PWD/$1" "${@:2}"
fi

case "$preset" in
  *-egl) drivers=() ;;
  # Mesa installs hardware drivers beside the software one, so the loader is
  # pointed at lavapipe alone.
  *-vulkan) drivers=(VK_DRIVER_FILES=/usr/local/share/vulkan/icd.d/lvp_icd.x86_64.json) ;;
  *)
    echo "Unsupported FreeBSD test preset: $preset" >&2
    exit 2
    ;;
esac

host=$("$MISE_MONOREPO_ROOT/tools/devices/freebsd-host")

# ssh joins its arguments into one command line for the remote shell, so each
# one is quoted for that shell.
quote() { printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"; }
command="cd $(quote "$MISE_MONOREPO_ROOT") && exec env"
# Software rendering is slow, so waits and the native hang watchdog stretch by
# the timeout scale.
for argument in \
  LIBGL_ALWAYS_SOFTWARE=true \
  "MLN_TEST_TIMEOUT_SCALE=${MLN_TEST_TIMEOUT_SCALE:-3}" \
  "MLN_FFI_TEST_FIXTURE_DIR=$MISE_MONOREPO_ROOT/tests/native/fixtures" \
  ${drivers[@]+"${drivers[@]}"} \
  timeout 300 \
  "$@"; do
  command+=" $(quote "$argument")"
done
exec ssh -o BatchMode=yes "$host" "$command"
