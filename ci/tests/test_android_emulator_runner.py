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
        # others; every other adb command succeeds. Each command whose stdin
        # ends is logged to the file FAKE_STDIN_EOF_LOG names, when it is set.
        adb = sdk / "platform-tools/adb"
        exited = self.directory / "exited"
        adb.write_text(
            "#!/usr/bin/env bash\n"
            'if [[ -n "$FAKE_STDIN_EOF_LOG" ]]; then\n'
            # Bash 3.2 rejects a fractional `read -t`.
            "  perl -MIO::Select -e 'exit !(IO::Select->new(\\*STDIN)->can_read(0.1) && !sysread STDIN, $_, 1)' &&\n"
            '    echo "$*" >>"$FAKE_STDIN_EOF_LOG"\n'
            "fi\n"
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
        # The runner finds the device helper under the repository root, and
        # the emulator's log where devices-cli keeps it.
        (self.directory / "tools").symlink_to(ROOT / "tools")
        logs = self.directory / "state/devices-cli/logs"
        logs.mkdir(parents=True)
        (logs / "emulator-5554.log").write_text("ERROR | the renderer gave up\n")
        self.environment = os.environ | {
            "ANDROID_HOME": str(sdk),
            "ANDROID_SERIAL": "emulator-5554",
            "MLN_FFI_ANDROID_EMULATOR_API": "26",
            "MISE_MONOREPO_ROOT": str(self.directory),
            "XDG_STATE_HOME": str(self.directory / "state"),
            "FAKE_EMULATOR_EXITS": "",
            "FAKE_TEST_OUTPUT": "",
            "FAKE_TEST_ADB_STATUS": "0",
            "FAKE_PUSH_ADB_STATUS": "0",
            "FAKE_ABI_ADB_STATUS": "0",
            "FAKE_STDIN_EOF_LOG": "",
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
            # The GitHub Actions runner gives a step a stdin that has already
            # ended.
            stdin=subprocess.DEVNULL,
        )

    def test_no_adb_call_sees_its_stdin_end(self):
        eof_log = self.directory / "stdin-eof.log"
        eof_log.touch()
        result = self.run_test(
            FAKE_TEST_OUTPUT="\\n__MLN_FFI_TEST_STATUS__=0\\n",
            FAKE_STDIN_EOF_LOG=str(eof_log),
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(eof_log.read_text(), "")

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
        self.assertIn(
            "adb exited 255 during shell getprop ro.product.cpu.abi on Android device emulator-5554.",
            result.stderr,
        )


class AndroidSerialTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android serial ")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.sdk = sdk = self.directory / "sdk"
        (sdk / "platform-tools").mkdir(parents=True)
        (sdk / "emulator").mkdir()
        (sdk / "emulator/source.properties").write_text("Pkg.Revision=37.3.5\n")
        # Every device runs FAKE_ABI at API FAKE_API.
        self.write_command(
            sdk / "platform-tools/adb",
            'case "$*" in\n'
            '  *"getprop ro.product.cpu.abi") printf "%s\\r\\n" "$FAKE_ABI" ;;\n'
            '  *"getprop ro.build.version.sdk") printf "%s\\r\\n" "$FAKE_API" ;;\n'
            "esac\n",
        )
        commands = self.directory / "commands"
        commands.mkdir()
        self.arguments = self.directory / "arguments"
        self.write_command(
            commands / "devices-cli",
            f'printf "%s\\n" "$@" > "{self.arguments}"\necho emulator-5556\n',
        )
        self.environment = os.environ | {
            "ANDROID_HOME": str(sdk),
            "ANDROID_SERIAL": "",
            "MLN_FFI_ANDROID_EMULATOR_API": "36",
            "PATH": f"{commands}{os.pathsep}{os.environ['PATH']}",
            "FAKE_ABI": "x86_64",
            "FAKE_API": "36",
        }

    def write_command(self, path, body):
        path.write_text("#!/usr/bin/env bash\n" + body)
        path.chmod(0o755)

    def serial(self, *arguments, **environment):
        return subprocess.run(
            ["bash", str(ROOT / "tools/devices/android-serial"), *arguments],
            env=self.environment | environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )

    def test_devices_cli_boots_the_requested_emulator(self):
        result = self.serial("x86_64", "--api", "26", FAKE_API="26")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "emulator-5556\n")
        self.assertEqual(
            self.arguments.read_text().splitlines()[:7],
            ["get", "android", "--api", "26", "--abi", "x86_64", "--"],
        )

    def test_an_sdk_without_the_emulator_installs_it_without_a_jdk(self):
        (self.sdk / "emulator/source.properties").unlink()
        (self.sdk / "cmdline-tools/latest/bin").mkdir(parents=True)
        installed = self.directory / "installed"
        self.write_command(
            self.sdk / "cmdline-tools/latest/bin/android",
            f'printf "%s\\n" "$@" > "{installed}"\n',
        )
        result = self.serial("x86_64")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            installed.read_text().splitlines(),
            [f"--sdk={self.sdk}", "sdk", "install", "--canary", "emulator"],
        )
        self.assertEqual(result.stdout, "emulator-5556\n")

    def test_the_api_level_defaults_to_the_shared_one(self):
        result = self.serial("x86_64")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("--api\n36\n", self.arguments.read_text())

    def test_android_serial_wins(self):
        result = self.serial("x86_64", ANDROID_SERIAL="device-1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "device-1\n")
        self.assertFalse(self.arguments.exists())

    def test_a_device_with_another_abi_or_api_level_is_refused(self):
        for environment in ({"FAKE_ABI": "arm64-v8a"}, {"FAKE_API": "26"}):
            with self.subTest(**environment):
                result = self.serial("x86_64", ANDROID_SERIAL="device-1", **environment)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertEqual(result.stdout, "")
                self.assertIn("Android device device-1 runs", result.stderr)
                self.assertIn("the tests need x86_64 at API 36", result.stderr)


if __name__ == "__main__":
    unittest.main()
