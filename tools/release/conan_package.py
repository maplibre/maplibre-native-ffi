"""Write the Conan local recipe index, retaining earlier releases' sources."""

from __future__ import annotations

import json
import pathlib
import shutil
from typing import TYPE_CHECKING

from native_package_release import ARCHIVE_PREFIX, ARCHIVE_SUFFIX
from release_version import parse_tag

if TYPE_CHECKING:
    from distribution import Release

RECIPE = pathlib.Path("recipes/maplibre-native-c")


def write(tree: pathlib.Path, release: Release, repository: pathlib.Path) -> None:
    checksums = release.assets / "SHA256SUMS"
    # Swift's local package check supplies only the XCFramework.
    if release.local and not checksums.exists():
        return
    sources = {}
    for line in checksums.read_text("utf-8").splitlines():
        digest, _, name = line.partition("  ")
        if not (name.startswith(ARCHIVE_PREFIX) and name.endswith(ARCHIVE_SUFFIX)):
            continue
        key = name.removeprefix(ARCHIVE_PREFIX).removesuffix(ARCHIVE_SUFFIX)
        sources[key] = {
            "url": (release.assets / name).resolve().as_uri()
            if release.local
            else release.url(name),
            "sha256": digest,
        }
    if release.local and not sources:
        return
    recipe = tree / RECIPE
    recipe.mkdir(parents=True, exist_ok=True)
    data_path = recipe / "conandata.yml"
    data = (
        json.loads(data_path.read_text("utf-8"))
        if data_path.exists()
        else {"sources": {}}
    )
    version = "0.0.0" if release.local else parse_tag(release.tag)[1]
    if sources:
        data["sources"][version] = sources
    # JSON is also YAML, and needs no additional dependency in the release job.
    data_path.write_text(
        json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    versions = {version: {"folder": "."} for version in data["sources"]}
    (recipe / "config.yml").write_text(
        json.dumps({"versions": versions}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    shutil.copyfile(
        repository / "tools/release/conan/conanfile.py", recipe / "conanfile.py"
    )
