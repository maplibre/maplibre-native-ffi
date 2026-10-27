from __future__ import annotations

import pathlib
import shutil
import tempfile
import unittest

from distribution import Release, write
from native_package_release import XCFRAMEWORK_NAME

REPOSITORY = pathlib.Path(__file__).resolve().parents[2]
DIGEST = "539f11cb25fe35a1e8b8e8b6f1c095cde9f93f0f8efddb4115469335d6d37b8a"


class DistributionTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = pathlib.Path(temporary.name)
        self.assets = root / "assets"
        self.assets.mkdir()
        (self.assets / "SHA256SUMS").write_text(
            f"{'0' * 64}  maplibre-native-c-macos-arm64-metal.tar.gz\n"
            f"{DIGEST}  {XCFRAMEWORK_NAME}\n"
        )
        (self.assets / XCFRAMEWORK_NAME).write_bytes(b"xcframework")
        self.tree = root / "tree"

    def test_a_release_pins_the_xcframework_by_url_and_checksum(self) -> None:
        write(self.tree, Release(self.assets, "v0.202611.0"), REPOSITORY)

        manifest = (self.tree / "Package.swift").read_text()
        self.assertIn(
            '    url: "https://github.com/maplibre/maplibre-native-ffi/releases/'
            f'download/v0.202611.0/{XCFRAMEWORK_NAME}",\n'
            f'    checksum: "{DIGEST}"\n',
            manifest,
        )
        self.assertFalse((self.tree / XCFRAMEWORK_NAME).exists())

    def test_a_local_release_copies_the_xcframework_into_the_package(self) -> None:
        write(self.tree, Release(self.assets), REPOSITORY)

        self.assertIn(
            f'    path: "{XCFRAMEWORK_NAME}"\n',
            (self.tree / "Package.swift").read_text(),
        )
        self.assertEqual((self.tree / XCFRAMEWORK_NAME).read_bytes(), b"xcframework")

    def test_the_package_takes_the_root_manifests_platforms_and_binding(self) -> None:
        write(self.tree, Release(self.assets, "v0.202611.0"), REPOSITORY)

        manifest = (self.tree / "Package.swift").read_text()
        root = (REPOSITORY / "Package.swift").read_text()
        self.assertEqual(manifest.partition("\n")[0], root.partition("\n")[0])
        self.assertIn('    .visionOS("1.0"),\n', manifest)
        self.assertIn('    path: "Sources/MaplibreNativeFFI",\n', manifest)
        self.assertIn(
            '.enableUpcomingFeature("NonisolatedNonsendingByDefault")', manifest
        )
        self.assertIn(
            '    .library(name: "MaplibreNativeC", targets: ["MaplibreNativeC"]),\n',
            manifest,
        )
        self.assertNotIn("bindings/swift", manifest)
        self.assertNotIn("GraphicsSupport", manifest)
        sources = REPOSITORY / "bindings/swift/Sources/MaplibreNativeFFI"
        self.assertEqual(
            sorted(
                path.relative_to(self.tree / "Sources/MaplibreNativeFFI")
                for path in (self.tree / "Sources/MaplibreNativeFFI").rglob("*")
            ),
            sorted(path.relative_to(sources) for path in sources.rglob("*")),
        )

    def test_the_linux_system_library_includes_the_installed_headers(self) -> None:
        write(self.tree, Release(self.assets, "v0.202611.0"), REPOSITORY)

        include = self.tree / "Sources/MaplibreNativeC/include"
        self.assertEqual(
            (include / "maplibre_native_c.h").read_text(),
            "#include <maplibre_native_c.h>\n",
        )
        self.assertEqual(
            (include / "plugin.h").read_text(),
            "#include <maplibre_native_c/plugin.h>\n",
        )
        self.assertEqual(
            (self.tree / "Sources/MaplibreNativeC/module.modulemap").read_text(),
            (
                REPOSITORY / "bindings/swift/Sources/MaplibreNativeC/module.modulemap"
            ).read_text(),
        )

    def test_the_tree_replaces_everything_but_the_git_directory(self) -> None:
        (self.tree / ".git").mkdir(parents=True)
        (self.tree / ".git/HEAD").write_text("ref: refs/heads/main\n")
        (self.tree / "Sources/Stale").mkdir(parents=True)
        (self.tree / "stale.txt").write_text("stale")

        write(self.tree, Release(self.assets, "v0.202611.0"), REPOSITORY)

        self.assertEqual(
            (self.tree / ".git/HEAD").read_text(), "ref: refs/heads/main\n"
        )
        self.assertEqual(
            sorted(path.name for path in self.tree.iterdir()),
            [".git", ".spi.yml", "LICENSE", "Package.swift", "README.md", "Sources"],
        )
        self.assertEqual(
            sorted(path.name for path in (self.tree / "Sources").iterdir()),
            ["MaplibreNativeC", "MaplibreNativeFFI"],
        )

    def test_a_release_without_the_xcframework_checksum_fails(self) -> None:
        (self.assets / "SHA256SUMS").write_text(f"{DIGEST}  other.zip\n")

        with self.assertRaisesRegex(SystemExit, "does not list"):
            write(self.tree, Release(self.assets, "v0.202611.0"), REPOSITORY)

    def test_the_tree_may_not_hold_the_repository(self) -> None:
        repository = self.tree / "checkout"
        shutil.copytree(REPOSITORY / "bindings/swift/Sources", repository / "Sources")

        with self.assertRaisesRegex(SystemExit, "write the tree elsewhere"):
            write(self.tree, Release(self.assets, "v0.202611.0"), repository)
        self.assertTrue((repository / "Sources").is_dir())


if __name__ == "__main__":
    unittest.main()
