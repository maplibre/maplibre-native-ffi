"""Exercise coverage selection, the generated gate, and retries."""

from __future__ import annotations

import ast
import copy
import json
import os
import pathlib
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from ruamel.yaml import YAML

from ci import select
from ci.coverage import (
    HYGIENE,
    dependencies,
    job_name,
    selected_jobs,
    unit_jobs,
    unit_passed,
)
from ci.generate_workflow import ROOT, ci_workflow, serialize, workflows
from ci.plan import plan
from ci.retry import main as retry_main
from ci.retry import retryable
from ci.workflow import load_configuration

ENV = {
    "GITHUB_REPOSITORY": "maplibre/maplibre-native-ffi",
    "GITHUB_RUN_ID": "100",
    "TYPESAFE_API_KEY": "key",
}
FORK, OLD, HEAD = "f" * 40, "0" * 40, "1" * 40


def pr(labels=(), number=7, head=HEAD):
    return {
        "action": "synchronize",
        "pull_request": {
            "number": number,
            "title": "Change",
            "labels": [{"name": label} for label in labels],
            "head": {"sha": head, "ref": "topic"},
            "base": {"sha": "b" * 40},
        },
    }


def shell_accepts(step, env):
    return (
        subprocess.run(
            ["bash", "-c", step["run"]],
            env={"PATH": os.environ["PATH"], **env},
            capture_output=True,
            check=False,
        ).returncode
        == 0
    )


