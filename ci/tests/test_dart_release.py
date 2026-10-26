from __future__ import annotations

import json
import pathlib
import tempfile
import unittest

from dart_release import prepare

CHANGELOG = """# Changelog

Each version is a release of MapLibre Native FFI.
"""
ARTIFACTS = [
    {
        "archive": "maplibre-native-c-android-arm64-egl.tar.gz",
        "sha256": "aaa",
        "targetPlatform": "android-arm64",
    },
    {
        "archive": "maplibre-native-c-macos-arm64-metal.tar.gz",
        "sha256": "bbb",
        "targetPlatform": "macos-arm64",
    },
]


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
        self.artifacts.write_text(json.dumps(ARTIFACTS))

    def test_pins_the_native_archives_and_stamps_the_version(self) -> None:
        prepare(self.package, "v0.202610.2", self.artifacts)

        self.assertEqual(
            json.loads((self.package / "hook/native_release.json").read_text()),
            {
                "tag": "v0.202610.2",
                "sha256": {
                    "maplibre-native-c-android-arm64-egl.tar.gz": "aaa",
                    "maplibre-native-c-macos-arm64-metal.tar.gz": "bbb",
                },
                "artifacts": ARTIFACTS,
            },
        )
        self.assertIn(
            "version: 0.202610.2\n", (self.package / "pubspec.yaml").read_text()
        )

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
