#!/usr/bin/env python3
"""CI-only repro harness for the offline database callback wedge.

Runs the Zig binding test executable through `zig build`, matching the regular CI
path closely enough to preserve Zig's test-server mode and test ordering. On a
hang, samples the `zig build` process and any child processes so the Actions log
and uploaded artifacts show where the process parked.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import time
from pathlib import Path


def run_text(args: list[str], *, timeout: float | None = None) -> str:
    return subprocess.run(
        args,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
    ).stdout


def descendants(pid: int) -> list[int]:
    ps = run_text(["ps", "-axo", "pid=,ppid="])
    children: dict[int, list[int]] = {}
    for line in ps.splitlines():
        parts = line.split()
        if len(parts) != 2:
            continue
        child, parent = map(int, parts)
        children.setdefault(parent, []).append(child)
    result: list[int] = []
    stack = list(children.get(pid, []))
    while stack:
        child = stack.pop()
        result.append(child)
        stack.extend(children.get(child, []))
    return result


def sample_processes(root_pid: int, artifact_dir: Path) -> None:
    artifact_dir.mkdir(parents=True, exist_ok=True)
    ps_output = run_text(["ps", "-axo", "pid,ppid,stat,comm,args"])
    (artifact_dir / "processes.txt").write_text(ps_output)
    pids = [root_pid] + descendants(root_pid)
    print(f"sampling pids: {pids}", flush=True)
    for pid in pids:
        out = artifact_dir / f"sample-{pid}.txt"
        try:
            subprocess.run(["sample", str(pid), "5", "-file", str(out)], timeout=12)
        except Exception as exc:  # noqa: BLE001 - diagnostic script
            out.write_text(f"sample failed for pid {pid}: {exc!r}\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--iterations", type=int, default=int(os.environ.get("REPRO_ITERATIONS", "200"))
    )
    parser.add_argument(
        "--timeout", type=float, default=float(os.environ.get("REPRO_TIMEOUT", "35"))
    )
    parser.add_argument(
        "--stress-procs",
        type=int,
        default=int(os.environ.get("REPRO_STRESS_PROCS", "0")),
    )
    parser.add_argument(
        "--artifact-dir",
        type=Path,
        default=Path(os.environ.get("REPRO_ARTIFACT_DIR", "offline-wedge-artifacts")),
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    cwd = repo / "bindings/zig"
    cmd = [
        "zig",
        "build",
        "--seed",
        "0x1f261fa1",
        "--test-timeout",
        "30s",
        "--prefix",
        "zig-out/macos-arm64-metal",
        "test",
        "--summary",
        "all",
        "--verbose",
        "-Dcmake-artifact-dir=../../build/macos-arm64-metal",
        "-Drender-backend=metal",
    ]
    env = os.environ.copy()
    env["MLN_FFI_TRACE_DATABASE_WAITS"] = "1"

    print(run_text(["sysctl", "-n", "hw.ncpu"]).strip() + " logical CPUs", flush=True)
    print("command: " + " ".join(cmd), flush=True)

    stress: list[subprocess.Popen[bytes]] = []
    try:
        for _ in range(args.stress_procs):
            stress.append(
                subprocess.Popen(
                    ["yes"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                )
            )
        if stress:
            print(f"started {len(stress)} CPU stress processes", flush=True)

        durations: list[float] = []
        for index in range(1, args.iterations + 1):
            started = time.monotonic()
            proc = subprocess.Popen(
                cmd,
                cwd=cwd,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            try:
                output, _ = proc.communicate(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                elapsed = time.monotonic() - started
                print(
                    f"HANG iteration={index} elapsed={elapsed:.3f}s pid={proc.pid}",
                    flush=True,
                )
                sample_processes(proc.pid, args.artifact_dir)
                proc.kill()
                output, _ = proc.communicate()
                (args.artifact_dir / "timeout-output.log").write_text(output)
                print(output[-12000:], flush=True)
                return 124

            elapsed = time.monotonic() - started
            durations.append(elapsed)
            if proc.returncode != 0:
                print(
                    f"FAIL iteration={index} elapsed={elapsed:.3f}s rc={proc.returncode}",
                    flush=True,
                )
                (args.artifact_dir / "failure-output.log").write_text(output)
                print(output[-12000:], flush=True)
                return proc.returncode
            if index % 10 == 0 or elapsed > 5:
                print(
                    f"iteration {index}/{args.iterations}: elapsed={elapsed:.3f}s "
                    f"max={max(durations):.3f}s avg={sum(durations) / len(durations):.3f}s",
                    flush=True,
                )
        print(
            f"completed {args.iterations} iterations; max={max(durations):.3f}s "
            f"avg={sum(durations) / len(durations):.3f}s",
            flush=True,
        )
        return 0
    finally:
        for proc in stress:
            proc.terminate()
        for proc in stress:
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    raise SystemExit(main())
