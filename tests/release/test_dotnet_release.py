from __future__ import annotations

import unittest

from dotnet_release import runtime_targets


class RuntimeTargetTests(unittest.TestCase):
    def test_each_dotnet_platform_preset_maps_to_its_runtime_identifier(self) -> None:
        targets = runtime_targets()

        self.assertEqual(
            {"macos-arm64-metal": "osx-arm64", "macos-x64-metal": "osx-x64"},
            targets["Metal"],
        )
        self.assertEqual("win-x64", targets["OpenGL"]["windows-x64-wgl"])
        self.assertEqual("osx-arm64", targets["OpenGL"]["macos-arm64-egl"])
        self.assertEqual(
            "linux-musl-arm64", targets["Vulkan"]["linux-musl-arm64-vulkan"]
        )
        self.assertEqual("android-arm", targets["Vulkan"]["android-arm-vulkan"])
        presets = {preset for backend in targets.values() for preset in backend}
        self.assertEqual(
            set(),
            {
                preset
                for preset in presets
                if preset.startswith(("emscripten-", "ios-", "tvos-", "ohos-"))
            },
        )


if __name__ == "__main__":
    unittest.main()
