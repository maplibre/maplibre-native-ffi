#!/usr/bin/env python3
"""Assemble MapLibreNativeC.xcframework.zip from the Apple Metal native packages.

Swift Package Manager takes the zip as a binary target. Its checksum is the
zip's SHA-256, which SHA256SUMS lists with the other release assets.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import plistlib
import re
import shutil
import subprocess
import tarfile
import tempfile
from collections.abc import Callable, Sequence

FRAMEWORK_NAME = "MapLibreNativeC"
XCFRAMEWORK_ZIP = f"{FRAMEWORK_NAME}.xcframework.zip"

# The Info.plist values for each preset in the XCFramework: the platform and
# the key that states the minimum OS version.
PLATFORMS = {
    "ios-arm64-metal": ("iPhoneOS", "MinimumOSVersion"),
    "ios-simulator-arm64-metal": ("iPhoneSimulator", "MinimumOSVersion"),
    "macos-arm64-metal": ("MacOSX", "LSMinimumSystemVersion"),
}

# maplibre_native_c.h doesn't include callback_adapter.h or plugin.h. The module
# map declares callback_adapter.h as a submodule and excludes plugin.h and
# mln/plugin/plugin_api.h, so that clang doesn't warn that the umbrella header
# is incomplete. plugin.h can't be part of the module, because its
# #include <mln/plugin/plugin_api.h> doesn't resolve inside a framework.
MODULE_MAP = f"""framework module {FRAMEWORK_NAME} {{
  umbrella header "maplibre_native_c.h"
  export *

  explicit module CallbackAdapter {{
    header "maplibre_native_c/callback_adapter.h"
    export *
  }}

  exclude header "maplibre_native_c/plugin.h"
  exclude header "mln/plugin/plugin_api.h"
}}
"""

Runner = Callable[[Sequence[str]], None]


def required_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise SystemExit(f"{name} is required")
    return value


def run(command: Sequence[str]) -> None:
    subprocess.run(command, check=True)


def cache_variable(presets: dict[str, dict], name: str, variable: str) -> str | None:
    preset = presets[name]
    value = preset.get("cacheVariables", {}).get(variable)
    if isinstance(value, dict):
        value = value.get("value")
    if value is not None:
        return value
    inherits = preset.get("inherits", [])
    for parent in [inherits] if isinstance(inherits, str) else inherits:
        value = cache_variable(presets, parent, variable)
        if value is not None:
            return value
    return None


def deployment_target(repository: pathlib.Path, preset: str) -> str:
    document = json.loads(
        (repository / "CMakePresets.json").read_text(encoding="utf-8")
    )
    configure = {entry["name"]: entry for entry in document["configurePresets"]}
    package = {entry["name"]: entry for entry in document.get("packagePresets", [])}
    if preset not in package:
        raise SystemExit(f"CMakePresets.json has no package preset {preset}")
    name = package[preset].get("configurePreset", preset)
    target = cache_variable(configure, name, "CMAKE_OSX_DEPLOYMENT_TARGET")
    if target is None:
        raise SystemExit(f"{name} sets no CMAKE_OSX_DEPLOYMENT_TARGET")
    return target


def bundle_version(version: str) -> str:
    # An Apple bundle version is one to three period-separated integers, so a
    # snapshot version such as 0.0.0-dev.202608070101 becomes 0.0.0.
    match = re.match(r"\d+(\.\d+){0,2}", version)
    if match is None:
        raise SystemExit(f"{version} doesn't start with a bundle version")
    return match[0]


def extract(archive: pathlib.Path, destination: pathlib.Path, sha: str) -> pathlib.Path:
    with tarfile.open(archive, "r:gz") as package:
        package.extractall(destination, filter="data")
    prefix = destination / archive.name.removesuffix(".tar.gz")
    descriptor = json.loads(
        (prefix / "share/maplibre-native-c/artifact.json").read_text(encoding="utf-8")
    )
    if descriptor.get("gitSha") != sha:
        raise SystemExit(
            f"{archive.name} records gitSha {descriptor.get('gitSha')!r}; expected {sha}"
        )
    return prefix


def info_plist(preset: str, version: str, minimum: str) -> bytes:
    platform, minimum_key = PLATFORMS[preset]
    return plistlib.dumps(
        {
            "CFBundleExecutable": FRAMEWORK_NAME,
            "CFBundleIdentifier": f"org.maplibre.{FRAMEWORK_NAME}",
            "CFBundleInfoDictionaryVersion": "6.0",
            "CFBundleName": FRAMEWORK_NAME,
            "CFBundlePackageType": "FMWK",
            "CFBundleShortVersionString": version,
            "CFBundleSupportedPlatforms": [platform],
            "CFBundleVersion": version,
            minimum_key: minimum,
        }
    )


def build_framework(
    prefix: pathlib.Path,
    preset: str,
    stage: pathlib.Path,
    version: str,
    minimum: str,
    runner: Runner,
) -> pathlib.Path:
    """Wraps the prefix's dylib and headers in a framework under stage.

    The macOS framework uses the versioned layout, and the iOS frameworks use
    the flat layout. The framework's resources include the project's LICENSE
    and the third-party notices in licenses/, which the binary distribution
    must contain.
    """
    framework = stage / preset / f"{FRAMEWORK_NAME}.framework"
    macos = preset.startswith("macos-")
    contents = framework / "Versions/A" if macos else framework
    (contents / "Modules").mkdir(parents=True)
    binary = contents / FRAMEWORK_NAME
    shutil.copy2(prefix / "lib/libmaplibre-native-c.dylib", binary)
    install_name = binary.relative_to(framework.parent)
    runner(["install_name_tool", "-id", f"@rpath/{install_name}", str(binary)])
    shutil.copytree(prefix / "include", contents / "Headers")
    (contents / "Modules/module.modulemap").write_text(MODULE_MAP, encoding="utf-8")
    resources = contents / "Resources" if macos else contents
    resources.mkdir(exist_ok=True)
    licenses = prefix / "share/maplibre-native-c"
    shutil.copy2(licenses / "LICENSE", resources / "LICENSE")
    shutil.copytree(licenses / "licenses", resources / "licenses")
    plist = info_plist(preset, version, minimum)
    if macos:
        (contents / "Resources/Info.plist").write_bytes(plist)
        (framework / "Versions/Current").symlink_to("A")
        for item in (FRAMEWORK_NAME, "Headers", "Modules", "Resources"):
            (framework / item).symlink_to(f"Versions/Current/{item}")
    else:
        (contents / "Info.plist").write_bytes(plist)
    return framework


def assemble(
    input_dir: pathlib.Path,
    output_dir: pathlib.Path,
    version: str,
    *,
    sha: str,
    repository: pathlib.Path,
    runner: Runner = run,
) -> pathlib.Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    output = output_dir / XCFRAMEWORK_ZIP
    with tempfile.TemporaryDirectory() as temporary:
        work = pathlib.Path(temporary)
        arguments = []
        for preset in sorted(PLATFORMS):
            archive = input_dir / f"maplibre-native-c-{preset}.tar.gz"
            if not archive.is_file():
                raise SystemExit(f"missing {archive.name}")
            prefix = extract(archive, work / "packages", sha)
            framework = build_framework(
                prefix,
                preset,
                work / "frameworks",
                bundle_version(version),
                deployment_target(repository, preset),
                runner,
            )
            arguments += ["-framework", str(framework)]
        xcframework = work / f"{FRAMEWORK_NAME}.xcframework"
        runner(
            [
                "xcodebuild",
                "-create-xcframework",
                *arguments,
                "-output",
                str(xcframework),
            ]
        )
        output.unlink(missing_ok=True)
        # --keepParent puts the XCFramework at the zip's top level, and ditto
        # keeps the macOS framework's symlinks. --norsrc leaves out the
        # AppleDouble files for extended attributes, which codesign rejects as
        # unsealed contents of a framework.
        runner(
            [
                "ditto",
                "-c",
                "-k",
                "--norsrc",
                "--keepParent",
                str(xcframework),
                str(output),
            ]
        )
    return output


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_dir", type=pathlib.Path)
    parser.add_argument("output_dir", type=pathlib.Path)
    parser.add_argument("version")

    args = parser.parse_args(arguments)
    assemble(
        args.input_dir,
        args.output_dir,
        args.version,
        sha=required_env("PUBLISH_SHA"),
        repository=pathlib.Path(required_env("MISE_MONOREPO_ROOT")),
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
