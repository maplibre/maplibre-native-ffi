"""Plan the CI matrix from the targets that `CMakePresets.json` defines.

Each visible configure preset is a target. Its `vendor.maplibre-native-ffi`
block, usually inherited from a hidden platform preset, names the Rubberstamp
coverage unit that runs it, the runner label it runs on, and the device its
suites run on, if any. A device is the name of the CI step that boots it and
the command that step runs. `ctest: false` marks a target whose native tests
CI runs only on the device the target names, if any. `miseEnv` names the mise
configuration environment that supplies the target's SDK, and `softwareGpu`
asks for a software driver for the target's backend on a runner without a
GPU. This script uses only the standard library, so CI can run it with the
runner's `python3` from a sparse checkout.
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib

from tools.ci.suites import OPT_IN_ARCHITECTURES, target_suites

ROOT = pathlib.Path(__file__).resolve().parents[2]
VENDOR = "maplibre-native-ffi"

# These platforms supply native inputs to Kotlin Maven staging.
KOTLIN_MAVEN_PLATFORMS = {
    "android",
    "emscripten",
    "freebsd",
    "linux-gnu",
    "linux-musl",
    "macos",
    "ios",
    "ios-simulator",
    "tvos",
    "tvos-simulator",
    "windows",
}

# These platforms supply the runtime packages of .NET NuGet packing, as
# RUNTIME_PLATFORMS in tools/release/dotnet_release.py names them.
DOTNET_NUGET_PLATFORMS = {"android", "linux-gnu", "linux-musl", "macos", "windows"}


@dataclasses.dataclass(frozen=True)
class Target:
    preset: str
    platform: str
    architecture: str
    backend: str
    unit: str
    runner: str
    # False when CI runs the target's native tests only on its device.
    tested: bool
    device: dict[str, str] | None
    mise_env: str
    software_gpu: bool


def packaged(platforms: set[str]):
    """Select a binding package's targets, which include no RISC-V or LoongArch."""
    return lambda target: (
        target.platform in platforms and target.architecture not in OPT_IN_ARCHITECTURES
    )


# The jobs other than target jobs: the unit that owns each, and which targets it
# consumes. A job runs whenever its unit does, and so do the targets it
# consumes.
JOBS = {
    "docs": ("docs", lambda target: False),
    "support-matrix": ("support-matrix", lambda target: False),
    "android-multi": ("android", lambda target: target.platform == "android"),
    # Every Metal preset is an Apple one, and each contributes one XCFramework
    # slice.
    "apple-xcframework": ("swiftpm", lambda target: target.backend == "metal"),
    "kotlin-maven": (
        "maven",
        packaged(KOTLIN_MAVEN_PLATFORMS),
    ),
    "dotnet-nuget": (
        "nuget",
        packaged(DOTNET_NUGET_PLATFORMS),
    ),
    "python-wheels": (
        "pypi",
        lambda target: python_wheel(target),
    ),
    "typescript-npm": (
        "npm",
        lambda target: (
            addon(target) == "typescript"
            or any(
                suite.task.endswith(":react-native") for suite in target_suites(target)
            )
        ),
    ),
    "arkts-ohpm": ("ohpm", lambda target: addon(target) == "arkts"),
    "swift-package": ("swiftpm", lambda target: target.backend == "metal"),
    "vcpkg-registry": ("vcpkg", lambda target: target.preset == "linux-gnu-x64-egl"),
    "conan-recipes": ("conan", lambda target: target.preset == "linux-gnu-x64-vulkan"),
    "xmake-package": ("xmake", lambda target: target.preset == "linux-gnu-x64-egl"),
}
# The jobs whose artifacts a job downloads, which run whenever it does. A job
# that a skipped one needs would skip it too, and a skip passes the required
# check.
JOB_NEEDS = {
    "swift-package": ("apple-xcframework",),
    "typescript-npm": ("apple-xcframework",),
}


def lineage(presets: list[dict], name: str) -> list[dict]:
    """Return a preset and its ancestors in the order CMake prefers their values.

    A preset's own value comes first, then each of its `inherits` in order,
    depth-first, so an earlier parent wins over a later one.
    """
    by_name = {preset["name"]: preset for preset in presets}

    def walk(name: str, path: tuple[str, ...]) -> list[dict]:
        if name in path:
            raise SystemExit(
                f"error: preset inheritance cycle: {' -> '.join((*path, name))}"
            )
        if name not in by_name:
            parent = f" inherited by {path[-1]!r}" if path else ""
            raise SystemExit(f"error: unknown preset {name!r}{parent}")
        preset = by_name[name]
        parents = preset.get("inherits", [])
        if isinstance(parents, str):
            parents = [parents]
        return [
            preset,
            *(item for parent in parents for item in walk(parent, (*path, name))),
        ]

    return walk(name, ())


def merged(chain: list[dict], fields) -> dict:
    """Merge a mapping across a lineage, taking each key from its first owner."""
    result: dict = {}
    for preset in reversed(chain):
        result.update(fields(preset))
    # A null cache variable unsets an inherited value.
    return {key: value for key, value in result.items() if value is not None}


