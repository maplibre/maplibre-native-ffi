"""Select affected CI jobs within PR readiness and platform-label coverage."""

from __future__ import annotations

import json
import os
import pathlib
import subprocess

from ci.affected import affected_roots
from ci.workflow import (
    consumer_roots,
    load_configuration,
    platform,
    preset_sets,
    runner,
)

ROOT = pathlib.Path.cwd()
DRAFT_TARGETS = {"linux-gnu-x64-egl", "linux-gnu-x64-vulkan"}
READY_TARGETS = DRAFT_TARGETS | {
    "macos-arm64-metal",
    "windows-x64-wgl",
    "windows-x64-vulkan",
    "android-x64-egl",
    "android-x64-vulkan",
    "emscripten-wasm32-webgl",
    "emscripten-wasm32-webgpu",
}
PLATFORM_GROUPS = {
    "ci:apple": {"macos", "ios", "ios-simulator", "tvos", "tvos-simulator"},
    "ci:android": {"android"},
    "ci:linux": {"linux-gnu", "linux-musl"},
    "ci:windows": {"windows"},
    "ci:ohos": {"ohos"},
}


def plan(event_name: str, event: dict, roots: set[str] | None = None) -> dict:
    if event_name not in {"pull_request", "push", "workflow_dispatch"}:
        raise ValueError(f"Unsupported CI event: {event_name}")
    tier = "full"
    labels = set()
    if event_name == "pull_request":
        # Use authorship so maintainer label events preserve Dependabot coverage.
        pr = event["pull_request"]
        labels = {label["name"] for label in pr["labels"]}
        if pr["user"]["login"] != "dependabot[bot]" and "ci:full" not in labels:
            tier = "draft" if pr["draft"] else "ready"

    source, presets = load_configuration(ROOT)
    targets, _, _, _ = preset_sets(presets)
    if not READY_TARGETS <= set(targets):
        raise ValueError("CI coverage references an unknown native target")
    selected = (
        set(targets)
        if tier == "full"
        else set(DRAFT_TARGETS if tier == "draft" else READY_TARGETS)
    )
    requested = {
        t
        for label in labels & PLATFORM_GROUPS.keys()
        for t in targets
        if platform(t) in PLATFORM_GROUPS[label]
    }
    selected.update(requested)
    if tier != "full" and roots is not None and "." not in roots:
        selected = {
            t for t in selected if t in requested or consumer_roots(source, t) & roots
        }

    expected = dict.fromkeys(["plan", "hygiene", "docs"], "success")
    expected.update(
        {
            f"target-{target}": "success" if target in selected else "skipped"
            for target in targets
        }
    )
    expected["android-multi"] = (
        "success" if tier == "full" or "ci:android" in labels else "skipped"
    )
    expected["kotlin-maven"] = "success" if tier == "full" else "skipped"
    # Shared toolchain keys need one writer among the jobs that actually run.
    claimed_runners = set()
    writers = []
    for target in targets:
        if target in selected and runner(target) not in claimed_runners:
            writers.append(f"target-{target}")
            claimed_runners.add(runner(target))
    return {"tier": tier, "expected": expected, "toolchain_writers": writers}


def select(event_name: str, event: dict, head: str) -> tuple[dict, str]:
    baseline = plan(event_name, event)
    if baseline["tier"] == "full":
        return baseline, "Full coverage requested."
    try:
        base = event["pull_request"]["base"]["sha"]
        roots = affected_roots(ROOT, base, head)
        selection = plan(event_name, event, roots)
    except (
        KeyError,
        TypeError,
        ValueError,
        OSError,
        subprocess.SubprocessError,
    ) as error:
        return (
            baseline,
            f"Affected selection unavailable; retaining the complete tier: {error}",
        )
    detail = ", ".join(f"`{root}`" for root in sorted(roots)) or "none"
    return (
        selection,
        f"Affected project roots: {detail}.\n\nCompared `{base}` to `{head}`.",
    )


def main() -> None:
    selection, explanation = select(
        os.environ["GITHUB_EVENT_NAME"],
        json.loads(pathlib.Path(os.environ["GITHUB_EVENT_PATH"]).read_text()),
        os.environ.get("GITHUB_SHA", ""),
    )
    with pathlib.Path(os.environ["GITHUB_OUTPUT"]).open("a") as output:
        for key, value in selection.items():
            encoded = (
                value
                if isinstance(value, str)
                else json.dumps(value, separators=(",", ":"))
            )
            print(f"{key}={encoded}", file=output)
    with pathlib.Path(os.environ["GITHUB_STEP_SUMMARY"]).open("a") as summary:
        print(f"CI tier: **{selection['tier']}**\n", file=summary)
        print(f"{explanation}\n", file=summary)
        print("Selected jobs (all required):\n", file=summary)
        for job, result in selection["expected"].items():
            if result == "success":
                print(f"- `{job}`", file=summary)
        print(
            "\nPlatform labels expand coverage; `ci:full` includes all targets and packaging.",
            file=summary,
        )


if __name__ == "__main__":
    main()
