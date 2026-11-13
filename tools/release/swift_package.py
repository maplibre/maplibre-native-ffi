"""Writes the Swift package into the distribution tree.

The package's manifest takes its platforms and the binding's target from the
repository's root Package.swift. The C library is the release's XCFramework on
Apple platforms and its artifact bundle elsewhere, each with every backend of
its platform. A target that links whichever applies, with the system libraries
that it needs, backs the binding and the `MaplibreNativeC` library product.
"""

from __future__ import annotations

import json
import pathlib
import re
import shutil
import tarfile
from typing import TYPE_CHECKING

from native_package_release import ARCHIVE_PREFIX, ARCHIVE_SUFFIX, XCFRAMEWORK_NAME
from swift_artifact_bundle import ARTIFACT, BUNDLE_NAME, TRIPLES

if TYPE_CHECKING:
    from distribution import Release

MANIFEST = "Package.swift"
SOURCES = "bindings/swift/Sources"
# The root manifest's platforms, and the binding's target, whose path and
# dependency the package's own layout replaces.
PLATFORMS = r"^  platforms: \[\n.*?^  \],$"
BINDING_TARGET = r'^  \.target\(\n    name: "MaplibreNativeFFI",\n.*?^  \),$'
BINDING_PATH = f'path: "{SOURCES}/MaplibreNativeFFI"'
BINDING_DEPENDENCIES = 'dependencies: ["MaplibreNativeC"]'
LINKAGE = "MaplibreNativeCLinkage"
APPLE_PLATFORMS = "[.macOS, .iOS, .macCatalyst, .tvOS, .visionOS]"
# The Swift platform of each archive platform that an artifact bundle holds.
SWIFT_PLATFORMS = {"linux-gnu": "linux", "android": "android", "windows": "windows"}
PKG_CONFIG_LIBRARY = re.compile(r"(?:^|\s)-l(\S+)")
OWN_LIBRARIES = {"maplibre-native-c"}
LINKAGE_SOURCE = """\
// This target compiles nothing. It links the C library, and the system
// libraries that the library needs, into whatever depends on it.
"""
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


def swift_platform(platform: str) -> str:
    return SWIFT_PLATFORMS[platform.rpartition("-")[0]]


class Platforms:
    """What each Swift platform's archives say about linking them statically."""

    def __init__(self, release: Release) -> None:
        self.libraries: dict[str, list[str]] = {}
        for archive in sorted(
            release.assets.glob(f"{ARCHIVE_PREFIX}*{ARCHIVE_SUFFIX}")
        ):
            root = archive.name.removesuffix(ARCHIVE_SUFFIX)
            platform = root.removeprefix(ARCHIVE_PREFIX)
            if platform not in TRIPLES:
                continue
            with tarfile.open(archive, "r:gz") as source:
                descriptor = json.load(
                    source.extractfile(f"{root}/share/maplibre-native-c/artifact.json")
                )
                pkg_config = source.extractfile(
                    f"{root}/share/pkgconfig/maplibre-native-c.pc"
                ).read()
            libraries = self.libraries.setdefault(swift_platform(platform), [])
            (libs,) = re.findall(r"^Libs:(.*)$", pkg_config.decode(), re.MULTILINE)
            for library in [
                *descriptor["link"]["libraries"],
                *PKG_CONFIG_LIBRARY.findall(libs),
            ]:
                if library not in OWN_LIBRARIES and library not in libraries:
                    libraries.append(library)

    def names(self) -> str:
        """The Swift platforms, as a manifest's list of them."""
        return f"[{', '.join(f'.{name}' for name in sorted(self.libraries))}]"


def location(tree: pathlib.Path, release: Release, name: str) -> str | None:
    """A binary target's location, or None for one that a local release lacks.

    A release pins its zip by URL and checksum, its SHA-256 digest, which
    SHA256SUMS lists. A local release copies it into the tree.
    """
    if release.local:
        if not (release.assets / name).exists():
            return None
        shutil.copyfile(release.assets / name, tree / name)
        return f'    path: "{name}"'
    return f'    url: "{release.url(name)}",\n    checksum: "{release.sha256(name)}"'


