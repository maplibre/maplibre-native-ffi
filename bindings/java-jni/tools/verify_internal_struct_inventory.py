#!/usr/bin/env python3
"""Verify Java JNI internal struct helper inventory from SPEC.md."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SPEC = ROOT / "bindings/java-jni/SPEC.md"
JAVA_ROOT = ROOT / "bindings/java-jni/src/main/java/org/maplibre/nativejni"
STRUCT_ROOT = JAVA_ROOT / "internal/struct"


def parse_struct_inventory() -> list[str]:
    text = SPEC.read_text(encoding="utf-8")
    start = text.index("## Java internal and package-private implementation inventory")
    end = text.index("## Rust bridge crate inventory", start)
    section = text[start:end]
    files: list[str] = []
    for line in section.splitlines():
        match = re.match(r"\| `internal\.struct\.([^`]+)`", line)
        if match:
            files.append(f"{match.group(1)}.java")
    return files


def main() -> int:
    expected = parse_struct_inventory()
    missing = [file for file in expected if not (STRUCT_ROOT / file).exists()]
    empty = [
        file
        for file in expected
        if (STRUCT_ROOT / file).exists()
        and "public final class" not in (STRUCT_ROOT / file).read_text(encoding="utf-8")
    ]

    if missing or empty:
        if missing:
            print("Missing internal struct helpers:", file=sys.stderr)
            for file in missing:
                print(
                    f"  internal.struct.{file.removesuffix('.java')}", file=sys.stderr
                )
        if empty:
            print(
                "Struct helper files without implementation classes:", file=sys.stderr
            )
            for file in empty:
                print(
                    f"  internal.struct.{file.removesuffix('.java')}", file=sys.stderr
                )
        return 1

    print(
        f"Verified {len(expected)} Java JNI internal struct helper files from SPEC.md."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
