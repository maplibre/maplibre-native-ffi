#!/usr/bin/env bash
# GitHub-hosted x64 runners list azure.archive.ubuntu.com at priority 1.
# That mirror periodically serves at tens of kB/s without erroring, so apt
# never fails over. mise bootstrap then spends tens of minutes on
# libclang-dev (~29 MB) while ARM jobs (ports.ubuntu.com) finish the same
# install in a second.
#
# Bound idle HTTP fetches, and wrap apt-get so a crawl is cut off, the Azure
# mirror is demoted, and the call is retried against archive.ubuntu.com.
# The wrapper sits on sudo's PATH ahead of /usr/bin/apt-get, so the explicit
# installs below and `mise bootstrap` both pick it up.
set -euo pipefail

sudo tee /etc/apt/apt.conf.d/99ci-acquire >/dev/null <<'EOF'
Acquire::Retries "5";
Acquire::http::Timeout "20";
Acquire::https::Timeout "20";
Acquire::ForceIPv4 "true";
DPkg::Lock::Timeout "60";
Dpkg::Use-Pty "0";
EOF

wrapper="$(mktemp)"
trap 'rm -f "${wrapper}"' EXIT
cat >"${wrapper}" <<'EOF'
#!/bin/bash
set -u
real=/usr/bin/apt-get
timeout_sec="${MAPLIBRE_APT_GET_TIMEOUT:-120}"

/usr/bin/timeout --foreground --signal=TERM --kill-after=15 "${timeout_sec}" "${real}" "$@"
status=$?
if [[ "${status}" -eq 0 ]]; then
  exit 0
fi
if [[ "${status}" -ne 124 && "${status}" -ne 137 ]]; then
  exit "${status}"
fi

echo "apt-get exceeded ${timeout_sec}s; demoting azure.archive.ubuntu.com and retrying" >&2
if [[ -f /etc/apt/apt-mirrors.txt ]] && grep -q 'azure.archive.ubuntu.com' /etc/apt/apt-mirrors.txt; then
  sed -i 's/priority:1/priority:9/' /etc/apt/apt-mirrors.txt
  echo "apt mirrors after demote:" >&2
  cat /etc/apt/apt-mirrors.txt >&2
fi

/usr/bin/timeout --foreground --signal=TERM --kill-after=15 180 "${real}" "$@"
exit $?
EOF

sudo install -m0755 "${wrapper}" /usr/local/bin/apt-get

echo "Wrote /etc/apt/apt.conf.d/99ci-acquire and /usr/local/bin/apt-get wrapper"
if [[ -f /etc/apt/apt-mirrors.txt ]]; then
  echo "Current apt mirrors:"
  cat /etc/apt/apt-mirrors.txt
fi
