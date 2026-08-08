#!/usr/bin/env python3

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import os
import pathlib
import re
import tarfile
import tempfile
from collections.abc import Iterable

from release_version import parse_version

ARCHIVE_PREFIX = "maplibre-native-c-"
ARCHIVE_SUFFIX = ".tar.gz"
RELEASE_NOTES_NAME = "RELEASE_NOTES.md"


def required_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise SystemExit(f"{name} is required")
    return value


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def package_presets(repository: pathlib.Path) -> list[str]:
    presets = json.loads(
        (repository / "CMakePresets.json").read_text(encoding="utf-8")
    ).get("packagePresets", [])
    names = sorted(preset["name"] for preset in presets)
    if not names:
        raise SystemExit("CMakePresets.json defines no native package presets")
    if len(names) != len(set(names)):
        raise SystemExit("CMakePresets.json defines duplicate native package presets")
    return names


def preset_from_archive(path: pathlib.Path) -> str:
    if not path.name.startswith(ARCHIVE_PREFIX) or not path.name.endswith(
        ARCHIVE_SUFFIX
    ):
        raise SystemExit(f"invalid native package archive name: {path.name}")
    return path.name[len(ARCHIVE_PREFIX) : -len(ARCHIVE_SUFFIX)]


def native_archives(
    release_dir: pathlib.Path, presets: Iterable[str]
) -> list[pathlib.Path]:
    expected = {
        f"{ARCHIVE_PREFIX}{preset}{ARCHIVE_SUFFIX}" for preset in sorted(presets)
    }
    actual = {path.name for path in release_dir.glob(f"*{ARCHIVE_SUFFIX}")}
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    if missing or unexpected:
        details = []
        if missing:
            details.append(f"missing {', '.join(missing)}")
        if unexpected:
            details.append(f"unexpected {', '.join(unexpected)}")
        raise SystemExit(f"invalid native package set: {'; '.join(details)}")
    return [release_dir / name for name in sorted(expected)]


def safe_member_name(member: tarfile.TarInfo, root: str) -> None:
    path = pathlib.PurePosixPath(member.name)
    if path.is_absolute() or ".." in path.parts:
        raise SystemExit(f"native package contains an unsafe path: {member.name}")
    if not path.parts or path.parts[0] != root:
        raise SystemExit(
            f"native package member {member.name} is outside expected root {root}"
        )
    if member.issym() or member.islnk():
        target = pathlib.PurePosixPath(member.linkname)
        if target.is_absolute() or ".." in target.parts:
            raise SystemExit(
                f"native package contains an unsafe link: "
                f"{member.name} -> {member.linkname}"
            )


def descriptor_bytes(
    content: bytes, archive: pathlib.Path, version: str, expected_sha: str
) -> bytes:
    try:
        descriptor = json.loads(content)
    except json.JSONDecodeError as error:
        raise SystemExit(
            f"{archive.name} has an invalid artifact.json: {error}"
        ) from error
    actual_sha = descriptor.get("gitSha")
    if actual_sha != expected_sha:
        raise SystemExit(
            f"{archive.name} records gitSha {actual_sha!r}; expected {expected_sha}"
        )
    relabeled = {}
    for key, value in descriptor.items():
        if key == "version":
            continue
        relabeled[key] = value
        if key == "gitSha":
            relabeled["version"] = version
    return (json.dumps(relabeled, indent=2) + "\n").encode()


def pkg_config_bytes(content: bytes, archive: pathlib.Path, version: str) -> bytes:
    text = content.decode()
    matches = list(re.finditer(r"^Version: .+$", text, re.MULTILINE))
    if len(matches) != 1:
        raise SystemExit(
            f"{archive.name} contains {len(matches)} pkg-config Version fields; expected one"
        )
    return re.sub(
        r"^Version: .+$", f"Version: {version}", text, count=1, flags=re.MULTILINE
    ).encode()


