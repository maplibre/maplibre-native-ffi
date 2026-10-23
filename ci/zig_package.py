"""Builds the Zig package that a release publishes beside its native archives.

The package holds the files that the repository's build.zig.zon lists in
`.paths`, at the same paths. Its manifest carries the release version and
declares each native archive as a lazy dependency, which build.zig selects by
target and backend.
"""

from __future__ import annotations

import gzip
import io
import pathlib
import re
import subprocess
import tarfile
from collections.abc import Mapping

PACKAGE_NAME = "maplibre-native-ffi-zig.tar.gz"
PACKAGE_ROOT = "maplibre-native-ffi-zig"
MANIFEST = "build.zig.zon"


def zig_hash(archive: pathlib.Path) -> str:
    """The Zig package hash of `archive`, which a dependency on it declares."""
    result = subprocess.run(
        ["zig", "fetch", str(archive)], check=False, capture_output=True, text=True
    )
    if result.returncode != 0:
        raise SystemExit(f"zig fetch {archive.name} failed:\n{result.stderr}")
    return result.stdout.strip()


def substitute(text: str, pattern: str, replacement: str) -> str:
    text, found = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if found != 1:
        raise SystemExit(f"{MANIFEST} has no match of {pattern!r}")
    return text


def package_paths(manifest: str) -> list[str]:
    match = re.search(
        r"^    \.paths = \.\{\n(.*?)^    \},$", manifest, re.MULTILINE | re.DOTALL
    )
    if match is None:
        raise SystemExit(f"{MANIFEST} lists no package paths")
    return re.findall(r'^\s*"([^"]+)",$', match[1], re.MULTILINE)


def published_manifest(
    manifest: str, version: str, archives: Mapping[str, tuple[str, str]]
) -> str:
    manifest = substitute(
        manifest, r'^    \.version = "0\.0\.0",$', f'    .version = "{version}",'
    )
    dependencies = "".join(
        f'        .@"{name}" = .{{\n'
        f'            .url = "{url}",\n'
        f'            .hash = "{digest}",\n'
        "            .lazy = true,\n"
        "        },\n"
        for name, (url, digest) in sorted(archives.items())
    )
    return substitute(
        manifest,
        r"^    \.dependencies = \.\{\n",
        "    .dependencies = .{\n" + dependencies,
    )


def add_file(package: tarfile.TarFile, name: str, content: bytes) -> None:
    member = tarfile.TarInfo(f"{PACKAGE_ROOT}/{name}")
    member.size = len(content)
    member.mode = 0o644
    package.addfile(member, io.BytesIO(content))


def write_package(
    destination: pathlib.Path,
    repository: pathlib.Path,
    version: str,
    archives: Mapping[str, tuple[str, pathlib.Path]],
) -> None:
    """Writes the Zig package to `destination`.

    `archives` maps each native preset to its archive's release URL and local
    copy, which yields the hash that the dependency on it declares.
    """
    manifest = (repository / MANIFEST).read_text(encoding="utf-8")
    files = {}
    for path in package_paths(manifest):
        source = repository / path
        sources = sorted(source.rglob("*")) if source.is_dir() else [source]
        for file in sources:
            if file.is_file():
                files[file.relative_to(repository).as_posix()] = file.read_bytes()
    hashes = {
        name: (url, zig_hash(archive)) for name, (url, archive) in archives.items()
    }
    files[MANIFEST] = published_manifest(manifest, version, hashes).encode()

    with (
        destination.open("wb") as output,
        gzip.GzipFile(filename="", mode="wb", fileobj=output, mtime=0) as compressed,
        tarfile.open(
            fileobj=compressed, mode="w", format=tarfile.PAX_FORMAT
        ) as package,
    ):
        for name in sorted(files):
            add_file(package, name, files[name])
