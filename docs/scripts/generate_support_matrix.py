import json
import pathlib
import re
from collections import defaultdict
from dataclasses import dataclass
from typing import Any

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

REPO_ROOT = pathlib.Path.cwd()
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "support-matrix.json"
SUPPORT_SOURCE = "ci/workflow.toml"


PLATFORM_LABELS = {
    "android": "Android",
    "emscripten": "Emscripten",
    "ios": "iOS",
    "linux-gnu": "Linux GNU",
    "linux-musl": "Linux musl",
    "macos": "macOS",
    "ohos": "OpenHarmony",
    "tvos": "tvOS",
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
    "linux-gnu-x64",
    "linux-gnu-arm64",
    "linux-musl-x64",
    "linux-musl-arm64",
    "macos-arm64",
    "windows-x64",
    "windows-arm64",
    "emscripten-wasm32",
    "android-arm",
    "android-arm64",
    "android-x64",
    "ios-arm64",
    "ios-simulator-arm64",
    "tvos-arm64",
    "tvos-simulator-arm64",
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
            return f"{self.platform}-simulator-{self.arch}"
        return f"{self.platform}-{self.arch}"


def coverage_from_preset(preset: str, status: str) -> Coverage:
    target_platform = platform(preset)
    simulator = target_platform.endswith("-simulator")
    base_platform = (
        target_platform.removesuffix("-simulator") if simulator else target_platform
    )
    return Coverage(
        platform=base_platform,
        arch=architecture(preset),
        backend={"egl": "opengl", "wgl": "opengl", "webgl": "opengl"}.get(
            backend(preset), backend(preset)
        ),
        status=status,
        simulator=simulator,
    )


def command_support(command: str) -> list[tuple[str, str]]:
    match = re.search(r"mise run //(bindings|examples)/([^: ]+):([^ ]+)", command)
    if match is None:
        return []
    kind, name, action = match.groups()

    if kind == "bindings" and name == "kotlin":
        if action == "android-build":
            return [("bindings-kotlin-android", "build-only")]
        if action == "ios-build":
            return [("bindings-kotlin-native", "build-only")]
        if action == "test":
            preset = command.rsplit(" ", 1)[-1]
            if preset.startswith(("ios-simulator-", "tvos-simulator-")):
                return [("bindings-kotlin-native", "tested")]
            # The test task runs the JVM suite on every host preset and adds
            # the Kotlin/Native suite where the binding declares a target.
            rows = [("bindings-kotlin-jvm", "tested")]
            if preset.startswith(("linux-gnu-x64-", "macos-arm64-")):
                rows.append(("bindings-kotlin-native", "tested"))
            return rows
        return []
    project_id = f"{kind}-{name}"

    status = "tested" if action in {"test", "run"} else "build-only"
    return [(project_id, status)]


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
        return f"{PLATFORM_LABELS[entry.platform]} Simulator {entry.arch}"
    if entry.platform in {"ios", "tvos"}:
        return f"{PLATFORM_LABELS[entry.platform]} device {entry.arch}"
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
            for project_id, status in command_support(command):
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
