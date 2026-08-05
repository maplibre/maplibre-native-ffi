#!/usr/bin/env python3
import json
import pathlib
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from typing import Any

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "support-matrix.json"
SUPPORT_SOURCE = "ci/workflow.toml"
sys.path.insert(0, str(REPO_ROOT))

from ci.workflow import (
    architecture,
    backend,
    consumer_commands,
    load_configuration,
    native_commands,
    platform,
    preset_sets,
    runtime_tested,
)

PLATFORM_LABELS = {
    "android": "Android",
    "emscripten": "Emscripten",
    "ios": "iOS",
    "linux": "Linux",
    "macos": "macOS",
    "ohos": "OpenHarmony",
    "windows": "Windows",
}
BACKEND_LABELS = {
    "metal": "Metal",
    "opengl": "OpenGL",
    "vulkan": "Vulkan",
    "webgpu": "WebGPU",
}
BACKEND_ORDER = ["vulkan", "opengl", "metal", "webgpu"]
ENVIRONMENT_ORDER = [
    "linux-x64",
    "linux-arm64",
    "macos-arm64",
    "windows-x64",
    "windows-arm64",
    "emscripten-wasm32",
    "android-arm64",
    "android-x64",
    "ios-arm64",
    "ios-simulator-arm64",
    "ohos-arm64",
    "ohos-x64",
]
STATUS_LABELS = {"tested": "Tested in CI", "build-only": "Built only"}
PROJECT_LABELS = {
    "bindings-dotnet": "C#",
    "bindings-kotlin-android": "Kotlin/Android",
    "bindings-kotlin-jvm": "Kotlin/JVM",
    "bindings-kotlin-native": "Kotlin/Native",
}


@dataclass(frozen=True)
class Coverage:
    platform: str
    arch: str
    backend: str
    status: str
    simulator: bool = False

    @property
    def environment(self) -> str:
        if self.simulator:
            return f"ios-simulator-{self.arch}"
        return f"{self.platform}-{self.arch}"


def coverage_from_preset(preset: str, status: str) -> Coverage:
    target_platform = platform(preset)
    return Coverage(
        platform="ios" if target_platform == "ios-simulator" else target_platform,
        arch=architecture(preset),
        backend={"egl": "opengl", "wgl": "opengl", "webgl": "opengl"}.get(
            backend(preset), backend(preset)
        ),
        status=status,
        simulator=target_platform == "ios-simulator",
    )


def command_support(command: str) -> tuple[str, str] | None:
    if command.startswith("bash scripts/run-ios-simulator-test.sh "):
        return "bindings-swift", "tested"

    match = re.search(r"mise run //(bindings|examples)/([^: ]+):([^ ]+)", command)
    if match is None:
        return None
    kind, name, action = match.groups()

    if kind == "bindings" and name == "swift" and action == "build:ios-simulator":
        return None
    if kind == "bindings" and name == "kotlin":
        project_id = {
            "androidBuild": "bindings-kotlin-android",
            "jvmTest": "bindings-kotlin-jvm",
            "nativeTest": "bindings-kotlin-native",
        }.get(action)
        if project_id is None:
            return None
    else:
        project_id = f"{kind}-{name}"

    status = (
        "tested"
        if action
        in {
            "test",
            "jvmTest",
            "nativeTest",
            "run",
            "test:android-emulator",
            "test:ios-simulator",
            "test:ohos-emulator",
        }
        else "build-only"
    )
    return project_id, status


def project_label(project_id: str) -> str:
    if project_id in PROJECT_LABELS:
        return PROJECT_LABELS[project_id]
    kind, name = project_id.split("-", 1)
    return name if kind == "examples" else name.capitalize()


def source_directory(project_id: str) -> str:
    if project_id.startswith("bindings-kotlin-"):
        return "bindings/kotlin"
    kind, name = project_id.split("-", 1)
    return f"{kind}/{name}"


def environment_sort_key(environment: str) -> tuple[int, str]:
    try:
        return (ENVIRONMENT_ORDER.index(environment), environment)
    except ValueError:
        return (len(ENVIRONMENT_ORDER), environment)


def backend_sort_key(value: str) -> tuple[int, str]:
    try:
        return (BACKEND_ORDER.index(value), value)
    except ValueError:
        return (len(BACKEND_ORDER), value)


def environment_label(entry: Coverage) -> str:
    if entry.simulator:
        return f"iOS Simulator {entry.arch}"
    if entry.platform == "ios":
        return f"iOS device {entry.arch}"
    return f"{PLATFORM_LABELS[entry.platform]} {entry.arch}"


def backend_rows(entries: list[Coverage]) -> list[dict[str, str]]:
    statuses: dict[str, set[str]] = defaultdict(set)
    for entry in entries:
        statuses[entry.backend].add(entry.status)
    return [
        {
            "backend": value,
            "backendLabel": BACKEND_LABELS[value],
            "status": "tested" if "tested" in statuses[value] else "build-only",
        }
        for value in sorted(statuses, key=backend_sort_key)
    ]


def environment_rows(entries: list[Coverage]) -> list[dict[str, Any]]:
    grouped: dict[str, list[Coverage]] = defaultdict(list)
    for entry in entries:
        grouped[entry.environment].append(entry)
    return [
        {
            "environment": environment,
            "environmentLabel": environment_label(grouped[environment][0]),
            "platform": grouped[environment][0].platform,
            "platformLabel": PLATFORM_LABELS[grouped[environment][0].platform],
            "arch": grouped[environment][0].arch,
            "archLabel": grouped[environment][0].arch,
            "backends": backend_rows(grouped[environment]),
        }
        for environment in sorted(grouped, key=environment_sort_key)
    ]


def project_rows(
    projects: dict[str, list[Coverage]], kind: str
) -> list[dict[str, Any]]:
    matching = {
        project_id: entries
        for project_id, entries in projects.items()
        if project_id.startswith(f"{kind}-")
    }
    return [
        {
            "id": project_id,
            "kind": "binding" if kind == "bindings" else "example",
            "label": project_label(project_id),
            "source": SUPPORT_SOURCE,
            "sourceDirectory": source_directory(project_id),
            "environments": environment_rows(entries),
        }
        for project_id, entries in sorted(
            matching.items(),
            key=lambda item: (project_label(item[0]).casefold(), item[0]),
        )
    ]


def support_matrix() -> dict[str, Any]:
    source, presets = load_configuration(REPO_ROOT)
    configured, _, tested, _ = preset_sets(presets)
    native = environment_rows(
        [
            coverage_from_preset(
                preset, "tested" if runtime_tested(preset, tested) else "build-only"
            )
            for preset in configured
        ]
    )

    projects: dict[str, list[Coverage]] = defaultdict(list)
    for preset in configured:
        commands = native_commands(preset, tested) + consumer_commands(source, preset)
        for command in commands:
            support = command_support(command)
            if support is None:
                continue
            project_id, status = support
            projects[project_id].append(coverage_from_preset(preset, status))

    return {
        "statuses": [
            {"status": status, "statusLabel": STATUS_LABELS[status]}
            for status in ("tested", "build-only")
        ],
        "environments": [
            {
                "environment": row["environment"],
                "environmentLabel": row["environmentLabel"],
            }
            for row in native
        ],
        "native": native,
        "bindings": project_rows(projects, "bindings"),
        "examples": project_rows(projects, "examples"),
    }


def main() -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(
        json.dumps(support_matrix(), indent=2, sort_keys=False) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
