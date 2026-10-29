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
import subprocess
import tarfile
import tempfile
import urllib.request
import zipfile
from collections.abc import Iterable

from release_version import parse_version
from sbom import SBOM_NAME, component, compose, stamp, write_json
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


def write_pin(path: pathlib.Path, tag: str, index: pathlib.Path | None = None) -> None:
    """Writes the pin of release `tag` to `path`: the release's artifact index.

    It reads the index from `index` if given, and otherwise downloads it from
    the release.
    """
    if index is None:
        url = f"{RELEASE_DOWNLOAD_URL}/{tag}/{ARTIFACTS_NAME}"
        with urllib.request.urlopen(url) as response:
            content = response.read().decode()
    else:
        content = index.read_text(encoding="utf-8")
    found = json.loads(content)["tag"]
    if found != tag:
        raise SystemExit(f"the artifact index of {tag} names the release {found}")
    path.write_text(content, encoding="utf-8")


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
    """The artifact descriptor with the published version and source commit."""
    descriptor = json.loads(content)
    descriptor["version"] = version
    descriptor["commit"] = sha
    return (json.dumps(descriptor, indent=2) + "\n").encode()


def sbom_bytes(content: bytes, version: str, sha: str) -> bytes:
    return (
        json.dumps(stamp(json.loads(content), version, sha), indent=2) + "\n"
    ).encode()


def rewrite_archive(archive: pathlib.Path, version: str, sha: str) -> None:
    root = archive.name.removesuffix(ARCHIVE_SUFFIX)
    pkg_config_name = f"{root}/share/pkgconfig/maplibre-native-c.pc"
    descriptor_name = f"{root}/share/maplibre-native-c/artifact.json"
    found_pkg_config = False
    found_descriptor = False
    found_sbom = False

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
                elif pathlib.PurePosixPath(member.name).name == SBOM_NAME:
                    content = sbom_bytes(source_file.read(), version, sha)
                    member.size = len(content)
                    output.addfile(member, io.BytesIO(content))
                    found_sbom |= (
                        member.name == f"{root}/share/maplibre-native-c/{SBOM_NAME}"
                    )
                else:
                    output.addfile(member, source_file)
        if not found_pkg_config:
            raise SystemExit(f"{archive.name} is missing maplibre-native-c.pc")
        if not found_descriptor:
            raise SystemExit(f"{archive.name} is missing artifact.json")
        if not found_sbom:
            raise SystemExit(f"{archive.name} is missing {SBOM_NAME}")
        os.replace(temporary_path, archive)
    finally:
        temporary_path.unlink(missing_ok=True)


def rewrite_xcframework(archive: pathlib.Path, version: str, sha: str) -> None:
    """Relabels each framework's Info.plist in the zip with the release version."""
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{archive.name}.", dir=archive.parent
    )
    temporary_path = pathlib.Path(temporary_name)
    found_plist = False
    found_sbom = False
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
                elif pathlib.PurePosixPath(member.filename).name == SBOM_NAME:
                    content = sbom_bytes(content, version, sha)
                    found_sbom = True
                # A fresh entry keeps the mode bits, which carry the macOS
                # framework's symlinks, and drops the source's size fields.
                copy = zipfile.ZipInfo(member.filename, member.date_time)
                copy.create_system = member.create_system
                copy.external_attr = member.external_attr
                copy.compress_type = member.compress_type
                output.writestr(copy, content)
        if not found_plist:
            raise SystemExit(f"{archive.name} contains no MaplibreNativeC.framework")
        if not found_sbom:
            raise SystemExit(f"{archive.name} contains no {SBOM_NAME}")
        os.replace(temporary_path, archive)
    finally:
        temporary_path.unlink(missing_ok=True)


def archive_descriptor(archive: pathlib.Path) -> dict:
    root = archive.name.removesuffix(ARCHIVE_SUFFIX)
    with tarfile.open(archive, "r:gz") as source:
        return json.load(
            source.extractfile(f"{root}/share/maplibre-native-c/artifact.json")
        )


def sbom_path(archive: pathlib.Path) -> pathlib.Path:
    return archive.with_name(
        archive.name.removesuffix(ARCHIVE_SUFFIX).removesuffix(".zip") + ".cdx.json"
    )


def publish_inventory(archive: pathlib.Path, inventory: dict, tag: str) -> None:
    # Only the sidecar hashes the final archive. Embedding that hash in the
    # archive's own inventory would make the checksum self-referential.
    inventory["metadata"]["component"].setdefault("externalReferences", []).append(
        {
            "type": "distribution",
            "url": f"{RELEASE_DOWNLOAD_URL}/{tag}/{archive.name}",
            "hashes": [{"alg": "SHA-256", "content": sha256(archive)}],
        }
    )
    write_json(sbom_path(archive), inventory)


def sbom_reference(archive: pathlib.Path, tag: str) -> dict:
    path = sbom_path(archive)
    return {
        "file": path.name,
        "url": f"{RELEASE_DOWNLOAD_URL}/{tag}/{path.name}",
        "sha256": sha256(path),
    }


