#!/usr/bin/env bash
# Builds the ArkTS test HAP for an OpenHarmony preset, runs it on the emulator,
# and fails unless every case passed and hypium ran as many cases as the
# harness registered.
#
#   test-arkts.sh <preset>
#
# The addon and the graphics test module must already be built for the
# preset, which the binding's test task does first. The suite's HTTP origin
# runs on this machine, and `hdc rport` forwards the same port on the device to
# it.
set -euo pipefail

preset=${1:?usage: test-arkts.sh <preset>}
binding="$(cd "$(dirname "$0")/.." && pwd)"
case "$preset" in
  ohos-x64-*) abi=x86_64 triple=x86_64-linux-ohos ;;
  ohos-arm64-*) abi=arm64-v8a triple=aarch64-linux-ohos ;;
  *)
    echo "error: test-arkts.sh takes an OpenHarmony preset, not $preset" >&2
    exit 2
    ;;
esac
build="$binding/native/build/$preset"
work="$build/hap-suite"
bundle=org.maplibre.nativeffi.suite
native_install="$(cd "$binding/../.." && pwd)/build/$preset/install"

rm -rf "$work"
mkdir -p "$work/ets" "$work/libs/$abi"
cp "$binding/test/arkts/TestAbility.ts" "$work/ets/TestAbility.ts"
# The addon links the shared C++ runtime, which the device does not carry.
cp "$OHOS_SDK_NATIVE/llvm/lib/$triple/libc++_shared.so" "$work/libs/$abi/"
# One bundle with two entries, the hypium runner and a spawned realm.
(
  cd "$binding/test/arkts"
  MAPLIBRE_NATIVE_FFI_ARKTS_BUNDLE="$work/ets" pnpm exec vp pack >/dev/null
)
# hypium, which mise.ohos.toml pins, is plain JavaScript with relative
# imports, so its files become records beside the bundle's.
hypium="$(mise where http:hypium)"
mkdir -p "$work/ets/hypium"
cp -R "$hypium/package/." "$work/ets/hypium/"
find "$work/ets/hypium" -type f ! -name '*.js' -delete
perl -pi -e 's#from "\@ohos/hypium"#from "./hypium/index"#' "$work/ets/TestRunner.js"
cp "$build/libmaplibre_native_ffi.so" \
  "$build/libmaplibre_native_ffi_test_graphics.so" \
  "$native_install/lib/libmln_test_graphics.so" "$work/libs/$abi/"
"$binding/scripts/build-hap.sh" "$work" "$bundle" "$abi"

key_script="$binding/../../scripts/ohos-emulator-connect-key.sh"
if ! connect_key=$("$key_script"); then
  mise run //:ohos-emulator:boot
  connect_key=$("$key_script")
fi
hdc=(hdc -t "$connect_key")

# The origin listens on a free port here, and the device reaches it on the
# same port.
port_file="$work/origin-port"
node "$binding/test/arkts/origin.ts" "$port_file" &
origin_pid=$!
cleanup() {
  touch "$work/finished"
  kill "$origin_pid" 2>/dev/null || true
  if [[ -n ${port:-} ]]; then
    "${hdc[@]}" fport rm "tcp:$port tcp:$port" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT
for ((attempt = 0; attempt < 100; attempt++)); do
  [[ -s $port_file ]] && break
  sleep 0.1
done
port=$(<"$port_file")
"${hdc[@]}" rport "tcp:$port" "tcp:$port" >/dev/null

"${hdc[@]}" uninstall "$bundle" >/dev/null 2>&1 || true
"${hdc[@]}" install -r "$work/entry_test.hap"
report="$work/report.txt"
# A case that never settles would hold `aa test` forever, so a watchdog stops
# the application at a deadline, which ends the run without hypium's tally.
deadline=${MLN_FFI_ARKTS_TIMEOUT:-2400}
(
  SECONDS=0
  while ((SECONDS < deadline)); do
    [[ -e $work/finished ]] && exit
    sleep 1
  done
  echo "error: the run took longer than $deadline seconds; stopping $bundle" >&2
  # The application's thread stacks tell a blocked thread from an idle loop.
  pid=$("${hdc[@]}" shell pidof "$bundle" | tr -d '\r') || true
  if [[ -n $pid ]]; then
    "${hdc[@]}" shell dumpcatcher -p "$pid" >&2 || true
  fi
  "${hdc[@]}" shell aa force-stop "$bundle" >/dev/null 2>&1
) &
"${hdc[@]}" shell aa test -b "$bundle" -m entry_test \
  -s unittest /ets/TestRunner -s timeout 300000 \
  -s origin "http://127.0.0.1:$port" |
  tee "$report.raw"
touch "$work/finished"
# The device ends its lines with carriage returns.
tr -d '\r' <"$report.raw" >"$report"

# `aa test` exits 0 whatever the cases did, so the report decides. A run that
# never printed hypium's tally did not finish.
registered=$(sed -n 's/.*harness registered \([0-9]*\) cases$/\1/p' "$report" | tail -1)
tally=$(grep -o 'Tests run: [0-9]*, Failure: [0-9]*, Error: [0-9]*, Pass: [0-9]*, Ignore: [0-9]*' "$report" | tail -1 || true)
if [[ -z $registered || -z $tally ]]; then
  echo "error: the suite did not report its registered cases and hypium's tally" >&2
  exit 1
fi
read -r ran failed errored passed ignored < <(sed -E 's/[^0-9]+/ /g' <<<"$tally")
echo "harness registered $registered cases; hypium ran $ran: $passed passed, $failed failed, $errored errors, $ignored ignored"
if ((ran != registered || passed != registered || failed + errored + ignored != 0)); then
  exit 1
fi
