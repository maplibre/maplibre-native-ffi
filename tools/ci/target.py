"""Run a native target's CI suites after its build, and report each one.

Every suite runs, even after an earlier one failed, so one run reports every
failed suite, but only when its prerequisites succeeded. The run fails when any
suite failed or did not run.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import signal
import subprocess
import time

from ci.workflow import load_configuration
from tools.ci.suites import BOOT, BUILD, Suite, target_suites

ROOT = pathlib.Path(__file__).resolve().parents[2]
SECONDS_PER_MINUTE = 60


def record(suite: Suite) -> dict[str, object]:
    return {
        "id": suite.id,
        "command": list(suite.command),
        "env": suite.env,
        "wrapper": suite.wrapper,
        "shell": suite.shell,
        "timeout-minutes": suite.timeout_minutes,
        "prerequisites": list(suite.prerequisites),
    }


def argv(suite: Suite) -> list[str]:
    command = list(suite.command)
    if suite.wrapper == "xvfb":
        return ["xvfb-run", "--auto-servernum", *command]
    if suite.wrapper == "windows-console":
        quoted = ", ".join(f"'{argument}'" for argument in command)
        script = f"./ci/with-windows-console.ps1 -Command @({quoted})"
        return ["pwsh", "-Command", f"{script}; exit $LASTEXITCODE"]
    return command


def environment(suite: Suite) -> dict[str, str]:
    # The arguments mise gave this task would reach a child task that declares
    # none of its own. Windows environment names ignore case, and Python reports
    # them in upper case there.
    env = {
        key: value
        for key, value in os.environ.items()
        if not key.lower().startswith("usage_")
    }
    if suite.wrapper == "xvfb":
        # Draw on the virtual display's X11 platform rather than the
        # surfaceless one that the job exports for the headless suites.
        env.pop("EGL_PLATFORM", None)
    env.update(suite.env)
    return env


class Cancelled(Exception):
    pass


def cancel(signum, frame):
    raise Cancelled


def execute(suite: Suite) -> tuple[str, str]:
    """Run one suite and return its result and a note for the summary."""
    timeout = (
        suite.timeout_minutes * SECONDS_PER_MINUTE if suite.timeout_minutes else None
    )
    process = subprocess.Popen(argv(suite), cwd=ROOT, env=environment(suite))
    try:
        code = process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
        return "failure", f"timed out after {suite.timeout_minutes} minutes"
    except BaseException:
        process.terminate()
        process.wait()
        raise
    if code != 0:
        return "failure", f"exited with {code}"
    return "success", ""


def escape(value: str) -> str:
    """Escape a workflow command property, such as a task name with colons."""
    return (
        value.replace("%", "%25")
        .replace("\r", "%0D")
        .replace("\n", "%0A")
        .replace(":", "%3A")
        .replace(",", "%2C")
    )


def summarize(results: list[tuple[Suite, str, str, float]]) -> None:
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not path:
        return
    lines = ["| Suite | Result | Time |", "| --- | --- | --- |"]
    for suite, result, note, seconds in results:
        detail = f"{result} ({note})" if note else result
        lines.append(
            f"| `{' '.join(suite.command[2:])}` | {detail} | {seconds:.0f} s |"
        )
    with open(path, "a", encoding="utf-8") as summary:
        summary.write("\n".join(lines) + "\n")


def run(suites: list[Suite]) -> int:
    # The job runs its build and boot steps first and runs this step only when
    # they succeed.
    outcomes = {BUILD: "success", BOOT: "success"}
    results: list[tuple[Suite, str, str, float]] = []
    signal.signal(signal.SIGTERM, cancel)
    try:
        for suite in suites:
            if any(outcomes.get(step) != "success" for step in suite.prerequisites):
                outcomes[suite.id] = "skipped"
                results.append((suite, "skipped", "a prerequisite failed", 0.0))
                print(f"Skip {suite.task}: a prerequisite failed", flush=True)
                continue
            print(f"::group::{suite.task}", flush=True)
            start = time.monotonic()
            try:
                result, note = execute(suite)
            finally:
                print("::endgroup::", flush=True)
            outcomes[suite.id] = result
            results.append((suite, result, note, time.monotonic() - start))
            if result != "success":
                print(f"::error title={escape(suite.task)}::{note}", flush=True)
    except (Cancelled, KeyboardInterrupt):
        # A cancelled job runs no further suites.
        summarize(results)
        return 130
    summarize(results)
    return 0 if all(result == "success" for _, result, _, _ in results) else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("preset")
    parser.add_argument(
        "--list",
        action="store_true",
        help="print each suite and how it runs, one JSON record per line",
    )
    args = parser.parse_args()
    _, presets = load_configuration(ROOT)
    suites = target_suites(presets, args.preset)
    if args.list:
        for suite in suites:
            print(json.dumps(record(suite)))
        return 0
    return run(suites)


if __name__ == "__main__":
    raise SystemExit(main())
