#!/usr/bin/env bash
# Builds the Python Android wheel and runs its suite in CPython's device testbed.
set -euo pipefail

preset=${1:?usage: test-python-android-device.sh <preset>}
case "$preset" in
  android-arm64-egl)
    abi=arm64-v8a
    cibw_arch=arm64_v8a
    system_graphics_library=libGLESv3.so
    test_requirements='pytest>=9,<10 pyopengl>=3.1.10,<4'
    ;;
  android-arm64-vulkan)
    abi=arm64-v8a
    cibw_arch=arm64_v8a
    system_graphics_library=libvulkan.so
    test_requirements='pytest>=9,<10'
    ;;
  android-x64-egl)
    abi=x86_64
    cibw_arch=x86_64
    system_graphics_library=libGLESv3.so
    test_requirements='pytest>=9,<10 pyopengl>=3.1.10,<4'
    ;;
  android-x64-vulkan)
    abi=x86_64
    cibw_arch=x86_64
    system_graphics_library=libvulkan.so
    test_requirements='pytest>=9,<10'
    ;;
  *)
    echo "The Python Android device suite does not support $preset." >&2
    exit 2
    ;;
esac

native_install_dir="$MISE_MONOREPO_ROOT/build/$preset/install"
if [[ ! -d "$native_install_dir" ]]; then
  echo "Missing native install prefix $native_install_dir; run mise run build $preset first." >&2
  exit 2
fi

mise run //:android-sdk-packages
if [[ "$preset" == android-x64-egl ]]; then
  # CPython 3.14's x86_64 Android runtime uses the legacy open syscall during
  # mimalloc initialization. API 26 rejects that syscall before Python can
  # load the wheel, so replace the Goldfish regression device with the default
  # emulator used by this binding's suite.
  mise run //:android-emulator:stop
fi
mise run //:android-emulator:boot "$abi"

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
cmake_bin=$(mise which cmake)
export PATH="${cmake_bin%/*}:$PATH"

export CIBW_BUILD="cp314-android_$cibw_arch"
export CIBW_ARCHS_ANDROID="$cibw_arch"
export CIBW_BUILD_FRONTEND='build[uv]'
export CIBW_CONFIG_SETTINGS_ANDROID='build-args=--no-default-features'
host_cc=$(command -v cc)
export CIBW_ENVIRONMENT_ANDROID="MAPLIBRE_NATIVE_C_INSTALL_DIR=$native_install_dir HOST_CC=$host_cc"
# The graphics loader comes from Android, so wheel repair leaves it external.
export CIBW_REPAIR_WHEEL_COMMAND_ANDROID="auditwheel repair --exclude $system_graphics_library --ldpaths {ldpaths} -w {dest_dir} {wheel}"
export CIBW_TEST_COMMAND_ANDROID='python -m pytest tests'
export CIBW_TEST_REQUIRES_ANDROID="$test_requirements"
export CIBW_TEST_RUNTIME='args: --connected emulator-5554'
export CIBW_TEST_SOURCES_ANDROID=tests

cd "$MISE_MONOREPO_ROOT/bindings/python"
python_test_status=0
uv run --project . --group android --no-sync \
  cibuildwheel --platform android \
  --output-dir "$MISE_MONOREPO_ROOT/build/android-emulator/$preset/python/wheelhouse" \
  . || python_test_status=$?
if ((python_test_status == 0)); then
  exit 0
fi

"$shared_android_home/platform-tools/adb" -s emulator-5554 logcat -d -b crash >&2 || true
exit "$python_test_status"
