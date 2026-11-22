"""Exercise how `ci:target` runs a target's suites and reports their results."""

from __future__ import annotations

import contextlib
import io
import os
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

from tools.ci import target
from tools.ci.suites import Suite


def suite(step_id: str, *prerequisites: str) -> Suite:
    command = ("mise", "run", f"//probe:{step_id}")
    return Suite(
        id=step_id,
        command=command,
        env={},
        prerequisites=prerequisites or ("build",),
    )


class RunTest(unittest.TestCase):
    def run_suites(self, suites: list[tuple[Suite, int]]) -> tuple[int, list[str], str]:
        """Run the suites as processes that exit with the given codes."""
        codes = {item.id: code for item, code in suites}
        ran: list[str] = []

        def argv(item: Suite) -> list[str]:
            ran.append(item.id)
            return [sys.executable, "-c", f"raise SystemExit({codes[item.id]})"]

        with tempfile.TemporaryDirectory() as directory:
            summary = pathlib.Path(directory) / "summary.md"
            output = io.StringIO()
            with (
                mock.patch.object(target, "argv", argv),
                mock.patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(summary)}),
                contextlib.redirect_stdout(output),
            ):
                status = target.run([item for item, _ in suites])
            return status, ran, output.getvalue() + summary.read_text()

    def test_a_failed_native_check_still_runs_the_later_suites(self):
        status, ran, output = self.run_suites(
            [
                (suite("test"), 1),
                (suite("archive"), 0),
                (suite("install", "build", "archive"), 0),
                (suite("consumer", "build", "archive", "install"), 0),
            ]
        )
        self.assertEqual(ran, ["test", "archive", "install", "consumer"])
        self.assertEqual(status, 1)
        self.assertIn("::error title=//probe%3Atest::exited with 1", output)
        self.assertIn("| `//probe:test` | failure (exited with 1) |", output)

    def test_a_failed_archive_or_install_skips_the_consumer_suites(self):
        for failed in ("archive", "install"):
            with self.subTest(failed=failed):
                status, ran, output = self.run_suites(
                    [
                        (suite("archive"), int(failed == "archive")),
                        (
                            suite("install", "build", "archive"),
                            int(failed == "install"),
                        ),
                        (suite("consumer", "build", "archive", "install"), 0),
                        (suite("check"), 0),
                    ]
                )
                self.assertNotIn("consumer", ran)
                # A suite that needs only the build still runs.
                self.assertIn("check", ran)
                self.assertEqual(status, 1)
                self.assertIn("| `//probe:consumer` | skipped", output)

    def test_later_consumer_suites_run_after_an_earlier_one_fails(self):
        status, ran, output = self.run_suites(
            [
                (suite("archive"), 0),
                (suite("install", "build", "archive"), 0),
                (suite("first", "build", "archive", "install"), 3),
                (suite("second", "build", "archive", "install"), 0),
            ]
        )
        self.assertEqual(ran, ["archive", "install", "first", "second"])
        self.assertEqual(status, 1)
        self.assertIn("::group:://probe:second", output)
        self.assertIn("| `//probe:second` | success |", output)

    def test_every_suite_passing_passes_the_task(self):
        status, ran, _ = self.run_suites([(suite("a"), 0), (suite("b"), 0)])
        self.assertEqual((status, ran), (0, ["a", "b"]))


class CommandTest(unittest.TestCase):
    def test_the_arguments_of_ci_target_do_not_reach_its_suites(self):
        # Windows reports the names in upper case.
        with mock.patch.dict(os.environ, {"usage_preset": "p", "USAGE_LIST": "false"}):
            env = target.environment(Suite("a", ("mise",), {"X": "1"}, ()))
        self.assertNotIn("usage_preset", env)
        self.assertNotIn("USAGE_LIST", env)
        self.assertEqual(env["X"], "1")
