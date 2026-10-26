from __future__ import annotations

import datetime
import hashlib
import io
import json
import os
import pathlib
import plistlib
import stat
import subprocess
import tarfile
import tempfile
import unittest
import zipfile
from unittest import mock

from native_package_release import ARTIFACTS_NAME, XCFRAMEWORK_NAME, prepare_assets
from release_version import next_release, require_current_month, validate_tag
from zig_package import PACKAGE_NAME as ZIG_PACKAGE_NAME
from zig_package import zig_hash

REPOSITORY = pathlib.Path(__file__).resolve().parents[2]


def run_git(repository: pathlib.Path, *arguments: str) -> None:
    subprocess.run(["git", *arguments], cwd=repository, check=True, capture_output=True)


def initialize_repository(repository: pathlib.Path) -> None:
    run_git(repository, "init", "--quiet")
    run_git(repository, "config", "user.name", "Release Test")
    run_git(repository, "config", "user.email", "release@example.com")
    run_git(repository, "config", "commit.gpgSign", "false")
    run_git(repository, "config", "tag.gpgSign", "false")
    run_git(repository, "commit", "--quiet", "--allow-empty", "-m", "initial")


def add_bytes(archive: tarfile.TarFile, name: str, content: bytes) -> None:
    member = tarfile.TarInfo(name)
    member.mode = 0o644
    member.mtime = 1
    member.size = len(content)
    archive.addfile(member, io.BytesIO(content))


FRAMEWORK_PLIST = plistlib.dumps(
    {"CFBundleShortVersionString": "0.1.0", "CFBundleVersion": "0.1.0"}
)
IOS_PLIST = "MaplibreNativeC.xcframework/ios-arm64/MaplibreNativeC.framework/Info.plist"
MACOS_FRAMEWORK = "MaplibreNativeC.xcframework/macos-arm64/MaplibreNativeC.framework"
MACOS_PLIST = f"{MACOS_FRAMEWORK}/Versions/A/Resources/Info.plist"
IOS_DSYM_PLIST = (
    "MaplibreNativeC.xcframework/ios-arm64/dSYMs/"
    "MaplibreNativeC.framework.dSYM/Contents/Info.plist"
)


def add_symlink(archive: zipfile.ZipFile, name: str, target: str) -> None:
    member = zipfile.ZipInfo(name)
    member.create_system = 3
    member.external_attr = (stat.S_IFLNK | 0o755) << 16
    archive.writestr(member, target)


def create_xcframework(path: pathlib.Path) -> None:
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr("MaplibreNativeC.xcframework/Info.plist", b"xcframework")
        archive.writestr(IOS_PLIST, FRAMEWORK_PLIST)
        archive.writestr(MACOS_PLIST, FRAMEWORK_PLIST)
        archive.writestr(IOS_DSYM_PLIST, FRAMEWORK_PLIST)
        add_symlink(archive, f"{MACOS_FRAMEWORK}/Headers", "Versions/Current/Headers")


