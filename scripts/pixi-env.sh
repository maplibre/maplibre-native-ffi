#!/usr/bin/env bash
set -eo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"

# Export Pixi's activated environment into mise. --locked keeps activation tied
# to pixi.lock, and --shell bash matches mise's env._.source execution model.
original_path="$PATH"
eval "$(pixi shell-hook --manifest-path "$repo_root/pixi.toml" --shell bash --locked --quiet)"

if [[ "${OS:-}" == Windows_NT ]]; then
  # Pixi's Windows packages expose tools and DLLs from Library/bin. This bridge
  # runs under Git Bash, so keep PATH in POSIX form for shell command lookup.
  conda_prefix_unix="$(cygpath -u "$CONDA_PREFIX")"
  export PATH="$conda_prefix_unix/Library/bin:$conda_prefix_unix/Scripts:$conda_prefix_unix/bin:$original_path"

  # GitHub Actions and Windows native processes use the canonical mixed-case
  # Path variable. Keep it in Windows form so it survives the handoff from
  # mise-action to later pwsh steps, which then launch Git Bash for mise tasks.
  conda_prefix_windows="$(cygpath -w "$CONDA_PREFIX")"
  path_windows_tail="${Path:-$(cygpath -w -p "$original_path")}"
  export Path="$conda_prefix_windows\\Library\\bin;$conda_prefix_windows\\Scripts;$conda_prefix_windows\\bin;$path_windows_tail"
fi
