#!/usr/bin/env bash
# Builds the Python Android wheel and runs its suite in CPython's device testbed.
set -euo pipefail

preset=${1:?usage: test-python-android-device.sh <preset>}
if [[ "$preset" != android-x64-egl ]]; then
  echo "The Python Android device suite tests android-x64-egl only, not $preset." >&2
  exit 2
fi

native_install_dir="$MISE_MONOREPO_ROOT/build/$preset/install"
if [[ ! -d "$native_install_dir" ]]; then
  echo "Missing native install prefix $native_install_dir; run mise run build $preset first." >&2
  exit 2
fi

mise run //:android-sdk-packages
mise run //:android-emulator:boot x86_64

# cibuildwheel pins its own NDK and removes other NDK versions from the SDK it
# manages. Give it an isolated SDK view so later repository tasks retain the
# NDK version installed by the Android preset.
shared_android_home=${ANDROID_HOME:?ANDROID_HOME must point to the repository Android SDK}
python_android_home="$MISE_MONOREPO_ROOT/build/android-emulator/$preset/python/android-sdk"
mkdir -p "$python_android_home/ndk"
if [[ ! -d "$python_android_home/cmdline-tools" ]]; then
  cp -a "$shared_android_home/cmdline-tools" "$python_android_home/cmdline-tools"
fi
for entry_name in build-tools cmake emulator licenses platform-tools platforms system-images; do
  sdk_entry="$shared_android_home/$entry_name"
  destination="$python_android_home/$entry_name"
  if [[ ! -e "$destination" && ! -L "$destination" ]]; then
    ln -s "$sdk_entry" "$destination"
  fi
done
export ANDROID_HOME="$python_android_home"
export ANDROID_SDK_ROOT="$python_android_home"

export CIBW_BUILD=cp314-android_x86_64
export CIBW_ARCHS_ANDROID=x86_64
export CIBW_BUILD_FRONTEND='build[uv]'
export CIBW_CONFIG_SETTINGS_ANDROID='build-args=--no-default-features'
export CIBW_ENVIRONMENT_ANDROID="MAPLIBRE_NATIVE_C_INSTALL_DIR=$native_install_dir"
# libGLESv3 is an Android system library, so wheel repair leaves it external.
export CIBW_REPAIR_WHEEL_COMMAND_ANDROID='auditwheel repair --exclude libGLESv3.so --ldpaths {ldpaths} -w {dest_dir} {wheel}'
export CIBW_TEST_COMMAND_ANDROID='python -m pytest tests'
export CIBW_TEST_REQUIRES_ANDROID='pytest>=9,<10 pyopengl>=3.1.10,<4'
export CIBW_TEST_RUNTIME='args: --connected emulator-5554'
export CIBW_TEST_SOURCES_ANDROID=tests

cd "$MISE_MONOREPO_ROOT/bindings/python"
exec uv run --project . --group android --no-sync \
  cibuildwheel --platform android \
  --output-dir "$MISE_MONOREPO_ROOT/build/android-emulator/$preset/python/wheelhouse" \
  .
