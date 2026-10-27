#!/usr/bin/env bash
# Commits the distribution tree for a release to the distribution repository's
# main branch, tags the commit with the release's tag, and pushes both:
#
#   publish-distribution.sh <remote> <tag> <assets>
#
# <assets> holds the release's SHA256SUMS. A rerun finds the tag that an earlier
# attempt pushed and stops, and one that stopped between the two pushes tags
# the commit it pushed. A tag that names another tree fails, since a published
# tag must not move.
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: publish-distribution.sh <remote> <tag> <assets>" >&2
  exit 2
fi
remote=$1
tag=$2
assets=$3
tools="$(cd "$(dirname "$0")" && pwd)"
checkout="$(mktemp -d)"
trap 'rm -rf "$checkout"' EXIT

# The repository is empty before the first release, so clone with no branch.
git clone --quiet --no-checkout "$remote" "$checkout"
git -C "$checkout" fetch --quiet --tags origin
git -C "$checkout" config user.name "github-actions[bot]"
git -C "$checkout" config user.email "41898282+github-actions[bot]@users.noreply.github.com"
git -C "$checkout" config commit.gpgSign false
git -C "$checkout" config tag.gpgSign false
if git -C "$checkout" rev-parse --verify --quiet refs/remotes/origin/main >/dev/null; then
  git -C "$checkout" checkout --quiet -B main refs/remotes/origin/main
else
  git -C "$checkout" checkout --quiet --orphan main
fi

python3 "$tools/distribution.py" "$checkout" "$assets" --tag "$tag"
git -C "$checkout" add --all
tree="$(git -C "$checkout" write-tree)"

if existing="$(git -C "$checkout" rev-parse --verify --quiet "refs/tags/$tag^{commit}")"; then
  if [[ "$(git -C "$checkout" rev-parse "$existing^{tree}")" != "$tree" ]]; then
    echo "$tag already names another tree in $remote" >&2
    exit 1
  fi
  echo "$tag is already published"
  exit 0
fi

if git -C "$checkout" rev-parse --verify --quiet HEAD >/dev/null &&
  [[ "$(git -C "$checkout" rev-parse 'HEAD^{tree}')" == "$tree" ]]; then
  # An earlier attempt pushed this commit and stopped before its tag.
  commit="$(git -C "$checkout" rev-parse HEAD)"
else
  git -C "$checkout" commit --quiet -m "MapLibre Native FFI ${tag#v}"
  commit="$(git -C "$checkout" rev-parse HEAD)"
  git -C "$checkout" push --quiet origin "$commit:refs/heads/main"
fi
git -C "$checkout" tag --annotate --message "MapLibre Native FFI ${tag#v}" "$tag" "$commit"
git -C "$checkout" push --quiet origin "refs/tags/$tag"
echo "published $tag as $commit"
