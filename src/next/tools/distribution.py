#!/usr/bin/env python3
"""Writes the Swift package of a maplibre-native-next release.

https://github.com/louwers/maplibre-native-next-ffi-distribution holds it, as
https://github.com/maplibre/maplibre-native-ffi-distribution holds the C++
implementation's: the C API from the release's XCFramework of
maplibre-native-next, which `mise run package-xcframework --implementation
maplibre-native-next` combines, and the Swift binding over it. The products
keep their names, MaplibreNativeC and MaplibreNativeFFI, so an app can take
either package.

Usage (from the repository root):

    python3 -I src/next/tools/distribution.py <tree> <xcframework zip> --tag <release>

<release> is the tag of the louwers/maplibre-native-ffi release that holds the
zip.
"""

import argparse
import hashlib
import pathlib
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/release"))

import swift_package

RELEASE_DOWNLOAD_URL = (
    "https://github.com/louwers/maplibre-native-ffi/releases/download"
)
README = """\
# MapLibre Native FFI on maplibre-native-next

The Swift package of [MapLibre Native FFI](https://github.com/maplibre/maplibre-native-ffi)'s
C API on [maplibre-native-next](https://github.com/louwers/maplibre-native-next),
a port of MapLibre Native's core to Rust. It mirrors
[maplibre-native-ffi-distribution](https://github.com/maplibre/maplibre-native-ffi-distribution)'s
package, with the same products: `MaplibreNativeC`, the C API, and
`MaplibreNativeFFI`, the Swift binding over it.

```swift
.package(url: "https://github.com/louwers/maplibre-native-next-ffi-distribution", exact: "{version}")
```

On a Mac, `MaplibreNativeC` is an XCFramework for macOS, iOS, Mac Catalyst,
tvOS and visionOS, from a release of
[louwers/maplibre-native-ffi](https://github.com/louwers/maplibre-native-ffi/releases)
(branch `implementation-switch`). maplibre-native-next implements part of the C
API so far: the runtime and its events, logging, delivery queues, maps with
their style and camera, and offscreen render sessions with texture readback,
which render through wgpu on Metal. The rest, such as Metal texture and surface
sessions, returns `MLN_STATUS_UNSUPPORTED`.

Each release replaces this repository's contents, so report issues to
[louwers/maplibre-native-ffi](https://github.com/louwers/maplibre-native-ffi).
"""


class Release:
    """The release that holds the XCFramework, as `swift_package` reads it."""

    local = False

    def __init__(self, tag: str, xcframework: pathlib.Path):
        self.tag = tag
        self.xcframework = xcframework

    def url(self, name: str) -> str:
        return f"{RELEASE_DOWNLOAD_URL}/{self.tag}/{self.xcframework.name}"

    def sha256(self, name: str) -> str:
        return hashlib.sha256(self.xcframework.read_bytes()).hexdigest()


def replace(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise SystemExit(f"Package.swift has no single {old!r}")
    return text.replace(old, new)


def write(tree: pathlib.Path, release: Release, version: str) -> None:
    """Replaces the package in `tree`, keeping its Git history."""
    tree.mkdir(parents=True, exist_ok=True)
    for entry in tree.iterdir():
        if entry.name == ".git":
            continue
        if entry.is_dir() and not entry.is_symlink():
            shutil.rmtree(entry)
        else:
            entry.unlink()
    (tree / "README.md").write_text(README.format(version=version), encoding="utf-8")
    shutil.copyfile(ROOT / "LICENSE", tree / "LICENSE")
    swift_package.write(tree, release, ROOT)
    manifest = (tree / "Package.swift").read_text(encoding="utf-8")
    manifest = replace(
        manifest,
        "// Each release of https://github.com/maplibre/maplibre-native-ffi generates\n"
        "// this package and replaces this repository's contents with it.",
        "// Each maplibre-native-next release of\n"
        "// https://github.com/louwers/maplibre-native-ffi generates this package and\n"
        "// replaces this repository's contents with it.",
    )
    manifest = replace(
        manifest, 'name: "maplibre-native-ffi",', 'name: "maplibre-native-next-ffi",'
    )
    (tree / "Package.swift").write_text(manifest, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("tree", type=pathlib.Path, help="The directory to write")
    parser.add_argument("xcframework", type=pathlib.Path, help="The XCFramework zip")
    parser.add_argument("--tag", required=True, help="The release that holds it")
    parser.add_argument(
        "--version", required=True, help="The package version, which its tag names"
    )
    args = parser.parse_args()
    write(args.tree, Release(args.tag, args.xcframework), args.version)
    return 0


if __name__ == "__main__":
    sys.exit(main())
