"""Coverage units and the CI jobs that they run."""

from __future__ import annotations

import fnmatch

from ci.workflow import backend, device_boot, platform, preset_sets, runner
from tools.ci.suites import target_suites

HYGIENE = "hygiene"
# These platforms supply native inputs to Kotlin Maven staging. The full run
# also retains its verified Maven repository for snapshot publication.
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
NAMES = {
    HYGIENE: "hygiene",
    "docs": "docs",
    "android-multi": "Android multi-ABI packaging",
    "apple-xcframework": "Apple XCFramework",
    "kotlin-maven": "Kotlin Maven",
}


ZIG_PROJECTS = (
    "//bindings/zig:",
    "//examples/zig-map:",
    "//examples/zig-readback:",
)
GRADLE_PROJECTS = ("//bindings/kotlin:", "//:kotlin:")
SWIFT_PROJECTS = ("//bindings/swift:", "//examples/swift-map:")


def target_rows(presets: dict) -> list[dict[str, object]]:
    configured, _, _, packaged = preset_sets(presets)
    rows = []
    # The toolchain cache key covers the runner OS, architecture and image, so
    # one row per runner label is enough to write every entry the others read.
    claimed_runners: set[str] = set()
    for preset in configured:
        suites = target_suites(presets, preset)
        tasks = [suite.task for suite in suites]
        row_runner = runner(preset)
        rows.append(
            {
                "preset": preset,
                "runner": row_runner,
                "package": preset in packaged,
                # The key hashes every `build.zig.zon` and is shared by all
                # rows on the same runner. Cache keys are immutable, so a row
                # that fetches only some projects would claim the key with an
                # incomplete cache.
                "zig": all(
                    any(task.startswith(project) for task in tasks)
                    for project in ZIG_PROJECTS
                ),
                # `setup-gradle` restores a Gradle user home and stops the
                # daemons in its post-action, so Windows file locks do not
                # outlive the job. A row with no Gradle build pays that setup
                # and teardown for an empty cache entry.
                "gradle": any(task.startswith(GRADLE_PROJECTS) for task in tasks),
                # A Swift package build resolves its dependencies.
                "swift": any(task.startswith(SWIFT_PROJECTS) for task in tasks),
                "boot": device_boot(preset),
                "save_toolchains": row_runner not in claimed_runners,
                "tasks": tasks,
            }
        )
        claimed_runners.add(row_runner)
    return rows


def job_name(job: str) -> str:
    return (
        f"target / {job.removeprefix('target-')}"
        if job.startswith("target-")
        else NAMES[job]
    )


def dependencies(presets: dict) -> dict[str, list[str]]:
    """Return the target jobs that each packaging job consumes."""
    packaged = [row["preset"] for row in target_rows(presets) if row["package"]]

    def targets(include) -> list[str]:
        return [f"target-{preset}" for preset in packaged if include(preset)]

    return {
        "android-multi": targets(lambda preset: platform(preset) == "android"),
        # Every Metal preset is an Apple one, and each contributes one
        # XCFramework slice.
        "apple-xcframework": targets(lambda preset: backend(preset) == "metal"),
        "kotlin-maven": targets(
            lambda preset: platform(preset) in KOTLIN_MAVEN_PLATFORMS
        ),
    }


def unit_jobs(source: dict, presets: dict) -> dict[str, list[str]]:
    """Map each coverage unit to the jobs it owns."""
    targets = [f"target-{row['preset']}" for row in target_rows(presets)]
    owned = {
        unit: [
            *(
                job
                for job in targets
                if any(
                    fnmatch.fnmatchcase(job.removeprefix("target-"), pattern)
                    for pattern in jobs.get("targets", ())
                )
            ),
            *jobs.get("jobs", ()),
        ]
        for unit, jobs in source["units"].items()
    }
    found = [job for owned_jobs in owned.values() for job in owned_jobs]
    expected = [*NAMES.keys() - {HYGIENE}, *targets]
    if sorted(found) != sorted(expected):
        raise ValueError(
            "Every CI job except hygiene must belong to exactly one coverage unit: "
            f"unowned={sorted(set(expected) - set(found))}, "
            f"repeated={sorted({job for job in found if found.count(job) > 1})}, "
            f"unknown={sorted(set(found) - set(expected))}"
        )
    return owned


def job_units(source: dict, presets: dict) -> dict[str, list[str]]:
    """Map each gated job to the units that run it.

    A unit runs the jobs it owns and every job that those consume.
    """
    needs = dependencies(presets)
    running: dict[str, list[str]] = {}
    for unit, jobs in unit_jobs(source, presets).items():
        for job in jobs:
            for runs in (job, *needs.get(job, ())):
                units = running.setdefault(runs, [])
                if unit not in units:
                    units.append(unit)
    return running
