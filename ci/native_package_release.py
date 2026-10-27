#!/usr/bin/env python3

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import os
import pathlib
import plistlib
import re
import tarfile
import tempfile
import urllib.request
import zipfile
from collections.abc import Iterable

from release_version import parse_version
from zig_package import PACKAGE_NAME as ZIG_PACKAGE_NAME
from zig_package import write_package as write_zig_package

ARCHIVE_PREFIX = "maplibre-native-c-"
ARCHIVE_SUFFIX = ".tar.gz"
ARTIFACTS_NAME = "maplibre-native-c-artifacts.json"
RELEASE_NOTES_NAME = "RELEASE_NOTES.md"
XCFRAMEWORK_NAME = "MaplibreNativeC.xcframework.zip"
RELEASE_DOWNLOAD_URL = (
    "https://github.com/maplibre/maplibre-native-ffi/releases/download"
)
# The Info.plist of a flat or versioned framework, or of its dSYM.
FRAMEWORK_INFO_PLIST = re.compile(
    r"(^|/)MaplibreNativeC\.framework(/Versions/A/Resources|\.dSYM/Contents)?/Info\.plist$"
)


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
    """The presets that a release publishes: every visible configure preset."""
    presets = json.loads(
        (repository / "CMakePresets.json").read_text(encoding="utf-8")
    ).get("configurePresets", [])
    names = sorted(
        preset["name"] for preset in presets if not preset.get("hidden", False)
    )
    if not names:
        raise SystemExit("CMakePresets.json defines no native configure presets")
    return names


def native_archives(
    release_dir: pathlib.Path, presets: Iterable[str]
) -> list[pathlib.Path]:
    expected = {
        f"{ARCHIVE_PREFIX}{preset}{ARCHIVE_SUFFIX}" for preset in sorted(presets)
    }
    actual = {
        path.name for path in release_dir.glob(f"{ARCHIVE_PREFIX}*{ARCHIVE_SUFFIX}")
    }
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


def pin(tag: str, artifacts: list[dict]) -> str:
    """The pin by which a published package downloads its native library.

    The pin holds the release tag and the release's artifact index.
    """
    content = {"tag": tag, "artifacts": artifacts}
    return json.dumps(content, indent=2) + "\n"


def write_pin(
    path: pathlib.Path, tag: str, artifacts: pathlib.Path | None = None
) -> None:
    """Writes the pin of release `tag` to `path`.

    It reads the release's artifact index from `artifacts` if given, and
    otherwise downloads it from the release.
    """
    if artifacts is None:
        url = f"{RELEASE_DOWNLOAD_URL}/{tag}/{ARTIFACTS_NAME}"
        with urllib.request.urlopen(url) as response:
            index = json.loads(response.read())
    else:
        index = json.loads(artifacts.read_text(encoding="utf-8"))
    path.write_text(pin(tag, index), encoding="utf-8")


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


def bundle_version(version: str) -> str:
    """The release version as an Apple bundle version.

    A bundle version is one to three period-separated integers, so a snapshot
    version such as 0.0.0-dev.202608070101 keeps its numeric prefix.
    """
    match = re.match(r"\d+(\.\d+){0,2}", version)
    if match is None:
        raise SystemExit(f"version {version} does not start with a bundle version")
    return match[0]


def framework_plist_bytes(content: bytes, version: str) -> bytes:
    plist = plistlib.loads(content)
    plist["CFBundleShortVersionString"] = bundle_version(version)
    plist["CFBundleVersion"] = bundle_version(version)
    return plistlib.dumps(plist)


def descriptor_bytes(content: bytes, version: str, sha: str) -> bytes:
    """The artifact descriptor with the published version and source commit.

    Nothing in this repository reads `gitSha` or `version`. They are there for
    consumers who unpack an archive, such as one from the rolling snapshot
    release, and need to match it to the commit it was built from.
    """
    descriptor = json.loads(content)
    descriptor["gitSha"] = sha
    descriptor["version"] = version
    return (json.dumps(descriptor, indent=2) + "\n").encode()


def rewrite_archive(archive: pathlib.Path, version: str, sha: str) -> None:
    root = archive.name.removesuffix(ARCHIVE_SUFFIX)
    pkg_config_name = f"{root}/share/pkgconfig/maplibre-native-c.pc"
    descriptor_name = f"{root}/share/maplibre-native-c/artifact.json"
    found_pkg_config = False
    found_descriptor = False

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
                if not member.isfile():
                    output.addfile(member)
                    continue

                source_file = source.extractfile(member)
                if source_file is None:
                    raise SystemExit(f"cannot read {member.name} from {archive.name}")
                if member.name == pkg_config_name:
                    content = pkg_config_bytes(source_file.read(), archive, version)
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
                    found_pkg_config = True
                elif member.name == descriptor_name:
                    content = descriptor_bytes(source_file.read(), version, sha)
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
                    found_descriptor = True
                elif FRAMEWORK_INFO_PLIST.search(member.name):
                    content = framework_plist_bytes(source_file.read(), version)
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
                else:
                    output.addfile(member, source_file)
        if not found_pkg_config:
            raise SystemExit(f"{archive.name} is missing maplibre-native-c.pc")
        if not found_descriptor:
            raise SystemExit(f"{archive.name} is missing artifact.json")
        os.replace(temporary_path, archive)
    finally:
        temporary_path.unlink(missing_ok=True)


