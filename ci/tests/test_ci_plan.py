"""Check the preset-driven CI plan against the generated workflow's planning."""

from __future__ import annotations

import collections
import unittest

from ci import coverage, workflow
from ci.workflow import load_configuration
from tools.ci import plan
from tools.ci.plan import ROOT


def presets(*configure: dict, package: tuple[dict, ...] = ()) -> dict:
    return {"configurePresets": list(configure), "packagePresets": list(package)}


def preset(name: str, inherits=None, hidden=False, vendor=None, **variables) -> dict:
    result: dict = {"name": name, "cacheVariables": variables}
    if inherits is not None:
        result["inherits"] = inherits
    if hidden:
        result["hidden"] = True
    if vendor is not None:
        result["vendor"] = {plan.VENDOR: vendor}
    return result


TARGET = {
    "MLN_FFI_TARGET_PLATFORM": "linux-gnu-x64",
    "MLN_FFI_RENDER_BACKEND": "vulkan",
}
RUNS = {"unit": "linux-x64", "runner": "linux"}


class ResolverTest(unittest.TestCase):
    def only(self, source: dict) -> plan.Target:
        (target,) = plan.targets(source)
        return target

    def test_a_string_inherits_names_one_parent(self):
        target = self.only(
            presets(
                preset("_parent", hidden=True, vendor=RUNS, **TARGET),
                preset("child", inherits="_parent"),
            )
        )
        self.assertEqual(
            (target.platform, target.architecture, target.backend, target.unit),
            ("linux-gnu", "x64", "vulkan", "linux-x64"),
        )

    def test_a_list_inherits_takes_each_key_from_its_first_parent_depth_first(self):
        target = self.only(
            presets(
                preset("_grandparent", hidden=True, vendor={"runner": "deep"}),
                preset(
                    "_first",
                    inherits="_grandparent",
                    hidden=True,
                    MLN_FFI_TARGET_PLATFORM="first-x64",
                ),
                preset(
                    "_second",
                    hidden=True,
                    vendor={"unit": "second", "runner": "shallow"},
                    MLN_FFI_TARGET_PLATFORM="second-arm64",
                    MLN_FFI_RENDER_BACKEND="metal",
                ),
                preset("child", inherits=["_first", "_second"]),
            )
        )
        # The first parent's own ancestor outranks the second parent, and each
        # vendor field merges on its own.
        self.assertEqual(
            (target.platform, target.backend, target.unit, target.runner),
            ("first", "metal", "second", "deep"),
        )

    def test_a_preset_value_outranks_every_parent(self):
        target = self.only(
            presets(
                preset("_parent", hidden=True, vendor=RUNS, **TARGET),
                preset(
                    "child",
                    inherits="_parent",
                    vendor={"runner": "own"},
                    MLN_FFI_TARGET_PLATFORM="linux-gnu-arm64",
                ),
            )
        )
        self.assertEqual(
            (target.architecture, target.unit, target.runner),
            ("arm64", "linux-x64", "own"),
        )

    def test_an_opengl_target_takes_its_backend_from_the_context_provider(self):
        target = self.only(
            presets(
                preset(
                    "child",
                    vendor=RUNS,
                    **{
                        **TARGET,
                        "MLN_FFI_RENDER_BACKEND": "opengl",
                        "MLN_FFI_OPENGL_CONTEXT_PROVIDER": "egl",
                    },
                )
            )
        )
        self.assertEqual(target.backend, "egl")

    def test_a_package_preset_marks_its_configure_preset_packaged(self):
        source = presets(
            preset("packaged", vendor=RUNS, **TARGET),
            preset("unpackaged", vendor=RUNS, **TARGET),
            package=(
                {"name": "_package", "hidden": True, "configurePreset": "packaged"},
                {"name": "packaged", "inherits": "_package"},
            ),
        )
        self.assertEqual(
            [(target.preset, target.package) for target in plan.targets(source)],
            [("packaged", True), ("unpackaged", False)],
        )

    def test_a_missing_parent_fails(self):
        with self.assertRaisesRegex(SystemExit, "unknown preset '_gone'"):
            plan.targets(presets(preset("child", inherits=["_gone"])))

    def test_an_inheritance_cycle_fails(self):
        with self.assertRaisesRegex(SystemExit, "cycle: child -> _a -> _b -> _a"):
            plan.targets(
                presets(
                    preset("_a", inherits="_b", hidden=True),
                    preset("_b", inherits="_a", hidden=True),
                    preset("child", inherits="_a"),
                )
            )

    def test_a_visible_preset_without_a_unit_or_runner_fails(self):
        for vendor, missing in (({"unit": "u"}, "runner"), ({"runner": "r"}, "unit")):
            with (
                self.subTest(missing=missing),
                self.assertRaisesRegex(SystemExit, f"resolves no vendor {missing}"),
            ):
                plan.targets(presets(preset("child", vendor=vendor, **TARGET)))

    def test_a_hidden_preset_needs_no_unit_or_runner(self):
        self.assertEqual(plan.targets(presets(preset("_parent", hidden=True))), [])


class PresetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)
        cls.targets = {target.preset: target for target in plan.targets(cls.presets)}

    def test_musl_overrides_the_unit_and_inherits_the_runner(self):
        for architecture in ("x64", "arm64"):
            with self.subTest(architecture=architecture):
                musl = self.targets[f"linux-musl-{architecture}-egl"]
                gnu = self.targets[f"linux-gnu-{architecture}-egl"]
                self.assertEqual(
                    (musl.platform, musl.unit, musl.runner),
                    ("linux-musl", "linux-musl", gnu.runner),
                )

    def test_apple_simulators_and_catalyst_override_the_platform(self):
        self.assertEqual(
            {
                name: self.targets[name].platform
                for name in (
                    "ios-arm64-metal",
                    "ios-simulator-arm64-metal",
                    "ios-maccatalyst-arm64-metal",
                    "tvos-simulator-arm64-metal",
                    "visionos-simulator-arm64-metal",
                )
            },
            {
                "ios-arm64-metal": "ios",
                "ios-simulator-arm64-metal": "ios-simulator",
                "ios-maccatalyst-arm64-metal": "ios-maccatalyst",
                "tvos-simulator-arm64-metal": "tvos-simulator",
                "visionos-simulator-arm64-metal": "visionos-simulator",
            },
        )

    def test_every_target_matches_the_generated_workflow(self):
        rows = coverage.target_rows(self.presets)
        self.assertEqual(list(self.targets), [row["preset"] for row in rows])
        for row in rows:
            name = row["preset"]
            with self.subTest(preset=name):
                target = self.targets[name]
                self.assertEqual(
                    (
                        target.platform,
                        target.architecture,
                        target.backend,
                        target.runner,
                        target.package,
                    ),
                    (
                        workflow.platform(name),
                        workflow.architecture(name),
                        workflow.backend(name),
                        workflow.runner(name),
                        row["package"],
                    ),
                )
                self.assertEqual(row["runner"], target.runner)

    def test_packaging_jobs_consume_the_generated_workflow_dependencies(self):
        expected = coverage.dependencies(self.presets)
        all_targets = list(self.targets.values())
        for job in expected:
            with self.subTest(job=job):
                self.assertEqual(
                    [f"target-{t.preset}" for t in plan.consumed(job, all_targets)],
                    expected[job],
                )
        self.assertEqual(plan.consumed("docs", all_targets), [])

    def selections(self) -> dict[str, tuple[bool, list[str]]]:
        units = list(self.source["units"])
        return {
            "all": (True, []),
            "empty": (False, []),
            "docs only": (False, ["docs"]),
            **{unit: (False, [unit]) for unit in units},
            "linux and maven": (False, ["linux-x64", "maven"]),
            "apple": (False, ["macos", "apple-mobile"]),
            "android and maven": (False, ["android", "maven"]),
            "web, ohos and freebsd": (False, ["web", "ohos", "freebsd"]),
            "every unit": (False, units),
        }

    def test_every_selection_runs_the_generated_workflow_jobs(self):
        running = coverage.job_units(self.source, self.presets)
        for name, (run_all, units) in self.selections().items():
            with self.subTest(selection=name):
                planned = plan.plan(self.presets, run_all, units)
                selected = [
                    *(f"target-{row['preset']}" for row in planned["matrix"]),
                    *planned["jobs"],
                ]
                expected = [
                    job
                    for job, job_units in running.items()
                    if run_all or set(units) & set(job_units)
                ]
                self.assertEqual(sorted(selected), sorted(expected))
                self.assertEqual(len(selected), len(set(selected)))

    def test_one_selected_row_per_runner_saves_toolchains(self):
        rows = coverage.target_rows(self.presets)
        everything = plan.plan(self.presets, True, [])["matrix"]
        self.assertEqual(
            [row["save_toolchains"] for row in everything],
            [row["save_toolchains"] for row in rows],
        )
        for name, (run_all, units) in self.selections().items():
            with self.subTest(selection=name):
                matrix = plan.plan(self.presets, run_all, units)["matrix"]
                saving = collections.Counter(
                    row["runner"] for row in matrix if row["save_toolchains"]
                )
                self.assertEqual(
                    saving, collections.Counter({row["runner"]: 1 for row in matrix})
                )

    def test_an_unknown_unit_fails(self):
        with self.assertRaisesRegex(SystemExit, "unknown coverage units: nope"):
            plan.plan(self.presets, False, ["nope"])


if __name__ == "__main__":
    unittest.main()
