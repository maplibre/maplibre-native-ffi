import os
import subprocess
import tempfile
import tomllib
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class AndroidPackagesTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android packages ")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.sdk = self.directory / "selected sdk"
        self.capture = self.directory / "sdkmanager arguments"
        catalog = tomllib.loads((ROOT / "gradle/libs.versions.toml").read_text())
        for package in (
            "cmake/test-cmake",
            f"platforms/android-{catalog['versions']['android-compileSdk']}",
            "platform-tools",
        ):
            (self.sdk / package).mkdir(parents=True)
        self.environment = os.environ | {
            "ANDROID_HOME": str(self.directory / "unselected sdk"),
            "ANDROID_SDK_ROOT": str(self.directory / "another sdk"),
            "MISE_TOOL_INSTALL_PATH": str(self.directory / "postinstall sdk"),
            "CAPTURE_PATH": str(self.capture),
        }

    def provision(self):
        return subprocess.run(
            [
                "bash",
                str(ROOT / ".mise/bin/sync-android-packages"),
                str(self.sdk),
                "test-java",
                "test-ndk",
                "test-cmake",
            ],
            env=self.environment,
            check=False,
            capture_output=True,
            text=True,
        )

    def test_complete_selected_sdk_is_reused_without_command_line_tools(self):
        (self.sdk / "ndk/test-ndk").mkdir(parents=True)
        result = self.provision()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "")

    def test_missing_packages_are_installed_into_the_selected_sdk(self):
        tools = self.sdk / "cmdline-tools/latest/bin"
        tools.mkdir(parents=True)
        sdkmanager = tools / "sdkmanager"
        sdkmanager.write_text(
            '#!/usr/bin/env bash\nprintf "%s\\n" "$@" > "$CAPTURE_PATH"\n'
        )
        sdkmanager.chmod(0o755)
        commands = self.directory / "commands"
        commands.mkdir()
        mise = commands / "mise"
        mise.write_text(
            "#!/usr/bin/env bash\n"
            'while [[ "$1" != -- ]]; do shift; done\n'
            "shift\n"
            'exec "$@"\n'
        )
        mise.chmod(0o755)
        self.environment["PATH"] = f"{commands}{os.pathsep}{os.defpath}"
        result = self.provision()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            self.capture.read_text().splitlines(),
            [f"--sdk_root={self.sdk}", "--install", "ndk;test-ndk"],
        )


class AndroidEmulatorPackagesTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android emulator packages ")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.sdk = self.directory / "selected sdk"
        tools = self.sdk / "cmdline-tools/latest/bin"
        tools.mkdir(parents=True)
        commands = self.directory / "commands"
        commands.mkdir()
        self.write_command(
            tools / "sdkmanager",
            'attempt=$(($(cat "$CAPTURE_PATH/count" 2>/dev/null || printf 0) + 1))\n'
            'echo "$attempt" > "$CAPTURE_PATH/count"\n'
            'printf "%s\\n" "$@" > "$CAPTURE_PATH/arguments-$attempt"\n'
            "cat >/dev/null\n"
            "if ((attempt <= SDK_FAILURES)); then exit 17; fi\n",
        )
        # Stop at AVD creation so the tests exercise provisioning without a device.
        self.write_command(
            tools / "avdmanager", 'touch "$CAPTURE_PATH/avd-reached"\nexit 73\n'
        )
        # Retries back off without waiting.
        self.write_command(commands / "sleep", "")
        self.environment = os.environ | {
            "ANDROID_HOME": str(self.sdk),
            "MISE_MONOREPO_ROOT": str(self.directory),
            "CAPTURE_PATH": str(self.directory),
            "PATH": f"{commands}{os.pathsep}{os.defpath}",
        }

    def write_command(self, path, body):
        path.write_text("#!/usr/bin/env bash\n" + body)
        path.chmod(0o755)

    def provision(self, failures):
        return subprocess.run(
            ["bash", str(ROOT / "scripts/boot-android-emulator.sh"), "26", "x86_64"],
            env=self.environment | {"SDK_FAILURES": str(failures)},
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )

    def test_transient_download_failures_recover_before_avd_creation(self):
        result = self.provision(failures=2)
        self.assertEqual(result.returncode, 73, result.stderr)
        self.assertTrue((self.directory / "avd-reached").exists())
        for attempt in range(1, 4):
            self.assertEqual(
                (self.directory / f"arguments-{attempt}").read_text().splitlines(),
                [
                    f"--sdk_root={self.sdk}",
                    "--install",
                    "platform-tools",
                    "emulator",
                    "system-images;android-26;default;x86_64",
                ],
            )

    def test_persistent_download_failures_stop_with_the_sdkmanager_status(self):
        result = self.provision(failures=3)
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertEqual((self.directory / "count").read_text().strip(), "3")
        self.assertFalse((self.directory / "avd-reached").exists())
        self.assertIn("installation failed after 3 attempts", result.stderr)
