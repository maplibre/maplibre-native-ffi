import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class AndroidEmulatorRunnerTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android runner ")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        sdk = self.directory / "sdk"
        (sdk / "platform-tools").mkdir(parents=True)
        # The test's own command ends as FAKE_TEST_ADB_STATUS says, after it
        # prints FAKE_TEST_OUTPUT, and a push ends as FAKE_PUSH_ADB_STATUS says.
        # The device log holds an adbd line among others; every other adb
        # command succeeds.
        adb = sdk / "platform-tools/adb"
        adb.write_text(
            "#!/usr/bin/env bash\n"
            'case "$*" in\n'
            '  *"getprop ro.product.cpu.abi"*) echo x86_64 ;;\n'
            '  *"getprop ro.build.version.sdk"*) echo 26 ;;\n'
            "  *__MLN_FFI_TEST_STATUS__*)\n"
            '    printf "%b" "$FAKE_TEST_OUTPUT"\n'
            '    exit "$FAKE_TEST_ADB_STATUS" ;;\n'
            '  *" push "*) exit "$FAKE_PUSH_ADB_STATUS" ;;\n'
            '  *" logcat "*)\n'
            '    echo "I adbd    : the adbd line"\n'
            '    echo "I vold    : the last line" ;;\n'
            "esac\n"
        )
        adb.chmod(0o755)
        for name in ("libmaplibre-native-c.so", "test"):
            (self.directory / name).write_text("")
        # The runner finds the serial script and the emulator's state under the
        # repository root.
        (self.directory / "scripts").symlink_to(ROOT / "scripts")
        self.state = self.directory / "build/android-emulator/mln-ffi-api-26-x86-64"
        self.state.mkdir(parents=True)
        (self.state / "serial").write_text("emulator-5554\n")
        (self.state / "emulator.log").write_text("ERROR | the renderer gave up\n")
        self.environment = os.environ | {
            "ANDROID_HOME": str(sdk),
            "ANDROID_SERIAL": "emulator-5554",
            "MISE_MONOREPO_ROOT": str(self.directory),
            "FAKE_TEST_OUTPUT": "",
            "FAKE_TEST_ADB_STATUS": "0",
            "FAKE_PUSH_ADB_STATUS": "0",
        }

    def run_test(self, emulator_pid, **environment):
        (self.state / "emulator.pid").write_text(f"{emulator_pid}\n")
        return subprocess.run(
            [
                "bash",
                str(ROOT / "scripts/run-android-emulator-test.sh"),
                "10",
                "x86_64",
                str(self.directory / "libmaplibre-native-c.so"),
                "--",
                str(self.directory / "test"),
            ],
            env=self.environment | environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )

    def exited_pid(self):
        process = subprocess.Popen(["true"])
        process.wait()
        return process.pid

    def test_a_lost_emulator_reports_the_end_of_its_log(self):
        result = self.run_test(self.exited_pid(), FAKE_TEST_ADB_STATUS="255")
        self.assertEqual(result.returncode, 255, result.stderr)
        self.assertIn("test returned no exit status; adb exited 255.", result.stderr)
        self.assertIn("emulator at emulator-5554 exited", result.stderr)
        self.assertIn("the renderer gave up", result.stderr)
        self.assertIn("the adbd line", result.stderr)

    def test_a_running_emulator_is_reported_without_its_log(self):
        result = self.run_test(os.getpid(), FAKE_TEST_ADB_STATUS="255")
        self.assertEqual(result.returncode, 255, result.stderr)
        self.assertIn("emulator at emulator-5554 is still running", result.stderr)
        self.assertNotIn("the renderer gave up", result.stderr)

    def test_a_failing_test_exits_with_its_status(self):
        result = self.run_test(
            os.getpid(), FAKE_TEST_OUTPUT="\\n__MLN_FFI_TEST_STATUS__=3\\n"
        )
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertNotIn("no exit status", result.stderr)
        self.assertNotIn("the adbd line", result.stderr)

    def test_a_lost_transport_after_a_reported_status_names_the_shell(self):
        result = self.run_test(
            os.getpid(),
            FAKE_TEST_OUTPUT="\\n__MLN_FFI_TEST_STATUS__=0\\n",
            FAKE_TEST_ADB_STATUS="255",
        )
        self.assertEqual(result.returncode, 255, result.stderr)
        self.assertIn(
            "adb exited 255 during the shell that ran test, after it reported exit status 0.",
            result.stderr,
        )
        self.assertIn("emulator at emulator-5554 is still running", result.stderr)
        self.assertIn("the adbd line", result.stderr)
        self.assertIn("the last line", result.stderr)

    def test_a_failed_push_names_what_it_pushed(self):
        result = self.run_test(os.getpid(), FAKE_PUSH_ADB_STATUS="1")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn(
            "adb exited 1 during the push of libmaplibre-native-c.so.", result.stderr
        )
        self.assertIn("the adbd line", result.stderr)


if __name__ == "__main__":
    unittest.main()