def rewrite_xcframework(archive: pathlib.Path, version: str) -> None:
    """Relabels each framework's Info.plist in the zip with the release version."""
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{archive.name}.", dir=archive.parent
    )
    temporary_path = pathlib.Path(temporary_name)
    found_plist = False
    try:
        with (
            os.fdopen(descriptor, "wb") as temporary,
            zipfile.ZipFile(archive) as source,
            zipfile.ZipFile(temporary, "w") as output,
        ):
            for member in source.infolist():
                content = source.read(member)
                if FRAMEWORK_INFO_PLIST.search(member.filename):
                    content = framework_plist_bytes(content, version)
                    found_plist = True
                # A fresh entry keeps the mode bits, which carry the macOS
                # framework's symlinks, and drops the source's size fields.
                copy = zipfile.ZipInfo(member.filename, member.date_time)
                copy.create_system = member.create_system
                copy.external_attr = member.external_attr
                copy.compress_type = member.compress_type
                output.writestr(copy, content)
        if not found_plist:
            raise SystemExit(f"{archive.name} contains no MaplibreNativeC.framework")
        os.replace(temporary_path, archive)
    finally:
        temporary_path.unlink(missing_ok=True)


def archive_descriptor(archive: pathlib.Path) -> dict:
    root = archive.name.removesuffix(ARCHIVE_SUFFIX)
    with tarfile.open(archive, "r:gz") as source:
        return json.load(
            source.extractfile(f"{root}/share/maplibre-native-c/artifact.json")
        )


def write_artifacts(path: pathlib.Path, archives: list[pathlib.Path]) -> list[dict]:
    """Writes the artifact index: each archive's name and digest, with its descriptor.

    A published package reads the index from its pin to choose an archive by
    the descriptor's fields, before it has downloaded any.
    """
    artifacts = [
        {"archive": archive.name, "sha256": sha256(archive)}
        | archive_descriptor(archive)
        for archive in archives
    ]
    path.write_text(json.dumps(artifacts, indent=2) + "\n", encoding="utf-8")
    return artifacts


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


def compare_url(previous_tag: str, tag: str) -> str:
    server_url = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    repository = required_env("GITHUB_REPOSITORY")
    return f"{server_url}/{repository}/compare/{previous_tag}...{tag}"


def write_notes(
    release_dir: pathlib.Path, channel: str, sha: str, version: str
) -> None:
    if channel == "snapshot":
        published = required_env("PUBLISH_TIMESTAMP")
        lines = [f"Unstable native package snapshot, published {published}."]
    else:
        lines = [f"MapLibre Native FFI {version}."]
    lines += [
        "",
        f"Commit: {commit_url(sha)}",
        f"CI run: {required_env('CI_RUN_URL')}",
        f"Publishing run: {workflow_url()}",
    ]
    if channel == "release":
        previous_tag = required_env("PUBLISH_PREVIOUS_TAG")
        lines += [
            "",
            f"Full changelog: {compare_url(previous_tag, required_env('PUBLISH_TAG'))}",
        ]
    lines.append("")
    (release_dir / RELEASE_NOTES_NAME).write_text("\n".join(lines), encoding="utf-8")


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
        rewrite_archive(archive, version, sha)
    xcframework = release_dir / XCFRAMEWORK_NAME
    if not xcframework.is_file():
        raise SystemExit(f"invalid native package set: missing {XCFRAMEWORK_NAME}")
    rewrite_xcframework(xcframework, version)
    tag = required_env("PUBLISH_TAG")
    artifacts_path = release_dir / ARTIFACTS_NAME
    artifacts = write_artifacts(artifacts_path, archives)
    zig_package = release_dir / ZIG_PACKAGE_NAME
    write_zig_package(
        zig_package,
        root,
        version,
        pin(tag, artifacts),
        {
            archive.name.removeprefix(ARCHIVE_PREFIX).removesuffix(ARCHIVE_SUFFIX): (
                f"{RELEASE_DOWNLOAD_URL}/{tag}/{archive.name}",
                archive,
            )
            for archive in archives
        },
    )
    write_notes(release_dir, channel, sha, version)
    write_checksums(release_dir, [*archives, xcframework, zig_package, artifacts_path])


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
