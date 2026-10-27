"""Check how the CI plan resolves targets from presets and selects jobs."""

from __future__ import annotations

import collections
import json
import unittest

from tools.ci import plan
from tools.ci.plan import ROOT


def presets(*configure: dict) -> dict:
    return {"configurePresets": list(configure)}


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

    def test_a_target_runs_ctest_unless_its_vendor_block_says_not(self):
        source = presets(
            preset("_device", hidden=True, vendor={"ctest": False}),
            preset("tested", vendor=RUNS, **TARGET),
            preset("untested", inherits="_device", vendor=RUNS, **TARGET),
        )
        self.assertEqual(
            [(target.preset, target.tested) for target in plan.targets(source)],
            [("tested", True), ("untested", False)],
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

    def test_a_preset_device_outranks_its_parents_device(self):
        parent = {**RUNS, "device": {"name": "parent", "command": "boot parent"}}
        own = {"device": {"name": "own", "command": "boot own"}}
        source = presets(
            preset("_parent", hidden=True, vendor=parent, **TARGET),
            preset("inherited", inherits="_parent"),
            preset("overridden", inherits="_parent", vendor=own),
        )
        self.assertEqual(
            {target.preset: target.device["name"] for target in plan.targets(source)},
            {"inherited": "parent", "overridden": "own"},
        )

    def test_a_device_needs_a_name_and_a_command(self):
        with self.assertRaisesRegex(SystemExit, "device without exactly a name"):
            plan.targets(
                presets(
                    preset("child", vendor={**RUNS, "device": {"name": "x"}}, **TARGET)
                )
            )


class PresetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.presets = json.loads((ROOT / "CMakePresets.json").read_text())
        cls.targets = {target.preset: target for target in plan.targets(cls.presets)}

    def test_musl_overrides_the_unit_and_inherits_the_gnu_runner(self):
        for architecture in ("x64", "arm64"):
            with self.subTest(architecture=architecture):
                musl = self.targets[f"linux-musl-{architecture}-egl"]
                gnu = self.targets[f"linux-gnu-{architecture}-egl"]
                self.assertEqual(
                    (musl.platform, musl.unit, musl.runner),
                    ("linux-musl", "linux-musl", gnu.runner),
                )
                self.assertEqual(gnu.unit, f"linux-{architecture}")

    def test_each_row_names_its_sdk_software_driver_and_zig_packages(self):
        # The platforms whose suites run every Zig project cache the packages,
        # those that run some fetch them, and the rest skip them.
        zig = {
            "linux-gnu": "cache",
            "macos": "cache",
            "linux-musl": "fetch",
            "android": "fetch",
            "ios-maccatalyst": "fetch",
            "windows": "fetch",
        }
        expected = {}
        for name, target in self.targets.items():
            mise_env = (
                target.platform if target.platform in ("emscripten", "ohos") else ""
            )
            # Only the Linux and Windows runners lack a GPU.
            gpu = target.platform in ("linux-gnu", "linux-musl", "windows")
            packages = zig.get(target.platform, "skip")
            # Windows ARM64 has no Zig toolchain.
            if name.startswith("windows-arm64-"):
                packages = "skip"
            expected[name] = (mise_env, target.backend if gpu else "", packages)
        matrix = plan.plan(self.presets, True, [])["matrix"]
        self.assertEqual(
            {
                row["preset"]: (row["mise_env"], row["software_gpu"], row["zig"])
                for row in matrix
            },
            expected,
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

    def selections(self) -> dict[str, tuple[bool, list[str]]]:
        units = sorted(
            {target.unit for target in self.targets.values()}
            | {unit for unit, _ in plan.JOBS.values()}
        )
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

    def test_a_selection_runs_its_units_targets_and_what_its_jobs_consume(self):
        all_targets = list(self.targets.values())
        for name, (run_all, units) in self.selections().items():
            with self.subTest(selection=name):
                planned = plan.plan(self.presets, run_all, units)
                selected = set(units) if not run_all else None
                owned = {
                    job
                    for job, (unit, _) in plan.JOBS.items()
                    if selected is None or unit in selected
                }
                owned |= {need for job in owned for need in plan.JOB_NEEDS.get(job, ())}
                jobs = [job for job in plan.JOBS if job in owned]
                self.assertEqual(planned["jobs"], jobs)
                expected = {
                    target.preset
                    for target in all_targets
                    if selected is None or target.unit in selected
                } | {
                    target.preset
                    for job in jobs
                    for target in plan.consumed(job, all_targets)
                }
                presets = [row["preset"] for row in planned["matrix"]]
                self.assertEqual(len(presets), len(set(presets)))
                self.assertEqual(set(presets), expected)

    def test_one_selected_row_per_runner_saves_toolchains(self):
        for name, (run_all, units) in self.selections().items():
            with self.subTest(selection=name):
                matrix = plan.plan(self.presets, run_all, units)["matrix"]
                saving = collections.Counter(
                    row["runner"] for row in matrix if row["save_toolchains"]
                )
                self.assertEqual(
                    saving, collections.Counter({row["runner"]: 1 for row in matrix})
                )

    def test_a_job_runs_the_jobs_whose_artifacts_it_downloads(self):
        self.assertEqual(
            plan.plan(self.presets, False, ["swiftpm"])["jobs"],
            ["apple-xcframework", "swift-package"],
        )

    def test_conan_runs_the_linux_archive_it_consumes(self):
        result = plan.plan(self.presets, False, ["conan"])
        self.assertEqual(result["jobs"], ["conan-recipes"])
        self.assertEqual(
            [row["preset"] for row in result["matrix"]], ["linux-gnu-x64-vulkan"]
        )

    def test_an_unknown_unit_fails(self):
        with self.assertRaisesRegex(SystemExit, "unknown coverage units: nope"):
            plan.plan(self.presets, False, ["nope"])


if __name__ == "__main__":
    unittest.main()
