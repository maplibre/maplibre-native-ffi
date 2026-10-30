#!/usr/bin/env bash
# Kotlin/Native has no Linux ARM64 compiler host. Run its x64 compiler in a
# container, then let the caller execute the ARM64 suite on the host.
set -euo pipefail

if [[ "$(uname -m)" == aarch64 && ! -r /proc/sys/fs/binfmt_misc/qemu-x86_64 ]]; then
  bash scripts/setup-qemu.sh x86_64
fi
image=maplibre-native-ffi-kotlin-linux-x64
docker build --platform linux/amd64 --tag "$image" - <<'DOCKERFILE'
FROM eclipse-temurin:25-jdk
RUN apt-get update && apt-get install --yes --no-install-recommends python3 git \
    && rm -rf /var/lib/apt/lists/*
DOCKERFILE
gradle_cache=${GRADLE_USER_HOME:-$HOME/.gradle}/kotlin-linux-arm64
konan_cache=${KONAN_DATA_DIR:-$HOME/.konan}
git_metadata=$(git rev-parse --path-format=absolute --git-common-dir)
mkdir -p "$gradle_cache" "$konan_cache"
user="$(id -u):$(id -g)"
if docker info --format '{{json .SecurityOptions}}' | grep -q rootless; then
  user=0:0
fi
exec docker run --rm --platform linux/amd64 --user "$user" \
  --volume "$PWD:$PWD" --workdir "$PWD" --env HOME="$HOME" \
  --env MLN_FFI_ANDROID_NDK_VERSION --env ANDROID_USER_HOME="$gradle_cache/android" \
  --volume "$git_metadata:$git_metadata:ro" \
  --volume "$gradle_cache:$gradle_cache" --env GRADLE_USER_HOME="$gradle_cache" \
  --volume "$konan_cache:$konan_cache" --env KONAN_DATA_DIR="$konan_cache" \
  "$image" ./gradlew "$@"
