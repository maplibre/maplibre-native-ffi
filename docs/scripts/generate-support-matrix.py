#!/usr/bin/env python3
import json
import pathlib
import re
import sys
import tomllib
from collections import defaultdict
from dataclasses import dataclass
from typing import Any

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "support-matrix.json"
GAPS_PATH = REPO_ROOT / "docs" / "support-gaps.toml"
sys.path.insert(0, str(REPO_ROOT))

from tools.ci import plan
from tools.ci.suites import runtime_tested, target_suites

PLATFORM_LABELS = {
    "linux-gnu": "Linux (GNU)",
    "linux-musl": "Linux (musl)",
    "macos": "macOS",
    "windows": "Windows (MSVC)",
    "windows-mingw": "Windows (MinGW)",
    "freebsd": "FreeBSD",
    "android": "Android",
    "ios": "iOS",
    "ios-maccatalyst": "Mac Catalyst",
    "tvos": "tvOS",
    "visionos": "visionOS",
    "ohos": "OpenHarmony",
    "emscripten": "Web (Emscripten)",
}
PLATFORM_ORDER = list(PLATFORM_LABELS)
# Apple mobile platforms list device and simulator rows separately.
DEVICE_PLATFORMS = {"ios", "tvos", "visionos"}
ARCH_ORDER = ["x64", "arm64", "arm", "riscv64", "loongarch64", "wasm32"]
BACKEND_LABELS = {
    "vulkan": "Vulkan",
    "opengl": "OpenGL",
    "metal": "Metal",
    "webgpu": "WebGPU",
}
BACKEND_ORDER = list(BACKEND_LABELS)
STATUS_LABELS = {
    "tested": "Tested in CI",
    "build-only": "Built only",
    "path-known": "Not started, path known",
    "blocked": "Blocked upstream",
    "unknown": "Not investigated",
    "not-applicable": "Does not apply",
}
GAP_STATUSES = {"path-known", "blocked"}
# A language whose bindings reach one platform through more than one mechanism
# has a column for each, named `<language>-<column>`.
BINDING_LABELS = {
    "c": "C",
    "dotnet": "C#",
    "dart": "Dart",
    "go": "Go",
    "kotlin-jvm": "Kotlin (JVM, Android)",
    # Kotlin/Wasm reaches only the browser, which Kotlin/Native does not.
    "kotlin-native": "Kotlin (Native, Wasm)",
    "python": "Python",
    "rust": "Rust",
    "swift": "Swift",
    "typescript": "TypeScript",
    "zig": "Zig",
}


@dataclass(frozen=True)
class Target:
    platform: str
    arch: str
    backend: str
    simulator: bool = False

    def sort_key(self) -> tuple[int, int, bool, int]:
        return (
            PLATFORM_ORDER.index(self.platform),
            ARCH_ORDER.index(self.arch),
            self.simulator,
            BACKEND_ORDER.index(self.backend),
        )

    def arch_label(self) -> str:
        if self.platform not in DEVICE_PLATFORMS:
            return self.arch
        kind = "simulator" if self.simulator else "device"
        return f"{kind} {self.arch}"


def target_from_preset(preset: plan.Target) -> Target:
    simulator = preset.platform.endswith("-simulator")
    return Target(
        platform=preset.platform.removesuffix("-simulator"),
        arch=preset.architecture,
        backend={"egl": "opengl", "wgl": "opengl", "webgl": "opengl"}.get(
            preset.backend, preset.backend
        ),
        simulator=simulator,
    )


def command_support(command: str) -> list[tuple[str, str]]:
    """Returns the bindings and statuses that a CI command establishes."""
    match = re.search(r"mise run //bindings/([^: ]+):([^ ]+)", command)
    if match is None:
        return []
    name, action = match.groups()

    if name == "kotlin":
        if action == "test:native":
            return [("kotlin-native", "tested")]
        if action == "android-build":
            return [("kotlin-jvm", "build-only")]
        # Both tasks compile the preset's Kotlin/Native target.
        if action in ("build", "ios-build"):
            return [("kotlin-native", "build-only")]
        if action == "test":
            preset = command.rsplit(" ", 1)[-1]
            if preset.startswith("android-"):
                return [("kotlin-jvm", "tested")]
            if preset.startswith(("ios-simulator-", "tvos-simulator-")):
                return [("kotlin-native", "tested")]
            if preset.startswith("emscripten-"):
                return [("kotlin-native", "tested")]
            # The test task runs the JVM suite on every host preset and adds
            # the Kotlin/Native suite where the binding declares a target.
            rows = [("kotlin-jvm", "tested")]
            if preset.startswith(("linux-gnu-x64-", "macos-arm64-")):
                rows.append(("kotlin-native", "tested"))
            return rows
        return []

    status = (
        "tested"
        if (action == "run" or action == "test" or action.startswith("test:"))
        else "build-only"
    )
    return [(name, status)]


def language(binding: str) -> str:
    return binding.split("-", 1)[0]


