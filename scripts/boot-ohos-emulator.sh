#!/usr/bin/env bash
# Boots the shared OpenHarmony emulator from the Oniro QEMU image that mise
# installed and waits until it answers hdc. Every checkout and session on the
# machine shares the one emulator that runs the image: an emulator that already
# runs it is reused, whoever started it and at whatever connect key, and keeps
# running afterwards.
#
# A new emulator forwards hdc from the first free port from 55555 on, and its
# log goes to the temporary directory.
set -euo pipefail

image_dir=$(cd -P "$(mise where http:oniro-emulator)/images" && pwd)
log_dir=${TMPDIR:-/tmp}
log_file="${log_dir%/}/ohos-emulator-$(basename "$(dirname "$image_dir")").log"
scripts="$MISE_MONOREPO_ROOT/scripts"

running() {
  "$scripts/ohos-emulator-process.sh" "$image_dir" >/dev/null
}

if connect_key=$("$scripts/ohos-emulator-connect-key.sh"); then
  echo "OpenHarmony emulator is ready at $connect_key."
  exit 0
fi

# Something may hold a port without accepting connections, so a port is free
# when it can be bound.
port_free() {
  perl -MIO::Socket::INET -e \
    'IO::Socket::INET->new(LocalAddr => "127.0.0.1", LocalPort => shift, Listen => 1) or exit 1' \
    "$1"
}

pid=
if running; then
  echo "Waiting for the running OpenHarmony emulator."
else
  port=
  for ((candidate = 55555; candidate <= 55655; candidate++)); do
    if port_free "$candidate"; then
      port=$candidate
      break
    fi
  done
  if [[ -z "$port" ]]; then
    echo "No free OpenHarmony emulator port on 127.0.0.1 between 55555 and 55655." >&2
    exit 1
  fi
  connect_key="127.0.0.1:$port"
  # The log is appended to, so an emulator that another session launches at the
  # same moment adds its lines rather than overwriting these.
  : >"$log_file"
  launcher=("$scripts/run-ohos-emulator.sh" "$image_dir" "$connect_key")
  if command -v setsid >/dev/null 2>&1; then
    nohup setsid "${launcher[@]}" </dev/null >>"$log_file" 2>&1 &
  else
    nohup "${launcher[@]}" </dev/null >>"$log_file" 2>&1 &
  fi
  pid=$!
  echo "Started OpenHarmony emulator as process $pid at $connect_key."
fi

for ((attempt = 0; attempt < 300; attempt++)); do
  if connect_key=$("$scripts/ohos-emulator-connect-key.sh"); then
    echo "OpenHarmony emulator is ready at $connect_key."
    exit 0
  fi
  # An emulator that finds the images locked by another session's launch, or
  # its port taken by it, exits, and the other session's emulator is then the
  # one to wait for.
  if ! { [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; } && ! running; then
    echo "OpenHarmony emulator exited before it became ready." >&2
    [[ ! -f "$log_file" ]] || tail -100 "$log_file" >&2
    exit 1
  fi
  sleep 1
done

echo "OpenHarmony emulator did not become ready within 300 seconds." >&2
[[ ! -f "$log_file" ]] || tail -100 "$log_file" >&2
exit 1
