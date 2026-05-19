#!/usr/bin/env bash
set -eo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"

# Export Pixi's activated environment into mise. --locked keeps activation tied
# to pixi.lock, and --shell bash matches mise's env._.source execution model.
eval "$(pixi shell-hook --manifest-path "$repo_root/pixi.toml" --shell bash --locked --quiet)"
