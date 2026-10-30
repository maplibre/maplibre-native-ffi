import json
import unittest

from tools.ci.plan import ROOT, targets
from tools.ci.suites import consumer_commands


class KotlinNativeAndroidTest(unittest.TestCase):
    def test_android_native_tests_run_only_on_emulator_targets(self):
        presets = json.loads((ROOT / "CMakePresets.json").read_text())
        found = {target.preset: target for target in targets(presets)}
        for backend in ("egl", "vulkan"):
            preset = f"android-x64-{backend}"
            commands = [
                " ".join(command) for command in consumer_commands(found[preset])
            ]
            self.assertIn(f"mise run //bindings/kotlin:test:native {preset}", commands)
            self.assertIn(f"mise run //bindings/kotlin:test {preset}", commands)
            arm = [
                " ".join(command)
                for command in consumer_commands(found[f"android-arm64-{backend}"])
            ]
            self.assertFalse(
                any("//bindings/kotlin:test:native" in line for line in arm)
            )
