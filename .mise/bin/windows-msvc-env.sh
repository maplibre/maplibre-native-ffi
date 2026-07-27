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
if [[ -r "$cache_file" ]]; then
  # shellcheck source=/dev/null
  source "$cache_file"
fi

# A cache written by a killed shell can be truncated, so treat a missing
# msvc_path as a miss rather than trusting a partial replay.
if [[ -z "$msvc_path" ]]; then
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
        cache_body+="export $name=$(printf '%q' "$value")"$'\n'
        ;;
    esac
  done < <(tr -d '\r' <<< "$environment")

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
  if cache_staging="$(mktemp "${cache_file}.XXXXXX" 2>/dev/null)"; then
    if ! { printf '%s' "$cache_body" > "$cache_staging" &&
      mv -f "$cache_staging" "$cache_file"; }; then
      rm -f "$cache_staging"
    fi
  fi
fi

export PATH="${crt_path:+$crt_path:}$standalone_llvm_bin:${msvc_path:+$msvc_path:}$original_path"
