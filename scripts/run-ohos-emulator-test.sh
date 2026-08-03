#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <test-executable> <native-library> <c++-library> <timeout-seconds> [test-argument ...]" >&2
  exit 2
fi

test_executable=$1
native_library=$2
cxx_library=$3
timeout_seconds=$4
shift 4
connect_key=127.0.0.1:55555
remote_dir=/data/local/tmp/maplibre-native-ffi
fixture_dir=${MLN_FFI_TEST_FIXTURE_DIR:-}

for local_file in "$test_executable" "$native_library"; do
  if [[ ! -f "$local_file" ]]; then
    echo "OpenHarmony emulator test input does not exist: $local_file" >&2
    exit 2
  fi
done
if [[ ! -f "$cxx_library" ]]; then
  echo "OpenHarmony emulator C++ library does not exist: $cxx_library" >&2
  exit 2
fi
if [[ ! "$timeout_seconds" =~ ^[0-9]+$ ]]; then
  echo "Invalid timeout: $timeout_seconds" >&2
  exit 2
fi
if [[ -n "$fixture_dir" && ! -d "$fixture_dir" ]]; then
  echo "OpenHarmony emulator fixture directory does not exist: $fixture_dir" >&2
  exit 2
fi

if ! hdc tconn "$connect_key" >/dev/null 2>&1 ||
  ! hdc -t "$connect_key" shell echo ready 2>/dev/null | tr -d '\r' | grep -qx ready; then
  echo "No OpenHarmony emulator is ready. Run 'mise run //:ohos-emulator:boot' first." >&2
  exit 2
fi

hdc -t "$connect_key" shell "rm -rf '$remote_dir' && mkdir -p '$remote_dir'"
hdc -t "$connect_key" file send "$test_executable" "$remote_dir/test-executable"
hdc -t "$connect_key" file send "$native_library" "$remote_dir/libmaplibre-native-c.so"
hdc -t "$connect_key" file send "$cxx_library" "$remote_dir/libc++_shared.so"

fixture_environment=
if [[ -n "$fixture_dir" ]]; then
  while IFS= read -r -d '' fixture; do
    relative_path=${fixture#"$fixture_dir"/}
    remote_fixture="$remote_dir/fixtures/$relative_path"
    hdc -t "$connect_key" shell "mkdir -p '$(dirname "$remote_fixture")'"
    hdc -t "$connect_key" file send "$fixture" "$remote_fixture"
  done < <(find "$fixture_dir" -type f -print0)
  fixture_environment="MLN_FFI_TEST_FIXTURE_DIR='$remote_dir/fixtures' "
fi

remote_command="cd '$remote_dir' && chmod 755 test-executable && ${fixture_environment}LD_LIBRARY_PATH='$remote_dir' ./test-executable"
for argument in "$@"; do
  printf -v quoted_argument '%q' "$argument"
  remote_command+=" $quoted_argument"
done
remote_command+="; test_status=\$?; printf '\n__MLN_FFI_TEST_STATUS__=%s\n' \"\$test_status\""

output_file=$(mktemp)
trap 'find "$output_file" -delete' EXIT
set +e
perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" \
  hdc -t "$connect_key" shell "$remote_command" | tee "$output_file"
transport_status=${PIPESTATUS[0]}
set -e
if ((transport_status != 0)); then
  exit "$transport_status"
fi
test_status=$(sed -n 's/^__MLN_FFI_TEST_STATUS__=//p' "$output_file" | tail -1 | tr -d '\r')
if [[ ! "$test_status" =~ ^[0-9]+$ ]]; then
  echo "OpenHarmony emulator test returned no exit status." >&2
  exit 1
fi
exit "$test_status"
