"""Plan the CI matrix from the targets that `CMakePresets.json` defines.

Each visible configure preset is a target. Its `vendor.maplibre-native-ffi`
block, usually inherited from a hidden platform preset, names the Rubberstamp
coverage unit that runs it, the runner label it runs on, and the device its
suites run on, if any. A device is the name of the CI step that boots it and
the command that step runs. `miseEnv` names the mise
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

from tools.ci import gaps
from tools.ci.suites import target_suites

ROOT = pathlib.Path(__file__).resolve().parents[2]
VENDOR = "maplibre-native-ffi"


@dataclasses.dataclass(frozen=True)
class Target:
    preset: str
    platform: str
    architecture: str
    backend: str
    unit: str
    runner: str
    device: dict[str, str] | None
    mise_env: str
    software_gpu: bool


def packaged(*bindings: str):
    """Select native inputs for the bindings that have no declared gap."""
    return lambda target: any(
        gaps.supported(binding, target, "package") for binding in bindings
    )


def dotnet_runtime_target(preset: str) -> tuple[str, str] | None:
    """The NuGet backend and RID, shared with the packager that consumes it."""
    target = gaps.target_from_name(preset)
    if gaps.gap("dotnet", target, "package"):
        return None
    rid_platform = {
        "linux-gnu": "linux",
        "linux-musl": "linux-musl",
        "macos": "osx",
        "windows": "win",
        "windows-mingw": "win",
        "android": "android",
        "ios": "ios",
        "ios-maccatalyst": "maccatalyst",
        "tvos": "tvos",
    }[target.platform]
    if target.simulator:
        rid_platform += "simulator"
    package_backend = {"opengl": "OpenGL", "metal": "Metal", "vulkan": "Vulkan"}[
        target.backend
    ]
    return package_backend, f"{rid_platform}-{target.arch}"


# The jobs other than target jobs: the unit that owns each, and which targets it
# consumes. A job runs whenever its unit does, and so do the targets it
# consumes.
JOBS = {
    "docs": ("docs", lambda target: False),
    "support-matrix": ("support-matrix", lambda target: False),
    "android-multi": (
        "android",
        lambda target: (
            target.platform == "android"
            and gaps.supported("kotlin-jvm", target, "package")
        ),
    ),
    # Every Metal preset is an Apple one, and each contributes one XCFramework
    # slice.
    "apple-xcframework": (
        "swiftpm",
        lambda target: (
            target.backend == "metal" and gaps.supported("c", target, "xcframework")
        ),
    ),
    "kotlin-maven": (
        "maven",
        packaged("kotlin-jvm", "kotlin-native"),
    ),
    "dotnet-nuget": (
        "nuget",
        lambda target: dotnet_runtime_target(target.preset) is not None,
    ),
    "python-wheels": (
        "pypi",
        lambda target: python_wheel(target),
    ),
    "typescript-npm": (
        "npm",
        lambda target: (
            target.platform != "ohos"
            and gaps.supported("typescript", target, "package")
        ),
    ),
    "arkts-ohpm": ("ohpm", lambda target: addon(target) == "arkts"),
    "swift-package": (
        "swiftpm",
        lambda target: (
            target.backend == "metal" and gaps.supported("swift", target, "package")
        ),
    ),
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
                device=device,
                mise_env=vendor.get("miseEnv", ""),
                software_gpu=vendor.get("softwareGpu", False),
            )
        )
    return result


def consumed(job: str, all_targets: list[Target]) -> list[Target]:
    """Return the targets that a job other than a target job consumes."""
    _, include = JOBS[job]
    return [
        target
        for target in all_targets
        if gaps.supported("c", target, "package") and include(target)
    ]


ZIG_PROJECTS = ("//bindings/zig:",)
GRADLE_PROJECTS = ("//bindings/kotlin:", "//:kotlin:")
SWIFT_PROJECTS = ("//bindings/swift:",)


def addon(target: Target) -> str:
    """Return which package's addon artifact the target job uploads, if any."""
    if not gaps.supported("typescript", target, "package"):
        return ""
    # Mobile hosts compile the sources shipped in the facade. The other
    # targets carry a prebuilt Node-API addon or browser module.
    if target.platform in {
        "android",
        "ios",
        "ios-simulator",
        "ios-maccatalyst",
        "tvos",
        "tvos-simulator",
        "visionos",
        "visionos-simulator",
    }:
        return ""
    return "arkts" if target.platform == "ohos" else "typescript"


def python_wheel(target: Target) -> bool:
    """Whether the target builds a Python runtime wheel for publication."""
    return gaps.supported("python", target, "package")


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
        if not gaps.supported("c", target, "build"):
            continue
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
    parser.add_argument(
        "--package-inputs",
        choices=list(JOBS),
        help="Print the native preset inputs of one package job, one per line",
    )
    args = parser.parse_args()
    presets = json.loads((ROOT / "CMakePresets.json").read_text())
    if args.package_inputs:
        print(
            "\n".join(
                target.preset
                for target in consumed(args.package_inputs, targets(presets))
            )
        )
    else:
        print(json.dumps(plan(presets, args.all == "true", json.loads(args.units))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
