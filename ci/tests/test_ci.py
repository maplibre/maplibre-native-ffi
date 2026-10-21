"""Exercise the coverage units, the required check, and retries."""

from __future__ import annotations

import copy
import json
import os
import re
import subprocess
import unittest

from ruamel.yaml import YAML

from ci.coverage import HYGIENE, unit_jobs
from ci.generate_workflow import ROOT, ci_workflow, workflows
from ci.retry import INFRASTRUCTURE_STEPS, retryable
from ci.workflow import XVFB_RUN, load_configuration, platform, preset_sets, target_rows

ALL = "fromJSON(needs.plan.outputs.all)"
UNIT = re.compile(r"contains\(fromJSON\(needs\.plan\.outputs\.units\), '([^']+)'\)")


class CoverageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)
        cls.jobs = ci_workflow(cls.source, cls.presets)["jobs"]

    def test_units_match_the_rubberstamp_config(self):
        config = YAML(typ="safe").load(ROOT / ".config/rubberstamp.yml")
        self.assertEqual(sorted(self.source["units"]), sorted(config["ci"]["units"]))

    def units(self, job: dict) -> set[str]:
        """Return the units that a job's condition runs it for."""
        units = UNIT.findall(job["if"])
        clauses = [
            f"contains(fromJSON(needs.plan.outputs.units), '{u}')" for u in units
        ]
        self.assertEqual(job["if"], " || ".join([ALL, *clauses]))
        return set(units)

    def test_each_unit_runs_its_jobs_and_what_they_consume(self):
        gated = {
            name: job
            for name, job in self.jobs.items()
            if name not in {"plan", HYGIENE, "required"}
        }
        owned = unit_jobs(self.source, self.presets)
        for unit, jobs in owned.items():
            with self.subTest(unit=unit):
                running = {n for n, job in gated.items() if unit in self.units(job)}
                self.assertLessEqual(set(jobs), running)
                for name in running:
                    waits = set(gated[name]["needs"]) - {"plan"}
                    self.assertLessEqual(waits, running, name)


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
    def test_required_check_accepts_only_successes_and_skips(self):
        source, presets = load_configuration(ROOT)
        workflow = ci_workflow(source, presets)
        step = workflow["jobs"]["required"]["steps"][0]
        needs = workflow["jobs"]["required"]["needs"]
        results = {job: {"result": "success"} for job in needs}

        def accepts(values, plan_result="success"):
            env = {
                "PATH": os.environ["PATH"],
                "PLAN": plan_result,
                "RESULTS": json.dumps(values),
            }
            command = ["bash", "-c", step["run"]]
            done = subprocess.run(command, env=env, capture_output=True, check=False)
            return done.returncode == 0

        self.assertTrue(accepts(results))
        skipped = {job: {"result": "skipped"} for job in needs if job != "plan"}
        self.assertTrue(accepts({**results, **skipped}))
        self.assertFalse(accepts(results, plan_result="failure"))
        self.assertFalse(accepts(results, plan_result="skipped"))
        for job in needs:
            for result in ("failure", "cancelled"):
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
