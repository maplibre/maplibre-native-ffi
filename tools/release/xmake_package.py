"""Writes the xmake repository into the distribution tree."""

from __future__ import annotations

import pathlib
import shutil
from typing import TYPE_CHECKING

from native_package_release import ARCHIVE_PREFIX, ARCHIVE_SUFFIX, sha256

if TYPE_CHECKING:
    from distribution import Release

PACKAGE = pathlib.Path("packages/m/maplibre-native-c/xmake.lua")
TEMPLATE = pathlib.Path(__file__).with_suffix(".lua")


def write(tree: pathlib.Path, release: Release, repository: pathlib.Path) -> None:
    version = release.tag.removeprefix("v") if release.tag else "0.0.0"
    if release.local:
        archives = sorted(release.assets.glob(f"{ARCHIVE_PREFIX}*{ARCHIVE_SUFFIX}"))
        entries = [(archive.name, sha256(archive)) for archive in archives]
    else:
        entries = [
            (name, digest)
            for line in (release.assets / "SHA256SUMS").read_text("utf-8").splitlines()
            for digest, _, name in [line.partition("  ")]
            if name.startswith(ARCHIVE_PREFIX) and name.endswith(ARCHIVE_SUFFIX)
        ]
    urls = []
    for name, digest in entries:
        preset = name.removeprefix(ARCHIVE_PREFIX).removesuffix(ARCHIVE_SUFFIX)
        if release.local:
            destination = tree / "archives" / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(release.assets / name, destination)
            url = destination.resolve().as_posix()
        else:
            url = release.url(name)
        urls.append(
            f'    add_urls("{url}", {{alias = "{preset}"}})\n'
            f'    add_versions("{preset}:{version}", "{digest}")\n'
        )
    destination = tree / PACKAGE
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(
        TEMPLATE.read_text("utf-8").replace("    -- RELEASE_ARCHIVES\n", "".join(urls)),
        encoding="utf-8",
    )
