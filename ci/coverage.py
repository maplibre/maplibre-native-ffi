"""Coverage units and the CI jobs that they run."""

from __future__ import annotations

import fnmatch

from ci.workflow import backend, platform, target_rows

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


def job_name(job: str) -> str:
    return (
        f"target / {job.removeprefix('target-')}"
        if job.startswith("target-")
        else NAMES[job]
    )


def dependencies(source: dict, presets: dict) -> dict[str, list[str]]:
    """Return the target jobs that each packaging job consumes."""
    packaged = [row["preset"] for row in target_rows(source, presets) if row["package"]]

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
    targets = [f"target-{row['preset']}" for row in target_rows(source, presets)]
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
    needs = dependencies(source, presets)
    running: dict[str, list[str]] = {}
    for unit, jobs in unit_jobs(source, presets).items():
        for job in jobs:
            for runs in (job, *needs.get(job, ())):
                units = running.setdefault(runs, [])
                if unit not in units:
                    units.append(unit)
    return running
