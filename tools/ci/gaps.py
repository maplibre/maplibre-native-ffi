"""Allowed platform gaps, shared by CI selection and the support matrix.

Bindings without a matching entry are supported. A C gap also applies to its
bindings, since they cannot reach a platform before the native library does.
"""

from __future__ import annotations

import pathlib
import tomllib
from dataclasses import dataclass
from functools import cache
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from tools.ci.plan import Target as NativeTarget

GAPS_PATH = pathlib.Path(__file__).resolve().parents[2] / "docs/support-gaps.toml"

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
ARCH_ORDER = ["x64", "x86", "arm64", "arm", "riscv64", "loongarch64", "wasm32"]
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


def target_from_name(preset: str) -> Target:
    platform, arch, backend = preset.rsplit("-", 2)
    return normalized_target(platform, arch, backend)


def normalized_target(platform: str, arch: str, backend: str) -> Target:
    return Target(
        platform={"mingw": "windows-mingw"}.get(
            platform.removesuffix("-simulator"), platform.removesuffix("-simulator")
        ),
        arch=arch,
        backend={"egl": "opengl", "wgl": "opengl", "webgl": "opengl"}.get(
            backend, backend
        ),
        simulator=platform.endswith("-simulator"),
    )


def target_from_preset(preset: NativeTarget) -> Target:
    return normalized_target(preset.platform, preset.architecture, preset.backend)


OPERATIONS = ("build", "test", "package", "xcframework")

SELECTORS = {
    "bindings": BINDING_LABELS,
    "platforms": PLATFORM_LABELS,
    "archs": ARCH_ORDER,
    "backends": BACKEND_LABELS,
    "operations": OPERATIONS,
    "simulators": (False, True),
}
# The statuses an entry may give. CI gives `tested` and `build-only`.
ENTRY_STATUSES = {*GAP_STATUSES, "not-applicable", "not-investigated"}
# Every row has a platform, an architecture, and a backend.
ROW_AXES = ("platforms", "archs", "backends")


@dataclass(frozen=True)
class Entry:
    """An `[[entry]]`: the cells where its axes intersect, and their status."""

    index: int
    selectors: dict[str, tuple[str | bool, ...]]
    status: str
    reason: str | None
    link: str | None

    def matches(self, binding: str, target: Target, operation: str = "test") -> bool:
        values = {
            "bindings": binding,
            "platforms": target.platform,
            "archs": target.arch,
            "backends": target.backend,
            "operations": operation,
            "simulators": target.simulator,
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
            Target(platform=platform, arch=arch, backend=backend, simulator=simulator)
            for platform in self.selectors["platforms"]
            for arch in self.selectors["archs"]
            for backend in self.selectors["backends"]
            for simulator in self.selectors.get("simulators", (False,))
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
        if simulators := self.selectors.get("simulators"):
            parts.append(
                "simulator"
                if simulators == (True,)
                else "device"
                if simulators == (False,)
                else "device and simulator"
            )
        if operations := self.selectors.get("operations"):
            parts.append("(" + ", ".join(operations) + ")")
        return " ".join(parts)


@cache
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


def gap(binding: str, target: Target, operation: str = "test") -> Entry | None:
    """The explicit gap for a cell, falling back to its native library's gap."""
    _, entries = load_gaps()
    for name in (binding, "c") if binding != "c" else ("c",):
        matches = [entry for entry in entries if entry.matches(name, target, operation)]
        if len(matches) > 1:
            raise SystemExit(f"error: multiple gaps explain {name} on {target}")
        if matches:
            return matches[0]
    return None


def supported(binding: str, target: NativeTarget, operation: str = "test") -> bool:
    return gap(binding, target_from_preset(target), operation) is None
