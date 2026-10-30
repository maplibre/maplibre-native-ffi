#!/bin/sh
# Installs the pinned Bun musl runtime and runs the prepared suite in Alpine.
set -eu
bun_version=${1:?usage: test-musl.sh <bun-version> <runtime>...}
shift
apk add --no-cache bash curl unzip libstdc++
case "$(uname -m)" in
  x86_64) bun_arch=x64 ;;
  aarch64) bun_arch=aarch64 ;;
esac
curl --fail --location --silent --show-error \
  "https://github.com/oven-sh/bun/releases/download/bun-v$bun_version/bun-linux-$bun_arch-musl.zip" \
  --output /tmp/bun.zip
unzip -q /tmp/bun.zip -d /tmp/bun
export PATH="/tmp/bun/bun-linux-$bun_arch-musl:$PATH"
exec bash "$(dirname "$0")/run-desktop.sh" "$@"