def check_cell(key: tuple[str, str], table: str) -> None:
    if key[0] not in BINDING_LABELS:
        raise SystemExit(f"error: unknown binding in {GAPS_PATH} {table}: {key}")
    if key[1] not in PLATFORM_LABELS:
        raise SystemExit(f"error: unknown platform in {GAPS_PATH} {table}: {key}")


def load_gaps() -> tuple[
    int,
    list[Target],
    dict[tuple[str, str], dict[str, str]],
    dict[tuple[str, str], str],
]:
    document = tomllib.loads(GAPS_PATH.read_text(encoding="utf-8"))
    planned = [
        Target(
            platform=entry["platform"],
            arch=entry["arch"],
            backend=value,
            simulator=entry.get("simulator", False),
        )
        for entry in document.get("planned", [])
        for value in entry["backends"]
    ]
    gaps: dict[tuple[str, str], dict[str, str]] = {}
    for entry in document.get("gap", []):
        key = (entry["binding"], entry["platform"])
        check_cell(key, "gap")
        if entry["status"] not in GAP_STATUSES:
            raise SystemExit(f"error: unknown status in {GAPS_PATH}: {key}")
        if key in gaps:
            raise SystemExit(f"error: duplicate gap in {GAPS_PATH}: {key}")
        gaps[key] = entry
    not_applicable: dict[tuple[str, str], str] = {}
    for entry in document.get("not-applicable", []):
        for platform in entry["platforms"]:
            key = (entry["binding"], platform)
            check_cell(key, "not-applicable")
            if key in gaps or key in not_applicable:
                raise SystemExit(f"error: duplicate cell in {GAPS_PATH}: {key}")
            not_applicable[key] = entry["reason"]
    return document["issue"], planned, gaps, not_applicable


def support_matrix() -> dict[str, Any]:
    presets = json.loads((REPO_ROOT / "CMakePresets.json").read_text())

    # coverage[(binding, target)] holds the best status that CI reached.
    coverage: dict[tuple[str, Target], str] = {}

    def record(binding: str, target: Target, status: str) -> None:
        if binding not in BINDING_LABELS:
            raise SystemExit(f"error: no binding label for {binding!r}")
        if coverage.get((binding, target)) != "tested":
            coverage[(binding, target)] = status

    for preset in plan.targets(presets):
        target = target_from_preset(preset)
        record("c", target, "tested" if runtime_tested(preset) else "build-only")
        commands = [" ".join(suite.command) for suite in target_suites(preset)]
        for command in commands:
            for binding, status in command_support(command):
                record(binding, target, status)

    issue, planned, gaps, not_applicable = load_gaps()
    built = {target for (name, target) in coverage if name == "c"}
    for target in planned:
        if target in built:
            raise SystemExit(
                f"error: CI builds planned target {target}; remove it from {GAPS_PATH}"
            )
    covered = {(name, target.platform) for (name, target) in coverage}
    for key in [*gaps, *not_applicable]:
        if key in covered:
            raise SystemExit(f"error: CI covers {key}; remove it from {GAPS_PATH}")

    def cell(name: str, target: Target) -> dict[str, str]:
        if (name, target) in coverage:
            return {"status": coverage[(name, target)]}
        reason = not_applicable.get((name, target.platform))
        if reason:
            return {"status": "not-applicable", "detail": reason}
        gap = gaps.get((name, target.platform))
        if gap:
            # Tooltips show plain text, so code spans drop their backticks.
            return {"status": gap["status"], "detail": gap["reason"].replace("`", "")}
        return {"status": "unknown"}

    platforms: dict[str, dict[tuple[str, str], dict[str, list[dict[str, Any]]]]] = (
        defaultdict(lambda: defaultdict(dict))
    )
    for target in sorted(built | set(planned), key=Target.sort_key):
        arch = (target.arch, target.arch_label())
        platforms[target.platform][arch][target.backend] = [
            cell(name, target) for name in BINDING_LABELS
        ]

    return {
        "issue": issue,
        "statuses": [
            {"status": status, "label": label}
            for status, label in STATUS_LABELS.items()
        ],
        "bindings": [
            {
                "id": name,
                "label": label,
                "sourceDirectory": (
                    "include" if name == "c" else f"bindings/{language(name)}"
                ),
            }
            for name, label in BINDING_LABELS.items()
        ],
        "platforms": [
            {
                "platform": name,
                "label": PLATFORM_LABELS[name],
                "architectures": [
                    {
                        "arch": arch,
                        "label": label,
                        "backends": [
                            {
                                "backend": value,
                                "label": BACKEND_LABELS[value],
                                "cells": cells,
                            }
                            for value, cells in backends.items()
                        ],
                    }
                    for (arch, label), backends in architectures.items()
                ],
            }
            for name, architectures in platforms.items()
        ],
        "gaps": [
            {
                **gaps[(name, platform_name)],
                "binding": BINDING_LABELS[name],
                "platform": PLATFORM_LABELS[platform_name],
            }
            for name in BINDING_LABELS
            for platform_name in PLATFORM_ORDER
            if (name, platform_name) in gaps
        ],
    }


def main() -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(
        json.dumps(support_matrix(), indent=2, sort_keys=False) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
