import json
import unittest

from tools.ci.plan import ROOT, targets
from tools.ci.suites import consumer_commands


class KotlinNativeAndroidTest(unittest.TestCase):
    def test_android_native_tests_run_only_on_emulator_targets(self):
        presets = json.loads((ROOT / "CMakePresets.json").read_text())
        found = {target.preset: target for target in targets(presets)}
        commands = [
            " ".join(command) for command, _ in consumer_commands(found["android-x64"])
        ]
        for backend in ("vulkan", "opengl"):
            self.assertIn(
                f"mise run //bindings/kotlin:test:native android-x64 {backend}",
                commands,
            )
            self.assertIn(
                f"mise run //bindings/kotlin:test:android android-x64 {backend}",
                commands,
            )
        arm = [
            " ".join(command)
            for command, _ in consumer_commands(found["android-arm64"])
        ]
        self.assertFalse(any("//bindings/kotlin:test:native" in line for line in arm))
