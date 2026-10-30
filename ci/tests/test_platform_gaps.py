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
        target = self.targets["linux-gnu-riscv64-vulkan"]
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
            if not entry.matches("dotnet", gaps.target_from_preset(target))
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
                    "mise run //bindings/dotnet:test " + target.preset
                ),
                [("dotnet", "tested")],
            )

    def test_nuget_inputs_match_its_packager_and_preserve_untested_assets(self):
        selected = plan.consumed("dotnet-nuget", list(self.targets.values()))
        self.assertEqual(
            {target.preset for target in selected},
            {name for name in self.targets if plan.dotnet_runtime_target(name)},
        )
        target = self.targets["macos-x64-metal"]
        self.assertIn(target, selected)
        self.assertEqual(
            plan.dotnet_runtime_target(target.preset), ("Metal", "osx-x64")
        )
        self.assertFalse(gaps.supported("dotnet", target))
        self.assertIn(
            "//bindings/dotnet:build",
            {suite.task for suite in suites.target_suites(target)},
        )
        self.assertIsNone(plan.dotnet_runtime_target("mingw-x64-vulkan"))
        self.assertIn(
            "//bindings/dotnet:test",
            {
                suite.task
                for suite in suites.target_suites(self.targets["mingw-x64-vulkan"])
            },
        )

    def test_package_gaps_control_each_planner_without_disabling_tests(self):
        targets = list(self.targets.values())
        cases = (
            ("dotnet", "dotnet-nuget", "linux-gnu-x64-vulkan"),
            ("python", "python-wheels", "linux-gnu-x64-vulkan"),
            ("typescript", "typescript-npm", "linux-gnu-x64-vulkan"),
            ("typescript", "arkts-ohpm", "ohos-x64-egl"),
            ("kotlin-jvm", "kotlin-maven", "freebsd-x64-egl"),
            ("swift", "swift-package", "macos-arm64-metal"),
        )
        issue, entries = gaps.load_gaps()
        for binding, job, preset in cases:
            target = self.targets[preset]
            metadata = gaps.target_from_preset(target)
            entry = gaps.Entry(
                len(entries),
                {
                    "bindings": (binding,),
                    "platforms": (metadata.platform,),
                    "archs": (metadata.arch,),
                    "backends": (metadata.backend,),
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
            target = self.targets["linux-gnu-x64-vulkan"]
            entry = gaps.Entry(
                len(entries),
                {
                    "bindings": (binding,),
                    "platforms": ("linux-gnu",),
                    "archs": ("x64",),
                    "backends": ("vulkan",),
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
                command = suites.binding_command(binding, target)
                self.assertEqual(
                    suites.command_support(" ".join(command)), [(binding, "build-only")]
                )
                self.assertTrue(plan.packaged(binding)(target))

    def test_unlisted_nuget_platform_fails_instead_of_silently_excluding_it(self):
        with (
            mock.patch.object(gaps, "load_gaps", return_value=(936, [])),
            self.assertRaises(KeyError),
        ):
            plan.dotnet_runtime_target("futureos-x64-vulkan")

    def test_xcframework_gap_preserves_the_per_target_native_archive(self):
        target = self.targets["macos-x64-metal"]
        self.assertFalse(gaps.supported("c", target, "xcframework"))
        self.assertTrue(gaps.supported("c", target, "package"))
        self.assertNotIn(
            target, plan.consumed("apple-xcframework", list(self.targets.values()))
        )

    def test_no_gap_means_every_binding_runs_on_every_runner_family(self):
        names = (
            "linux-gnu-riscv64-egl",
            "linux-musl-loongarch64-vulkan",
            "macos-x64-metal",
            "windows-arm64-wgl",
            "mingw-x64-vulkan",
            "freebsd-x64-egl",
            "emscripten-wasm32-webgl",
            "ohos-x64-egl",
            "android-x64-egl",
            "android-arm64-egl",
            "ios-arm64-metal",
            "ios-simulator-arm64-metal",
            "ios-maccatalyst-arm64-metal",
            "tvos-arm64-metal",
            "visionos-simulator-arm64-metal",
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [])):
            for name in names:
                with self.subTest(preset=name):
                    self.assertIn(
                        ["mise", "run", "test", name],
                        suites.native_checks(self.targets[name]),
                    )
                    commands = suites.consumer_commands(self.targets[name])
                    bindings = {
                        binding
                        for command in commands
                        for binding, _ in suites.command_support(" ".join(command))
                    }
                    self.assertEqual(bindings, set(gaps.BINDING_LABELS) - {"c"})
            # A new architecture inherits support without another opt-in knob.
            target = dataclasses.replace(
                self.targets[names[0]], architecture="future64"
            )
            self.assertEqual(
                len(suites.consumer_commands(target)), len(gaps.BINDING_LABELS) - 1
            )

    def test_selectors_normalize_backends_and_apple_simulators(self):
        entry = gaps.Entry(
            0,
            {"bindings": ("rust",), "platforms": ("ios",), "backends": ("metal",)},
            "path-known",
            "runner needed",
            None,
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            for name in ("ios-arm64-metal", "ios-simulator-arm64-metal"):
                self.assertFalse(gaps.supported("rust", self.targets[name]))
                self.assertTrue(gaps.supported("swift", self.targets[name]))
            self.assertTrue(gaps.supported("rust", self.targets["tvos-arm64-metal"]))
        entry = dataclasses.replace(
            entry,
            selectors={
                "bindings": ("rust",),
                "platforms": ("linux-gnu",),
                "backends": ("opengl",),
            },
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            self.assertFalse(gaps.supported("rust", self.targets["linux-gnu-x64-egl"]))
            self.assertTrue(
                gaps.supported("rust", self.targets["linux-gnu-x64-vulkan"])
            )

    def test_a_native_gap_excludes_its_bindings_and_target_job(self):
        target = self.targets["linux-gnu-x64-vulkan"]
        entry = gaps.Entry(
            0,
            {
                "bindings": ("c",),
                "platforms": ("linux-gnu",),
                "archs": ("x64",),
                "backends": ("vulkan",),
            },
            "path-known",
            "native build needed",
            None,
        )
        with mock.patch.object(gaps, "load_gaps", return_value=(936, [entry])):
            self.assertEqual(suites.consumer_commands(target), [])
            result = plan.plan(self.presets, False, ["linux-x64"])
            self.assertNotIn(target.preset, {row["preset"] for row in result["matrix"]})
            self.assertIn(
                "linux-gnu-x64-egl", {row["preset"] for row in result["matrix"]}
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
