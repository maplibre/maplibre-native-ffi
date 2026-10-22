#!/usr/bin/env python3
"""Prepares the Dart package for publication from a release's SHA256SUMS."""

from __future__ import annotations

import argparse
import json
import pathlib
import re

from native_package_release import ARCHIVE_PREFIX, ARCHIVE_SUFFIX
from release_version import parse_tag

PIN_PATH = pathlib.Path("hook/native_release.json")
RELEASES_URL = "https://github.com/maplibre/maplibre-native-ffi/releases/tag"


def archive_digests(checksums: str) -> dict[str, str]:
    """The SHA-256 digest of each native archive that SHA256SUMS lists."""
    digests = {}
    for line in checksums.splitlines():
        digest, _, name = line.partition("  ")
        if name.startswith(ARCHIVE_PREFIX) and name.endswith(ARCHIVE_SUFFIX):
            digests[name] = digest
    if not digests:
        raise SystemExit("SHA256SUMS lists no native archives")
    return dict(sorted(digests.items()))


def prepare(package: pathlib.Path, tag: str, checksums: str) -> None:
    _, version = parse_tag(tag)

    pin = {"tag": tag, "sha256": archive_digests(checksums)}
    (package / PIN_PATH).write_text(json.dumps(pin, indent=2) + "\n", encoding="utf-8")

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
    parser.add_argument("checksums", type=pathlib.Path)
    args = parser.parse_args(arguments)
    prepare(args.package, args.tag, args.checksums.read_text())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