def manifest(
    root_manifest: str, platforms: Platforms, locations: dict[str, str | None]
) -> str:
    tools_version = root_manifest.partition("\n")[0]
    if not tools_version.startswith("// swift-tools-version:"):
        raise SystemExit(f"{MANIFEST} does not start with its tools version")
    binding = block(root_manifest, BINDING_TARGET)
    for old, new in (
        (BINDING_PATH, 'path: "Sources/MaplibreNativeFFI"'),
        (BINDING_DEPENDENCIES, f'dependencies: ["{LINKAGE}"]'),
    ):
        if binding.count(old) != 1:
            raise SystemExit(f"the MaplibreNativeFFI target in {MANIFEST} has no {old}")
        binding = binding.replace(old, new)

    binaries = []
    dependencies = []
    for target, where in locations.items():
        if where is None:
            continue
        binaries.append(f'  .binaryTarget(\n    name: "{target}",\n{where}\n  ),')
        if target == "MaplibreNativeC":
            dependencies.append(
                f'      .target(name: "{target}", '
                f"condition: .when(platforms: {APPLE_PLATFORMS})),"
            )
            continue
        names = platforms.names()
        if names == "[]":
            raise SystemExit(f"no native archive describes {target}")
        dependencies.append(
            f'      .target(\n        name: "{target}",\n'
            f"        condition: .when(platforms: {names})\n      ),"
        )
    if platforms.libraries and locations[ARTIFACT] is None:
        raise SystemExit(
            f"the native archives for Swift's platforms need {BUNDLE_NAME}"
        )
    linker_settings = "\n".join(
        f'      .linkedLibrary("{library}", .when(platforms: [.{name}])),'
        for name, libraries in sorted(platforms.libraries.items())
        for library in libraries
    )
    return f"""\
{tools_version}

// Each release of https://github.com/maplibre/maplibre-native-ffi generates
// this package and replaces this repository's contents with it.

import PackageDescription

// The C library is the XCFramework on Apple platforms, and elsewhere the
// artifact bundle. Each carries every backend of its platform. SwiftPM downloads
// every binary target, whatever platform it builds for.
let targets: [Target] = [
{chr(10).join(binaries)}
  .target(
    name: "{LINKAGE}",
    dependencies: [
{chr(10).join(dependencies)}
    ],
    path: "Sources/{LINKAGE}",
    linkerSettings: [
{linker_settings}
    ]
  ),
{binding}
]

let package = Package(
  name: "maplibre-native-ffi",
{block(root_manifest, PLATFORMS)}
  products: [
    .library(name: "MaplibreNativeFFI", targets: ["MaplibreNativeFFI"]),
    .library(name: "MaplibreNativeC", targets: ["{LINKAGE}"]),
  ],
  targets: targets
)
"""


def write(tree: pathlib.Path, release: Release, repository: pathlib.Path) -> None:
    names = {"MaplibreNativeC": XCFRAMEWORK_NAME, ARTIFACT: BUNDLE_NAME}
    if release.local and not any(
        (release.assets / name).exists() for name in names.values()
    ):
        return
    locations = {
        target: location(tree, release, name) for target, name in names.items()
    }
    root_manifest = (repository / MANIFEST).read_text(encoding="utf-8")
    (tree / MANIFEST).write_text(
        manifest(root_manifest, Platforms(release), locations), encoding="utf-8"
    )
    (tree / ".spi.yml").write_text(SPI_MANIFEST, encoding="utf-8")
    shutil.copytree(
        repository / SOURCES / "MaplibreNativeFFI", tree / "Sources/MaplibreNativeFFI"
    )
    linkage = tree / "Sources" / LINKAGE
    linkage.mkdir(parents=True)
    (linkage / "Linkage.swift").write_text(LINKAGE_SOURCE, encoding="utf-8")
