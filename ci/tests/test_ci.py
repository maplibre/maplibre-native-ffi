"""Exercise coverage planning, the required check, and retries."""

from __future__ import annotations

import ast
import json
import os
import pathlib
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from ci import select
from ci.coverage import HYGIENE, dependencies, job_name, selected_jobs, unit_jobs
from ci.generate_workflow import ROOT, ci_workflow
from ci.plan import plan
from ci.retry import retryable
from ci.workflow import load_configuration

ENV = {"GITHUB_REPOSITORY": "maplibre/maplibre-native-ffi", "GITHUB_RUN_ID": "100"}
FORK, OLD, NEWER, HEAD = "f" * 40, "0" * 40, "2" * 40, "1" * 40


def pr(labels=()):
    return {
        "pull_request": {
            "number": 7,
            "title": "Change",
            "labels": [{"name": label} for label in labels],
            "head": {"sha": HEAD, "ref": "topic"},
            "base": {"sha": "b" * 40},
        }
    }


class PlanTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.config = select.load_config()
        source, presets = load_configuration(ROOT)
        cls.owned = unit_jobs(cls.config, source, presets)
        cls.units = [unit["id"] for unit in cls.config["units"]]

    def passing_run(self, units, *, run_id=90, head=OLD, number=7):
        jobs = [
            {"name": job_name(job), "conclusion": "success"}
            for unit in units
            for job in self.owned[unit]
            if job != "kotlin-maven"
        ]
        if "maven" in units:
            # The reusable workflow nests its jobs and skips publishing in PRs.
            jobs += [
                {"name": "Kotlin Maven / verify", "conclusion": "success"},
                {"name": "Kotlin Maven / publish", "conclusion": "skipped"},
            ]
        run = {"id": run_id, "head_sha": head, "pull_requests": [{"number": number}]}
        return run, jobs

    def run_plan(self, event, *, runs=(), choose=None, unchanged_since=()):
        """Plan with fake git history, earlier runs, and Jev answers."""
        bases = []

        def api(path):
            if "/workflows/" in path:
                return {"workflow_runs": [run for run, _ in runs]}
            run_id = int(path.split("/runs/")[1].split("/")[0])
            jobs = next(jobs for run, jobs in runs if run["id"] == run_id)
            return {"total_count": len(jobs), "jobs": jobs}

        def jev(base, head, message):
            bases.append(base)
            return {u: {"run": u == "windows", "reason": "Jev"} for u in self.units}

        def git(*args):
            if args[0] == "merge-base":
                return f"{FORK}\n"
            if args[0] == "diff":
                return "" if args[-2] in unchanged_since else "src/map/map.cpp\n"
            return "Commit message\n"

        with (
            patch("ci.plan.select.git", side_effect=git),
            patch("ci.plan.select.own_changes_base", side_effect=lambda b, _: b),
            patch("ci.plan.present", return_value=True),
        ):
            decisions = plan(
                self.config,
                self.owned,
                "pull_request",
                event,
                ENV | {"TYPESAFE_API_KEY": "key"},
                api=api,
                choose=choose or jev,
            )
        return {unit: entry["run"] for unit, entry in decisions.items()}, bases

    def test_main_ci_full_and_missing_keys_run_everything(self):
        for event_name, event, env in [
            ("push", {}, ENV),
            ("workflow_dispatch", {}, ENV),
            ("pull_request", pr(["ci:full"]), ENV | {"TYPESAFE_API_KEY": "key"}),
            ("pull_request", pr(), ENV),
        ]:
            with self.subTest(event_name=event_name, env=sorted(env)):
                decisions = plan(self.config, self.owned, event_name, event, env)
                self.assertEqual(set(decisions), set(self.owned))
                self.assertTrue(all(entry["run"] for entry in decisions.values()))

    def test_a_new_pull_request_runs_hygiene_jev_choices_and_labels(self):
        decisions, bases = self.run_plan(pr(["ci:android", "unrelated"]))
        self.assertEqual(bases, [FORK])
        selected = {unit for unit, run in decisions.items() if run}
        self.assertEqual(selected, {HYGIENE, "windows", "android"})

    def test_each_unit_judges_changes_since_it_last_passed(self):
        failed, failed_jobs = self.passing_run(["windows"], run_id=94, head=NEWER)
        failed_jobs[0]["conclusion"] = "failure"
        runs = [
            self.passing_run(["linux-x64", "maven"], run_id=92, head=NEWER),
            self.passing_run(["linux-x64", HYGIENE, "docs"], run_id=91),
            # Another pull request on a reused branch name proves nothing.
            self.passing_run(self.units, run_id=93, number=8),
            (failed, failed_jobs),
        ]
        judged = set()

        def jev(base, head, message):
            judged.add(base)
            return {u: {"run": False, "reason": "Jev"} for u in self.units}

        decisions, _ = self.run_plan(pr(), runs=runs, choose=jev)
        self.assertEqual(judged, {FORK, OLD, NEWER})
        # A unit that failed most recently runs again whatever Jev answers.
        selected = {unit for unit, run in decisions.items() if run}
        self.assertEqual(selected, {HYGIENE, "windows"})

    def test_nothing_new_reruns_nothing_that_passed(self):
        cases = {
            "unchanged head": (
                [self.passing_run([HYGIENE, *self.units], head=HEAD)],
                False,
            ),
            "main merged in": ([self.passing_run([HYGIENE, *self.units])], True),
        }
        for name, (runs, hygiene) in cases.items():
            with self.subTest(name):
                decisions, bases = self.run_plan(pr(), runs=runs, unchanged_since={OLD})
                self.assertEqual(bases, [])
                self.assertEqual(decisions[HYGIENE], hygiene)
                self.assertFalse(any(decisions[unit] for unit in self.units))

    def test_selection_failure_runs_everything(self):
        def jev(base, head, message):
            raise TimeoutError("slow")

        decisions, _ = self.run_plan(pr(), choose=jev)
        self.assertTrue(all(decisions.values()))

    def test_planner_parses_with_the_runner_system_python(self):
        # The plan and label jobs run the ubuntu-slim system Python, not the
        # project's.
        for name in ("plan", "select", "coverage", "workflow", "label"):
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
                stdout=subprocess.DEVNULL,
                env={
                    "PATH": os.environ["PATH"],
                    "GITHUB_EVENT_NAME": "pull_request",
                    "GITHUB_EVENT_PATH": str(root / "event"),
                    "GITHUB_OUTPUT": str(root / "output"),
                    "GITHUB_STEP_SUMMARY": str(root / "summary"),
                },
            )
            jobs = json.loads((root / "output").read_text().removeprefix("jobs="))
            self.assertEqual(len(jobs), sum(map(len, self.owned.values())))
            summary = (root / "summary").read_text()
            for unit in self.owned:
                self.assertIn(f"| ✅ | {unit} | full: no Jev API key |", summary)


