"""Capture a Linux CI emulator's threads on its first readiness timeout."""

from __future__ import annotations

import itertools
import os
import shlex
import subprocess
import sys
from datetime import UTC, datetime
from pathlib import Path


def emulator_processes(proc: Path, avd: str, adb_arguments: list[str]):
    # Boot launches QEMU with an explicit console port. Match that port for a
    # serial-specific call, even when it names another AVD; `adb devices` has
    # no serial, so use the AVD the caller is waiting for instead.
    serial = adb_arguments[1] if adb_arguments[:1] == ["-s"] else None
    for entry in proc.glob("[0-9]*/cmdline"):
        try:
            arguments = os.fsdecode(entry.read_bytes()).rstrip("\0").split("\0")
            if not Path(arguments[0]).name.startswith("qemu-system-"):
                continue
            options = dict(itertools.pairwise(arguments))
            name = options.get("-avd") or next(
                (arg[1:] for arg in arguments if arg.startswith("@")), None
            )
            if not name:
                continue
            if serial:
                if serial != f"emulator-{options.get('-port')}":
                    continue
            elif name != avd:
                continue
            # PID alone can be reused after a restart. Field 22 is the start
            # time, after the parenthesized process name (which can contain spaces).
            started = (entry.parent / "stat").read_text().rsplit(")", 1)[1].split()[19]
            yield entry.parent, name, started, arguments
        except OSError, IndexError:
            # A process may disappear while /proc is being read.
            continue


def capture(avd: str, adb_arguments: list[str], proc: Path = Path("/proc")) -> None:
    destination = Path(os.environ["MLN_FFI_ANDROID_DIAGNOSTICS_DIR"])
    destination.mkdir(parents=True, exist_ok=True)
    found = False
    for process, name, started, arguments in emulator_processes(
        proc, avd, adb_arguments
    ):
        found = True
        directory = destination / f"{process.name}-{started}"
        try:
            directory.mkdir()
        except FileExistsError:
            continue
        # mkdir claims the capture across separate readiness processes. Keep
        # the directory even if GDB fails, so retries cannot repeatedly attach.
        (directory / "context.txt").write_text(
            f"Time: {datetime.now(UTC).isoformat()}\n"
            f"Timed out: adb {shlex.join(adb_arguments)}\n"
            f"Emulator: {shlex.join(arguments)}\n"
        )
        sdk = Path(os.environ["ANDROID_HOME"])
        sources = {
            "emulator-version.txt": sdk / "emulator/source.properties",
            "adb-version.txt": sdk / "platform-tools/source.properties",
            "process-status.txt": process / "status",
            "emulator.log": Path(os.environ.get("TMPDIR", "/tmp"))
            / f"android-emulator-{name}.log",
        }
        for filename, source in sources.items():
            try:
                (directory / filename).write_bytes(source.read_bytes())
            except OSError as error:
                (directory / filename).write_text(f"{source}: {error}\n")
        with (directory / "threads.txt").open("w") as output:
            try:
                subprocess.run(
                    [
                        "ps",
                        "-L",
                        "-p",
                        process.name,
                        "-o",
                        "pid,tid,stat,wchan:32,comm",
                    ],
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    timeout=5,
                    check=False,
                )
            except (OSError, subprocess.TimeoutExpired) as error:
                output.write(f"Thread listing failed: {error}\n")
        with (directory / "backtrace.txt").open("w") as output:
            # The runner may restrict sibling ptrace. The privileged timeout
            # also owns GDB, so it can kill it if attachment or unwinding hangs.
            command = [
                "sudo",
                "-n",
                "timeout",
                "--kill-after=5s",
                "20s",
                "gdb",
                "-q",
                "-nx",
                "-batch",
                "-ex",
                "set pagination off",
                "-ex",
                "set debuginfod enabled off",
                "-ex",
                "info sharedlibrary",
                "-ex",
                "info proc mappings",
                "-ex",
                "thread apply all bt 40",
                "-ex",
                "detach",
                "-p",
                process.name,
            ]
            try:
                result = subprocess.run(
                    command,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                    timeout=30,
                    check=False,
                )
                output.write(f"\nGDB command exit status: {result.returncode}\n")
            except (OSError, subprocess.TimeoutExpired) as error:
                output.write(f"\nGDB command failed: {error}\n")
        print(f"Android emulator diagnostics: {directory}", file=sys.stderr)
    if not found:
        (destination / "missing-emulator.txt").write_text(
            f"Timed out: adb {shlex.join(adb_arguments)}\n"
            f"No matching emulator process remains for {avd}.\n"
        )


if __name__ == "__main__":
    capture(sys.argv[1], sys.argv[2:])
