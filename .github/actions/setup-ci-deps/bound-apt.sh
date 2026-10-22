#!/usr/bin/env bash
# x64 runners list azure.archive.ubuntu.com first: GitHub-hosted runners in
# /etc/apt/apt-mirrors.txt, and Blacksmith runners in
# /etc/apt/blacksmith-ubuntu-mirrors.txt. That mirror periodically serves at
# tens of kB/s without erroring, so apt never fails over. mise bootstrap then
# spends tens of minutes on libclang-dev (~29 MB) while ARM jobs
# (ports.ubuntu.com) finish the same install in a second.
#
# Demote Azure before any apt-get so archive.ubuntu.com is tried first, and
# bound idle HTTP fetches so a hung connection fails over instead of sitting
# until the job timeout. Do not SIGTERM apt-get: interrupting dpkg mid-unpack
# leaves the package database unusable.
set -euo pipefail

sudo tee /etc/apt/apt.conf.d/99ci-acquire >/dev/null <<'EOF'
Acquire::Retries "5";
Acquire::http::Timeout "20";
Acquire::https::Timeout "20";
Acquire::ForceIPv4 "true";
DPkg::Lock::Timeout "60";
Dpkg::Use-Pty "0";
EOF

echo "Wrote /etc/apt/apt.conf.d/99ci-acquire"

# Both lists use apt's mirror+file format: a URL, then tab-separated
# key:value pairs, where a lower priority is tried first.
for mirrors in /etc/apt/apt-mirrors.txt /etc/apt/blacksmith-ubuntu-mirrors.txt; do
  [[ -f "$mirrors" ]] || continue
  if grep -q 'azure\.archive\.ubuntu\.com' "$mirrors"; then
    demoted="$(awk '/azure\.archive\.ubuntu\.com/ && !sub(/priority:[0-9]+/, "priority:9") { $0 = $0 "\tpriority:9" } { print }' "$mirrors")"
    printf '%s\n' "$demoted" | sudo tee "$mirrors" >/dev/null
    # A list of Azure alone would leave apt nowhere to fail over to.
    if ! grep -q '://archive\.ubuntu\.com' "$mirrors"; then
      printf 'https://archive.ubuntu.com/ubuntu/\tpriority:2\n' | sudo tee -a "$mirrors" >/dev/null
    fi
  fi
  echo "Current apt mirrors in $mirrors:"
  cat "$mirrors"
done
