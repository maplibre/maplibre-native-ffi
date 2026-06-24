#!/usr/bin/env bash
set -euo pipefail

branch="${1:-cursor/webgpu-emdawn-minimal-bcb7}"
remote="${2:-sargunv}"

cd "$(git rev-parse --show-toplevel)/third_party/maplibre-native"

if ! git remote get-url "$remote" >/dev/null 2>&1; then
  git remote add "$remote" "https://github.com/sargunv/maplibre-native.git"
fi

current_branch="$(git branch --show-current)"
if [[ "$current_branch" != "$branch" ]]; then
  git checkout "$branch"
fi

git push -u "$remote" "$branch"

echo "Pushed ${branch} to ${remote}/maplibre-native"
echo "Open a PR against maplibre/maplibre-native from:"
echo "  https://github.com/sargunv/maplibre-native/compare/${branch}?expand=1"
