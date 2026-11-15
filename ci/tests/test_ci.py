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
            ["android-x86", "android-x64"],
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
        android = "tools/devices/android-serial"
        freebsd = ("FreeBSD virtual machine", "tools/devices/freebsd-host")
        self.assertEqual(
            {
                name: (target.device["name"], target.device["command"])
                for name, target in targets().items()
                if target.device
            },
            {
                "freebsd-x64": freebsd,
                "ios-simulator-arm64": ("iOS simulator", "devices-cli get ios"),
                "tvos-simulator-arm64": ("tvOS simulator", "devices-cli get tvos"),
                "visionos-simulator-arm64": (
                    "visionOS simulator",
                    "devices-cli get visionos",
                ),
                "android-x86": ("Android emulator", f"{android} android-x86"),
                "android-x64": ("Android emulator", f"{android} android-x64"),
                "ohos-x64": ("OpenHarmony emulator", "tools/devices/ohos-key"),
                # Each emulated libc and architecture has a root of its own.
                **{
                    f"linux-{libc}-{arch}": (
                        "QEMU user mode",
                        f"tools/devices/linux-root {libc} {arch}",
                    )
                    for libc in ("gnu", "musl")
                    for arch in ("riscv64", "loongarch64")
                },
            },
        )


class SuitesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.targets = targets()

    def test_typescript_linux_and_freebsd_suites_publish_their_addons(self):
        expected = {
            f"linux-{libc}-{arch}"
            for libc in ("gnu", "musl")
            for arch in ("x64", "arm64", "riscv64", "loongarch64")
        } | {"freebsd-x64"}
        build_only = {"linux-gnu-riscv64"}
        commands = {
            name: [
                suite.command[2:]
                for suite in suites.target_suites(target)
                if suite.task
                in {"//bindings/typescript:test", "//bindings/typescript:build"}
            ]
            for name, target in self.targets.items()
            if (target.platform.startswith("linux-") or target.platform == "freebsd")
        }
        self.assertEqual(
            commands,
            {
                name: [("//bindings/typescript:build", name)]
                if name in build_only
                else [
                    ("//bindings/typescript:test", name, "vulkan"),
                    ("//bindings/typescript:test", name, "opengl"),
                ]
                for name in expected
            },
        )
        for name in expected:
            with self.subTest(preset=name):
                self.assertEqual(plan.addon(self.targets[name]), "typescript")

    def test_a_target_cannot_run_one_task_as_two_suites(self):
        target = self.targets["windows-x64"]
        command = ["mise", "run", "//bindings/go:test", target.preset, "opengl"]
        with (
            mock.patch.object(
                suites,
                "consumer_commands",
                return_value=[(command, "opengl"), (command, "opengl")],
            ),
            self.assertRaisesRegex(SystemExit, "'bindings-go-test-opengl'"),
        ):
            suites.target_suites(target)

    def test_a_target_runs_each_backends_native_suites_once(self):
        for name, backends in (
            ("linux-gnu-x64", ("vulkan", "opengl")),
            ("windows-x64", ("vulkan", "opengl")),
            ("macos-arm64", ("metal",)),
            # The x86 emulator can't run Vulkan, as support-gaps.toml records.
            ("android-x86", ("opengl",)),
            # No ARM Android device is configured.
            ("android-arm64", ()),
        ):
            with self.subTest(preset=name):
                self.assertEqual(
                    [
                        (suite.command, suite.backend)
                        for suite in suites.target_suites(self.targets[name])
                        if suite.task == "test"
                    ],
                    [
                        (("mise", "run", "test", name, backend), backend)
                        for backend in backends
                    ],
                )

    def test_a_binding_tests_each_backend_or_builds_once(self):
        go = {
            name: [
                suite.command[2:]
                for suite in suites.target_suites(self.targets[name])
                if suite.task.startswith("//bindings/go:")
                and suite.task != "//bindings/go:test:package"
            ]
            for name in ("linux-gnu-x64", "android-x86", "android-arm64")
        }
        self.assertEqual(
            go,
            {
                "linux-gnu-x64": [
                    ("//bindings/go:test", "linux-gnu-x64", "vulkan"),
                    ("//bindings/go:test", "linux-gnu-x64", "opengl"),
                ],
                "android-x86": [("//bindings/go:test", "android-x86", "opengl")],
                "android-arm64": [("//bindings/go:build", "android-arm64")],
            },
        )

    def test_only_the_windows_vulkan_native_suites_need_the_desktop(self):
        wrapped = {
            (target.preset, suite.backend)
            for target in self.targets.values()
            for suite in suites.target_suites(target)
            if suite.wrapper == "windows-console"
        }
        self.assertEqual(wrapped, {("windows-x64", "vulkan"), ("mingw-x64", "vulkan")})

    def test_consumer_suites_use_the_packaged_install(self):
        for target in self.targets.values():
            for suite in suites.target_suites(target):
                if suite.task.startswith("//bindings/"):
                    with self.subTest(preset=target.preset, task=suite.task):
                        self.assertEqual(suite.env, {"MISE_TASK_SKIP": "//:build"})
                        self.assertIn("archive", suite.prerequisites)
                        self.assertIn("install", suite.prerequisites)

    def test_dart_ios_runs_on_the_simulator_and_builds_for_the_device(self):
        for preset, command in (
            ("ios-simulator-arm64", ("//bindings/dart:test:ios", "metal")),
            ("ios-arm64", ("//bindings/dart:build:mobile",)),
        ):
            commands = [
                suite.command for suite in suites.target_suites(self.targets[preset])
            ]
            self.assertIn(("mise", "run", command[0], preset, *command[1:]), commands)

    def test_swift_runs_on_windows_msvc_and_mingw(self):
        for preset in ("windows-x64", "windows-arm64", "mingw-x64"):
            for backend in ("vulkan", "opengl"):
                with self.subTest(preset=preset, backend=backend):
                    commands = [
                        suite.command
                        for suite in suites.target_suites(self.targets[preset])
                    ]
                    self.assertIn(
                        ("mise", "run", "//bindings/swift:test", preset, backend),
                        commands,
                    )

    def test_swift_builds_android_arm_targets_and_tests_x64(self):
        for architecture in ("arm", "arm64", "x64", "x86"):
            preset = f"android-{architecture}"
            with self.subTest(preset=preset):
                commands = [
                    suite.command[2:]
                    for suite in suites.target_suites(self.targets[preset])
                    if suite.task.startswith("//bindings/swift:")
                ]
                expected = {
                    "x86": [],
                    "x64": [
                        ("//bindings/swift:test", preset, "vulkan"),
                        ("//bindings/swift:test", preset, "opengl"),
                    ],
                }.get(architecture, [("//bindings/swift:build", preset)])
                self.assertEqual(commands, expected)

    def test_dotnet_android_builds_packages_and_tests_emulator_targets(self):
        for preset, expected in (
            ("android-arm", [("build",)]),
            ("android-arm64", [("build",)]),
            ("android-x86", [("test", "opengl")]),
            ("android-x64", [("test", "vulkan"), ("test", "opengl")]),
        ):
            commands = [
                suite.command[2:]
                for suite in suites.target_suites(self.targets[preset])
                if suite.task.startswith("//bindings/dotnet:")
            ]
            self.assertEqual(
                commands,
                [
                    (f"//bindings/dotnet:{action}", preset, *backend)
                    for action, *backend in expected
                ],
            )

    def test_dotnet_apple_builds_devices_and_tests_simulators_and_catalyst(self):
        for preset, action in (
            ("ios-arm64", "build"),
            ("ios-simulator-arm64", "test"),
            ("ios-maccatalyst-arm64", "test"),
            ("tvos-arm64", "build"),
            ("tvos-simulator-arm64", "test"),
        ):
            commands = [
                suite.command for suite in suites.target_suites(self.targets[preset])
            ]
            backend = ("metal",) if action == "test" else ()
            self.assertIn(
                ("mise", "run", f"//bindings/dotnet:{action}", preset, *backend),
                commands,
            )

    def test_hermes_builds_mobile_hosts_and_tests_device_targets(self):
        for preset, expected in (
            ("android-arm", [()]),
            ("android-arm64", [()]),
            ("android-x86", [("opengl",)]),
            ("android-x64", [("vulkan",), ("opengl",)]),
            ("ios-arm64", [()]),
            ("ios-simulator-arm64", [("metal",)]),
            ("ios-maccatalyst-arm64", [("metal",)]),
            ("tvos-arm64", [()]),
            ("tvos-simulator-arm64", [("metal",)]),
            ("visionos-arm64", [()]),
            ("visionos-simulator-arm64", [("metal",)]),
        ):
            commands = [
                suite.command[2:]
                for suite in suites.target_suites(self.targets[preset])
                if suite.task.endswith(":hermes")
            ]
            self.assertEqual(
                commands,
                [
                    (
                        f"//bindings/typescript:{'test' if backend else 'build'}:hermes",
                        preset,
                        *backend,
                    )
                    for backend in expected
                ],
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
        target = "target / android-x64"
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
                    job("target / ohos-x64", "Set up CI dependencies"),
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
