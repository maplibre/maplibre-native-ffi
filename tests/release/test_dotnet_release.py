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

        self.assertEqual(
            {
                "macos-arm64-metal": "osx-arm64",
                "macos-x64-metal": "osx-x64",
                "ios-arm64-metal": "ios-arm64",
                "ios-simulator-arm64-metal": "iossimulator-arm64",
                "ios-maccatalyst-arm64-metal": "maccatalyst-arm64",
                "tvos-arm64-metal": "tvos-arm64",
                "tvos-simulator-arm64-metal": "tvossimulator-arm64",
            },
            targets["Metal"],
        )
        self.assertEqual("win-x64", targets["OpenGL"]["windows-x64-wgl"])
        self.assertEqual("osx-arm64", targets["OpenGL"]["macos-arm64-egl"])
        self.assertEqual(
            "linux-musl-arm64", targets["Vulkan"]["linux-musl-arm64-vulkan"]
        )
        self.assertEqual("android-arm", targets["Vulkan"]["android-arm-vulkan"])
        self.assertEqual("android-x86", targets["OpenGL"]["android-x86-egl"])
        self.assertEqual("android-x86", targets["Vulkan"]["android-x86-vulkan"])
        presets = {preset for backend in targets.values() for preset in backend}
        self.assertEqual(
            set(),
            {
                preset
                for preset in presets
                if preset.startswith(("emscripten-", "visionos-", "ohos-"))
            },
        )


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
                root / "install", root / "output", "ios-maccatalyst-arm64-metal"
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
