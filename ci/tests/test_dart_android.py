import json
import unittest

from tools.ci.plan import ROOT, targets
from tools.ci.suites import consumer_commands


class DartAndroidTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        presets = json.loads((ROOT / "CMakePresets.json").read_text())
        cls.targets = {target.preset: target for target in targets(presets)}

    def test_dart_android_runs_on_emulator_targets_and_builds_for_arm(self):
        for preset, commands in (
            (
                "android-x64",
                [
                    "mise run //bindings/dart:test:android android-x64 vulkan",
                    "mise run //bindings/dart:test:android android-x64 opengl",
                ],
            ),
            ("android-arm64", ["mise run //bindings/dart:build:mobile android-arm64"]),
        ):
            self.assertEqual(
                [
                    " ".join(command)
                    for command, _ in consumer_commands(self.targets[preset])
                    if "//bindings/dart:" in command[2]
                ],
                commands,
            )

    def test_android_x86_has_no_flutter_target(self):
        self.assertFalse(
            any(
                "//bindings/dart:" in command[2]
                for command, _ in consumer_commands(self.targets["android-x86"])
            )
        )
