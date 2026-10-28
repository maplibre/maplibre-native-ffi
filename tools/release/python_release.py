#!/usr/bin/env python3
"""Stamps the Python distributions with a version, before their wheels build.

The API distribution in bindings/python holds the Python modules, and each
runtime distribution in bindings/python/runtimes/<backend> holds the extension
for one render backend. Every distribution takes the version, and every runtime
pins the API to it. With a native install prefix, the runtime for the prefix's
backend also takes the license notices of the native library that it embeds.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil

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


def stamp(project: pathlib.Path, version: str, license_files: list[str]) -> None:
    pyproject = project / "pyproject.toml"
    substitute(pyproject, r'^version = "0\.0\.0"$', f'version = "{version}"')
    substitute(
        pyproject,
        r'^(license = "BSD-2-Clause")$',
        r"\1" + f"\nlicense-files = {json.dumps(license_files)}",
    )
    shutil.copy2(LICENSE, project / "LICENSE")


def prepare(
    package: pathlib.Path, version: str, prefix: pathlib.Path | None = None
) -> None:
    # A snapshot builds toward the next release, as <release>.devSTAMP.
    parse_version(DEV_SUFFIX.sub("", version))
    stamp(package, version, ["LICENSE"])
    for backend in BACKENDS:
        runtime = package / "runtimes" / backend
        substitute(
            runtime / "pyproject.toml",
            r'"maplibre-native-ffi==0\.0\.0"',
            f'"maplibre-native-ffi=={version}"',
        )
        stamp(runtime, version, ["LICENSE", "licenses/*"])
    if prefix is None:
        return

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


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("package", type=pathlib.Path)
    parser.add_argument("version")
    parser.add_argument("--prefix", type=pathlib.Path)
    args = parser.parse_args(arguments)
    prepare(args.package, args.version, args.prefix)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
