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
    "wasi": "WASI",
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
    "not-investigated": "Not investigated",
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
    # Kotlin's columns name what each ships: JARs for the JVM and Android, whose
    # AAR wraps one, and KLIBs for Kotlin/Native and Kotlin/Wasm.
    "kotlin-jvm": "Kotlin (JAR)",
    "kotlin-native": "Kotlin (KLIB)",
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


SELECTORS = {
    "bindings": BINDING_LABELS,
    "platforms": PLATFORM_LABELS,
    "archs": ARCH_ORDER,
    "backends": BACKEND_LABELS,
}
# The statuses an entry may give. CI gives `tested` and `build-only`.
ENTRY_STATUSES = {*GAP_STATUSES, "not-applicable", "not-investigated"}
# Every row has a platform, an architecture, and a backend.
ROW_AXES = ("platforms", "archs", "backends")


@dataclass(frozen=True)
class Entry:
    """An `[[entry]]`: the cells where its axes intersect, and their status."""

    index: int
    selectors: dict[str, tuple[str, ...]]
    status: str
    reason: str | None
    link: str | None

    def matches(self, binding: str, target: Target) -> bool:
        values = {
            "bindings": binding,
            "platforms": target.platform,
            "archs": target.arch,
            "backends": target.backend,
        }
        return all(values[key] in allowed for key, allowed in self.selectors.items())

    def sort_key(self) -> tuple[int, int]:
        """Orders entries by their bindings, then their platforms."""
        bindings = self.selectors.get("bindings", ())
        platforms = self.selectors.get("platforms", ())
        return (
            min((list(BINDING_LABELS).index(name) for name in bindings), default=-1),
            min((PLATFORM_ORDER.index(name) for name in platforms), default=-1),
        )

    def binding(self) -> str:
        """Names the columns it selects, as `C#, Dart`."""
        bindings = self.selectors.get("bindings")
        if not bindings:
            return "Every binding"
        return ", ".join(BINDING_LABELS[name] for name in bindings)

    def rows(self) -> list[Target]:
        """The targets it names in full, whose cells it gives a status to."""
        if not all(axis in self.selectors for axis in ROW_AXES):
            return []
        return [
            Target(platform=platform, arch=arch, backend=backend)
            for platform in self.selectors["platforms"]
            for arch in self.selectors["archs"]
            for backend in self.selectors["backends"]
        ]

    def describe(self) -> str:
        return f"[[entry]] {self.index + 1} in {GAPS_PATH}"

    def scope(self, targets: list[Target]) -> str:
        """Names the rows it selects, as `macOS x64 with Metal`.

        An axis that spans every row of its platforms goes unnamed.
        """
        platforms = self.selectors.get("platforms")
        parts: list[str] = [
            ", ".join(PLATFORM_LABELS[name] for name in platforms)
            if platforms
            else "every platform"
        ]
        rows = [
            target
            for target in targets
            if not platforms or target.platform in platforms
        ]
        archs = self.selectors.get("archs")
        if archs and not {target.arch for target in rows} <= set(archs):
            parts.append(", ".join(archs))
        backends = self.selectors.get("backends")
        if backends and not {target.backend for target in rows} <= set(backends):
            parts.append("with " + ", ".join(BACKEND_LABELS[name] for name in backends))
        return " ".join(parts)


def load_gaps() -> tuple[int, list[Entry]]:
    document = tomllib.loads(GAPS_PATH.read_text(encoding="utf-8"))
    unknown = document.keys() - {"issue", "entry"}
    if unknown:
        raise SystemExit(f"error: unknown tables in {GAPS_PATH}: {sorted(unknown)}")
    entries = []
    for index, entry in enumerate(document.get("entry", [])):
        # A misspelled axis would be left out, which would cover more cells.
        if entry.keys() - SELECTORS.keys() - {"status", "reason", "link"}:
            raise SystemExit(f"error: unknown keys in {GAPS_PATH} [[entry]]: {entry}")
        status = entry.get("status")
        if status not in ENTRY_STATUSES:
            raise SystemExit(f"error: unknown status in {GAPS_PATH} [[entry]]: {entry}")
        if status != "not-investigated" and not entry.get("reason"):
            raise SystemExit(f"error: {GAPS_PATH} [[entry]] needs a reason: {entry}")
        selectors = {}
        for axis in SELECTORS.keys() & entry.keys():
            values = entry[axis]
            if not isinstance(values, list) or not values:
                raise SystemExit(
                    f"error: {axis} in {GAPS_PATH} [[entry]] must be a non-empty list: {entry}"
                )
            unknown = [value for value in values if value not in SELECTORS[axis]]
            if unknown:
                raise SystemExit(f"error: unknown {axis} in {GAPS_PATH}: {unknown}")
            selectors[axis] = tuple(values)
        entries.append(
            Entry(
                index=index,
                selectors=selectors,
                status=status,
                reason=entry.get("reason"),
                link=entry.get("link"),
            )
        )
    return document["issue"], entries


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

    issue, entries = load_gaps()
    built = {target for (name, target) in coverage if name == "c"}
    named = {target for entry in entries for target in entry.rows()}
    targets = sorted(built | named, key=Target.sort_key)

    # explained[(binding, target)] holds the entry for a cell CI does not reach.
    explained: dict[tuple[str, Target], Entry] = {}
    for entry in entries:
        cells = [
            (name, target)
            for name in BINDING_LABELS
            for target in targets
            if entry.matches(name, target)
        ]
        if not cells:
            raise SystemExit(f"error: {entry.describe()} matches no cell")
        for cell in cells:
            if cell in coverage:
                raise SystemExit(
                    f"error: CI reaches {cell}, which {entry.describe()} "
                    "explains; narrow or remove the entry"
                )
            if cell in explained:
                raise SystemExit(
                    f"error: {explained[cell].describe()} and {entry.describe()} "
                    f"both explain {cell}"
                )
            explained[cell] = entry

    def blocked_c(target: Target) -> Entry | None:
        entry = explained.get(("c", target))
        return entry if entry and entry.status == "blocked" else None

    def cell(name: str, target: Target) -> dict[str, str]:
        if (name, target) in coverage:
            return {"status": coverage[(name, target)]}
        # A binding cannot reach a target before the C library does.
        entry = explained.get((name, target)) or blocked_c(target)
        if entry is None or entry.reason is None:
            return {"status": "not-investigated"}
        # Tooltips show plain text, so code spans drop their backticks.
        return {"status": entry.status, "detail": entry.reason.replace("`", "")}

    platforms: dict[str, dict[tuple[str, str], dict[str, list[dict[str, Any]]]]] = (
        defaultdict(lambda: defaultdict(dict))
    )
    for target in targets:
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
                "status": entry.status,
                "binding": entry.binding(),
                "scope": entry.scope(targets),
                "reason": entry.reason or "",
                **({"link": entry.link} if entry.link else {}),
            }
            for entry in sorted(entries, key=Entry.sort_key)
            if entry.status in GAP_STATUSES
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
