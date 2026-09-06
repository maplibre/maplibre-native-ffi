"""Select CI coverage from PR readiness and persistent platform labels."""

from __future__ import annotations

import json
import os
import pathlib

from ci.workflow import load_configuration, platform, preset_sets

ROOT = pathlib.Path(__file__).resolve().parents[1]
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


def plan(event_name: str, event: dict) -> dict:
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

    _, presets = load_configuration(ROOT)
    targets, _, _, _ = preset_sets(presets)
    if not READY_TARGETS <= set(targets):
        raise ValueError("CI coverage references an unknown native target")
    selected = (
        set(targets)
        if tier == "full"
        else set(DRAFT_TARGETS if tier == "draft" else READY_TARGETS)
    )
    for label in labels & PLATFORM_GROUPS.keys():
        selected.update(t for t in targets if platform(t) in PLATFORM_GROUPS[label])

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
    return {"tier": tier, "expected": expected}


def main() -> None:
    selection = plan(
        os.environ["GITHUB_EVENT_NAME"],
        json.loads(pathlib.Path(os.environ["GITHUB_EVENT_PATH"]).read_text()),
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
