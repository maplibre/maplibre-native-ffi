import os
import shutil
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
        # The runner finds the emulator script under the repository root, and
        # the emulator's log in the temporary directory.
        (self.directory / "tools").symlink_to(ROOT / "tools")
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
        self.assertIn("adb exited 255 during the ABI query.", result.stderr)
        self.assertIn("the adbd line", result.stderr)


class AndroidEmulatorTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android emulator ")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        sdk = self.directory / "sdk"
        (sdk / "platform-tools").mkdir(parents=True)
        # Booted emulators run the shared x86_64 AVDs at their ports, and an
        # unrelated AVD holds the port of the API 26 arm64-v8a one. Their
        # consoles answer at once, but the adb server, which has just started,
        # finds no device until `adb devices` has awaited its first scan.
        scanned = self.directory / "scanned"
        adb = sdk / "platform-tools/adb"
        adb.write_text(
            "#!/usr/bin/env bash\n"
            'case "$*" in\n'
            f'  devices) touch "{scanned}" ;;\n'
            f'  *"getprop sys.boot_completed"*) [[ -e "{scanned}" ]] && echo 1 ;;\n'
            '  "-s emulator-5582 emu avd name") printf "api-36-x86_64\\r\\nOK\\r\\n" ;;\n'
            '  "-s emulator-5580 emu avd name") printf "api-26-x86_64\\r\\nOK\\r\\n" ;;\n'
            '  "-s emulator-5578 emu avd name") printf "pixel-fold\\r\\nOK\\r\\n" ;;\n'
            "  *) exit 1 ;;\n"
            "esac\n"
        )
        adb.chmod(0o755)
        self.commands = self.directory / "commands"
        self.commands.mkdir()
        self.environment = os.environ | {
            "ANDROID_HOME": str(sdk),
            "ANDROID_SERIAL": "",
            "MLN_FFI_ANDROID_EMULATOR_API": "36",
            "PATH": f"{self.commands}{os.pathsep}{os.environ['PATH']}",
        }
        self.environment.pop("MLN_FFI_ANDROID_DIAGNOSTICS_DIR", None)
        self.script = ROOT / "tools/devices/android-emulator"

    def write_command(self, name, body):
        command = self.commands / name
        command.write_text("#!/usr/bin/env bash\n" + body)
        command.chmod(0o755)

    def emulator(self, *arguments):
        return subprocess.run(
            ["bash", str(self.script), *arguments],
            env=self.environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )

    def test_each_api_level_has_its_own_port(self):
        result = self.emulator("serial", "x86_64", "--api", "26")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "emulator-5580\n")

    def test_the_api_level_defaults_to_the_shared_one(self):
        result = self.emulator("serial", "x86_64")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "emulator-5582\n")

    def test_no_emulator_runs_at_the_port(self):
        result = self.emulator("serial", "arm64-v8a")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(result.stdout, "")

    def test_a_foreign_avd_at_the_port_is_named(self):
        result = self.emulator("serial", "arm64-v8a", "--api", "26")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(result.stdout, "")
        self.assertIn("emulator-5578 runs the AVD pixel-fold", result.stderr)

    def test_an_unsupported_pair_names_the_supported_ones(self):
        result = self.emulator("serial", "x86_64", "--api", "30")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("API 30 x86_64", result.stderr)
        self.assertIn("API 26 arm64-v8a", result.stderr)

    def test_the_avd_running_at_another_port_is_not_adopted(self):
        # An older checkout booted the AVD at the first free port.
        self.write_command("pgrep", "echo 4242\n")
        self.write_command(
            "ps", "echo qemu-system-aarch64 -avd api-36-arm64-v8a -port 5554\n"
        )
        result = self.emulator("boot", "arm64-v8a")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn(
            "api-36-arm64-v8a already runs as process 4242 at emulator-5554",
            result.stderr,
        )
        self.assertIn("android-emulator:stop arm64-v8a --api 36", result.stderr)

    def test_a_failed_collector_preserves_readiness_failure_and_stdout(self):
        (self.directory / "tools/devices").mkdir(parents=True)
        (self.directory / "ci").mkdir()
        self.script = self.directory / "tools/devices/android-emulator"
        shutil.copyfile(ROOT / "tools/devices/android-emulator", self.script)
        # Simulate SIGALRM on each adb call without making the test wait thirty
        # seconds.
        self.write_command(
            "perl",
            f'[[ "$4" != */adb ]] || exit 142\nexec "{shutil.which("perl")}" "$@"\n',
        )
        collector = self.directory / "ci/android_emulator_diagnostics.py"
        collector.write_text(
            "import sys\n"
            "print('collector called:', sys.argv[1:])\n"
            "raise SystemExit(1)\n"
        )
        result = self.emulator("serial", "x86_64", "--api", "26")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertNotIn("collector called", result.stderr)
        self.environment["MLN_FFI_ANDROID_DIAGNOSTICS_DIR"] = str(
            self.directory / "diagnostics"
        )
        result = self.emulator("serial", "x86_64", "--api", "26")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertIn(
            "adb -s emulator-5580 emu avd name missed its 30-second deadline",
            result.stderr,
        )
        self.assertIn(
            "collector called: ['api-26-x86_64', '-s', 'emulator-5580', 'emu', 'avd', 'name']",
            result.stderr,
        )
        self.assertIn("emulator diagnostics failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
