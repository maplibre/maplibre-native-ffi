import unittest

from tools.ci.suites import android_commands


class KotlinNativeAndroidTest(unittest.TestCase):
    def test_android_native_tests_run_only_on_emulator_targets(self):
        for backend in ("egl", "vulkan"):
            preset = f"android-x64-{backend}"
            commands = android_commands(preset, "x86_64")
            self.assertIn(f"mise run //bindings/kotlin:test:native {preset}", commands)
            self.assertIn(f"mise run //bindings/kotlin:test {preset}", commands)
            arm = android_commands(f"android-arm64-{backend}", "arm64-v8a")
            self.assertFalse(
                any("//bindings/kotlin:test:native" in line for line in arm)
            )
