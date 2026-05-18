#!/usr/bin/env bash
set -eo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"

# Export Pixi's activated environment into mise. --locked keeps activation tied
# to pixi.lock, and --shell bash matches mise's env._.source execution model.
eval "$(pixi shell-hook --manifest-path "$repo_root/pixi.toml" --shell bash --locked --quiet)"

case "$(uname -s)" in
  Linux*)
    pixi_library_dir="$($repo_root/scripts/pixi-library-dir)"
    pixi_rpath_flag="-C link-arg=-Wl,-rpath,$pixi_library_dir"
    if [[ -n "${RUSTFLAGS:-}" ]]; then
      export RUSTFLAGS="$RUSTFLAGS $pixi_rpath_flag"
    else
      export RUSTFLAGS="$pixi_rpath_flag"
    fi
    ;;
esac
