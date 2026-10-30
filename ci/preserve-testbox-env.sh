#!/usr/bin/env bash
# Keeps a Testbox job's toolchain and setup exports for its SSH commands, minus
# credentials and step-local state. SSH shells read .bashrc before Ubuntu's
# non-interactive early return.
set -euo pipefail

umask 077
(
  temp_prefix=$RUNNER_TEMP
  path=()
  IFS=: read -ra entries <<< "$PATH"
  for entry in "${entries[@]}"; do
    [[ "$entry" == "$RUNNER_TEMP"* ]] || path+=("$entry")
  done
  PATH="$(IFS=:; echo "${path[*]}")"
  for name in $(compgen -e); do
    case "$name" in
      ACTIONS_*|INPUT_*|RUNNER_*|GITHUB_TOKEN|GITHUB_ENV|GITHUB_PATH|GITHUB_OUTPUT|GITHUB_STATE|GITHUB_STEP_SUMMARY|GITHUB_ACTION*|SSH_*|BASH_ENV|ENV|PWD|OLDPWD|SHLVL|*_TOKEN*|*_SECRET*|*_PASSWORD*|*_KEY*|*_CREDENTIAL*)
        unset "$name" ;;
      *) [[ "${!name}" != *"${temp_prefix:?}"* ]] || unset "$name" ;;
    esac
  done
  export -p
) > ~/.testbox-env
sed -i '1i [ -f ~/.testbox-env ] && . ~/.testbox-env' ~/.bashrc