class Fixture(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.config = select.load_config()
        cls.source, cls.presets = load_configuration(ROOT)
        cls.owned = unit_jobs(cls.config, cls.source, cls.presets)
        cls.units = [unit["id"] for unit in cls.config["units"]]


class CoverageTest(Fixture):
    def test_every_job_belongs_to_exactly_one_unit(self):
        jobs = ci_workflow(self.source, self.presets)["jobs"]
        owned = sorted(job for jobs_ in self.owned.values() for job in jobs_)
        self.assertEqual(owned, sorted(set(jobs) - {"plan", "required"}))
        for change in ("drop", "repeat"):
            config = copy.deepcopy(self.config)
            windows = next(unit for unit in config["units"] if unit["id"] == "windows")
            if change == "drop":
                windows["targets"] = []
            else:
                windows["targets"].append("linux-gnu-x64-*")
            with self.subTest(change=change), self.assertRaises(ValueError):
                unit_jobs(config, self.source, self.presets)

    def test_selected_packaging_brings_the_targets_it_consumes(self):
        needs = dependencies(self.source, self.presets)
        apple = selected_jobs(self.owned, needs, ["apple-mobile"])
        self.assertIn("target-macos-arm64-metal", apple)
        self.assertNotIn("target-macos-arm64-vulkan", apple)
        maven = selected_jobs(self.owned, needs, ["maven"])
        self.assertIn("target-windows-arm64-vulkan", maven)
        self.assertFalse([job for job in maven if "musl" in job or "ohos" in job])

    def test_a_unit_passes_only_when_every_job_succeeded(self):
        jobs = ["target-linux-gnu-x64-egl", "kotlin-maven"]
        passing = {
            "target / linux-gnu-x64-egl": "success",
            "Kotlin Maven / staging / apple": "success",
            "Kotlin Maven / verification": "success",
        }
        self.assertTrue(unit_passed(jobs, passing))
        self.assertFalse(
            unit_passed(jobs, {**passing, "Kotlin Maven / verification": "failure"})
        )
        self.assertFalse(unit_passed(jobs, {"target / linux-gnu-x64-egl": "success"}))
        self.assertFalse(
            unit_passed(jobs, {**passing, "target / linux-gnu-x64-egl": "skipped"})
        )


class PlanTest(Fixture):
    def run_plan(
        self, event, *, runs=(), choose=None, env=ENV, event_name="pull_request"
    ):
        """Plan with fake git history, workflow runs, and Jev answers."""
        calls = []

        def api(path):
            if "/workflows/" in path:
                return {"workflow_runs": [run for run, _ in runs]}
            run_id = int(path.split("/runs/")[1].split("/")[0])
            jobs = next(jobs for run, jobs in runs if run["id"] == run_id)
            return {"total_count": len(jobs), "jobs": jobs}

        def default_choose(base, head, message):
            calls.append(base)
            return {unit: (unit == "windows", "Jev") for unit in self.units}

        def git(*args):
            return f"{FORK}\n" if args[0] == "merge-base" else "Commit message\n"

        with (
            patch("ci.plan.select.git", side_effect=git),
            patch("ci.plan.present", return_value=True),
        ):
            decisions = plan(
                self.config,
                self.owned,
                event_name,
                event,
                env,
                api=api,
                choose=choose or default_choose,
            )
        return {unit: selected for unit, (selected, _) in decisions.items()}, calls

    def passing_run(self, units, *, run_id=90, head=OLD, number=7):
        # The Kotlin Maven reusable workflow reports nested job names.
        jobs = [
            {
                "name": job_name(job) + (" / verify" if job == "kotlin-maven" else ""),
                "conclusion": "success",
            }
            for unit in units
            for job in self.owned[unit]
        ]
        return {
            "id": run_id,
            "head_sha": head,
            "pull_requests": [{"number": number}],
        }, jobs

    def test_main_labels_and_missing_keys_run_everything(self):
        cases = [
            ("push", {}, ENV),
            ("workflow_dispatch", {}, ENV),
            ("pull_request", pr(["ci:full"]), ENV),
            (
                "pull_request",
                pr(),
                {k: v for k, v in ENV.items() if k != "TYPESAFE_API_KEY"},
            ),
        ]
        for event_name, event, env in cases:
            with self.subTest(event_name=event_name, env=sorted(env)):
                decisions = plan(self.config, self.owned, event_name, event, env)
                self.assertTrue(all(selected for selected, _ in decisions.values()))
                self.assertEqual(set(decisions), set(self.owned))

    def test_a_new_pull_request_runs_hygiene_and_what_jev_selects(self):
        decisions, calls = self.run_plan(pr(["ci:android", "unrelated"]))
        self.assertEqual(calls, [FORK])
        selected = {unit for unit, run in decisions.items() if run}
        self.assertEqual(selected, {HYGIENE, "windows", "android"})

    def test_each_unit_judges_only_changes_since_it_last_passed(self):
        runs = [
            self.passing_run(["linux-x64"], run_id=92, head="2" * 40),
            self.passing_run(["linux-x64", HYGIENE, "docs"], run_id=91),
            # Another pull request on a reused branch name proves nothing.
            self.passing_run(self.units, run_id=93, number=8),
        ]
        bases = {}

        def choose(base, head, message):
            bases[base] = message
            return {unit: (False, "Jev") for unit in self.units}

        decisions, _ = self.run_plan(pr(), runs=runs, choose=choose)
        self.assertEqual(set(bases), {FORK, OLD, "2" * 40})
        self.assertTrue(decisions[HYGIENE])
        self.assertFalse(any(decisions[unit] for unit in self.units))

    def test_an_unchanged_head_reruns_nothing_that_passed_there(self):
        runs = [self.passing_run([HYGIENE, "linux-x64"], head=HEAD)]
        decisions, calls = self.run_plan(pr(), runs=runs)
        self.assertEqual(calls, [FORK])
        self.assertFalse(decisions[HYGIENE] or decisions["linux-x64"])
        self.assertTrue(decisions["windows"])

    def test_failed_or_missing_jobs_are_not_evidence(self):
        run, jobs = self.passing_run(["windows", HYGIENE], head=HEAD)
        jobs[0]["conclusion"] = "failure"
        decisions, _ = self.run_plan(
            pr(), runs=[(run, jobs[1:]), (run | {"id": 89}, jobs)]
        )
        self.assertTrue(decisions["windows"])
        self.assertFalse(decisions[HYGIENE])

    def test_selection_failure_runs_everything(self):
        def choose(base, head, message):
            raise TimeoutError("slow")

        decisions, _ = self.run_plan(pr(), choose=choose)
        self.assertTrue(all(decisions.values()))

    def test_planner_parses_with_the_runner_system_python(self):
        # The plan job runs the ubuntu-slim system Python, not the project's.
        for name in ("plan", "select", "coverage", "workflow"):
            path = ROOT / "ci" / f"{name}.py"
            with self.subTest(module=name):
                ast.parse(path.read_text(), str(path), feature_version=(3, 12))

    def test_entrypoint_runs_outside_checkout_without_project_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "event").write_text(json.dumps(pr()))
            subprocess.run(
                ["bash", str(ROOT / ".mise/tasks/ci/plan")],
                cwd=root,
                check=True,
                env={
                    "PATH": os.environ["PATH"],
                    "GITHUB_EVENT_NAME": "pull_request",
                    "GITHUB_EVENT_PATH": str(root / "event"),
                    "GITHUB_OUTPUT": str(root / "output"),
                    "GITHUB_STEP_SUMMARY": str(root / "summary"),
                },
            )
            output = (root / "output").read_text()
            jobs = json.loads(output.removeprefix("jobs="))
            expected = set(ci_workflow(self.source, self.presets)["jobs"]) - {
                "plan",
                "required",
            }
            self.assertEqual(set(jobs), expected)


