#!/usr/bin/env bash
# Runs native test executables in the OpenHarmony emulator, booting it when
# needed. Every executable runs and reports its own exit status; the first
# failure stops the batch.
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <timeout-seconds> <native-library> <c++-library> [test-argument ...] -- <test-executable ...>" >&2
  exit 2
fi

timeout_seconds=$1
native_library=$2
cxx_library=$3
shift 3
test_arguments=()
while (($#)) && [[ $1 != -- ]]; do
  test_arguments+=("$1")
  shift
done
if [[ ${1:-} == -- ]]; then
  shift
fi
if (($# == 0)); then
  echo "No test executables given." >&2
  exit 2
fi
test_executables=("$@")

connect_key=127.0.0.1:55555
remote_dir=/data/local/tmp/maplibre-native-ffi
fixture_dir=${MLN_FFI_TEST_FIXTURE_DIR:-}

for local_file in "$native_library" "$cxx_library" "${test_executables[@]}"; do
  if [[ ! -f "$local_file" ]]; then
    echo "OpenHarmony emulator test input does not exist: $local_file" >&2
    exit 2
  fi
done
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
  mise run //:ohos-emulator:boot
fi

hdc -t "$connect_key" shell "rm -rf '$remote_dir' && mkdir -p '$remote_dir'"
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

for test_executable in "${test_executables[@]}"; do
  echo "Running $(basename "$test_executable") in the OpenHarmony emulator."
  hdc -t "$connect_key" file send "$test_executable" "$remote_dir/test-executable"

  remote_command="cd '$remote_dir' && chmod 755 test-executable && ${fixture_environment}LD_LIBRARY_PATH='$remote_dir' ./test-executable"
  for argument in ${test_arguments[@]+"${test_arguments[@]}"}; do
    printf -v quoted_argument '%q' "$argument"
    remote_command+=" $quoted_argument"
  done
  remote_command+="; test_status=\$?; printf '\n__MLN_FFI_TEST_STATUS__=%s\n' \"\$test_status\""

  output_file=$(mktemp)
  set +e
  perl -e 'alarm shift; exec @ARGV' "$timeout_seconds" \
    hdc -t "$connect_key" shell "$remote_command" | tee "$output_file"
  transport_status=${PIPESTATUS[0]}
  set -e
  test_status=$(sed -n 's/^__MLN_FFI_TEST_STATUS__=//p' "$output_file" | tail -1 | tr -d '\r')
  rm -f "$output_file"
  if ((transport_status != 0)); then
    exit "$transport_status"
  fi
  if [[ ! "$test_status" =~ ^[0-9]+$ ]]; then
    echo "OpenHarmony emulator test returned no exit status." >&2
    exit 1
  fi
  if ((test_status != 0)); then
    exit "$test_status"
  fi
  hdc -t "$connect_key" shell "rm -f '$remote_dir/test-executable'"
done
