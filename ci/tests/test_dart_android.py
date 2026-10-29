import json
import unittest

from tools.ci.plan import ROOT, targets
from tools.ci.suites import android_commands


class DartAndroidTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        presets = json.loads((ROOT / "CMakePresets.json").read_text())
        cls.targets = {target.preset: target for target in targets(presets)}

    def test_dart_android_runs_on_emulator_targets_and_builds_for_arm(self):
        for backend in ("egl", "vulkan"):
            for arch, abi, action in (
                ("x64", "x86_64", "test:android"),
                ("arm64", "arm64-v8a", "build:mobile"),
            ):
                preset = f"android-{arch}-{backend}"
                self.assertIn(
                    f"mise run //bindings/dart:{action} {preset}",
                    android_commands(self.targets[preset], abi),
                )

    def test_android_x86_has_no_flutter_target(self):
        for backend in ("egl", "vulkan"):
            preset = f"android-x86-{backend}"
            self.assertFalse(
                any(
                    "//bindings/dart:" in command
                    for command in android_commands(self.targets[preset], "x86")
                )
            )
