#!/usr/bin/env bash
# Boots the Android emulator headless and waits until it is ready. Idempotent:
# the emulator that this script started earlier in this checkout is reused.
# Installs the emulator, platform-tools, and system image packages on first
# run, which is also when their licenses are accepted.
#
# The emulator takes the first free console port, and its adb serial is
# recorded beside its PID. Emulators that this checkout did not start keep
# running untouched, whatever port they hold.
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
avd_name="mln-ffi-api-$api-${image_arch//_/-}"
sdk_root="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}"
state_root="$MISE_MONOREPO_ROOT/build/android-emulator"
state_dir="$state_root/$avd_name"
pid_file="$state_dir/emulator.pid"
serial_file="$state_dir/serial"
log_file="$state_dir/emulator.log"
# The AVD lives in the build tree rather than the user's ~/.android, so a
# checkout owns the device it boots and removing the tree removes it.
export ANDROID_AVD_HOME="$state_dir/avd"

# adb arrives with platform-tools, which the package check below installs, so
# an SDK that has never had it yet reaches that step rather than failing here.
adb="$sdk_root/platform-tools/adb"

# Succeeds when the emulator at the serial runs this AVD and has booted.
emulator_ready() {
  [[ -x "$adb" ]] || return 1
  [[ "$("$adb" -s "$1" emu avd name 2>/dev/null | sed -n '1s/\r$//p')" == "$avd_name" ]] &&
    "$adb" -s "$1" shell getprop sys.boot_completed 2>/dev/null |
    tr -d '\r' | grep -qx 1
}

# A recorded PID counts only while it still runs this AVD. An exited emulator
# leaves its files behind, and the OS may reuse its PID.
pid=
serial=
if [[ -f "$pid_file" ]]; then
  pid=$(<"$pid_file")
  if [[ "$pid" =~ ^[0-9]+$ ]] &&
    kill -0 "$pid" 2>/dev/null &&
    ps -p "$pid" -o args= | grep -q "$avd_name"; then
    if [[ -f "$serial_file" ]]; then
      serial=$(<"$serial_file")
    else
      echo "Android emulator process $pid has no recorded serial; run mise run //:android-emulator:stop first." >&2
      exit 2
    fi
  else
    pid=
    rm -f "$pid_file" "$serial_file"
  fi
fi
if [[ -n "$pid" ]] && emulator_ready "$serial"; then
  echo "Android emulator is ready at $serial."
  exit 0
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

if [[ -n "$pid" ]]; then
  echo "Waiting for Android emulator process $pid at $serial."
fi

# A checkout runs one mise-managed emulator at a time, so a task that finds it
# never installs an artifact into a guest of the wrong API or architecture.
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
  rm -f "$other_pid_file" "${other_pid_file%/*}/serial"
done
shopt -u nullglob

# The console listens on an even port and adb on the next one. adb lists the
# emulators that it knows, including ones another checkout or tool started, and
# a connection test finds a listener that adb does not know.
port_bound() {
  (: <"/dev/tcp/127.0.0.1/$1") 2>/dev/null
}

if [[ -z "$pid" ]]; then
  used_ports=" "
  while read -r device _; do
    if [[ "$device" == emulator-* ]]; then
      used_ports+="${device#emulator-} "
    fi
  done < <("$adb" devices | tr -d '\r')
  port=
  for ((candidate = 5554; candidate <= 5682; candidate += 2)); do
    if [[ "$used_ports" != *" $candidate "* ]] &&
      ! port_bound "$candidate" && ! port_bound $((candidate + 1)); then
      port=$candidate
      break
    fi
  done
  if [[ -z "$port" ]]; then
    echo "No free Android emulator console port between 5554 and 5682." >&2
    exit 1
  fi
  serial="emulator-$port"

  # SwiftShader draws the GLES and Vulkan targets in software, which is what a
  # runner without a GPU has. The suite renders offscreen, so no window is
  # needed and no snapshot is written: a run starts from the installed image.
  launcher=(
    "$sdk_root/emulator/emulator"
    -avd "$avd_name"
    -port "$port"
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
  printf '%s\n' "$serial" >"$serial_file"
  echo "Started Android emulator process $pid at $serial."
fi

for ((attempt = 0; attempt < 600; attempt++)); do
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "Android emulator exited before it became ready." >&2
    tail -100 "$log_file" >&2
    rm -f "$pid_file" "$serial_file"
    exit 1
  fi
  if emulator_ready "$serial"; then
    echo "Android emulator is ready at $serial."
    exit 0
  fi
  sleep 1
done

echo "Android emulator did not become ready within 600 seconds." >&2
tail -100 "$log_file" >&2
exit 1
