#!/usr/bin/env bash

case "$(uname -s)" in
  MINGW* | MSYS* | CYGWIN*) ;;
  *) return 0 ;;
esac

windows_host_architecture="${MLN_FFI_WINDOWS_HOST_ARCHITECTURE:-$(uname -m)}"
case "$windows_host_architecture" in
  aarch64 | arm64) msvc_arch=arm64 ;;
  x86_64 | amd64) msvc_arch=x64 ;;
  *) echo "Unsupported Windows host architecture: $windows_host_architecture" >&2; return 1 ;;
esac

standalone_llvm_bin='/c/Program Files/LLVM/bin'
prefer_standalone_llvm() {
  if [[ ! -x "$standalone_llvm_bin/clang-cl.exe" ]]; then
    echo "Standalone LLVM's clang-cl.exe was not found in $standalone_llvm_bin" >&2
    return 1
  fi

  IFS=: read -r -a path_entries <<< "$PATH"
  filtered_path=
  for entry in "${path_entries[@]}"; do
    [[ "$entry" == "$standalone_llvm_bin" ]] && continue
    filtered_path="${filtered_path:+$filtered_path:}$entry"
  done
  export PATH="$standalone_llvm_bin${filtered_path:+:$filtered_path}"

  if [[ -f "$standalone_llvm_bin/libclang.dll" ]]; then
    export LIBCLANG_PATH="$standalone_llvm_bin"
  fi
}

original_path="$PATH"
prefer_standalone_llvm || return 1

current_vscmd_arch="${VSCMD_ARG_TGT_ARCH:-}"
if [[ -n "${VCTOOLSINSTALLDIR:-${VCToolsInstallDir:-}}" &&
  "${current_vscmd_arch,,}" == "$msvc_arch" ]]; then
  return 0
fi

vswhere='/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe'
if [[ ! -x "$vswhere" ]]; then
  echo "Visual Studio Installer's vswhere.exe was not found" >&2
  return 1
fi

vs_query() {
  "$vswhere" -latest -version '[17.0,18.0)' -products '*' \
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
    -property "$1" | tr -d '\r' | sed -n '1p'
}

vs_install="$(vs_query installationPath)"
if [[ -z "$vs_install" ]]; then
  echo "Visual Studio 2022 with the Desktop development with C++ workload was not found" >&2
  return 1
fi

# `set` reports every variable cmd.exe inherited, not just what VsDevCmd added.
# Replaying an inherited value in a later shell is what makes it wrong: a GitHub
# Actions step's $GITHUB_ENV names a file the runner consumes and retires when
# that step ends, so a second step handed the first one's path writes where
# nothing reads. Keep the cache to what VsDevCmd itself contributed.
is_vsdevcmd_contribution() {
  local name="$1" value="$2"
  case "$name" in
    # Per-step or per-shell state VsDevCmd never sets. MSYS2 can rewrite paths
    # on the way into cmd.exe, so these can differ without VsDevCmd's help.
    GITHUB_* | RUNNER_* | ACTIONS_* | INPUT_* | \
      HOME | PWD | OLDPWD | SHLVL | CD | PROMPT | \
      TMP | TEMP | TMPDIR | BASH_ENV | MSYS* | CYGWIN)
      return 1
      ;;
  esac
  [[ -z "${!name+set}" ]] && return 0
  [[ "${!name}" != "$value" ]]
}

# Every mise task, and on CI every `shell: bash` step, starts a fresh Git Bash
# that sources this file. Resolving the environment from scratch each time costs
# a cmd.exe running VsDevCmd.bat plus a walk of the redist tree, which is slow
# everywhere and is the dominant source of process creation on Windows ARM64,
# where the MSYS2 runtime crashes under that churn. Resolving it once per
# toolchain and replaying the result keeps later shells to one vswhere pair.
#
# The key covers everything the cached values derive from: the install path, the
# installed version, and the target architecture. An in-place Visual Studio
# update changes the version and so retires the entry.
vs_version="$(vs_query installationVersion)"
cache_key="${vs_install//[^A-Za-z0-9]/_}-${vs_version//[^A-Za-z0-9]/_}-${msvc_arch}"
cache_file="${TMPDIR:-/tmp}/mln-msvc-env-${cache_key}.sh"

