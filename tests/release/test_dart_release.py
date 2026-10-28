from __future__ import annotations

import json
import pathlib
import tempfile
import unittest

from dart_release import prepare

CHANGELOG = """# Changelog

Each version is a release of MapLibre Native FFI.
"""
INDEX = {
    "schemaVersion": 0,
    "tag": "v0.202610.2",
    "version": "0.202610.2",
    "commit": "a" * 40,
    "platforms": [{"id": "android-arm64", "defaultBackend": "opengl"}],
    "artifacts": [
        {
            "platform": "android-arm64",
            "backend": "opengl",
            "archive": "maplibre-native-c-android-arm64-egl.tar.gz",
            "sha256": "aaa",
        }
    ],
}


class DartReleaseTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.package = pathlib.Path(temporary.name)
        (self.package / "hook").mkdir()
        (self.package / "pubspec.yaml").write_text(
            "name: maplibre_native_ffi\nversion: 0.0.0\n"
        )
        (self.package / "CHANGELOG.md").write_text(CHANGELOG)
        self.artifacts = self.package / "maplibre-native-c-artifacts.json"
        self.artifacts.write_text(json.dumps(INDEX))

    def test_pins_the_native_archives_and_stamps_the_version(self) -> None:
        prepare(self.package, "v0.202610.2", self.artifacts)

        self.assertEqual(
            json.loads((self.package / "hook/native_release.json").read_text()),
            INDEX,
        )
        self.assertIn(
            "version: 0.202610.2\n", (self.package / "pubspec.yaml").read_text()
        )

    def test_refuses_the_index_of_another_release(self) -> None:
        with self.assertRaisesRegex(SystemExit, "names the release v0.202610.2"):
            prepare(self.package, "v0.202610.3", self.artifacts)

    def test_the_changelog_names_the_version_above_earlier_ones(self) -> None:
        (self.package / "CHANGELOG.md").write_text(
            CHANGELOG + "\n## 0.202610.1\n\nEarlier.\n"
        )

        prepare(self.package, "v0.202610.2", self.artifacts)

        changelog = (self.package / "CHANGELOG.md").read_text()
        self.assertLess(
            changelog.index("## 0.202610.2"), changelog.index("## 0.202610.1")
        )
        self.assertIn("/releases/tag/v0.202610.2", changelog)


if __name__ == "__main__":
    unittest.main()