def targets(presets: dict) -> list[Target]:
    configure = presets.get("configurePresets", [])
    result = []
    for preset in configure:
        # Hidden presets carry settings for others to inherit and name no target.
        if preset.get("hidden", False):
            continue
        name = preset["name"]
        chain = lineage(configure, name)
        variables = merged(chain, lambda item: item.get("cacheVariables", {}))
        vendor = merged(chain, lambda item: item.get("vendor", {}).get(VENDOR, {}))
        missing = [field for field in ("unit", "runner") if field not in vendor]
        if missing:
            raise SystemExit(
                f"error: preset {name!r} resolves no vendor {', '.join(missing)}"
            )
        device = vendor.get("device")
        if device is not None and sorted(device) != ["command", "name"]:
            raise SystemExit(
                f"error: preset {name!r} names a device without exactly a name "
                "and a command"
            )
        target_platform, _, architecture = variables[
            "MLN_FFI_TARGET_PLATFORM"
        ].rpartition("-")
        backend = variables["MLN_FFI_RENDER_BACKEND"]
        if backend == "opengl":
            backend = variables["MLN_FFI_OPENGL_CONTEXT_PROVIDER"]
        result.append(
            Target(
                preset=name,
                platform=target_platform,
                architecture=architecture,
                backend=backend,
                unit=vendor["unit"],
                runner=vendor["runner"],
                tested=vendor.get("ctest", True),
                device=device,
                mise_env=vendor.get("miseEnv", ""),
                software_gpu=vendor.get("softwareGpu", False),
            )
        )
    return result


def consumed(job: str, all_targets: list[Target]) -> list[Target]:
    """Return the targets that a job other than a target job consumes."""
    _, include = JOBS[job]
    return [target for target in all_targets if include(target)]


ZIG_PROJECTS = ("//bindings/zig:",)
GRADLE_PROJECTS = ("//bindings/kotlin:", "//:kotlin:")
SWIFT_PROJECTS = ("//bindings/swift:",)


def addon(target: Target) -> str:
    """Return which package's addon artifact the target job uploads, if any."""
    tasks = [suite.task for suite in target_suites(target)]
    # The npm packages carry the addon that the TypeScript suite built against
    # the packaged native artifact and tested, for each preset that
    # bindings/typescript/scripts/pack.ts packs.
    if target.platform in {"linux-gnu", "linux-musl", "macos", "emscripten"} and (
        "//bindings/typescript:test" in tasks
    ):
        return "typescript"
    # The OHPM packages carry the addon that each OpenHarmony job built against
    # the packaged native artifact. npm packing downloads every
    # typescript-addon artifact, so these take another prefix.
    if target.platform == "ohos" and any(
        f"//bindings/typescript:{action}" in tasks for action in ("build", "test")
    ):
        return "arkts"
    return ""


def python_wheel(target: Target) -> bool:
    """Whether the target builds a Python runtime wheel for publication."""
    return any(
        suite.task in {"//bindings/python:test", "//bindings/python:build-runtime"}
        for suite in target_suites(target)
    )


def row(target: Target, save_toolchains: bool) -> dict[str, object]:
    """Return how the target job sets up, runs, and uploads for one target."""
    tasks = [suite.task for suite in target_suites(target)]
    # The key hashes every `build.zig.zon` and is shared by all rows on the
    # same runner. Cache keys are immutable, so a row that fetches only some
    # projects would claim the key with an incomplete cache. A row that runs no
    # Zig project skips the fetch; Windows ARM64 has no Zig toolchain to run it.
    zig_projects = [
        any(task.startswith(project) for task in tasks) for project in ZIG_PROJECTS
    ]
    zig = "cache" if all(zig_projects) else "fetch" if any(zig_projects) else "skip"
    return {
        "preset": target.preset,
        "runner": target.runner,
        "save_toolchains": save_toolchains,
        "zig": zig,
        "mise_env": target.mise_env,
        # The backend that the software driver serves.
        "software_gpu": target.backend if target.software_gpu else "",
        # `setup-gradle` restores a Gradle user home and stops the daemons in
        # its post-action, so Windows file locks do not outlive the job. A row
        # with no Gradle build pays that setup and teardown for an empty cache
        # entry.
        "gradle": any(task.startswith(GRADLE_PROJECTS) for task in tasks),
        # A Swift package build resolves its dependencies.
        "swift": any(task.startswith(SWIFT_PROJECTS) for task in tasks),
        # The boot step's name and command.
        "device": target.device["name"] if target.device else "",
        "device_command": target.device["command"] if target.device else "",
        "addon": addon(target),
        "python_wheel": python_wheel(target),
        # devices-cli writes the logs of the Android emulators it boots.
        "emulator_logs": target.platform == "android" and target.device is not None,
    }


def plan(presets: dict, run_all: bool, units: list[str]) -> dict[str, list]:
    """Select the jobs that Rubberstamp's `all` and `units` outputs run."""
    all_targets = targets(presets)
    known = {target.unit for target in all_targets} | {
        unit for unit, _ in JOBS.values()
    }
    unknown = sorted(set(units) - known)
    if unknown:
        raise SystemExit(f"error: unknown coverage units: {', '.join(unknown)}")
    selected = known if run_all else set(units)
    planned = {job for job, (unit, _) in JOBS.items() if unit in selected}
    planned |= {need for job in planned for need in JOB_NEEDS.get(job, ())}
    jobs = [job for job in JOBS if job in planned]
    needed = {target.preset for job in jobs for target in consumed(job, all_targets)}
    matrix = []
    # The toolchain cache key covers the runner OS, architecture and image, so
    # one row per runner label is enough to write every entry the others read.
    claimed: set[str] = set()
    for target in all_targets:
        if target.unit not in selected and target.preset not in needed:
            continue
        matrix.append(row(target, target.runner not in claimed))
        claimed.add(target.runner)
    return {"matrix": matrix, "jobs": jobs}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--all",
        choices=["true", "false"],
        default="false",
        help="Rubberstamp's `all` output: whether to run every unit",
    )
    parser.add_argument(
        "--units",
        default="[]",
        help="Rubberstamp's `units` output: a JSON array of unit names",
    )
    args = parser.parse_args()
    presets = json.loads((ROOT / "CMakePresets.json").read_text())
    print(json.dumps(plan(presets, args.all == "true", json.loads(args.units))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