class WorkflowTest(Fixture):
    def test_serialized_workflows_load_as_generated(self):
        for name, document in workflows(self.source, self.presets).items():
            with self.subTest(name=name):
                self.assertEqual(YAML(typ="safe").load(serialize(document)), document)

    def test_required_gate_accepts_only_selected_successes_and_other_skips(self):
        step = ci_workflow(self.source, self.presets)["jobs"]["required"]["steps"][0]
        expected = json.loads(step["env"]["EXPECTED"])
        selected = selected_jobs(
            self.owned,
            dependencies(self.source, self.presets),
            [HYGIENE, "apple-mobile"],
        )
        results = {
            name: {"result": "success" if name in selected else "skipped"}
            for name in expected
        }

        def accepts(values, selection=selected, plan_result="success"):
            return shell_accepts(
                step,
                {
                    "PLAN": plan_result,
                    "EXPECTED": json.dumps(expected),
                    "SELECTED": json.dumps(selection),
                    "RESULTS": json.dumps(values),
                },
            )

        self.assertTrue(accepts(results))
        self.assertFalse(accepts(results, plan_result="failure"))
        self.assertFalse(accepts(results, []))
        self.assertFalse(accepts(results, [*selected, "unknown"]))
        for name in expected:
            for result in {"success", "failure", "skipped", "cancelled"} - {
                results[name]["result"]
            }:
                self.assertFalse(
                    accepts({**results, name: {"result": result}}), (name, result)
                )


class RetryTest(unittest.TestCase):
    def test_api_failures_retry_with_backoff_and_recheck_run_attempt(self):
        jobs = {
            "jobs": [
                {"name": "plan", "conclusion": "failure"},
                {"name": "ci-required", "conclusion": "failure"},
            ]
        }
        error = subprocess.CalledProcessError(1, "gh")
        for failure_at in ("run", "jobs", "post"):
            responses = {
                "run": [error, '{"run_attempt": 1}', json.dumps([jobs])],
                "jobs": [
                    '{"run_attempt": 1}',
                    error,
                    '{"run_attempt": 1}',
                    json.dumps([jobs]),
                ],
                "post": [
                    '{"run_attempt": 1}',
                    json.dumps([jobs]),
                    '{"run_attempt": 2}',
                ],
            }[failure_at]
            with (
                self.subTest(failure_at=failure_at),
                patch.dict(
                    os.environ,
                    {"CI_RUN_ID": "90", "GITHUB_REPOSITORY": ENV["GITHUB_REPOSITORY"]},
                ),
                patch("ci.retry.subprocess.check_output", side_effect=responses),
                patch(
                    "ci.retry.subprocess.run",
                    side_effect=error if failure_at == "post" else None,
                ) as post,
                patch("ci.retry.time.sleep") as sleep,
                patch("builtins.print"),
            ):
                retry_main()
                sleep.assert_called_once_with(2)
                self.assertEqual(post.call_count, 1)
        with (
            patch.dict(
                os.environ,
                {"CI_RUN_ID": "90", "GITHUB_REPOSITORY": ENV["GITHUB_REPOSITORY"]},
            ),
            patch("ci.retry.subprocess.check_output", side_effect=error) as get,
            patch("ci.retry.time.sleep") as sleep,
            patch("builtins.print"),
        ):
            with self.assertRaises(subprocess.CalledProcessError):
                retry_main()
            self.assertEqual(get.call_count, 4)
            self.assertEqual([call.args[0] for call in sleep.call_args_list], [2, 4, 8])

    def test_one_primary_failure_retries_with_the_required_check(self):
        jobs = [
            {"name": "target / linux-gnu-x64-egl", "conclusion": "failure"},
            {"name": "ci-required", "conclusion": "failure"},
            {"name": "Kotlin Maven / verify", "conclusion": "skipped"},
        ]
        self.assertTrue(retryable(jobs))
        self.assertFalse(retryable(jobs[1:]))
        self.assertFalse(retryable(jobs + [{"name": "other", "conclusion": "failure"}]))
        self.assertFalse(
            retryable(jobs + [{"name": "other", "conclusion": "cancelled"}])
        )