def create_archive(path: pathlib.Path, preset: str) -> None:
    root = f"maplibre-native-c-{preset}"
    with tarfile.open(path, "w:gz") as archive:
        add_bytes(
            archive,
            f"{root}/Frameworks/MaplibreNativeC.framework/Info.plist",
            FRAMEWORK_PLIST,
        )
        add_bytes(
            archive,
            f"{root}/share/pkgconfig/maplibre-native-c.pc",
            b"Name: maplibre-native-c\nVersion: 0.1.0\n",
        )
        add_bytes(
            archive,
            f"{root}/share/maplibre-native-c/artifact.json",
            json.dumps({"targetPlatform": "linux-gnu-x64"}).encode(),
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

    def test_the_first_release_has_no_previous_tag(self) -> None:
        tag = f"v0.{self.month}.0"
        self.tag(tag)

        version, previous = validate_tag(tag, repository=self.repository, now=self.now)

        self.assertEqual(version, f"0.{self.month}.0")
        self.assertEqual(previous, "")

    def test_rejects_a_stale_month(self) -> None:
        previous = (self.now.replace(day=1) - datetime.timedelta(days=1)).strftime(
            "%Y%m"
        )
        with self.assertRaisesRegex(SystemExit, "current UTC month"):
            require_current_month(f"0.{previous}.0", now=self.now)

    def test_legacy_core_and_binding_tags_continue_the_revisions(self) -> None:
        self.tag(f"core/v0.{self.month}.0")
        self.tag(f"bindings/kotlin/v0.{self.month}.0")
        self.tag(f"core/v0.{self.month}.1")
        self.tag(f"bindings/kotlin/v0.{self.month}.1")
        tag = f"v0.{self.month}.2"
        self.tag(tag)

        version, previous = validate_tag(tag, repository=self.repository, now=self.now)

        self.assertEqual(version, f"0.{self.month}.2")
        self.assertIn(
            previous, {f"core/v0.{self.month}.1", f"bindings/kotlin/v0.{self.month}.1"}
        )

    def test_rejects_a_version_that_a_legacy_tag_published(self) -> None:
        self.tag(f"bindings/kotlin/v0.{self.month}.0")
        tag = f"v0.{self.month}.0"
        self.tag(tag)

        with self.assertRaisesRegex(SystemExit, "not newer"):
            validate_tag(tag, repository=self.repository, now=self.now)

    def test_rejects_a_revision_gap(self) -> None:
        self.tag(f"v0.{self.month}.0")
        tag = f"v0.{self.month}.2"
        self.tag(tag)

        with self.assertRaisesRegex(SystemExit, r"expected \[0, 1, 2\]"):
            validate_tag(tag, repository=self.repository, now=self.now)

    def test_rejects_a_legacy_tag(self) -> None:
        tag = f"core/v0.{self.month}.0"
        self.tag(tag)

        with self.assertRaisesRegex(SystemExit, "expected vAPI_EPOCH"):
            validate_tag(tag, repository=self.repository, now=self.now)

    def test_the_next_release_follows_this_months_latest_tag(self) -> None:
        self.tag(f"core/v0.{self.month}.0")
        self.tag(f"v0.{self.month}.1")

        version = next_release(repository=self.repository, now=self.now)

        self.assertEqual(str(version), f"0.{self.month}.2")

    def test_the_next_release_starts_a_new_month_at_revision_zero(self) -> None:
        previous = (self.now.replace(day=1) - datetime.timedelta(days=1)).strftime(
            "%Y%m"
        )
        self.tag(f"v1.{previous}.3")

        version = next_release(repository=self.repository, now=self.now)

        self.assertEqual(str(version), f"1.{self.month}.0")

    def test_the_first_release_is_revision_zero_of_epoch_zero(self) -> None:
        version = next_release(repository=self.repository, now=self.now)

        self.assertEqual(str(version), f"0.{self.month}.0")


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
        self.xcframework = self.release_dir / XCFRAMEWORK_NAME
        create_xcframework(self.xcframework)
        self.write_zig_sources()

    def write_zig_sources(self) -> None:
        manifest = (REPOSITORY / "build.zig.zon").read_text()
        (self.repository / "build.zig.zon").write_text(manifest)
        (self.repository / "build.zig").write_text("// build\n")
        (self.repository / "LICENSE").write_text("license\n")
        (self.repository / "README.md").write_text("not packaged\n")
        sources = self.repository / "bindings/zig/src"
        sources.mkdir(parents=True)
        (sources / "maplibre_native_ffi.zig").write_text("// binding\n")

    def write_presets(self, *presets: str) -> None:
        (self.repository / "CMakePresets.json").write_text(
            json.dumps({"packagePresets": [{"name": name} for name in presets]})
        )

    def archive(self) -> pathlib.Path:
        archive = self.release_dir / f"maplibre-native-c-{self.preset}.tar.gz"
        create_archive(archive, self.preset)
        return archive

    def prepare(self, channel: str, version: str | None = None) -> None:
        environment = {
            "CI_RUN_URL": "https://github.com/maplibre/repo/actions/runs/1",
            "GITHUB_REPOSITORY": "maplibre/repo",
            "GITHUB_RUN_ID": "2",
            "PUBLISH_PREVIOUS_TAG": "v0.202607.0",
            "PUBLISH_SHA": self.sha,
            "PUBLISH_TAG": f"v{self.version}",
            "PUBLISH_TIMESTAMP": "2026-08-07 01:01 UTC",
        }
        with mock.patch.dict(os.environ, environment, clear=False):
            prepare_assets(
                self.release_dir, channel, version or self.version, self.repository
            )

    def assert_assets(self, channel: str, version: str, title: str) -> str:
        archive = self.archive()
        self.prepare(channel, version)

        with tarfile.open(archive, "r:gz") as package:
            pkg_config = package.extractfile(
                f"maplibre-native-c-{self.preset}/share/pkgconfig/maplibre-native-c.pc"
            ).read()
            descriptor = json.load(
                package.extractfile(
                    f"maplibre-native-c-{self.preset}/share/maplibre-native-c/artifact.json"
                )
            )
        checksums = dict(
            reversed(line.split("  "))
            for line in (self.release_dir / "SHA256SUMS").read_text().splitlines()
        )
        notes = (self.release_dir / "RELEASE_NOTES.md").read_text()

        self.assertIn(f"Version: {version}".encode(), pkg_config)
        self.assertEqual(
            descriptor,
            {"targetPlatform": "linux-gnu-x64", "gitSha": self.sha, "version": version},
        )
        self.assertEqual(
            checksums,
            {
                path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (
                    archive,
                    self.xcframework,
                    self.release_dir / ZIG_PACKAGE_NAME,
                    self.release_dir / ARTIFACTS_NAME,
                )
            },
        )
        self.assertEqual(
            json.loads((self.release_dir / ARTIFACTS_NAME).read_text()),
            [
                {
                    "archive": archive.name,
                    "sha256": checksums[archive.name],
                    **descriptor,
                }
            ],
        )
        self.assertIn(title, notes)
        self.assertIn(f"/commit/{self.sha}", notes)
        return notes

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

    def test_refuses_a_release_without_the_xcframework(self) -> None:
        self.archive()
        self.xcframework.unlink()

        with self.assertRaisesRegex(SystemExit, f"missing {XCFRAMEWORK_NAME}"):
            self.prepare("release")

    def test_relabels_every_framework_with_the_bundle_version(self) -> None:
        archive = self.archive()
        self.prepare("snapshot", "0.0.0-dev.202608070101")

        with tarfile.open(archive, "r:gz") as package:
            packaged = plistlib.load(
                package.extractfile(
                    f"maplibre-native-c-{self.preset}/Frameworks/"
                    "MaplibreNativeC.framework/Info.plist"
                )
            )
        with zipfile.ZipFile(self.xcframework) as xcframework:
            combined = [
                plistlib.loads(xcframework.read(name))
                for name in (IOS_PLIST, MACOS_PLIST, IOS_DSYM_PLIST)
            ]
            link = xcframework.getinfo(f"{MACOS_FRAMEWORK}/Headers")
            link_target = xcframework.read(link)

        for plist in (packaged, *combined):
            self.assertEqual(plist["CFBundleShortVersionString"], "0.0.0")
            self.assertEqual(plist["CFBundleVersion"], "0.0.0")
        self.assertTrue(stat.S_ISLNK(link.external_attr >> 16))
        self.assertEqual(link_target, b"Versions/Current/Headers")

    def test_release_assets_include_version_provenance_and_checksums(self) -> None:
        notes = self.assert_assets(
            "release", self.version, f"MapLibre Native FFI {self.version}."
        )

        self.assertIn(f"/compare/v0.202607.0...v{self.version}", notes)

    def test_snapshot_assets_include_version_provenance_and_checksums(self) -> None:
        notes = self.assert_assets(
            "snapshot",
            "0.0.0-dev.202608070101",
            "Unstable native package snapshot, published 2026-08-07 01:01 UTC.",
        )

        self.assertNotIn("/compare/", notes)

    def test_zig_package_declares_each_native_archive_lazily(self) -> None:
        archive = self.archive()
        self.prepare("release")

        with tarfile.open(self.release_dir / ZIG_PACKAGE_NAME, "r:gz") as package:
            names = package.getnames()
            manifest = (
                package.extractfile("maplibre-native-ffi-zig/build.zig.zon")
                .read()
                .decode()
            )
            pin = json.load(
                package.extractfile(
                    "maplibre-native-ffi-zig/bindings/zig/native_release.json"
                )
            )

        self.assertEqual(
            names,
            [
                "maplibre-native-ffi-zig/LICENSE",
                "maplibre-native-ffi-zig/bindings/zig/native_release.json",
                "maplibre-native-ffi-zig/bindings/zig/src/maplibre_native_ffi.zig",
                "maplibre-native-ffi-zig/build.zig",
                "maplibre-native-ffi-zig/build.zig.zon",
            ],
        )
        artifacts = json.loads((self.release_dir / ARTIFACTS_NAME).read_text())
        self.assertEqual(
            pin,
            {
                "tag": f"v{self.version}",
                "sha256": {archive.name: artifacts[0]["sha256"]},
                "artifacts": artifacts,
            },
        )
        self.assertIn(f'.version = "{self.version}",', manifest)
        self.assertIn(
            f'        .@"{self.preset}" = .{{\n'
            '            .url = "https://github.com/maplibre/maplibre-native-ffi/'
            f'releases/download/v{self.version}/{archive.name}",\n'
            f'            .hash = "{zig_hash(archive)}",\n'
            "            .lazy = true,\n"
            "        },\n",
            manifest,
        )

    def test_zig_fetches_the_zig_package(self) -> None:
        self.archive()
        self.prepare("snapshot", "0.0.0-dev.202608070101")

        package_hash = zig_hash(self.release_dir / ZIG_PACKAGE_NAME)

        self.assertTrue(
            package_hash.startswith("maplibre_native_ffi-0.0.0-dev.202608070101-"),
            package_hash,
        )


if __name__ == "__main__":
    unittest.main()
