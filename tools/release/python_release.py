#!/usr/bin/env python3
"""License runtime builds and version Python packages for publication.

Runtime wheels carry their native notices at build time. Release and snapshot
packing changes their distribution metadata while preserving the tested binary.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import zipfile

from release_version import parse_version

BACKENDS = ("opengl", "metal", "vulkan")
DESCRIPTOR_PATH = pathlib.Path("share/maplibre-native-c/artifact.json")
LICENSE = pathlib.Path(__file__).resolve().parents[2] / "LICENSE"
DEV_SUFFIX = re.compile(r"\.dev[0-9]+\Z")


def substitute(path: pathlib.Path, pattern: str, replacement: str) -> None:
    text, found = re.subn(pattern, replacement, path.read_text(), flags=re.MULTILINE)
    if found != 1:
        raise SystemExit(f"{path} has {found} matches of {pattern!r}; expected 1")
    path.write_text(text, encoding="utf-8")


def prepare(package: pathlib.Path, version: str) -> None:
    # A snapshot builds toward the next release, as <release>.devSTAMP.
    parse_version(DEV_SUFFIX.sub("", version))
    substitute(
        package / "pyproject.toml", r'^version = "0\.0\.0"$', f'version = "{version}"'
    )
    shutil.copy2(LICENSE, package / "LICENSE")


def license_runtime(package: pathlib.Path, prefix: pathlib.Path) -> None:
    descriptor = json.loads((prefix / DESCRIPTOR_PATH).read_text())
    backend = descriptor["backend"]
    if backend not in BACKENDS:
        raise SystemExit(f"no Python runtime serves the {backend} backend")
    # The wheel embeds the native library, so it carries the notices of the
    # third-party code in it.
    notices = next(prefix.glob("share/*/licenses"), None)
    if notices is None:
        raise SystemExit(f"{prefix} carries no licenses")
    licenses = package / "runtimes" / backend / "licenses"
    shutil.rmtree(licenses, ignore_errors=True)
    shutil.copytree(notices, licenses)
    shutil.copy2(LICENSE, licenses.parent / "LICENSE")


def pack(wheels: pathlib.Path, output: pathlib.Path, version: str) -> None:
    # Target builds need no wheel tooling; only the packaging job repacks wheels.
    from wheel.wheelfile import WheelFile

    parse_version(DEV_SUFFIX.sub("", version))
    inputs = sorted(wheels.rglob("*.whl"))
    if not inputs:
        raise SystemExit(f"no runtime wheels in {wheels}")
    output.mkdir(parents=True, exist_ok=True)
    for wheel in inputs:
        distribution, old_version, tags = wheel.name.split("-", 2)
        old_info = f"{distribution}-{old_version}.dist-info/"
        new_info = f"{distribution}-{version}.dist-info/"
        destination = output / f"{distribution}-{version}-{tags}"
        with zipfile.ZipFile(wheel) as source, WheelFile(destination, "w") as target:
            for entry in source.infolist():
                if entry.filename == old_info + "RECORD":
                    continue
                data = source.read(entry)
                if entry.filename == old_info + "METADATA":
                    for field in ("Version: ", "Requires-Dist: maplibre-native-ffi=="):
                        old = f"{field}{old_version}\n".encode()
                        if old not in data:
                            raise SystemExit(f"{wheel} has no matching {field.strip()}")
                        data = data.replace(old, f"{field}{version}\n".encode(), 1)
                entry.filename = entry.filename.replace(old_info, new_info, 1)
                # WheelFile regenerates RECORD, including each file's hash and size.
                target.writestr(entry, data)


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    licenses_parser = commands.add_parser("licenses")
    licenses_parser.add_argument("package", type=pathlib.Path)
    licenses_parser.add_argument("prefix", type=pathlib.Path)
    pack_parser = commands.add_parser("pack")
    pack_parser.add_argument("package", type=pathlib.Path)
    pack_parser.add_argument("version")
    pack_parser.add_argument("wheels", type=pathlib.Path)
    args = parser.parse_args(arguments)
    if args.command == "licenses":
        license_runtime(args.package, args.prefix)
    else:
        prepare(args.package, args.version)
        pack(args.wheels, args.package / "dist", args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
