"""Coverage units and the CI jobs that run them."""

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


def unit_jobs(config: dict, source: dict, presets: dict) -> dict[str, list[str]]:
    """Map each unit, and hygiene, to the jobs it owns."""
    targets = [f"target-{row['preset']}" for row in target_rows(source, presets)]
    owned = {HYGIENE: [HYGIENE]}
    for unit in config["units"]:
        owned[unit["id"]] = [
            *(
                job
                for job in targets
                if any(
                    fnmatch.fnmatchcase(job.removeprefix("target-"), pattern)
                    for pattern in unit.get("targets", ())
                )
            ),
            *unit.get("jobs", ()),
        ]
    jobs = [job for owned_jobs in owned.values() for job in owned_jobs]
    expected = [HYGIENE, *NAMES.keys() - {HYGIENE}, *targets]
    if sorted(jobs) != sorted(expected):
        raise ValueError(
            "Every CI job must belong to exactly one coverage unit: "
            f"unowned={sorted(set(expected) - set(jobs))}, "
            f"repeated={sorted({job for job in jobs if jobs.count(job) > 1})}, "
            f"unknown={sorted(set(jobs) - set(expected))}"
        )
    return owned


def selected_jobs(
    owned: dict[str, list[str]], needs: dict[str, list[str]], units
) -> list[str]:
    """Return the jobs of the selected units and every job they consume."""
    jobs = {job for unit in units for job in owned[unit]}
    for job in list(jobs):
        jobs.update(needs.get(job, ()))
    return sorted(jobs)


def unit_passed(jobs: list[str], conclusions: dict[str, str]) -> bool:
    """Whether every job of a unit succeeded in one workflow run."""
    for job in jobs:
        name = job_name(job)
        # A reusable workflow reports its jobs as `<caller> / <job>`.
        matches = [
            conclusion
            for reported, conclusion in conclusions.items()
            if reported == name or reported.startswith(f"{name} / ")
        ]
        if not matches or any(conclusion != "success" for conclusion in matches):
            return False
    return True
