"""Baseline, ready, and extended CI coverage."""

from __future__ import annotations

from ci.workflow import consumer_roots, platform, preset_sets, runner

PLATFORMS = {
    "apple": {
        "macos",
        "ios",
        "ios-simulator",
        "ios-maccatalyst",
        "tvos",
        "tvos-simulator",
    },
    "android": {"android"},
    "linux": {"linux-gnu", "linux-musl"},
    "windows": {"windows"},
    "ohos": {"ohos"},
}
GROUPS = ("baseline", "ready", "extended")
STATE_EVENTS = {"ready_for_review", "converted_to_draft", "labeled", "unlabeled"}


def workflow_file(group: str) -> str:
    return "ci.yml" if group == "extended" else f"ci-{group}.yml"


def workflow_name(group: str) -> str:
    return "CI" if group == "extended" else f"CI {group}"


def check_name(group: str) -> str:
    return "ci-required" if group == "extended" else f"ci-required ({group})"


def group_targets(source: dict, presets: dict, group: str) -> set[str]:
    targets = set(preset_sets(presets)[0])
    baseline = set(source["coverage"]["baseline"])
    ready = set(source["coverage"]["ready"])
    if (
        not baseline
        or not ready
        or baseline & ready
        or not (baseline | ready) <= targets
    ):
        raise ValueError(
            "Baseline and ready coverage must be nonempty, disjoint native targets"
        )
    return {"baseline": baseline, "ready": ready, "extended": targets}[group]


def scope(group: str, event_name: str, event: dict) -> str:
    if group not in GROUPS:
        raise ValueError(f"Unknown CI group: {group}")
    if event_name in {"push", "workflow_dispatch"}:
        if group != "extended":
            raise ValueError("Only extended CI handles main and manual runs")
        return "full"
    if event_name != "pull_request":
        raise ValueError(f"Unsupported CI event: {event_name}")
    pr = event["pull_request"]
    labels = {label["name"] for label in pr["labels"]}
    if group == "baseline":
        return "baseline"
    if group == "ready":
        return "" if pr["draft"] else "ready"
    if pr["user"]["login"] == "dependabot[bot]" or "ci:full" in labels:
        return "full"
    return "+".join(sorted(name for name in PLATFORMS if f"ci:{name}" in labels))


def selected_jobs(
    source: dict,
    presets: dict,
    group: str,
    selection: str,
    roots: set[str] | None = None,
) -> list[str]:
    targets = group_targets(source, presets, group)
    if not selection:
        return []
    jobs = []
    requested: set[str] = set()
    if group == "extended":
        if selection == "full":
            jobs.extend(["hygiene", "docs", "android-multi", "kotlin-maven"])
        else:
            platforms = set().union(*(PLATFORMS[name] for name in selection.split("+")))
            targets = {target for target in targets if platform(target) in platforms}
            requested = set(targets)
            if "android" in platforms:
                jobs.append("android-multi")
    elif group == "baseline":
        jobs.extend(["hygiene", "docs"])
    if roots is not None and "." not in roots and selection != "full":
        targets = {
            target
            for target in targets
            if target in requested or consumer_roots(source, target) & roots
        }
    return sorted([*jobs, *(f"target-{target}" for target in targets)])


def toolchain_writers(jobs: list[str], presets: dict) -> list[str]:
    selected = set(jobs)
    claimed: set[str] = set()
    writers = []
    for target in preset_sets(presets)[0]:
        job = f"target-{target}"
        if job in selected and runner(target) not in claimed:
            writers.append(job)
            claimed.add(runner(target))
    return writers
