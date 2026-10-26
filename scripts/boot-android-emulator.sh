#!/usr/bin/env bash
# Boots the shared Android emulator for an API level and ABI headless and waits
# until it is ready. Every checkout and session on the machine shares one
# emulator per API level and ABI: an emulator that already runs the AVD is
# reused, whoever started it, and keeps running afterwards. Installs the
# emulator, platform-tools, and system image packages on first run, which is
# also when their licenses are accepted.
#
# The AVD is named api-<api>-<abi> and lives in the user's AVD home, where it is
# created when it is missing. The emulator takes the first free console port,
# and its log goes to the temporary directory, where
# run-android-emulator-test.sh finds it.
set -euo pipefail

usage="usage: boot-android-emulator.sh <api> <abi> <minimum-emulator-version>"
api=${1:?$usage}
image_arch=${2:?$usage}
emulator_version=${3:?$usage}
case "$image_arch" in
  arm64-v8a | x86_64) ;;
  *)
    echo "Unsupported Android emulator image architecture: $image_arch" >&2
    exit 2
    ;;
esac
image="system-images;android-$api;default;$image_arch"
avd_name="api-$api-$image_arch"
sdk_root="${ANDROID_HOME:?ANDROID_HOME must point at an Android SDK}"
log_dir=${TMPDIR:-/tmp}
log_file="${log_dir%/}/android-emulator-$avd_name.log"
serial_script="$MISE_MONOREPO_ROOT/scripts/android-device-serial.sh"
# The default AVD home, named so that avdmanager and the emulator use the same
# directory.
export ANDROID_AVD_HOME="${ANDROID_AVD_HOME:-$HOME/.android/avd}"

# adb arrives with platform-tools, which the package check below installs, so
# an SDK that has never had it yet reaches that step rather than failing here.
adb="$sdk_root/platform-tools/adb"

# Prints the serial of the emulator that runs the AVD once it has booted. A
# serial that the caller selected names some other device, so it is ignored.
ready_serial() {
  ANDROID_SERIAL='' "$serial_script" "$image_arch" "$api"
}

# Succeeds while an emulator process runs the AVD, including one that has only
# just started and that adb does not list yet. The emulator takes the AVD as
# `-avd <name>` or `@<name>`.
avd_running() {
  pgrep -f -- "(-avd |@)$avd_name( |\$)" >/dev/null
}

if serial=$(ready_serial); then
  echo "Android emulator $avd_name is ready at $serial."
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

# Installs packages from the given channel and the more stable ones.
install_packages() {
  local channel=$1
  shift
  echo "Installing Android SDK packages: $*"
  # One `y` per license prompt, as a here-string rather than a pipe: `yes` would
  # take a SIGPIPE when sdkmanager stops reading, and that becomes the
  # pipeline's status.
  local accepts install_status
  accepts="$(printf 'y\n%.0s' "$@")"
  for attempt in 1 2 3; do
    if "$sdkmanager" --sdk_root="$sdk_root" --channel="$channel" --install "$@" <<<"$accepts"; then
      return
    else
      install_status=$?
    fi
    if ((attempt == 3)); then
      echo "Android SDK package installation failed after $attempt attempts." >&2
      exit "$install_status"
    fi
    echo "Android SDK package installation failed (attempt $attempt/3); retrying." >&2
    sleep "$((attempt * 5))"
  done
}

# Each package with the directory that proves it is installed, so a rerun starts
# no JVM and reaches no network.
missing=()
for entry in "platform-tools|platform-tools" "$image|${image//;//}"; do
  if [[ ! -d "$sdk_root/${entry#*|}" ]]; then
    missing+=("${entry%%|*}")
  fi
done
if ((${#missing[@]})); then
  install_packages 0 "${missing[@]}"
fi

# Emulators before the minimum can hang for good when a guest process exits
# while it still owns GLES objects (gfxstream b/561516389). Until the stable
# channel carries the fix, the update comes from the dev channel.
emulator_revision() {
  sed -n 's/^Pkg\.Revision=//p' "$sdk_root/emulator/source.properties" 2>/dev/null |
    tr -d '\r'
}
emulator_current() {
  [[ "$(printf '%s\n' "$emulator_version" "$(emulator_revision)" |
    sort -V | head -n1)" == "$emulator_version" ]]
}
if ! emulator_current; then
  install_packages 2 emulator
  if ! emulator_current; then
    echo "The Android emulator is at revision $(emulator_revision), below $emulator_version." >&2
    exit 1
  fi
fi

avd_exists() {
  "$sdk_root/emulator/emulator" -list-avds 2>/dev/null | tr -d '\r' | grep -qx "$avd_name"
}
if ! avd_exists; then
  # avdmanager succeeds without creating anything when the AVD home is missing,
  # as it is on a fresh runner. `no` declines the custom hardware profile
  # prompt. Everything the suite depends on is a launch flag below rather than
  # a stored device setting. When another session creates the AVD at the same
  # moment, avdmanager fails here, and that session's AVD serves both.
  mkdir -p "$ANDROID_AVD_HOME"
  "$avdmanager" create avd --name "$avd_name" --package "$image" --device pixel_6 <<<"no" ||
    avd_exists
fi

# The console listens on an even port and adb on the next one. adb lists the
# emulators that it knows, including ones another session or tool started, and
# a connection test finds a listener that adb does not know.
port_bound() {
  (: <"/dev/tcp/127.0.0.1/$1") 2>/dev/null
}

pid=
if avd_running; then
  echo "Waiting for the running Android emulator $avd_name."
else
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
  # The log is appended to, so an emulator that another session launches at the
  # same moment adds its lines rather than overwriting these.
  : >"$log_file"

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
      # The emulator's renderer makes JIT code executable in memory that its
      # bundled tcmalloc takes with sbrk, which SELinux refuses as execheap on
      # Fedora. Memory that tcmalloc maps instead needs only execmem.
      export TCMALLOC_SKIP_SBRK=true
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
    nohup setsid "${launcher[@]}" </dev/null >>"$log_file" 2>&1 &
  else
    nohup "${launcher[@]}" </dev/null >>"$log_file" 2>&1 &
  fi
  pid=$!
  echo "Started Android emulator $avd_name as process $pid at $serial."
fi

# A check can wait on a call that misses its deadline, so the wait is bounded by
# the clock rather than by the number of checks.
deadline=$((SECONDS + 600))
while ((SECONDS < deadline)); do
  if serial=$(ready_serial); then
    echo "Android emulator $avd_name is ready at $serial."
    exit 0
  fi
  # An emulator that finds the AVD locked by another session's launch exits,
  # and the other session's emulator is then the one to wait for.
  if ! { [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; } && ! avd_running; then
    echo "Android emulator $avd_name exited before it became ready." >&2
    [[ ! -f "$log_file" ]] || tail -100 "$log_file" >&2
    exit 1
  fi
  sleep 1
done

echo "Android emulator $avd_name did not become ready within 600 seconds." >&2
[[ ! -f "$log_file" ]] || tail -100 "$log_file" >&2
exit 1