msvc_path=
crt_path=
cache_complete=
if [[ -r "$cache_file" ]]; then
  # shellcheck source=/dev/null
  source "$cache_file"
fi

# The cache sets its completion marker last, so a truncated file reads as a miss
# rather than a partial replay. The resolved values cannot serve as the marker:
# both are legitimately empty when this shell already has the entries VsDevCmd
# would add.
if [[ -z "$cache_complete" ]]; then
  vs_dev_cmd="${vs_install}\\Common7\\Tools\\VsDevCmd.bat"
  loader="$(mktemp "${TMPDIR:-/tmp}/mln-vsdev.XXXXXX.bat")"
  cat > "$loader" <<EOF
@echo off
call "$vs_dev_cmd" -arch=$msvc_arch -host_arch=$msvc_arch >nul
set
EOF

  loader_windows="$(cygpath -w "$loader")"
  if ! environment="$(cmd.exe //d //s //c "$loader_windows")"; then
    rm -f "$loader"
    echo "VsDevCmd.bat failed to initialize the Visual Studio environment" >&2
    return 1
  fi
  rm -f "$loader"

  cache_body=
  while IFS='=' read -r name value; do
    [[ "$name" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || continue
    case "$name" in
      Path | PATH) msvc_path="$(cygpath -u -p "$value")" ;;
      *)
        export "$name=$value"
        if is_vsdevcmd_contribution "$name" "$value"; then
          cache_body+="export $name=$(printf '%q' "$value")"$'\n'
        fi
        ;;
    esac
  done < <(tr -d '\r' <<< "$environment")

  # cmd.exe inherited this shell's PATH, so its Path is those entries plus
  # VsDevCmd's. Keep the additions for the same reason the variables above are
  # filtered: the inherited half belongs to the shell that generated the cache,
  # and prepending its copy here would let stale entries win. The tail of PATH
  # below supplies each sourcing shell's own entries.
  inherited_path=":$PATH:"
  vs_path=
  while IFS= read -r entry; do
    [[ -n "$entry" ]] || continue
    [[ "$inherited_path" == *":$entry:"* ]] && continue
    vs_path="${vs_path:+$vs_path:}$entry"
  done < <(tr ':' '\n' <<< "$msvc_path")
  msvc_path="$vs_path"

  redist_root="$(cygpath -u "${vs_install}\\VC\\Redist\\MSVC")"
  crt_path="$(find "$redist_root" -path "*/${msvc_arch}/Microsoft.VC143.CRT/msvcp140_codecvt_ids.dll" -print 2>/dev/null | sort -r | sed -n '1p')"
  if [[ -n "$crt_path" ]]; then
    crt_path="$(dirname "$crt_path")"
  fi

  # Publish through a rename so a concurrent shell sees either no entry or a
  # complete one. Writing the cache is best effort: a failure here costs the
  # next shell a rebuild, which is what it would have paid anyway.
  cache_body+="msvc_path=$(printf '%q' "$msvc_path")"$'\n'
  cache_body+="crt_path=$(printf '%q' "$crt_path")"$'\n'
  cache_body+="cache_complete=1"$'\n'
  if cache_staging="$(mktemp "${cache_file}.XXXXXX" 2>/dev/null)"; then
    if ! { printf '%s' "$cache_body" > "$cache_staging" &&
      mv -f "$cache_staging" "$cache_file"; }; then
      rm -f "$cache_staging"
    fi
  fi
fi

export PATH="${crt_path:+$crt_path:}$standalone_llvm_bin:${msvc_path:+$msvc_path:}$original_path"
