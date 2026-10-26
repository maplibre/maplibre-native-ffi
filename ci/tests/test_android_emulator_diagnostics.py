import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from ci.android_emulator_diagnostics import capture, emulator_processes


class AndroidEmulatorDiagnosticsTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="android diagnostics ")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.proc = self.root / "proc"
        self.destination = self.root / "diagnostics"
        self.sdk = self.root / "sdk"
        for package in ("emulator", "platform-tools"):
            (self.sdk / package).mkdir(parents=True)
            (self.sdk / package / "source.properties").write_text("Pkg.Revision=test\n")
        self.environment = {
            "MLN_FFI_ANDROID_DIAGNOSTICS_DIR": str(self.destination),
            "ANDROID_HOME": str(self.sdk),
            "TMPDIR": str(self.root),
        }
        self.process(100, "api-26-x86_64", 5554)
        self.process(200, "api-36-x86_64", 5556)
        (self.root / "android-emulator-api-36-x86_64.log").write_text("emulator log\n")

    def process(self, pid, avd, port, started=1234):
        directory = self.proc / str(pid)
        directory.mkdir(parents=True, exist_ok=True)
        arguments = [
            "/sdk/emulator/qemu-system-x86_64-headless",
            "-avd",
            avd,
            "-port",
            str(port),
        ]
        (directory / "cmdline").write_text("\0".join(arguments) + "\0")
        (directory / "stat").write_text(
            f"{pid} (qemu system) S " + "0 " * 18 + f"{started} 0"
        )
        (directory / "status").write_text("State: S (sleeping)\n")

    def test_serial_selects_its_process_even_when_another_avd_was_requested(self):
        matches = list(
            emulator_processes(
                self.proc,
                "api-26-x86_64",
                ["-s", "emulator-5556", "emu", "avd", "name"],
            )
        )
        self.assertEqual(
            [(p.name, avd, start) for p, avd, start, _ in matches],
            [("200", "api-36-x86_64", "1234")],
        )

    def test_devices_timeout_selects_only_the_requested_avd(self):
        matches = list(emulator_processes(self.proc, "api-26-x86_64", ["devices"]))
        self.assertEqual([p.name for p, *_ in matches], ["100"])

    def test_non_emulator_and_exited_processes_are_ignored(self):
        (self.proc / "100/cmdline").write_text("/bin/sleep\0-avd\0api-26-x86_64\0")
        (self.proc / "200/stat").unlink()
        self.assertEqual(
            list(emulator_processes(self.proc, "api-26-x86_64", ["devices"])), []
        )

    def test_capture_is_once_per_process_lifetime_even_when_gdb_fails(self):
        with (
            patch.dict(os.environ, self.environment),
            patch("ci.android_emulator_diagnostics.subprocess.run") as run,
        ):
            run.return_value.returncode = 1
            arguments = ["-s", "emulator-5556", "emu", "avd", "name"]
            capture("api-26-x86_64", arguments, self.proc)
            capture("api-26-x86_64", arguments, self.proc)
            self.assertEqual(run.call_count, 2)  # ps and GDB, once each
            directory = self.destination / "200-1234"
            self.assertEqual((directory / "emulator.log").read_text(), "emulator log\n")
            self.assertEqual(
                (directory / "emulator-version.txt").read_text(), "Pkg.Revision=test\n"
            )
            self.assertIn("exit status: 1", (directory / "backtrace.txt").read_text())
            self.assertIn("emu avd name", (directory / "context.txt").read_text())
            self.assertFalse((self.destination / "100-1234").exists())
            command = run.call_args.args[0]
            self.assertEqual(
                command[:5], ["sudo", "-n", "timeout", "--kill-after=5s", "20s"]
            )
            self.assertIn("thread apply all bt 40", command)
            self.assertIn("detach", command)
            self.process(200, "api-36-x86_64", 5556, started=5678)
            capture("api-26-x86_64", arguments, self.proc)
            self.assertEqual(run.call_count, 4)

    def test_diagnostic_timeouts_are_recorded_without_preventing_the_next_capture(self):
        with (
            patch.dict(os.environ, self.environment),
            patch(
                "ci.android_emulator_diagnostics.subprocess.run",
                side_effect=subprocess.TimeoutExpired("probe", 5),
            ),
        ):
            capture("api-26-x86_64", ["devices"], self.proc)
        directory = self.destination / "100-1234"
        self.assertIn("Thread listing failed", (directory / "threads.txt").read_text())
        self.assertIn("GDB command failed", (directory / "backtrace.txt").read_text())

    def test_disappeared_emulator_leaves_a_diagnostic(self):
        with patch.dict(os.environ, self.environment):
            capture("missing", ["devices"], self.proc)
        self.assertIn(
            "No matching emulator process",
            (self.destination / "missing-emulator.txt").read_text(),
        )
