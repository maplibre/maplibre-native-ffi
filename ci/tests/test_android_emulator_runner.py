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
        # The ABI query ends as FAKE_ABI_ADB_STATUS says. The test's own
        # command ends as FAKE_TEST_ADB_STATUS says, after it
        # prints FAKE_TEST_OUTPUT, and when FAKE_EMULATOR_EXITS is set, the
        # emulator's console stops naming its AVD from then on. A push ends as
        # FAKE_PUSH_ADB_STATUS says. The device log holds an adbd line among
        # others; every other adb command succeeds.
        adb = sdk / "platform-tools/adb"
        exited = self.directory / "exited"
        adb.write_text(
            "#!/usr/bin/env bash\n"
            'case "$*" in\n'
            '  *"getprop ro.product.cpu.abi"*) echo x86_64; exit "$FAKE_ABI_ADB_STATUS" ;;\n'
            '  *"getprop ro.build.version.sdk"*) echo 26 ;;\n'
            f'  *"emu avd name"*) [[ ! -e "{exited}" ]] || exit 1; echo api-26-x86_64 ;;\n'
            "  *__MLN_FFI_TEST_STATUS__*)\n"
            f'    [[ -z "$FAKE_EMULATOR_EXITS" ]] || touch "{exited}"\n'
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
        # The runner finds the serial script under the repository root, and
        # the emulator's log in the temporary directory.
        (self.directory / "scripts").symlink_to(ROOT / "scripts")
        (self.directory / "android-emulator-api-26-x86_64.log").write_text(
            "ERROR | the renderer gave up\n"
        )
        self.environment = os.environ | {
            "ANDROID_HOME": str(sdk),
            "ANDROID_SERIAL": "emulator-5554",
            "MISE_MONOREPO_ROOT": str(self.directory),
            "TMPDIR": str(self.directory),
            "FAKE_EMULATOR_EXITS": "",
            "FAKE_TEST_OUTPUT": "",
            "FAKE_TEST_ADB_STATUS": "0",
            "FAKE_PUSH_ADB_STATUS": "0",
            "FAKE_ABI_ADB_STATUS": "0",
        }

    def run_test(self, **environment):
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

    def test_a_lost_emulator_reports_the_end_of_its_log(self):
        result = self.run_test(FAKE_EMULATOR_EXITS="1", FAKE_TEST_ADB_STATUS="255")
        self.assertEqual(result.returncode, 255, result.stderr)
        self.assertIn("test returned no exit status; adb exited 255.", result.stderr)
        self.assertIn("emulator at emulator-5554 exited", result.stderr)
        self.assertIn("the renderer gave up", result.stderr)
        self.assertIn("the adbd line", result.stderr)

    def test_a_running_emulator_is_reported_without_its_log(self):
        result = self.run_test(FAKE_TEST_ADB_STATUS="255")
        self.assertEqual(result.returncode, 255, result.stderr)
        self.assertIn("emulator at emulator-5554 is still running", result.stderr)
        self.assertNotIn("the renderer gave up", result.stderr)

    def test_a_failing_test_exits_with_its_status(self):
        result = self.run_test(FAKE_TEST_OUTPUT="\\n__MLN_FFI_TEST_STATUS__=3\\n")
        self.assertEqual(result.returncode, 3, result.stderr)
        self.assertNotIn("no exit status", result.stderr)
        self.assertNotIn("the adbd line", result.stderr)

    def test_a_lost_transport_after_a_reported_status_names_the_shell(self):
        result = self.run_test(
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
        result = self.run_test(FAKE_PUSH_ADB_STATUS="1")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn(
            "adb exited 1 during the push of libmaplibre-native-c.so.", result.stderr
        )
        self.assertIn("the adbd line", result.stderr)

    def test_a_failed_device_query_exits_with_the_adb_status(self):
        result = self.run_test(FAKE_ABI_ADB_STATUS="255")
        self.assertEqual(result.returncode, 255, result.stderr)
        self.assertIn("adb exited 255 during the ABI query.", result.stderr)
        self.assertIn("the adbd line", result.stderr)


class AndroidDeviceSerialTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android serial ")
        self.addCleanup(temporary.cleanup)
        sdk = Path(temporary.name) / "sdk"
        (sdk / "platform-tools").mkdir(parents=True)
        # Two booted emulators run the shared AVDs of one ABI at two API levels.
        adb = sdk / "platform-tools/adb"
        adb.write_text(
            "#!/usr/bin/env bash\n"
            'case "$*" in\n'
            "  devices) printf 'List of devices attached\\nemulator-5554\\tdevice\\nemulator-5556\\tdevice\\n' ;;\n"
            '  "-s emulator-5554 emu avd name") printf "api-36-x86_64\\r\\nOK\\r\\n" ;;\n'
            '  "-s emulator-5556 emu avd name") printf "api-26-x86_64\\r\\nOK\\r\\n" ;;\n'
            '  *"getprop sys.boot_completed"*) echo 1 ;;\n'
            "  *) exit 1 ;;\n"
            "esac\n"
        )
        adb.chmod(0o755)
        self.environment = os.environ | {
            "ANDROID_HOME": str(sdk),
            "ANDROID_SERIAL": "",
            "MLN_FFI_ANDROID_EMULATOR_API": "36",
        }

    def serial(self, *arguments):
        return subprocess.run(
            ["bash", str(ROOT / "scripts/android-device-serial.sh"), *arguments],
            env=self.environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )

    def test_the_emulator_that_runs_the_api_level_is_chosen(self):
        result = self.serial("x86_64", "26")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "emulator-5556\n")

    def test_the_api_level_defaults_to_the_shared_one(self):
        result = self.serial("x86_64")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "emulator-5554\n")

    def test_no_emulator_runs_another_abi(self):
        result = self.serial("arm64-v8a", "26")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(result.stdout, "")


if __name__ == "__main__":
    unittest.main()
