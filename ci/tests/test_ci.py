"""Exercise suite configuration, the required check, and retries."""

from __future__ import annotations

import copy
import json
import os
import subprocess
import unittest

from ci.generate_workflow import ROOT, ci_workflow
from ci.retry import retryable
from ci.workflow import load_configuration


class WorkflowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)

    def test_android_emulator_diagnostics_are_uploaded_even_after_recovery(self):
        jobs = ci_workflow(self.source, self.presets)["jobs"]
        enabled = []
        for name, job in jobs.items():
            if "MLN_FFI_ANDROID_DIAGNOSTICS_DIR" not in job.get("env", {}):
                continue
            enabled.append(name)
            upload = next(
                step
                for step in job["steps"]
                if step.get("name") == "Upload Android emulator diagnostics"
            )
            self.assertEqual(upload["if"], "always()")
            self.assertEqual(upload["with"]["if-no-files-found"], "ignore")
        self.assertEqual(
            enabled, ["target-android-x64-egl", "target-android-x64-vulkan"]
        )

    def test_a_job_cannot_run_one_task_as_two_suites(self):
        source = copy.deepcopy(self.source)
        source["suites"].append(
            {
                "platforms": ["windows"],
                "commands": [{"task": "//bindings/rust:test", "args": ["release"]}],
            }
        )
        with self.assertRaisesRegex(SystemExit, "'bindings-rust-test'"):
            ci_workflow(source, self.presets)

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
