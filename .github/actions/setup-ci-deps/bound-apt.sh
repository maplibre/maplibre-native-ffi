#!/usr/bin/env bash
# GitHub-hosted x64 runners list azure.archive.ubuntu.com at priority 1.
# That mirror periodically serves at tens of kB/s without erroring, so apt
# never fails over. mise bootstrap then spends tens of minutes on
# libclang-dev (~29 MB) while ARM jobs (ports.ubuntu.com) finish the same
# install in a second.
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

if [[ -f /etc/apt/apt-mirrors.txt ]] && grep -q 'azure.archive.ubuntu.com' /etc/apt/apt-mirrors.txt; then
  sudo sed -i 's/priority:1/priority:9/' /etc/apt/apt-mirrors.txt
fi

echo "Wrote /etc/apt/apt.conf.d/99ci-acquire"
if [[ -f /etc/apt/apt-mirrors.txt ]]; then
  echo "Current apt mirrors:"
  cat /etc/apt/apt-mirrors.txt
fi
