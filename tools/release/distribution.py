#!/usr/bin/env python3
"""Writes the tree of the distribution repository for a release.

Some package managers read a package from a Git repository,
with the native library's URL and checksum in a manifest that only exists after
the release builds it. Each release updates
https://github.com/maplibre/maplibre-native-ffi-distribution with this tree, in
one commit that it tags with the release's tag. The registry retains its
version history. Each package manager's module
writes its files into the tree from `WRITERS`.
"""

from __future__ import annotations

import argparse
import dataclasses
import pathlib
import shutil

import swift_package
import vcpkg_registry
from native_package_release import RELEASE_DOWNLOAD_URL

ROOT = pathlib.Path(__file__).resolve().parents[2]
CHECKSUMS_NAME = "SHA256SUMS"
WRITERS = (swift_package.write, vcpkg_registry.write)
README = """\
# MapLibre Native FFI distribution

This repository holds the packages of each
[MapLibre Native FFI](https://github.com/maplibre/maplibre-native-ffi) release
for package managers that read a Git repository. Each release updates its
packages in one commit, tagged with the release's version. Report issues and
send changes to MapLibre Native FFI, since the next release overwrites this
repository.

## vcpkg

Register this repository with a release commit as the baseline in
`vcpkg-configuration.json`:

```json
{
  "registries": [{
    "kind": "git",
    "repository": "https://github.com/maplibre/maplibre-native-ffi-distribution",
    "baseline": "<release-commit>",
    "packages": ["maplibre-native-c"]
  }]
}
```

Select exactly one renderer feature in your `vcpkg.json` dependencies, for
example `{"name": "maplibre-native-c", "features": ["opengl"]}`. The other
features are `metal` and `vulkan`; availability depends on the target platform.
The package supplies Release binaries for both Debug and Release consumers.

```cmake
find_package(maplibre-native-c CONFIG REQUIRED)
target_link_libraries(app PRIVATE maplibre-native-c::maplibre-native-c)
```

- [Install](https://maplibre.org/maplibre-native-ffi/install/)
- [Guides](https://maplibre.org/maplibre-native-ffi/guides/create-a-map/)
"""


@dataclasses.dataclass(frozen=True)
class Release:
    """The release whose assets the tree's packages take.

    `assets` holds the release's native archives and SHA256SUMS. Without a
    `tag`, the release is local: packages use copies or file URLs of the native
    assets, so CI can build consumers before any release exists.
    """

    assets: pathlib.Path
    tag: str | None = None

    @property
    def local(self) -> bool:
        return self.tag is None

    def url(self, name: str) -> str:
        if self.tag is None:
            raise SystemExit(f"a local release has no download URL for {name}")
        return f"{RELEASE_DOWNLOAD_URL}/{self.tag}/{name}"

    def sha256(self, name: str) -> str:
        for line in (self.assets / CHECKSUMS_NAME).read_text("utf-8").splitlines():
            digest, _, listed = line.partition("  ")
            if listed == name:
                return digest
        raise SystemExit(f"{CHECKSUMS_NAME} does not list {name}")


def write(
    tree: pathlib.Path, release: Release, repository: pathlib.Path = ROOT
) -> None:
    """Replaces the packages in `tree`, preserving Git and registry version history."""
    if repository.resolve().is_relative_to(tree.resolve()):
        raise SystemExit(f"{tree} holds {repository}; write the tree elsewhere")
    tree.mkdir(parents=True, exist_ok=True)
    for entry in tree.iterdir():
        if entry.name in {".git", "versions"}:
            continue
        if entry.is_dir() and not entry.is_symlink():
            shutil.rmtree(entry)
        else:
            entry.unlink()
    (tree / "README.md").write_text(README, encoding="utf-8")
    shutil.copyfile(repository / "LICENSE", tree / "LICENSE")
    for writer in WRITERS:
        writer(tree, release, repository)


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tree", type=pathlib.Path, help="The directory to write")
    parser.add_argument(
        "assets", type=pathlib.Path, help="The directory that holds the assets"
    )
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--tag", help="The release tag whose assets to download")
    source.add_argument(
        "--local",
        action="store_true",
        help="Copy the native packages into the tree rather than download them",
    )
    args = parser.parse_args(arguments)
    write(args.tree, Release(args.assets, args.tag))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
