#!/usr/bin/env bash
# Boots the Android emulator headless and waits until it is ready. Idempotent:
# an already-ready emulator, or one this script started earlier, is reused.
# Installs the emulator, platform-tools, and system image packages on first
# run, which is also when their licenses are accepted.
set -euo pipefail

api=${1:?usage: boot-android-emulator.sh <api> <abi>}
image_arch=${2:?usage: boot-android-emulator.sh <api> <abi>}
case "$image_arch" in
  arm64-v8a | x86_64) ;;
  *)
    echo "Unsupported Android emulator image architecture: $image_arch" >&2
    exit 2
    ;;
esac
image="system-images;android-$api;default;$image_arch"
serial=emulator-5554
avd_name="mln-ffi-api-$api-${image_arch//_/-}"
sdk_root="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}"
state_root="$MISE_MONOREPO_ROOT/build/android-emulator"
state_dir="$state_root/$avd_name"
pid_file="$state_dir/emulator.pid"
log_file="$state_dir/emulator.log"
# The AVD lives in the build tree rather than the user's ~/.android, so a
# checkout owns the device it boots and removing the tree removes it.
export ANDROID_AVD_HOME="$state_dir/avd"

# adb arrives with platform-tools, which the package check below installs, so
# an SDK that has never had it yet reaches that step rather than failing here.
adb="$sdk_root/platform-tools/adb"
if [[ -x "$adb" ]]; then
  running_avd=$("$adb" -s "$serial" emu avd name 2>/dev/null | sed -n '1s/\r$//p') ||
    running_avd=
  if [[ "$running_avd" == "$avd_name" ]] &&
    "$adb" -s "$serial" shell getprop sys.boot_completed 2>/dev/null |
    tr -d '\r' | grep -qx 1; then
    echo "Android emulator is ready at $serial."
    exit 0
  fi
  if [[ -n "$running_avd" && "$running_avd" != "$avd_name" ]]; then
    echo "Android emulator $running_avd already owns $serial; stop it before booting $avd_name." >&2
    exit 2
  fi
fi

sdkmanager=
for tools_dir in "$sdk_root"/cmdline-tools/latest/bin "$sdk_root"/cmdline-tools/*/bin; do
  if [[ -x "$tools_dir/sdkmanager" ]]; then
    sdkmanager="$tools_dir/sdkmanager"
    avdmanager="$tools_dir/avdmanager"
    break
  fi
done
if [[ -z "$sdkmanager" ]]; then
  echo "No sdkmanager under $sdk_root/cmdline-tools." >&2
  exit 1
fi

# Each package with the directory that proves it is installed, so a rerun starts
# no JVM and reaches no network.
missing=()
for entry in "platform-tools|platform-tools" "emulator|emulator" "$image|${image//;//}"; do
  if [[ ! -d "$sdk_root/${entry#*|}" ]]; then
    missing+=("${entry%%|*}")
  fi
done
if ((${#missing[@]})); then
  echo "Installing Android SDK packages: ${missing[*]}"
  # One `y` per license prompt, as a here-string rather than a pipe: `yes` would
  # take a SIGPIPE when sdkmanager stops reading, and that becomes the
  # pipeline's status.
  accepts="$(printf 'y\n%.0s' "${missing[@]}")"
  "$sdkmanager" --sdk_root="$sdk_root" --install "${missing[@]}" <<<"$accepts"
fi

mkdir -p "$ANDROID_AVD_HOME" "$state_dir"
if [[ ! -d "$ANDROID_AVD_HOME/$avd_name.avd" ]]; then
  # `no` declines the custom hardware profile prompt. Everything the suite
  # depends on is a launch flag below rather than a stored device setting.
  "$avdmanager" create avd --name "$avd_name" --package "$image" --device pixel_6 <<<"no"
fi

if [[ -f "$pid_file" ]]; then
  pid=$(<"$pid_file")
  if [[ "$pid" =~ ^[0-9]+$ ]] &&
    kill -0 "$pid" 2>/dev/null &&
    ps -p "$pid" -o args= | grep -q "$avd_name"; then
    echo "Waiting for Android emulator process $pid."
  else
    rm -f "$pid_file"
  fi
fi

# One emulator owns the fixed adb serial. Stop a running AVD before switching
# architectures so that a test never installs an artifact into the wrong guest.
shopt -s nullglob
other_pid_files=("$state_root"/emulator.pid "$state_root"/*/emulator.pid)
for other_pid_file in "${other_pid_files[@]}"; do
  [[ -f "$other_pid_file" ]] || continue
  [[ "$other_pid_file" == "$pid_file" ]] && continue
  other_pid=$(<"$other_pid_file")
  if [[ "$other_pid" =~ ^[0-9]+$ ]] && kill -0 "$other_pid" 2>/dev/null; then
    echo "Another mise-managed Android emulator is running; stop it before booting $avd_name." >&2
    exit 2
  fi
  rm -f "$other_pid_file"
done
shopt -u nullglob

if [[ ! -f "$pid_file" ]]; then
  # SwiftShader draws the GLES and Vulkan targets in software, which is what a
  # runner without a GPU has. The suite renders offscreen, so no window is
  # needed and no snapshot is written: a run starts from the installed image.
  launcher=(
    "$sdk_root/emulator/emulator"
    -avd "$avd_name"
    -no-window
    -no-audio
    -no-boot-anim
    -no-snapshot
    -no-metrics
    -gpu swiftshader
    -memory 4096
    -camera-back none
    -camera-front none
  )
  case "$(uname -s)" in
    Darwin | CYGWIN* | MINGW* | MSYS*) launcher+=(-accel on) ;;
    Linux)
      if [[ -r /dev/kvm && -w /dev/kvm ]]; then
        launcher+=(-accel on)
      else
        echo "KVM is inaccessible; using QEMU software emulation." >&2
        launcher+=(-accel off)
      fi
      ;;
    *) launcher+=(-accel off) ;;
  esac
  if command -v setsid >/dev/null 2>&1; then
    nohup setsid "${launcher[@]}" </dev/null >"$log_file" 2>&1 &
  else
    nohup "${launcher[@]}" </dev/null >"$log_file" 2>&1 &
  fi
  pid=$!
  printf '%s\n' "$pid" >"$pid_file"
  echo "Started Android emulator process $pid."
fi

pid=$(<"$pid_file")
for ((attempt = 0; attempt < 600; attempt++)); do
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "Android emulator exited before it became ready." >&2
    tail -100 "$log_file" >&2
    rm -f "$pid_file"
    exit 1
  fi
  if "$adb" -s "$serial" shell getprop sys.boot_completed 2>/dev/null |
    tr -d '\r' | grep -qx 1; then
    echo "Android emulator is ready at $serial."
    exit 0
  fi
  sleep 1
done

echo "Android emulator did not become ready within 600 seconds." >&2
tail -100 "$log_file" >&2
exit 1
