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
