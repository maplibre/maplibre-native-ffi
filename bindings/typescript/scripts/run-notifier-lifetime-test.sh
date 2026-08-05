#!/usr/bin/env bash
# Builds and runs the support layer's notifier lifetime test.
#
# The rule it proves is a threading one, so it is built with the address
# sanitizer by default: the failure it guards against is a read of freed
# memory, which without a sanitizer shows up as a wrong answer somewhere else
# or as nothing at all. `MLN_ABI_TEST_HOOKS` compiles in the hook the test uses
# to widen the window a stop has to survive; nothing else defines it.
set -euo pipefail

preset="${1:-linux-x64-vulkan}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
support="$root/bindings/typescript/host-support"
install_dir="$root/build/$preset/install"
sanitizer="${MLN_NOTIFIER_SANITIZER:-address}"

if [[ ! -d "$install_dir/include" ]]; then
  echo "no native install at $install_dir; build the preset first" >&2
  exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

cc -std=c23 -g -O1 -pthread \
  "-fsanitize=$sanitizer" \
  -DMLN_ABI_TEST_HOOKS \
  -I"$support/include" \
  -I"$support/generated" \
  -I"$install_dir/include" \
  "$support/src/mln_abi.c" \
  "$support/tests/notifier_lifetime.c" \
  -L"$install_dir/lib" \
  -lmaplibre-native-c \
  -o "$work/notifier-lifetime"

LD_LIBRARY_PATH="$install_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  "$work/notifier-lifetime"
