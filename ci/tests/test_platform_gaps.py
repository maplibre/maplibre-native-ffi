"""The gap file is the only permission to omit a binding from CI."""

from __future__ import annotations

import dataclasses
import importlib.util
import json
import sys
import unittest
from unittest import mock

from tools.ci import gaps, plan, suites


def matrix_module():
    spec = importlib.util.spec_from_file_location(
        "support_matrix", plan.ROOT / "docs/scripts/generate-support-matrix.py"
    )
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class PlatformGapsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.presets = json.loads((plan.ROOT / "CMakePresets.json").read_text())
        cls.targets = {target.preset: target for target in plan.targets(cls.presets)}

    def test_removing_a_gap_enables_tests_and_package_inputs(self):
        target = self.targets["linux-gnu-riscv64"]
        self.assertNotIn(
            "//bindings/dotnet:test",
            {suite.task for suite in suites.target_suites(target)},
        )
        self.assertNotIn(
            target, plan.consumed("dotnet-nuget", list(self.targets.values()))
        )
        issue, entries = gaps.load_gaps()
        remaining = [
            entry
            for entry in entries
            if not any(entry.matches("dotnet", cell) for cell in gaps.cells(target))
        ]
        with mock.patch.object(gaps, "load_gaps", return_value=(issue, remaining)):
            self.assertIn(
                "//bindings/dotnet:test",
                {suite.task for suite in suites.target_suites(target)},
            )
            self.assertIn(
                target, plan.consumed("dotnet-nuget", list(self.targets.values()))
            )
            self.assertEqual(
                suites.command_support(
                    f"mise run //bindings/dotnet:test {target.preset} vulkan"
                ),
                [("dotnet", "tested")],
            )

    def test_jvm_runtime_jars_follow_package_gaps(self):
        target = self.targets["macos-x64"]
        runtimes = plan.jvm_runtimes(list(self.targets.values()))
        self.assertEqual(runtimes["natives-macos-x64"], "macos-x64")
        for arch in ("x64", "arm64"):
            self.assertEqual(runtimes[f"natives-windows-{arch}"], f"windows-{arch}")
            self.assertEqual(
                runtimes[f"natives-windows-mingw-{arch}"], f"windows-mingw-{arch}"
            )
        entry = gaps.Entry(
            0,
            {
                "bindings": ("kotlin-jvm",),
                "platforms": ("macos",),
                "archs": ("x64",),
                "operations": ("package",),
            },
            "path-known",
            "runtime JAR needed",
            None,
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            self.assertEqual(plan.jvm_runtimes([target]), {})
        entry = dataclasses.replace(
            entry, selectors={**entry.selectors, "operations": ("test",)}
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            self.assertEqual(
                plan.jvm_runtimes([target]), {"natives-macos-x64": "macos-x64"}
            )

    def test_windows_native_kotlin_tests_do_not_imply_a_runtime_package(self):
        target = self.targets["windows-x64"]
        self.assertTrue(gaps.supported("kotlin-native", target, "build"))
        self.assertTrue(gaps.supported("kotlin-native", target, "test"))
        self.assertFalse(gaps.supported("kotlin-native", target, "package"))
        self.assertIn(
            "//bindings/kotlin:test:native",
            {suite.task for suite in suites.target_suites(target)},
        )
        self.assertFalse(
            gaps.supported("kotlin-native", self.targets["windows-arm64"], "test")
        )
        self.assertNotIn(
            "windows-x64",
            plan.package_platforms("kotlin-native", list(self.targets.values())),
        )

    def test_nuget_inputs_match_its_packager_and_preserve_macos_x64(self):
        selected = plan.consumed("dotnet-nuget", list(self.targets.values()))
        self.assertEqual(
            {target.preset for target in selected},
            {name for name in self.targets if plan.dotnet_runtime_target(name)},
        )
        target = self.targets["macos-x64"]
        self.assertIn(target, selected)
        self.assertEqual(plan.dotnet_runtime_target(target.preset), ("", "osx-x64"))
        self.assertTrue(gaps.supported("dotnet", target))
        self.assertIn(
            "//bindings/dotnet:test",
            {suite.task for suite in suites.target_suites(target)},
        )
        self.assertEqual(plan.dotnet_runtime_target("windows-x64"), ("", "win-x64"))
        self.assertEqual(plan.dotnet_runtime_target("mingw-x64"), (".MinGW", "win-x64"))
        self.assertIn(
            "//bindings/dotnet:test",
            {suite.task for suite in suites.target_suites(self.targets["mingw-x64"])},
        )

    def test_package_gaps_control_each_planner_without_disabling_tests(self):
        targets = list(self.targets.values())
        cases = (
            ("dotnet", "dotnet-nuget", "linux-gnu-x64"),
            ("python", "python-wheels", "linux-gnu-x64"),
            ("typescript", "typescript-npm", "linux-gnu-x64"),
            ("typescript", "arkts-ohpm", "ohos-x64"),
            ("kotlin-jvm", "kotlin-maven", "freebsd-x64"),
            ("swift", "swift-package", "macos-arm64"),
            ("swift", "swift-artifact-bundles", "linux-gnu-x64"),
        )
        issue, entries = gaps.load_gaps()
        for binding, job, preset in cases:
            target = self.targets[preset]
            metadata = gaps.cell(target, target.default_backend)
            # A package carries every backend, so a package gap names them all.
            entry = gaps.Entry(
                len(entries),
                {
                    "bindings": (binding,),
                    "platforms": (metadata.platform,),
                    "archs": (metadata.arch,),
                    "operations": ("package",),
                },
                "path-known",
                "package integration needed",
                None,
            )
            with self.subTest(job=job):
                self.assertIn(target, plan.consumed(job, targets))
                original = suites.consumer_commands(target)
                with mock.patch.object(
                    gaps, "load_gaps", return_value=(issue, [*entries, entry])
                ):
                    self.assertNotIn(target, plan.consumed(job, targets))
                    self.assertEqual(suites.consumer_commands(target), original)
                    if binding == "typescript":
                        self.assertEqual(plan.addon(target), "")
                    if binding == "python":
                        self.assertFalse(plan.python_wheel(target))

    def test_test_gaps_fall_back_to_build_without_removing_package_inputs(self):
        issue, entries = gaps.load_gaps()
        for binding in (
            "dotnet",
            "python",
            "typescript",
            "kotlin-jvm",
            "kotlin-native",
        ):
            target = self.targets["linux-gnu-x64"]
            entry = gaps.Entry(
                len(entries),
                {
                    "bindings": (binding,),
                    "platforms": ("linux-gnu",),
                    "archs": ("x64",),
                    "operations": ("test",),
                },
                "path-known",
                "test runner needed",
                None,
            )
            with (
                self.subTest(binding=binding),
                mock.patch.object(
                    gaps, "load_gaps", return_value=(issue, [*entries, entry])
                ),
            ):
                self.assertEqual(gaps.tested_backends(binding, target), ())
                commands = [
                    command
                    for command, backend in suites.consumer_commands(target)
                    if suites.LINUX_ARM64_KLIB not in command
                    and any(
                        name == binding
                        for name, _ in suites.command_support(" ".join(command))
                    )
                ]
                self.assertEqual(
                    [suites.command_support(" ".join(command)) for command in commands],
                    [[(binding, "build-only")]],
                )
                self.assertTrue(plan.packaged(binding)(target))

    def test_a_backend_test_gap_keeps_the_targets_other_backends(self):
        issue, entries = gaps.load_gaps()
        target = self.targets["linux-gnu-x64"]
        entry = gaps.Entry(
            len(entries),
            {
                "bindings": ("go",),
                "platforms": ("linux-gnu",),
                "backends": ("opengl",),
                "operations": ("test",),
            },
            "path-known",
            "driver needed",
            None,
        )
        with mock.patch.object(
            gaps, "load_gaps", return_value=(issue, [*entries, entry])
        ):
            self.assertEqual(gaps.tested_backends("go", target), ("vulkan",))
            self.assertIn(
                (
                    ["mise", "run", "//bindings/go:test", target.preset, "vulkan"],
                    "vulkan",
                ),
                suites.consumer_commands(target),
            )
            self.assertNotIn(
                ["mise", "run", "//bindings/go:test", target.preset, "opengl"],
                [command for command, _ in suites.consumer_commands(target)],
            )

    def test_unlisted_nuget_platform_fails_instead_of_silently_excluding_it(self):
        with (
            mock.patch.object(gaps, "load_gaps", return_value=(936, [])),
            self.assertRaises(KeyError),
        ):
            plan.dotnet_runtime_target(
                dataclasses.replace(self.targets["linux-gnu-x64"], platform="futureos")
            )

    def test_xcframework_includes_both_macos_archives(self):
        inputs = plan.consumed("apple-xcframework", list(self.targets.values()))
        for arch in ("arm64", "x64"):
            target = self.targets[f"macos-{arch}"]
            self.assertTrue(gaps.supported("c", target, "package"))
            self.assertIn(target, inputs)

    def test_no_gap_means_every_binding_runs_on_every_runner_family(self):
        names = (
            "linux-gnu-riscv64",
            "linux-musl-loongarch64",
            "macos-x64",
            "windows-arm64",
            "mingw-x64",
            "freebsd-x64",
            "web-wasm32-webgl",
            "ohos-x64",
            "android-x64",
            "android-arm64",
            "ios-arm64",
            "ios-simulator-arm64",
            "ios-maccatalyst-arm64",
            "tvos-arm64",
            "visionos-simulator-arm64",
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [])):
            for name in names:
                target = self.targets[name]
                with self.subTest(preset=name):
                    for backend in target.backends:
                        self.assertIn(
                            (["mise", "run", "test", name, backend], backend),
                            suites.native_checks(target),
                        )
                    commands = suites.consumer_commands(target)
                    for backend in target.backends:
                        bindings = {
                            binding
                            for command, tested in commands
                            if tested == backend
                            for binding, _ in suites.command_support(" ".join(command))
                        }
                        self.assertEqual(bindings, set(gaps.BINDING_LABELS) - {"c"})
            # A new architecture inherits support without another opt-in knob.
            target = dataclasses.replace(
                self.targets[names[0]], architecture="future64"
            )
            # Each binding tests each backend, and the GObject library archives.
            self.assertEqual(
                len(suites.consumer_commands(target)),
                (len(gaps.BINDING_LABELS) - 1) * len(target.backends) + 1,
            )

    def test_selectors_normalize_backends_and_apple_simulators(self):
        entry = gaps.Entry(
            0,
            {"bindings": ("go",), "platforms": ("ios",), "backends": ("metal",)},
            "path-known",
            "runner needed",
            None,
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            for name in ("ios-arm64", "ios-simulator-arm64"):
                self.assertFalse(gaps.supported("go", self.targets[name]))
                self.assertTrue(gaps.supported("swift", self.targets[name]))
            self.assertTrue(gaps.supported("go", self.targets["tvos-arm64"]))
        entry = dataclasses.replace(
            entry,
            selectors={
                "bindings": ("go",),
                "platforms": ("linux-gnu",),
                "backends": ("opengl",),
            },
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            target = self.targets["linux-gnu-x64"]
            self.assertFalse(gaps.supported("go", target, "test", "opengl"))
            self.assertTrue(gaps.supported("go", target, "test", "vulkan"))
            # The package carries both, so the binding still builds.
            self.assertTrue(gaps.supported("go", target, "build"))

    def test_a_native_gap_excludes_its_bindings_and_target_job(self):
        target = self.targets["linux-gnu-x64"]
        entry = gaps.Entry(
            0,
            {
                "bindings": ("c",),
                "platforms": ("linux-gnu",),
                "archs": ("x64",),
            },
            "path-known",
            "native build needed",
            None,
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            self.assertEqual(suites.consumer_commands(target), [])
            result = plan.plan(self.presets, False, ["linux-x64"])
            self.assertNotIn(target.preset, {row["preset"] for row in result["matrix"]})
        # A native test gap on one backend leaves the other's suites.
        entry = dataclasses.replace(
            entry,
            selectors={
                **entry.selectors,
                "backends": ("vulkan",),
                "operations": ("test",),
            },
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            self.assertEqual(
                suites.native_checks(target)[0],
                (["mise", "run", "test", target.preset, "opengl"], "opengl"),
            )
            self.assertNotIn(
                "vulkan",
                {backend for _, backend in suites.consumer_commands(target)},
            )

    def test_the_matrix_rejects_a_missing_suite_without_a_gap(self):
        matrix = matrix_module()
        with (
            mock.patch.object(suites, "consumer_commands", return_value=[]),
            self.assertRaisesRegex(SystemExit, "CI does not reach.*support-gaps.toml"),
        ):
            matrix.support_matrix()

    def test_every_current_missing_cell_has_an_explicit_gap(self):
        matrix = matrix_module().support_matrix()
        for platform in matrix["platforms"]:
            for arch in platform["architectures"]:
                for backend in arch["backends"]:
                    target = gaps.Target(
                        platform["platform"], arch["arch"], backend["backend"]
                    )
                    for binding, cell in zip(matrix["bindings"], backend["cells"]):
                        if cell["status"] not in {"tested", "build-only"}:
                            self.assertIsNotNone(
                                gaps.gap(binding["id"], target), (binding, target)
                            )
