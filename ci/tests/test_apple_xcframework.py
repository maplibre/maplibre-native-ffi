from __future__ import annotations

import io
import json
import pathlib
import plistlib
import shutil
import tarfile
import tempfile
import unittest
from collections.abc import Sequence

from apple_xcframework import PLATFORMS, assemble, bundle_version


def add_bytes(archive: tarfile.TarFile, name: str, content: bytes) -> None:
    member = tarfile.TarInfo(name)
    member.mode = 0o644
    member.size = len(content)
    archive.addfile(member, io.BytesIO(content))


def create_archive(path: pathlib.Path, preset: str, sha: str) -> None:
    root = f"maplibre-native-c-{preset}"
    with tarfile.open(path, "w:gz") as archive:
        add_bytes(
            archive,
            f"{root}/share/maplibre-native-c/artifact.json",
            json.dumps({"gitSha": sha}).encode(),
        )
        add_bytes(archive, f"{root}/share/maplibre-native-c/LICENSE", b"license")
        add_bytes(
            archive,
            f"{root}/share/maplibre-native-c/licenses/maplibre-native.md",
            b"notice",
        )
        add_bytes(archive, f"{root}/include/maplibre_native_c.h", b"umbrella")
        add_bytes(archive, f"{root}/include/maplibre_native_c/base.h", b"base")
        add_bytes(archive, f"{root}/include/mln/plugin/plugin_api.h", b"plugin")
        add_bytes(archive, f"{root}/lib/libmaplibre-native-c.dylib", preset.encode())


class AppleXcframeworkTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.root = pathlib.Path(self.temporary_directory.name)
        self.input_dir = self.root / "input"
        self.input_dir.mkdir()
        self.frameworks = self.root / "frameworks"
        self.commands: list[list[str]] = []
        self.sha = "b" * 40
        (self.root / "CMakePresets.json").write_text(
            json.dumps(
                {
                    "configurePresets": [
                        {
                            "name": "_macos-arm64",
                            "cacheVariables": {"CMAKE_OSX_DEPLOYMENT_TARGET": "14.3"},
                        },
                        {
                            "name": "_ios-arm64",
                            "cacheVariables": {
                                "CMAKE_OSX_DEPLOYMENT_TARGET": {
                                    "type": "STRING",
                                    "value": "15.5",
                                }
                            },
                        },
                        {"name": "_metal", "cacheVariables": {}},
                        {
                            "name": "macos-arm64-metal",
                            "inherits": ["_macos-arm64", "_metal"],
                        },
                        {
                            "name": "ios-arm64-metal",
                            "inherits": ["_ios-arm64", "_metal"],
                        },
                        {"name": "ios-simulator-arm64-metal", "inherits": "_ios-arm64"},
                    ],
                    "packagePresets": [
                        {"name": preset, "configurePreset": preset}
                        for preset in PLATFORMS
                    ],
                }
            )
        )

    def runner(self, command: Sequence[str]) -> None:
        self.commands.append(list(command))
        # assemble deletes its work directory before it returns, so keep a copy
        # of the frameworks that xcodebuild would combine.
        if command[0] == "xcodebuild":
            for framework in command[3:-2:2]:
                path = pathlib.Path(framework)
                shutil.copytree(
                    path, self.frameworks / path.parent.name / path.name, symlinks=True
                )

    def archives(self, sha: str | None = None) -> None:
        for preset in PLATFORMS:
            create_archive(
                self.input_dir / f"maplibre-native-c-{preset}.tar.gz",
                preset,
                sha or self.sha,
            )

    def assemble(self, version: str = "0.202608.0") -> pathlib.Path:
        return assemble(
            self.input_dir,
            self.root / "output",
            version,
            sha=self.sha,
            repository=self.root,
            runner=self.runner,
        )

    def framework(self, preset: str) -> pathlib.Path:
        return self.frameworks / preset / "MapLibreNativeC.framework"

    def test_wraps_each_dylib_in_a_framework_with_an_rpath_install_name(self) -> None:
        self.archives()
        output = self.assemble()

        install_names = sorted(
            command[2] for command in self.commands if command[0] == "install_name_tool"
        )
        self.assertEqual(
            [
                "@rpath/MapLibreNativeC.framework/MapLibreNativeC",
                "@rpath/MapLibreNativeC.framework/MapLibreNativeC",
                "@rpath/MapLibreNativeC.framework/Versions/A/MapLibreNativeC",
            ],
            install_names,
        )
        xcodebuild = next(
            command for command in self.commands if command[0] == "xcodebuild"
        )
        self.assertEqual(xcodebuild.count("-framework"), len(PLATFORMS))
        self.assertEqual(
            self.commands[-1][:5] + [self.commands[-1][-1]],
            ["ditto", "-c", "-k", "--norsrc", "--keepParent", str(output)],
        )
        self.assertEqual(output.name, "MapLibreNativeC.xcframework.zip")

    def test_macos_framework_takes_the_versioned_layout(self) -> None:
        self.archives()
        self.assemble()

        framework = self.framework("macos-arm64-metal")
        self.assertEqual((framework / "Versions/Current").readlink(), pathlib.Path("A"))
        for item in ("MapLibreNativeC", "Headers", "Modules", "Resources"):
            self.assertEqual(
                (framework / item).readlink(), pathlib.Path(f"Versions/Current/{item}")
            )
        self.assertEqual(
            (framework / "MapLibreNativeC").read_bytes(), b"macos-arm64-metal"
        )
        self.assertEqual(
            (framework / "Headers/mln/plugin/plugin_api.h").read_bytes(), b"plugin"
        )
        module_map = (framework / "Modules/module.modulemap").read_text()
        self.assertIn('umbrella header "maplibre_native_c.h"', module_map)
        self.assertIn('header "maplibre_native_c/callback_adapter.h"', module_map)
        self.assertIn('exclude header "mln/plugin/plugin_api.h"', module_map)
        self.assertEqual((framework / "Resources/LICENSE").read_bytes(), b"license")
        self.assertEqual(
            (framework / "Resources/licenses/maplibre-native.md").read_bytes(),
            b"notice",
        )
        info = plistlib.loads((framework / "Resources/Info.plist").read_bytes())
        self.assertEqual(info["LSMinimumSystemVersion"], "14.3")
        self.assertEqual(info["CFBundleSupportedPlatforms"], ["MacOSX"])
        self.assertEqual(info["CFBundleVersion"], "0.202608.0")
        self.assertEqual(info["CFBundleInfoDictionaryVersion"], "6.0")

    def test_ios_frameworks_take_the_flat_layout(self) -> None:
        self.archives()
        self.assemble("0.0.0-dev.202608070101")

        for preset, platform in (
            ("ios-arm64-metal", "iPhoneOS"),
            ("ios-simulator-arm64-metal", "iPhoneSimulator"),
        ):
            framework = self.framework(preset)
            self.assertFalse((framework / "Versions").exists())
            self.assertEqual(
                (framework / "MapLibreNativeC").read_bytes(), preset.encode()
            )
            self.assertTrue((framework / "Headers/maplibre_native_c/base.h").is_file())
            self.assertEqual((framework / "LICENSE").read_bytes(), b"license")
            self.assertEqual(
                (framework / "licenses/maplibre-native.md").read_bytes(), b"notice"
            )
            info = plistlib.loads((framework / "Info.plist").read_bytes())
            self.assertEqual(info["MinimumOSVersion"], "15.5")
            self.assertEqual(info["CFBundleSupportedPlatforms"], [platform])
            self.assertEqual(info["CFBundleShortVersionString"], "0.0.0")

    def test_refuses_a_missing_package(self) -> None:
        self.archives()
        (self.input_dir / "maplibre-native-c-ios-arm64-metal.tar.gz").unlink()

        with self.assertRaisesRegex(
            SystemExit, "missing maplibre-native-c-ios-arm64-metal"
        ):
            self.assemble()

    def test_refuses_a_package_from_another_commit(self) -> None:
        self.archives("a" * 40)

        with self.assertRaisesRegex(SystemExit, "expected"):
            self.assemble()

    def test_bundle_version_drops_the_pre_release_suffix(self) -> None:
        self.assertEqual(bundle_version("0.202608.1"), "0.202608.1")
        self.assertEqual(bundle_version("0.0.0-dev.202608070101"), "0.0.0")


if __name__ == "__main__":
    unittest.main()
