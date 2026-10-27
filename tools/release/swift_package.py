"""Writes the Swift package into the distribution tree.

The package's manifest takes its platforms and the binding's target from the
repository's root Package.swift, and adds the C library as the `MaplibreNativeC`
library product. On a Mac, the C library is the release's XCFramework. On
Linux, it is a system library over an installed prefix that pkg-config finds,
as the root package's is.
"""

from __future__ import annotations

import pathlib
import re
import shutil
from typing import TYPE_CHECKING

from native_package_release import XCFRAMEWORK_NAME

if TYPE_CHECKING:
    from distribution import Release

MANIFEST = "Package.swift"
SOURCES = "bindings/swift/Sources"
# The root manifest's platforms, and the binding's target, whose path the
# package's own layout replaces.
PLATFORMS = r"^  platforms: \[\n.*?^  \],$"
BINDING_TARGET = r'^  \.target\(\n    name: "MaplibreNativeFFI",\n.*?^  \),$'
BINDING_PATH = f'path: "{SOURCES}/MaplibreNativeFFI"'
# The root package's shims include the repository's headers by relative path,
# and the package's include the installed ones.
SHIM_INCLUDE = re.compile(r'^#include "(?:\.\./)+include/([^"]+)"$', re.MULTILINE)
SPI_MANIFEST = """\
version: 1
external_links:
  documentation: https://maplibre.org/maplibre-native-ffi/reference/swift/documentation/maplibrenativeffi/
"""


def block(manifest: str, pattern: str) -> str:
    match = re.search(pattern, manifest, re.MULTILINE | re.DOTALL)
    if match is None:
        raise SystemExit(f"{MANIFEST} has no match of {pattern!r}")
    return match[0]


def binary_target(tree: pathlib.Path, release: Release) -> str:
    """The binary target's location: the release's zip, or a copy in the tree.

    SwiftPM's checksum of a zip is its SHA-256 digest, which SHA256SUMS lists.
    """
    if release.local:
        shutil.copyfile(release.assets / XCFRAMEWORK_NAME, tree / XCFRAMEWORK_NAME)
        return f'    path: "{XCFRAMEWORK_NAME}"'
    return (
        f'    url: "{release.url(XCFRAMEWORK_NAME)}",\n'
        f'    checksum: "{release.sha256(XCFRAMEWORK_NAME)}"'
    )


def manifest(root_manifest: str, location: str) -> str:
    tools_version = root_manifest.partition("\n")[0]
    if not tools_version.startswith("// swift-tools-version:"):
        raise SystemExit(f"{MANIFEST} does not start with its tools version")
    binding = block(root_manifest, BINDING_TARGET)
    if binding.count(BINDING_PATH) != 1:
        raise SystemExit(
            f"the MaplibreNativeFFI target in {MANIFEST} has no {BINDING_PATH}"
        )
    binding = binding.replace(BINDING_PATH, 'path: "Sources/MaplibreNativeFFI"')
    return f"""\
{tools_version}

// Each release of https://github.com/maplibre/maplibre-native-ffi generates
// this package and replaces this repository's contents with it.

import PackageDescription

// SwiftPM evaluates the manifest on the build host, so a Mac takes the
// XCFramework for every Apple platform. Linux builds against an installed C
// library that pkg-config finds.
#if os(macOS)
  let native: Target = .binaryTarget(
    name: "MaplibreNativeC",
{location}
  )
#else
  let native: Target = .systemLibrary(
    name: "MaplibreNativeC",
    path: "Sources/MaplibreNativeC",
    pkgConfig: "maplibre-native-c"
  )
#endif

let targets: [Target] = [
  native,
{binding}
]

let package = Package(
  name: "maplibre-native-ffi",
{block(root_manifest, PLATFORMS)}
  products: [
    .library(name: "MaplibreNativeFFI", targets: ["MaplibreNativeFFI"]),
    .library(name: "MaplibreNativeC", targets: ["MaplibreNativeC"]),
  ],
  targets: targets
)
"""


def write(tree: pathlib.Path, release: Release, repository: pathlib.Path) -> None:
    root_manifest = (repository / MANIFEST).read_text(encoding="utf-8")
    (tree / MANIFEST).write_text(
        manifest(root_manifest, binary_target(tree, release)), encoding="utf-8"
    )
    (tree / ".spi.yml").write_text(SPI_MANIFEST, encoding="utf-8")
    shutil.copytree(
        repository / SOURCES / "MaplibreNativeFFI", tree / "Sources/MaplibreNativeFFI"
    )

    system_library = repository / SOURCES / "MaplibreNativeC"
    destination = tree / "Sources/MaplibreNativeC"
    (destination / "include").mkdir(parents=True)
    shutil.copyfile(
        system_library / "module.modulemap", destination / "module.modulemap"
    )
    for shim in sorted((system_library / "include").iterdir()):
        text, found = SHIM_INCLUDE.subn(r"#include <\1>", shim.read_text("utf-8"))
        if found != 1:
            raise SystemExit(
                f"{shim} includes {found} repository headers; expected one"
            )
        (destination / "include" / shim.name).write_text(text, encoding="utf-8")