def artifact_index(tag: str, archives: list[pathlib.Path]) -> dict:
    """The artifact index of release `tag`, which holds `archives`.

    A published package pins the index to choose an archive for its target
    and backend before it has downloaded any. Each descriptor's release fields
    and platform appear once.
    """
    index = None
    platforms: dict[str, dict] = {}
    artifacts: dict[tuple[str, str], dict] = {}
    for archive in archives:
        descriptor = archive_descriptor(archive)
        release = {
            "schemaVersion": descriptor["schemaVersion"],
            "tag": tag,
            "version": descriptor["version"],
            "commit": descriptor["commit"],
        }
        if index is None:
            index = release
        elif release != index:
            raise SystemExit(f"{archive.name} is from another build than the rest")
        platform = descriptor["platform"]
        if platforms.setdefault(platform["id"], platform) != platform:
            raise SystemExit(
                f"{archive.name} describes {platform['id']} unlike another archive"
            )
        key = (platform["id"], descriptor["backend"])
        if key in artifacts:
            raise SystemExit(f"{archive.name} duplicates {' '.join(key)}")
        artifacts[key] = {
            "platform": key[0],
            "backend": key[1],
            "archive": archive.name,
            "sha256": sha256(archive),
            "sbom": sbom_reference(archive, tag),
        }
    if index is None:
        raise SystemExit("an artifact index needs at least one archive")
    for platform in platforms.values():
        if (platform["id"], platform["defaultBackend"]) not in artifacts:
            raise SystemExit(
                f"no archive has {platform['id']}'s default backend, "
                f"{platform['defaultBackend']}"
            )
    return index | {
        "platforms": [platforms[name] for name in sorted(platforms)],
        "artifacts": list(artifacts.values()),
    }


def write_artifacts(
    path: pathlib.Path,
    tag: str,
    archives: list[pathlib.Path],
    xcframework: pathlib.Path,
) -> str:
    """Writes the artifact index of release `tag` to `path`, and returns it."""
    index = artifact_index(tag, archives)
    index["xcframework"] = {
        "archive": xcframework.name,
        "sha256": sha256(xcframework),
        "sbom": sbom_reference(xcframework, tag),
    }
    content = json.dumps(index, indent=2) + "\n"
    path.write_text(content, encoding="utf-8")
    return content


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


def tag_notes(repository: pathlib.Path, tag: str) -> str:
    """Returns the notes of an annotated release tag: its message below the
    title line, without the signature. A lightweight tag has none."""
    fields = subprocess.run(
        [
            "git",
            "-C",
            str(repository),
            "for-each-ref",
            "--format=%(objecttype)%00%(contents:body)",
            f"refs/tags/{tag}",
        ],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    object_type, _, body = fields.partition("\0")
    return body.strip() if object_type == "tag" else ""


def write_notes(
    release_dir: pathlib.Path,
    channel: str,
    sha: str,
    version: str,
    repository: pathlib.Path,
) -> None:
    links = [
        f"[Commit]({commit_url(sha)})",
        f"[CI run]({required_env('CI_RUN_URL')})",
        f"[Publishing run]({workflow_url()})",
    ]
    if channel == "snapshot":
        published = required_env("PUBLISH_TIMESTAMP")
        notes = f"Unstable native package snapshot, published {published}."
    else:
        tag = required_env("PUBLISH_TAG")
        notes = tag_notes(repository, tag) or f"MapLibre Native FFI {version}."
        previous_tag = required_env("PUBLISH_PREVIOUS_TAG")
        links.insert(0, f"[Full changelog]({compare_url(previous_tag, tag)})")
    (release_dir / RELEASE_NOTES_NAME).write_text(
        f"{notes}\n\n{' · '.join(links)}\n", encoding="utf-8"
    )


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
    rewrite_xcframework(xcframework, version, sha)
    tag = required_env("PUBLISH_TAG")
    for archive in archives:
        archive_root = archive.name.removesuffix(ARCHIVE_SUFFIX)
        with tarfile.open(archive, "r:gz") as source:
            inventory = json.load(
                source.extractfile(
                    f"{archive_root}/share/maplibre-native-c/{SBOM_NAME}"
                )
            )
        publish_inventory(archive, inventory, tag)
    with zipfile.ZipFile(xcframework) as source:
        inventories = [
            json.loads(source.read(name))
            for name in sorted(source.namelist())
            if pathlib.PurePosixPath(name).name == SBOM_NAME
        ]
    root_component = component("MaplibreNativeC.xcframework", version, "BSD-2-Clause")
    root_component["group"] = "org.maplibre.nativeffi"
    publish_inventory(xcframework, compose(root_component, inventories), tag)
    artifacts_path = release_dir / ARTIFACTS_NAME
    index = write_artifacts(artifacts_path, tag, archives, xcframework)
    zig_package = release_dir / ZIG_PACKAGE_NAME
    write_zig_package(
        zig_package,
        root,
        version,
        index,
        {
            archive.name: (f"{RELEASE_DOWNLOAD_URL}/{tag}/{archive.name}", archive)
            for archive in archives
        },
    )
    write_notes(release_dir, channel, sha, version, root)
    write_checksums(
        release_dir,
        [
            *archives,
            xcframework,
            zig_package,
            artifacts_path,
            *(sbom_path(archive) for archive in [*archives, xcframework]),
        ],
    )


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