def rewrite_archive(archive: pathlib.Path, *, expected_sha: str, version: str) -> None:
    preset = preset_from_archive(archive)
    root = f"{ARCHIVE_PREFIX}{preset}"
    descriptor_name = f"{root}/share/maplibre-native-c/artifact.json"
    pkg_config_name = f"{root}/share/pkgconfig/maplibre-native-c.pc"
    found_descriptor = False
    found_pkg_config = False

    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{archive.name}.", dir=archive.parent
    )
    temporary_path = pathlib.Path(temporary_name)
    try:
        with (
            os.fdopen(descriptor, "wb") as temporary,
            tarfile.open(archive, "r:gz") as source,
            gzip.GzipFile(
                filename="", mode="wb", fileobj=temporary, mtime=0
            ) as compressed,
            tarfile.open(
                fileobj=compressed, mode="w", format=tarfile.PAX_FORMAT
            ) as output,
        ):
            for member in source:
                safe_member_name(member, root)
                if not member.isfile():
                    output.addfile(member)
                    continue

                source_file = source.extractfile(member)
                if source_file is None:
                    raise SystemExit(f"cannot read {member.name} from {archive.name}")
                if member.name == descriptor_name:
                    content = descriptor_bytes(
                        source_file.read(), archive, version, expected_sha
                    )
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
                    found_descriptor = True
                elif member.name == pkg_config_name:
                    content = pkg_config_bytes(source_file.read(), archive, version)
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
                    found_pkg_config = True
                else:
                    output.addfile(member, source_file)
        if not found_descriptor:
            raise SystemExit(f"{archive.name} is missing artifact.json")
        if not found_pkg_config:
            raise SystemExit(f"{archive.name} is missing maplibre-native-c.pc")
        os.replace(temporary_path, archive)
    finally:
        temporary_path.unlink(missing_ok=True)


def write_checksums(release_dir: pathlib.Path, files: list[pathlib.Path]) -> None:
    lines = [f"{sha256(path)}  {path.name}" for path in files]
    (release_dir / "SHA256SUMS").write_text("\n".join(lines) + "\n", encoding="utf-8")


def workflow_url() -> str:
    server_url = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    repository = required_env("GITHUB_REPOSITORY")
    return f"{server_url}/{repository}/actions/runs/{required_env('GITHUB_RUN_ID')}"


def commit_url(sha: str) -> str:
    server_url = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    repository = required_env("GITHUB_REPOSITORY")
    return f"{server_url}/{repository}/commit/{sha}"


def write_notes(
    release_dir: pathlib.Path, channel: str, sha: str, version: str
) -> None:
    title = (
        "Unstable native package snapshot."
        if channel == "snapshot"
        else f"Native package {version}."
    )
    notes = "\n".join(
        [
            title,
            "",
            f"Commit: {commit_url(sha)}",
            f"CI run: {required_env('CI_RUN_URL')}",
            f"Publishing run: {workflow_url()}",
            "",
        ]
    )
    (release_dir / RELEASE_NOTES_NAME).write_text(notes, encoding="utf-8")


def prepare_assets(
    release_dir: pathlib.Path,
    channel: str,
    version: str,
    repository: pathlib.Path | None = None,
) -> None:
    if channel == "release":
        parse_version(version)
    root = repository or pathlib.Path(required_env("MISE_MONOREPO_ROOT"))
    sha = required_env("PUBLISH_SHA")
    archives = native_archives(release_dir, package_presets(root))
    for archive in archives:
        rewrite_archive(archive, expected_sha=sha, version=version)
    write_notes(release_dir, channel, sha, version)
    write_checksums(release_dir, archives)


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("channel", choices=("snapshot", "release"))
    parser.add_argument("release_dir", type=pathlib.Path)
    parser.add_argument("version")

    args = parser.parse_args(arguments)
    prepare_assets(args.release_dir, args.channel, args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
