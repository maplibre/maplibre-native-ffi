"""Plan the CI matrix from the targets that `CMakePresets.json` defines.

Each visible configure preset is a target. Its `vendor.maplibre-native-ffi`
block, usually inherited from a hidden platform preset, names the Rubberstamp
coverage unit that runs it and the runner label it runs on. This script uses
only the standard library, so CI can run it with the runner's `python3`.
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
VENDOR = "maplibre-native-ffi"

# These platforms supply native inputs to Kotlin Maven staging.
KOTLIN_MAVEN_PLATFORMS = {
    "android",
    "linux-gnu",
    "macos",
    "ios",
    "ios-simulator",
    "tvos",
    "tvos-simulator",
    "windows",
}


@dataclasses.dataclass(frozen=True)
class Target:
    preset: str
    platform: str
    architecture: str
    backend: str
    unit: str
    runner: str
    package: bool


# The jobs other than target jobs: the unit that owns each, and which packaged
# targets it consumes. A job runs whenever its unit does, and so do the targets
# it consumes.
JOBS = {
    "docs": ("docs", lambda target: False),
    "android-multi": ("android", lambda target: target.platform == "android"),
    # Every Metal preset is an Apple one, and each contributes one XCFramework
    # slice.
    "apple-xcframework": ("apple-mobile", lambda target: target.backend == "metal"),
    "kotlin-maven": (
        "maven",
        lambda target: target.platform in KOTLIN_MAVEN_PLATFORMS,
    ),
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
    packaged = set()
    for preset in presets.get("packagePresets", []):
        if preset.get("hidden", False):
            continue
        chain = lineage(presets["packagePresets"], preset["name"])
        packaged.add(
            next(item["configurePreset"] for item in chain if "configurePreset" in item)
        )
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
                package=name in packaged,
            )
        )
    return result


def consumed(job: str, all_targets: list[Target]) -> list[Target]:
    """Return the packaged targets that a job other than a target job consumes."""
    _, include = JOBS[job]
    return [target for target in all_targets if target.package and include(target)]


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
    jobs = [job for job, (unit, _) in JOBS.items() if unit in selected]
    needed = {target.preset for job in jobs for target in consumed(job, all_targets)}
    matrix = []
    # The toolchain cache key covers the runner OS, architecture and image, so
    # one row per runner label is enough to write every entry the others read.
    claimed: set[str] = set()
    for target in all_targets:
        if target.unit not in selected and target.preset not in needed:
            continue
        matrix.append(
            {
                "preset": target.preset,
                "runner": target.runner,
                "package": target.package,
                "save_toolchains": target.runner not in claimed,
            }
        )
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