class DescribeTest(unittest.TestCase):
    def test_an_added_submodule_has_no_upstream_commits(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)

            def git(*args):
                return subprocess.run(
                    ["git", "-c", "user.name=t", "-c", "user.email=t@t", *args],
                    cwd=root,
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout.strip()

            git("init", "--quiet")
            git("commit", "--quiet", "--allow-empty", "-m", "base")
            base = git("rev-parse", "HEAD")
            git("update-index", "--add", "--cacheinfo", f"160000,{base},vendor/lib")
            git("commit", "--quiet", "-m", "add submodule")
            with patch.object(select, "ROOT", root):
                change = select.describe_change(base, "HEAD", "")["change"]
        self.assertEqual(
            change["submodule_updates"],
            [{"path": "vendor/lib", "upstream_commits": []}],
        )


class GateTest(unittest.TestCase):
    def test_required_check_accepts_only_selected_successes_and_other_skips(self):
        source, presets = load_configuration(ROOT)
        step = ci_workflow(source, presets)["jobs"]["required"]["steps"][0]
        expected = json.loads(step["env"]["EXPECTED"])
        owned = unit_jobs(select.load_config(), source, presets)
        needs = dependencies(source, presets)
        selected = selected_jobs(owned, needs, [HYGIENE, "apple-mobile"])
        results = {
            job: {"result": "success" if job in selected else "skipped"}
            for job in expected
        }

        def accepts(values, selection=selected, plan_result="success"):
            env = {
                "PATH": os.environ["PATH"],
                "PLAN": plan_result,
                "EXPECTED": json.dumps(expected),
                "SELECTED": json.dumps(selection),
                "RESULTS": json.dumps(values),
            }
            command = ["bash", "-c", step["run"]]
            done = subprocess.run(command, env=env, capture_output=True, check=False)
            return done.returncode == 0

        self.assertTrue(accepts(results))
        self.assertTrue(accepts({job: {"result": "skipped"} for job in expected}, []))
        self.assertFalse(accepts(results, plan_result="failure"))
        self.assertFalse(accepts(results, []))
        self.assertFalse(accepts(results, [*selected, "unknown"]))
        for job in expected:
            for result in {"success", "failure", "skipped", "cancelled"} - {
                results[job]["result"]
            }:
                changed = {**results, job: {"result": result}}
                self.assertFalse(accepts(changed), (job, result))


class RetryTest(unittest.TestCase):
    def test_only_one_primary_failure_with_the_required_check_retries(self):
        jobs = [
            {"name": "target / linux-gnu-x64-egl", "conclusion": "failure"},
            {"name": "ci-required", "conclusion": "failure"},
            {"name": "Kotlin Maven / verify", "conclusion": "skipped"},
        ]
        self.assertTrue(retryable(jobs))
        self.assertFalse(retryable(jobs[1:]))
        for other in ("failure", "cancelled"):
            self.assertFalse(retryable([*jobs, {"name": "other", "conclusion": other}]))
