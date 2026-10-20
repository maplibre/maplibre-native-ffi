"""Exercise coverage planning, the required check, and retries."""

from __future__ import annotations

import ast
import copy
import json
import os
import pathlib
import re
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from ci import select
from ci.coverage import HYGIENE, dependencies, job_name, selected_jobs, unit_jobs
from ci.generate_workflow import ROOT, ci_workflow, workflows
from ci.plan import plan
from ci.retry import INFRASTRUCTURE_STEPS, retryable
from ci.workflow import XVFB_RUN, load_configuration, platform, preset_sets, target_rows

ENV = {"GITHUB_REPOSITORY": "maplibre/maplibre-native-ffi", "GITHUB_RUN_ID": "100"}
FORK, OLD, NEWER, HEAD = "f" * 40, "0" * 40, "2" * 40, "1" * 40


def pr(labels=()):
    return {
        "pull_request": {
            "number": 7,
            "title": "Change",
            "state": "open",
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

    def test_a_closed_pull_request_selects_nothing(self):
        event = pr(["ci:full"])
        event["pull_request"]["state"] = "closed"
        decisions = plan(
            self.config,
            self.owned,
            "pull_request",
            event,
            ENV | {"TYPESAFE_API_KEY": "key"},
        )
        self.assertEqual(set(decisions), set(self.owned))
        self.assertTrue(
            all(
                entry["rule"] == "pull request is closed"
                for entry in decisions.values()
            )
        )
        self.assertFalse(any(entry["run"] for entry in decisions.values()))

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


class BindingWorkflowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)
        cls.workflows = workflows(cls.source, cls.presets)

    def test_presets_that_opt_out_of_ci_name_no_target(self):
        local = {"vendor": {"maplibre-native-ffi": {"ci": False}}}
        presets = {
            "configurePresets": [{"name": "host"}, {"name": "host-local", **local}],
            "buildPresets": [
                {"name": "host", "configurePreset": "host"},
                {"name": "host-local", "configurePreset": "host-local"},
            ],
            "testPresets": [
                {"name": "host", "configurePreset": "host"},
                {"name": "host-local", "configurePreset": "host-local"},
            ],
        }
        self.assertEqual(preset_sets(presets), (["host"], {"host"}, {"host"}, set()))

    def test_window_commands_get_a_display_on_linux_and_keep_their_caches(self):
        rows = {row["preset"]: row for row in target_rows(self.source, self.presets)}
        for preset, row in rows.items():
            commands = row["native_commands"] + row.get("consumer_commands", [])
            with self.subTest(preset=preset):
                wrapped = [c for c in commands if c.startswith(XVFB_RUN)]
                self.assertEqual(bool(wrapped), platform(preset) == "linux-gnu")
        linux = rows["linux-gnu-x64-egl"]
        self.assertIn(
            f"{XVFB_RUN} mise run //examples/zig-map:smoke linux-gnu-x64-egl",
            linux["consumer_commands"],
        )
        # The wrapped Zig and Gradle examples still count toward their caches.
        self.assertTrue(linux["zig"])
        self.assertTrue(linux["gradle"])

    def test_a_suite_environment_reaches_its_commands_in_the_listed_targets(self):
        source = copy.deepcopy(self.source)
        source["suites"].append(
            {
                "platforms": ["windows"],
                "commands": [
                    {"task": "//examples/c-map:check"},
                    {"task": "//examples/c-map:smoke"},
                ],
                "environments": [
                    {"include": ["windows-x64-wgl"], "env": {"STEP_PROBE": "1"}}
                ],
            }
        )
        jobs = ci_workflow(source, self.presets)["jobs"]
        environments = {
            (name, step["run"]): step.get("env", {})
            for name, job in jobs.items()
            for step in job.get("steps", [])
            if "STEP_PROBE" in step.get("env", {})
            or (
                name.startswith("target-windows-")
                and "//examples/c-map:" in step.get("run", "")
            )
        }
        # The packaged target's consumer environment still applies.
        probed = {"MISE_TASK_SKIP": "//:build", "STEP_PROBE": "1"}
        plain = {"MISE_TASK_SKIP": "//:build"}
        self.assertEqual(
            environments,
            {
                (f"target-{preset}", f"mise run //examples/c-map:{task} {preset}"): (
                    probed if preset == "windows-x64-wgl" else plain
                )
                for preset in (
                    "windows-x64-wgl",
                    "windows-x64-vulkan",
                    "windows-arm64-wgl",
                    "windows-arm64-vulkan",
                )
                for task in ("check", "smoke")
            },
        )

    def test_suites_run_past_a_failed_suite_and_the_last_step_fails_the_job(self):
        for document in self.workflows.values():
            for name, job in document["jobs"].items():
                if not name.startswith("target-"):
                    continue
                with self.subTest(job=name):
                    steps = job["steps"]
                    ids = [step.get("id") for step in steps]
                    suites = [i for i in ids if i and i.startswith("suite-")]
                    self.assertTrue(suites)
                    for step in steps[ids.index("build") + 1 :]:
                        if step.get("id") in {"archive", "install", *suites}:
                            self.assertTrue(
                                step["if"].startswith("${{ !cancelled() && "),
                                step,
                            )
                    gate = steps[-1]
                    self.assertEqual(gate["name"], "Fail if any suite failed")
                    self.assertEqual(
                        re.findall(r"steps\.(suite-\d+)\.outcome", gate["if"]),
                        suites,
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
    def test_only_infrastructure_step_failures_retry(self):
        def job(name, *failed):
            return {
                "name": name,
                "conclusion": "failure",
                "steps": [
                    {"name": "Set up job", "conclusion": "success"},
                    *({"name": step, "conclusion": "failure"} for step in failed),
                ],
            }

        aggregates = [
            {"name": "ci-required", "conclusion": "failure"},
            {"name": "Kotlin Maven / verify", "conclusion": "skipped"},
        ]
        target = "target / android-x64-egl"
        for failed in (
            ["Set up CI dependencies"],
            ["Boot Android emulator"],
            ["Resolve Swift packages"],
            ["Post Set up CI dependencies"],
        ):
            with self.subTest(failed=failed):
                self.assertTrue(retryable([job(target, *failed), *aggregates]))
        suite = "Run mise run //bindings/kotlin:test android-x64-egl"
        for jobs in (
            [job(target, suite)],
            [job(target, suite, "Fail if any suite failed")],
            [job(target, "Boot Android emulator", suite)],
            # A job that failed without a failed step timed out, possibly in a
            # hung test.
            [job(target)],
            [job(target, "Boot Android emulator"), job("hygiene", suite)],
        ):
            with self.subTest(jobs=jobs):
                self.assertFalse(retryable([*jobs, *aggregates]))
        self.assertTrue(
            retryable(
                [
                    job(target, "Boot Android emulator"),
                    job("target / ohos-x64-egl", "Set up CI dependencies"),
                    *aggregates,
                ]
            )
        )
        self.assertFalse(retryable(aggregates))
        self.assertFalse(retryable([job(target, "Set up CI dependencies")]))
        self.assertFalse(
            retryable(
                [
                    job(target, "Set up CI dependencies"),
                    *aggregates,
                    {"name": "other", "conclusion": "cancelled"},
                ]
            )
        )
        self.assertTrue(
            retryable(
                [
                    job("plan", "Plan coverage"),
                    job("ci-required", "Check coverage result"),
                ]
            )
        )

    def test_generated_workflows_name_every_infrastructure_step(self):
        source, presets = load_configuration(ROOT)
        names = {
            step.get("name")
            for document in workflows(source, presets).values()
            for job in document["jobs"].values()
            for step in job.get("steps", [])
        }
        # GitHub names its own setup and teardown steps.
        self.assertEqual(INFRASTRUCTURE_STEPS - names, {"Set up job", "Complete job"})
