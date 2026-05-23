#!/usr/bin/env python3
"""Verify Java JNI native declaration and registration coverage from SPEC.md."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SPEC = ROOT / "bindings/java-jni/SPEC.md"
JAVA_BRIDGE_ROOT = (
    ROOT / "bindings/java-jni/src/main/java/org/maplibre/nativejni/internal/bridge"
)
RUST_BRIDGE = ROOT / "bindings/java-jni/native/src/lib.rs"


def parse_coverage_map() -> dict[str, list[str]]:
    text = SPEC.read_text(encoding="utf-8")
    start = text.index("## Native method coverage map")
    end = text.index("## Test implementation map", start)
    section = text[start:end]

    groups: dict[str, list[str]] = {}
    current: str | None = None
    for line in section.splitlines():
        heading = re.fullmatch(r"### `([^`]+)`", line.strip())
        if heading:
            current = heading.group(1)
            groups[current] = []
            continue
        bullet = re.fullmatch(r"- `(mln_[^`]+)`(?:.*)?", line.strip())
        if bullet and current:
            groups[current].append(bullet.group(1).split()[0])
    return groups


def java_native_methods(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8")
    return set(re.findall(r"public\s+static\s+native\s+\S+\s+(mln_\w+)\s*\(", text))


def rust_registered_methods() -> set[str]:
    text = RUST_BRIDGE.read_text(encoding="utf-8")
    return set(re.findall(r'"(mln_\w+)"', text))


def rust_recorded_unsupported_methods() -> set[str]:
    text = RUST_BRIDGE.read_text(encoding="utf-8")
    methods: set[str] = set()
    for match in re.finditer(
        r"recorded_unsupported_status_methods\(&\[(.*?)\]\)", text, re.S
    ):
        methods.update(re.findall(r'"(mln_\w+)"', match.group(1)))
    return methods


def spec_recorded_unsupported_methods() -> set[str]:
    text = SPEC.read_text(encoding="utf-8")
    start = text.index("### Recorded unsupported or replaced C helper coverage")
    end = text.index("### `BaseNative`", start)
    section = text[start:end]
    return set(re.findall(r"- `(mln_\w+)`:", section))


def direct_invalid_argument_returns() -> list[tuple[int, str]]:
    text = RUST_BRIDGE.read_text(encoding="utf-8")
    direct_returns: list[tuple[int, str]] = []
    current_function: str | None = None
    for line_number, line in enumerate(text.splitlines(), start=1):
        function = re.match(r"fn\s+(\w+)", line)
        if function:
            current_function = function.group(1)
        if (
            "sys::MLN_STATUS_INVALID_ARGUMENT" in line
            and current_function != "jni_invalid_argument"
        ):
            direct_returns.append((line_number, line.strip()))
    return direct_returns


def main() -> int:
    groups = parse_coverage_map()
    rust_methods = rust_registered_methods()
    unsupported_methods = rust_recorded_unsupported_methods()
    recorded_unsupported_methods = spec_recorded_unsupported_methods()
    missing_classes: list[str] = []
    missing_java: list[tuple[str, str]] = []
    missing_rust: list[tuple[str, str]] = []
    covered_methods = {
        function for functions in groups.values() for function in functions
    }
    java_methods_by_name = {
        method
        for path in JAVA_BRIDGE_ROOT.glob("*.java")
        for method in java_native_methods(path)
    }
    extra_java = sorted(java_methods_by_name - covered_methods)
    extra_rust = sorted(rust_methods - covered_methods)
    undocumented_unsupported = sorted(
        unsupported_methods - recorded_unsupported_methods
    )
    stale_recorded_unsupported = sorted(
        recorded_unsupported_methods - unsupported_methods
    )
    direct_invalid_returns = direct_invalid_argument_returns()

    for group, functions in groups.items():
        java_path = JAVA_BRIDGE_ROOT / f"{group}.java"
        if not java_path.exists():
            missing_classes.append(group)
            for function in functions:
                missing_java.append((group, function))
                if function not in rust_methods:
                    missing_rust.append((group, function))
            continue
        java_methods = java_native_methods(java_path)
        for function in functions:
            if function not in java_methods:
                missing_java.append((group, function))
            if function not in rust_methods:
                missing_rust.append((group, function))

    if (
        missing_classes
        or missing_java
        or missing_rust
        or extra_java
        or extra_rust
        or undocumented_unsupported
        or stale_recorded_unsupported
        or direct_invalid_returns
    ):
        if missing_classes:
            print("Missing Java native coverage classes:", file=sys.stderr)
            for group in missing_classes:
                print(f"  {group}", file=sys.stderr)
        if missing_java:
            print("Missing Java native declarations:", file=sys.stderr)
            for group, function in missing_java:
                print(f"  {group}.{function}", file=sys.stderr)
        if missing_rust:
            print("Missing Rust JNI registrations:", file=sys.stderr)
            for group, function in missing_rust:
                print(f"  {group}.{function}", file=sys.stderr)
        if extra_java:
            print(
                "Java native declarations missing from SPEC coverage:", file=sys.stderr
            )
            for function in extra_java:
                print(f"  {function}", file=sys.stderr)
        if extra_rust:
            print("Rust JNI registrations missing from SPEC coverage:", file=sys.stderr)
            for function in extra_rust:
                print(f"  {function}", file=sys.stderr)
        if undocumented_unsupported:
            print(
                "Rust unsupported registrations missing SPEC reasons:", file=sys.stderr
            )
            for function in undocumented_unsupported:
                print(f"  {function}", file=sys.stderr)
        if stale_recorded_unsupported:
            print(
                "SPEC records unsupported helpers not registered as unsupported:",
                file=sys.stderr,
            )
            for function in stale_recorded_unsupported:
                print(f"  {function}", file=sys.stderr)
        if direct_invalid_returns:
            print(
                "Direct JNI MLN_STATUS_INVALID_ARGUMENT returns bypass diagnostics:",
                file=sys.stderr,
            )
            for line_number, line in direct_invalid_returns:
                print(f"  {RUST_BRIDGE}:{line_number}: {line}", file=sys.stderr)
        return 1

    total = sum(len(functions) for functions in groups.values())
    print(
        f"Verified {total} JNI native declarations and Rust registrations from SPEC.md; "
        f"{len(unsupported_methods)} recorded unsupported helper replacements."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
