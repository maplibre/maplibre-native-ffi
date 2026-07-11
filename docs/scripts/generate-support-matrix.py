#!/usr/bin/env python3
import json
import pathlib
from collections import defaultdict
from dataclasses import dataclass
from typing import Any


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "support-matrix.json"
SUPPORT_SOURCE = "docs/scripts/generate-support-matrix.py"

PLATFORM_LABELS = {
    "android": "Android",
    "ios": "iOS",
    "linux": "Linux",
    "macos": "macOS",
    "ohos": "OpenHarmony",
    "windows": "Windows",
}
BACKEND_LABELS = {"metal": "Metal", "opengl": "OpenGL", "vulkan": "Vulkan"}
BACKEND_ORDER = ["vulkan", "opengl", "metal"]
ENVIRONMENT_ORDER = [
    "linux-x64",
    "linux-arm64",
    "macos-arm64",
    "windows-x64",
    "windows-arm64",
    "android-arm64",
    "android-x64",
    "ios-arm64",
    "ios-simulator-arm64",
    "ohos-arm64",
]
STATUS_LABELS = {"tested": "Tested in CI", "build-only": "Built only"}


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


def coverage(
    platform: str, arch: str, backend: str, status: str = "tested"
) -> Coverage:
    return Coverage(platform, arch, backend, status)


def entry_with_status(entry: Coverage, status: str) -> Coverage:
    return Coverage(entry.platform, entry.arch, entry.backend, status, entry.simulator)


LINUX_X64 = coverage("linux", "x64", "vulkan")
MACOS_ARM64 = coverage("macos", "arm64", "metal")
WINDOWS_X64 = coverage("windows", "x64", "vulkan")
IOS_DEVICE = coverage("ios", "arm64", "metal", "build-only")
IOS_SIMULATOR = Coverage("ios", "arm64", "metal", "tested", simulator=True)
HOST_DESKTOP = [LINUX_X64, MACOS_ARM64, WINDOWS_X64]

NATIVE_COVERAGE = [
    coverage("linux", "x64", "vulkan"),
    coverage("linux", "x64", "opengl"),
    coverage("linux", "arm64", "vulkan"),
    coverage("linux", "arm64", "opengl"),
    coverage("macos", "arm64", "metal"),
    coverage("macos", "arm64", "vulkan"),
    coverage("macos", "arm64", "opengl"),
    coverage("windows", "x64", "vulkan"),
    coverage("windows", "x64", "opengl"),
    coverage("windows", "arm64", "vulkan"),
    coverage("windows", "arm64", "opengl"),
    coverage("android", "arm64", "vulkan", "build-only"),
    coverage("android", "arm64", "opengl", "build-only"),
    coverage("android", "x64", "vulkan", "build-only"),
    coverage("android", "x64", "opengl", "build-only"),
    coverage("ios", "arm64", "metal", "build-only"),
    IOS_SIMULATOR,
    coverage("ohos", "arm64", "vulkan", "build-only"),
    coverage("ohos", "arm64", "opengl", "build-only"),
]

ANDROID_BINDING = [
    coverage("android", arch, backend, "build-only")
    for arch in ("arm64", "x64")
    for backend in ("vulkan", "opengl")
]
ANDROID_MAP = [
    coverage("android", arch, "opengl", "build-only") for arch in ("arm64", "x64")
]

BINDINGS = [
    ("bindings-dotnet", "C#", "bindings/dotnet", HOST_DESKTOP),
    (
        "bindings-kotlin-android",
        "Kotlin/Android",
        "bindings/kotlin",
        ANDROID_BINDING,
    ),
    (
        "bindings-kotlin-jvm",
        "Kotlin/JVM",
        "bindings/kotlin",
        HOST_DESKTOP,
    ),
    (
        "bindings-kotlin-native",
        "Kotlin/Native",
        "bindings/kotlin",
        [LINUX_X64, MACOS_ARM64],
    ),
    ("bindings-go", "Go", "bindings/go", [LINUX_X64, MACOS_ARM64]),
    ("bindings-python", "Python", "bindings/python", [LINUX_X64, MACOS_ARM64]),
    (
        "bindings-rust",
        "Rust",
        "bindings/rust",
        HOST_DESKTOP
        + [
            coverage("ohos", "arm64", "vulkan", "build-only"),
            coverage("ohos", "arm64", "opengl", "build-only"),
        ],
    ),
    (
        "bindings-swift",
        "Swift",
        "bindings/swift",
        [LINUX_X64, MACOS_ARM64, IOS_DEVICE, IOS_SIMULATOR],
    ),
    ("bindings-zig", "Zig", "bindings/zig", HOST_DESKTOP + [IOS_SIMULATOR]),
]

EXAMPLES = [
    ("examples-android-map", "android-map", "examples/android-map", ANDROID_MAP),
    (
        "examples-compose-map",
        "compose-map",
        "examples/compose-map",
        [entry_with_status(item, "build-only") for item in HOST_DESKTOP],
    ),
    (
        "examples-dotnet-map",
        "dotnet-map",
        "examples/dotnet-map",
        [entry_with_status(item, "build-only") for item in HOST_DESKTOP],
    ),
    (
        "examples-lwjgl-map",
        "lwjgl-map",
        "examples/lwjgl-map",
        [entry_with_status(item, "build-only") for item in HOST_DESKTOP],
    ),
    (
        "examples-rust-map",
        "rust-map",
        "examples/rust-map",
        [entry_with_status(item, "build-only") for item in HOST_DESKTOP],
    ),
    (
        "examples-swift-map",
        "swift-map",
        "examples/swift-map",
        [entry_with_status(MACOS_ARM64, "build-only"), IOS_DEVICE, IOS_SIMULATOR],
    ),
    (
        "examples-zig-map",
        "zig-map",
        "examples/zig-map",
        [entry_with_status(item, "build-only") for item in HOST_DESKTOP],
    ),
    ("examples-zig-readback", "zig-readback", "examples/zig-readback", HOST_DESKTOP),
]


def environment_sort_key(environment: str) -> tuple[int, str]:
    try:
        return (ENVIRONMENT_ORDER.index(environment), environment)
    except ValueError:
        return (len(ENVIRONMENT_ORDER), environment)


def backend_sort_key(backend: str) -> tuple[int, str]:
    try:
        return (BACKEND_ORDER.index(backend), backend)
    except ValueError:
        return (len(BACKEND_ORDER), backend)


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
            "backend": backend,
            "backendLabel": BACKEND_LABELS[backend],
            "status": "build-only" if "build-only" in statuses[backend] else "tested",
        }
        for backend in sorted(statuses, key=backend_sort_key)
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
    projects: list[tuple[str, str, str, list[Coverage]]], kind: str
) -> list[dict[str, Any]]:
    return [
        {
            "id": project_id,
            "kind": kind,
            "label": label,
            "source": SUPPORT_SOURCE,
            "sourceDirectory": source_directory,
            "environments": environment_rows(entries),
        }
        for project_id, label, source_directory, entries in sorted(
            projects, key=lambda project: (project[1].casefold(), project[0])
        )
    ]


def support_matrix() -> dict[str, Any]:
    native = environment_rows(NATIVE_COVERAGE)
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
        "bindings": project_rows(BINDINGS, "binding"),
        "examples": project_rows(EXAMPLES, "example"),
    }


def main() -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(
        json.dumps(support_matrix(), indent=2, sort_keys=False) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
