"""Exercise the CI workflow, suite configuration, the required check, and retries."""

from __future__ import annotations

import json
import os
import subprocess
import unittest
from unittest import mock

from ruamel.yaml import YAML

from ci.retry import retryable
from tools.ci import plan, suites
from tools.ci.plan import ROOT


def load_yaml(path: str) -> dict:
    return YAML(typ="safe").load((ROOT / path).read_text())


def presets() -> dict:
    return json.loads((ROOT / "CMakePresets.json").read_text())


def targets() -> dict[str, plan.Target]:
    return {target.preset: target for target in plan.targets(presets())}


class WorkflowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.jobs = load_yaml(".github/workflows/ci-coverage.yml")["jobs"]
        cls.matrix = plan.plan(presets(), True, [])["matrix"]

    def step(self, job: str, name: str) -> dict:
        (step,) = [step for step in self.jobs[job]["steps"] if step.get("name") == name]
        return step

    def test_android_emulator_logs_are_uploaded_even_after_recovery(self):
        upload = self.step("target", "Upload Android emulator logs")
        self.assertEqual(upload["if"], "always() && matrix.emulator_logs")
        self.assertEqual(upload["with"]["if-no-files-found"], "ignore")
        self.assertEqual(
            [row["preset"] for row in self.matrix if row["emulator_logs"]],
            ["android-x64-egl", "android-x64-vulkan"],
        )

    def test_the_target_job_runs_its_suites_after_build_and_boot(self):
        steps = self.jobs["target"]["steps"]
        runs = [step for step in steps if step.get("name") == "Run suites"]
        self.assertEqual(len(runs), 1)
        self.assertEqual(runs[0]["run"], "mise run ci:target ${{ matrix.preset }}")
        # The default success() gate keeps it, and the uploads after it,
        # behind the build and boot steps.
        self.assertNotIn("if", runs[0])
        ready = max(
            index
            for index, step in enumerate(steps)
            if step.get("id") in ("build", "boot")
        )
        self.assertIs(steps[ready + 1], runs[0])

    def test_the_target_job_reads_every_matrix_field(self):
        text = (ROOT / ".github/workflows/ci-coverage.yml").read_text()
        for field in self.matrix[0]:
            with self.subTest(field=field):
                self.assertIn(f"matrix.{field}", text)

    def test_each_job_other_than_a_target_job_gates_on_the_plan(self):
        for job in plan.JOBS:
            with self.subTest(job=job):
                self.assertEqual(
                    self.jobs[job]["if"],
                    f"contains(fromJSON(needs.plan.outputs.jobs), '{job}')",
                )
                # A packaging job consumes the target matrix's artifacts, and
                # those of the jobs it names in JOB_NEEDS.
                if job not in ("docs", "support-matrix"):
                    self.assertEqual(
                        self.jobs[job]["needs"],
                        ["plan", "target", *plan.JOB_NEEDS.get(job, ())],
                    )

    def test_the_required_check_needs_every_job_that_can_fail_the_run(self):
        expected = set(self.jobs) - {"required", "support-issue"}
        self.assertEqual(set(self.jobs["required"]["needs"]), expected)


class GateTest(unittest.TestCase):
    def test_required_check_accepts_only_successes_and_skips(self):
        required = load_yaml(".github/workflows/ci-coverage.yml")["jobs"]["required"]
        step = required["steps"][0]
        needs = required["needs"]
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
        # Rubberstamp skips the units a change does not need, and an empty
        # matrix skips the target job.
        skipped = {job: {"result": "skipped"} for job in needs if job != "plan"}
        self.assertTrue(accepts({**results, **skipped}))
        # A failed or skipped plan skips every gated job, which must not pass.
        failed_plan = {**skipped, "plan": {"result": "failure"}}
        self.assertFalse(accepts(failed_plan, plan_result="failure"))
        self.assertFalse(accepts(results, plan_result="skipped"))
        for job in needs:
            for result in ("failure", "cancelled"):
                changed = {**results, job: {"result": result}}
                self.assertFalse(accepts(changed), (job, result))


class VendorTest(unittest.TestCase):
    def test_every_vendor_unit_is_a_rubberstamp_unit_and_every_unit_runs_a_job(self):
        units = set(load_yaml(".config/rubberstamp.yml")["ci"]["units"])
        owned = {target.unit for target in targets().values()}
        owned |= {unit for unit, _ in plan.JOBS.values()}
        self.assertEqual(owned, units)

    def test_every_runner_label_is_known(self):
        labels = set(
            load_yaml(".github/jactionlint.yaml")["self-hosted-runner"]["labels"]
        )
        # The GitHub-hosted labels that targets run on.
        labels |= {"windows-11-arm"}
        for target in targets().values():
            with self.subTest(preset=target.preset):
                self.assertIn(target.runner, labels)

    def test_each_device_target_boots_its_device(self):
        android = "tools/devices/android-serial x86_64"
        freebsd = ("FreeBSD virtual machine", "tools/devices/freebsd-host")
        self.assertEqual(
            {
                name: (target.device["name"], target.device["command"])
                for name, target in targets().items()
                if target.device
            },
            {
                "freebsd-x64-egl": freebsd,
                "freebsd-x64-vulkan": freebsd,
                "ios-simulator-arm64-metal": ("iOS simulator", "devices-cli get ios"),
                "tvos-simulator-arm64-metal": (
                    "tvOS simulator",
                    "devices-cli get tvos",
                ),
                "visionos-simulator-arm64-metal": (
                    "visionOS simulator",
                    "devices-cli get visionos",
                ),
                # The EGL target's suites run on API 26, as the root `test`
                # task and the device test scripts ask for it.
                "android-x64-egl": ("Android emulator", f"{android} --api 26"),
                "android-x64-vulkan": ("Android emulator", android),
                # Only x64 EGL runs on the OpenHarmony emulator.
                "ohos-x64-egl": ("OpenHarmony emulator", "tools/devices/ohos-key"),
                # Each emulated libc and architecture has a root of its own.
                **{
                    f"linux-{libc}-{arch}-{backend}": (
                        "QEMU user mode",
                        f"tools/devices/linux-root {libc} {arch}",
                    )
                    for libc in ("gnu", "musl")
                    for arch in ("riscv64", "loongarch64")
                    for backend in ("egl", "vulkan")
                },
            },
        )


class SuitesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.targets = targets()

    def test_typescript_linux_suites_publish_their_tested_addons(self):
        expected = {
            f"linux-{libc}-{arch}-{backend}"
            for libc, architectures in (
                ("gnu", ("x64", "arm64", "riscv64", "loongarch64")),
                ("musl", ("x64", "arm64")),
            )
            for arch in architectures
            for backend in ("egl", "vulkan")
        }
        tested = {
            name
            for name, target in self.targets.items()
            if target.platform.startswith("linux-")
            and any(
                suite.task == "//bindings/typescript:test"
                for suite in suites.target_suites(target)
            )
        }
        self.assertEqual(tested, expected)
        for name in expected:
            with self.subTest(preset=name):
                self.assertEqual(plan.addon(self.targets[name]), "typescript")

    def test_a_target_cannot_run_one_task_as_two_suites(self):
        extra = {
            "platforms": ["windows"],
            "commands": [{"task": "//bindings/rust:test", "args": ["release"]}],
        }
        with (
            mock.patch.object(suites, "SUITES", [*suites.SUITES, extra]),
            self.assertRaisesRegex(SystemExit, "'bindings-rust-test'"),
        ):
            suites.target_suites(self.targets["windows-x64-wgl"])

    def test_a_suite_environment_reaches_its_commands_in_the_listed_targets(self):
        extra = {
            "platforms": ["windows"],
            "commands": [
                {"task": "//probe:check"},
                {"task": "//probe:test"},
            ],
            "environments": [
                {"include": ["windows-x64-wgl"], "env": {"STEP_PROBE": "1"}}
            ],
        }
        presets = (
            "windows-x64-wgl",
            "windows-x64-vulkan",
            "windows-arm64-wgl",
            "windows-arm64-vulkan",
        )
        with mock.patch.object(suites, "SUITES", [*suites.SUITES, extra]):
            environments = {
                (preset, suite.command): suite.env
                for preset in presets
                for suite in suites.target_suites(self.targets[preset])
                if "STEP_PROBE" in suite.env or suite.task.startswith("//probe:")
            }
        # The packaged target's consumer environment still applies.
        probed = {"MISE_TASK_SKIP": "//:build", "STEP_PROBE": "1"}
        plain = {"MISE_TASK_SKIP": "//:build"}
        self.assertEqual(
            environments,
            {
                (preset, ("mise", "run", f"//probe:{task}", preset)): (
                    probed if preset == "windows-x64-wgl" else plain
                )
                for preset in presets
                for task in ("check", "test")
            },
        )

    def test_dart_ios_runs_on_the_simulator_and_builds_for_the_device(self):
        for preset, action in (
            ("ios-simulator-arm64-metal", "test:ios"),
            ("ios-arm64-metal", "build:mobile"),
        ):
            commands = [
                suite.command for suite in suites.target_suites(self.targets[preset])
            ]
            self.assertIn(
                ("mise", "run", f"//bindings/dart:{action}", preset), commands
            )

    def test_hermes_builds_mobile_hosts_and_tests_device_targets(self):
        for preset, action in (
            ("android-arm-egl", "build"),
            ("android-arm-vulkan", "build"),
            ("android-arm64-egl", "build"),
            ("android-arm64-vulkan", "build"),
            ("android-x86-egl", "build"),
            ("android-x86-vulkan", "build"),
            ("android-x64-egl", "test"),
            ("android-x64-vulkan", "test"),
            ("ios-arm64-metal", "build"),
            ("ios-simulator-arm64-metal", "test"),
            ("ios-maccatalyst-arm64-metal", "test"),
            ("tvos-arm64-metal", "build"),
            ("tvos-simulator-arm64-metal", "test"),
            ("visionos-arm64-metal", "build"),
            ("visionos-simulator-arm64-metal", "test"),
        ):
            commands = [
                suite.command for suite in suites.target_suites(self.targets[preset])
            ]
            self.assertEqual(
                commands.count(
                    ("mise", "run", f"//bindings/typescript:{action}:hermes", preset)
                ),
                1,
                preset,
            )


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
            ["Set up job"],
            ["Set up CI dependencies"],
            ["Boot Android emulator"],
            ["Boot FreeBSD virtual machine"],
            ["Resolve Swift packages"],
            ["Post Set up CI dependencies"],
        ):
            with self.subTest(failed=failed):
                self.assertTrue(retryable([job(target, *failed), *aggregates]))
        suite = "Run suites"
        for jobs in (
            [job(target, suite)],
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


if __name__ == "__main__":
    unittest.main()
