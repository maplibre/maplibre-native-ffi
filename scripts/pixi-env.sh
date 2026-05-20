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
fi
