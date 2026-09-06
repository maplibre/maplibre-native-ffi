from __future__ import annotations

import datetime
import hashlib
import io
import json
import os
import pathlib
import subprocess
import tarfile
import tempfile
import unittest
from unittest import mock

from ci.native_package_release import prepare_assets
from ci.release_version import (
    family,
    require_current_month,
    validate_tag,
)


def run_git(repository: pathlib.Path, *arguments: str) -> None:
    subprocess.run(["git", *arguments], cwd=repository, check=True, capture_output=True)


def initialize_repository(repository: pathlib.Path) -> None:
    run_git(repository, "init", "--quiet")
    run_git(repository, "config", "user.name", "Release Test")
    run_git(repository, "config", "user.email", "release@example.com")
    run_git(repository, "config", "tag.gpgSign", "false")
    run_git(repository, "commit", "--quiet", "--allow-empty", "-m", "initial")


def add_bytes(archive: tarfile.TarFile, name: str, content: bytes) -> None:
    member = tarfile.TarInfo(name)
    member.mode = 0o644
    member.mtime = 1
    member.size = len(content)
    archive.addfile(member, io.BytesIO(content))


def create_archive(path: pathlib.Path, preset: str, sha: str) -> None:
    root = f"maplibre-native-c-{preset}"
    with tarfile.open(path, "w:gz") as archive:
        add_bytes(
            archive,
            f"{root}/share/maplibre-native-c/artifact.json",
            (
                json.dumps(
                    {
                        "gitSha": sha,
                        "version": "0.1.0",
                        "renderBackend": "vulkan",
                        "targetPlatform": "linux-gnu-x64",
                        "zigTarget": "x86_64-linux-gnu",
                    },
                    indent=2,
                )
                + "\n"
            ).encode(),
        )
        add_bytes(
            archive,
            f"{root}/share/pkgconfig/maplibre-native-c.pc",
            b"Name: maplibre-native-c\nVersion: 0.1.0\n",
        )
        add_bytes(archive, f"{root}/lib/libmaplibre-native-c.so", b"binary")


class ReleaseVersionTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.repository = pathlib.Path(self.temporary_directory.name)
        initialize_repository(self.repository)
        self.now = datetime.datetime.now(datetime.UTC)
        self.month = self.now.strftime("%Y%m")

    def tag(self, name: str) -> None:
        run_git(self.repository, "tag", name)

    def test_accepts_an_unregistered_binding_family(self) -> None:
        tag = f"bindings/lua/v0.{self.month}.0"
        self.tag(tag)

        release_family, version, previous = validate_tag(
            tag, repository=self.repository, now=self.now
        )

        self.assertEqual(release_family.slug, "lua")
        self.assertEqual(version, f"0.{self.month}.0")
        self.assertEqual(previous, "")

    def test_rejects_a_stale_month(self) -> None:
        previous = (self.now.replace(day=1) - datetime.timedelta(days=1)).strftime(
            "%Y%m"
        )
        with self.assertRaisesRegex(SystemExit, "current UTC month"):
            require_current_month(f"0.{previous}.0", family("core"), now=self.now)

    def test_validates_revisions_and_returns_the_previous_family_tag(self) -> None:
        first = f"core/v0.{self.month}.0"
        second = f"core/v0.{self.month}.1"
        self.tag(first)
        self.tag(second)
        self.tag(f"bindings/kotlin/v0.{self.month}.0")

        release_family, version, previous = validate_tag(
            second, repository=self.repository, now=self.now
        )

        self.assertEqual(release_family.slug, "core")
        self.assertEqual(version, f"0.{self.month}.1")
        self.assertEqual(previous, first)

    def test_rejects_a_revision_gap(self) -> None:
        self.tag(f"core/v0.{self.month}.0")
        tag = f"core/v0.{self.month}.2"
        self.tag(tag)

        with self.assertRaisesRegex(SystemExit, r"expected \[0, 1, 2\]"):
            validate_tag(tag, repository=self.repository, now=self.now)


class NativePackageReleaseTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.repository = pathlib.Path(self.temporary_directory.name)
        self.release_dir = self.repository / "release"
        self.release_dir.mkdir()
        self.preset = "linux-gnu-x64-vulkan"
        self.sha = "b" * 40
        self.version = "0.202608.0"
        self.write_presets(self.preset)

    def write_presets(self, *presets: str) -> None:
        (self.repository / "CMakePresets.json").write_text(
            json.dumps({"packagePresets": [{"name": name} for name in presets]})
        )

    def archive(self, sha: str | None = None) -> pathlib.Path:
        archive = self.release_dir / f"maplibre-native-c-{self.preset}.tar.gz"
        create_archive(archive, self.preset, sha or self.sha)
        return archive

    def prepare(self, channel: str, version: str | None = None) -> None:
        environment = {
            "CI_RUN_URL": "https://github.com/maplibre/repo/actions/runs/1",
            "GITHUB_REPOSITORY": "maplibre/repo",
            "GITHUB_RUN_ID": "2",
            "PUBLISH_SHA": self.sha,
        }
        with mock.patch.dict(os.environ, environment, clear=False):
            prepare_assets(
                self.release_dir, channel, version or self.version, self.repository
            )

    def assert_assets(self, channel: str, version: str, title: str) -> None:
        archive = self.archive()
        self.prepare(channel, version)

        with tarfile.open(archive, "r:gz") as package:
            root = f"maplibre-native-c-{self.preset}"
            descriptor = json.load(
                package.extractfile(f"{root}/share/maplibre-native-c/artifact.json")
            )
            pkg_config = package.extractfile(
                f"{root}/share/pkgconfig/maplibre-native-c.pc"
            ).read()
        checksum, name = (
            (self.release_dir / "SHA256SUMS").read_text().strip().split("  ")
        )
        notes = (self.release_dir / "RELEASE_NOTES.md").read_text()

        self.assertEqual(descriptor["version"], version)
        self.assertIn(f"Version: {version}".encode(), pkg_config)
        self.assertEqual(checksum, hashlib.sha256(archive.read_bytes()).hexdigest())
        self.assertEqual(name, archive.name)
        self.assertIn(title, notes)
        self.assertIn(f"/commit/{self.sha}", notes)

    def test_refuses_an_incomplete_native_release(self) -> None:
        self.write_presets("linux-gnu-x64-egl", self.preset)
        self.archive()

        with self.assertRaisesRegex(SystemExit, "missing"):
            self.prepare("release")

    def test_published_archive_is_reproducible_and_preserves_its_payload(self) -> None:
        archive = self.archive()
        self.prepare("release")
        first_digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        self.prepare("release")

        self.assertEqual(first_digest, hashlib.sha256(archive.read_bytes()).hexdigest())
        with tarfile.open(archive, "r:gz") as package:
            library = package.extractfile(
                f"maplibre-native-c-{self.preset}/lib/libmaplibre-native-c.so"
            ).read()
        self.assertEqual(library, b"binary")

    def test_rejects_an_archive_from_another_commit(self) -> None:
        self.archive("a" * 40)
        with self.assertRaisesRegex(SystemExit, "expected"):
            self.prepare("release")

    def test_rejects_archive_escapes(self) -> None:
        with tarfile.open(self.archive(), "w:gz") as package:
            add_bytes(package, "../escape", b"payload")

        with self.assertRaisesRegex(SystemExit, "unsafe path"):
            self.prepare("release")

        with tarfile.open(self.archive(), "w:gz") as package:
            link = tarfile.TarInfo(f"maplibre-native-c-{self.preset}/escape")
            link.type = tarfile.SYMTYPE
            link.linkname = "../../escape"
            package.addfile(link)

        with self.assertRaisesRegex(SystemExit, "unsafe link"):
            self.prepare("release")

    def test_release_assets_include_version_provenance_and_checksums(self) -> None:
        self.assert_assets("release", self.version, f"Native package {self.version}.")

    def test_snapshot_assets_include_version_provenance_and_checksums(self) -> None:
        self.assert_assets(
            "snapshot", "0.0.0-dev.202608070101", "Unstable native package snapshot."
        )


if __name__ == "__main__":
    unittest.main()
