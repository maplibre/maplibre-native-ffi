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

export CIBW_BUILD=cp314-android_x86_64
export CIBW_ARCHS_ANDROID=x86_64
export CIBW_BUILD_FRONTEND='build[uv]'
export CIBW_ENVIRONMENT_ANDROID="MAPLIBRE_NATIVE_C_INSTALL_DIR=$native_install_dir"
export CIBW_TEST_COMMAND_ANDROID='python -m pytest tests'
export CIBW_TEST_REQUIRES_ANDROID='pytest>=9,<10 pyopengl>=3.1.10,<4'
export CIBW_TEST_RUNTIME='args: --connected emulator-5554'
export CIBW_TEST_SOURCES_ANDROID=tests

cd "$MISE_MONOREPO_ROOT/bindings/python"
exec uv run --project . --group android --no-sync \
  cibuildwheel --platform android \
  --output-dir "$MISE_MONOREPO_ROOT/build/android-emulator/$preset/python/wheelhouse" \
  .
