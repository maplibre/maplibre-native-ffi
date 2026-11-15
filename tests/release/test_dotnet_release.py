from __future__ import annotations

import pathlib
import plistlib
import stat
import tempfile
import unittest
import zipfile

from dotnet_release import copy_apple_framework, runtime_targets


class RuntimeTargetTests(unittest.TestCase):
    def test_each_dotnet_platform_preset_maps_to_its_runtime_identifier(self) -> None:
        targets = runtime_targets()

        self.assertEqual({"", ".MinGW"}, set(targets))
        runtime = targets[""]
        for preset, rid in {
            "macos-arm64": "osx-arm64",
            "macos-x64": "osx-x64",
            "ios-arm64": "ios-arm64",
            "ios-simulator-arm64": "iossimulator-arm64",
            "ios-maccatalyst-arm64": "maccatalyst-arm64",
            "tvos-arm64": "tvos-arm64",
            "tvos-simulator-arm64": "tvossimulator-arm64",
            "windows-x64": "win-x64",
            "linux-gnu-x64": "linux-x64",
            "linux-musl-arm64": "linux-musl-arm64",
            "android-arm": "android-arm",
            "android-x86": "android-x86",
        }.items():
            self.assertEqual(rid, runtime[preset], preset)
        presets = {preset for package in targets.values() for preset in package}
        self.assertEqual(
            set(),
            {
                preset
                for preset in presets
                if preset.startswith(("web-", "visionos-", "ohos-"))
            },
        )

    def test_mingw_runtime_package_does_not_share_msvc_assets(self) -> None:
        targets = runtime_targets()
        self.assertEqual(
            {"mingw-x64": "win-x64", "mingw-arm64": "win-arm64"}, targets[".MinGW"]
        )
        for arch in ("x64", "arm64"):
            self.assertNotIn(f"mingw-{arch}", targets[""])
            self.assertEqual(f"win-{arch}", targets[""][f"windows-{arch}"])
        for package, presets in targets.items():
            self.assertEqual(len(presets), len(set(presets.values())), package)


class AppleFrameworkTests(unittest.TestCase):
    def test_catalyst_zip_preserves_versioned_framework_symlinks(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            framework = root / "install/Frameworks/MaplibreNativeC.framework"
            binary = framework / "Versions/A/MaplibreNativeC"
            binary.parent.mkdir(parents=True)
            binary.write_bytes(b"native library")
            binary.chmod(0o755)
            (framework / "Versions/Current").symlink_to("A", target_is_directory=True)
            (framework / "MaplibreNativeC").symlink_to(
                "Versions/Current/MaplibreNativeC"
            )
            copy_apple_framework(
                root / "install", root / "output", "ios-maccatalyst-arm64"
            )
            with zipfile.ZipFile(
                root / "output/MaplibreNativeC.xcframework.zip"
            ) as archive:
                info = plistlib.loads(
                    archive.read("MaplibreNativeC.xcframework/Info.plist")
                )
                library = info["AvailableLibraries"][0]
                self.assertEqual("ios", library["SupportedPlatform"])
                self.assertEqual("maccatalyst", library["SupportedPlatformVariant"])
                prefix = "MaplibreNativeC.xcframework/ios-arm64-maccatalyst/MaplibreNativeC.framework/"
                link = archive.getinfo(prefix + "MaplibreNativeC")
                self.assertTrue(stat.S_ISLNK(link.external_attr >> 16))
                self.assertEqual(
                    b"Versions/Current/MaplibreNativeC", archive.read(link)
                )
                self.assertEqual(
                    b"native library",
                    archive.read(prefix + "Versions/A/MaplibreNativeC"),
                )


if __name__ == "__main__":
    unittest.main()
