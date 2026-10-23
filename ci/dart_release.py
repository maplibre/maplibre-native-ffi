#!/usr/bin/env python3
"""Prepares the Dart package for publication at a release tag."""

from __future__ import annotations

import argparse
import pathlib
import re

from native_package_release import write_pin
from release_version import parse_tag

PIN_PATH = pathlib.Path("hook/native_release.json")
RELEASES_URL = "https://github.com/maplibre/maplibre-native-ffi/releases/tag"


def prepare(
    package: pathlib.Path, tag: str, checksums: pathlib.Path | None = None
) -> None:
    _, version = parse_tag(tag)

    write_pin(package / PIN_PATH, tag, checksums)

    pubspec = package / "pubspec.yaml"
    text, count = re.subn(
        r"^version: .*$", f"version: {version}", pubspec.read_text(), flags=re.MULTILINE
    )
    if count != 1:
        raise SystemExit(f"{pubspec} has {count} version fields; expected one")
    pubspec.write_text(text, encoding="utf-8")

    # pub.dev shows the changelog, and pub warns when it omits the version.
    changelog = package / "CHANGELOG.md"
    title, _, rest = changelog.read_text().partition("\n\n")
    intro, _, entries = rest.partition("\n\n")
    entry = f"## {version}\n\nSee the [release notes]({RELEASES_URL}/{tag}).\n"
    changelog.write_text(
        f"{title}\n\n{intro}\n\n{entry}" + (f"\n{entries}" if entries else ""),
        encoding="utf-8",
    )


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("package", type=pathlib.Path)
    parser.add_argument("tag")
    parser.add_argument(
        "--checksums",
        type=pathlib.Path,
        help="A SHA256SUMS to pin, rather than the release's",
    )
    args = parser.parse_args(arguments)
    prepare(args.package, args.tag, args.checksums)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
