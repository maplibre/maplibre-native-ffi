import unittest

from ci.workflow import android_commands


class DartAndroidTest(unittest.TestCase):
    def test_dart_android_runs_on_emulator_targets_and_builds_for_arm(self):
        for backend in ("egl", "vulkan"):
            for arch, abi, action in (
                ("x64", "x86_64", "test:android"),
                ("arm64", "arm64-v8a", "build:mobile"),
            ):
                preset = f"android-{arch}-{backend}"
                self.assertIn(
                    f"mise run //bindings/dart:{action} {preset}",
                    android_commands(preset, abi),
                )
