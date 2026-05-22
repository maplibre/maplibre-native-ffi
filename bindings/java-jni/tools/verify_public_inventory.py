#!/usr/bin/env python3
"""Verify the Java JNI public source inventory recorded in SPEC.md."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SPEC = ROOT / "bindings/java-jni/SPEC.md"
JAVA_ROOT = ROOT / "bindings/java-jni/src/main/java"
PACKAGE_ROOT = JAVA_ROOT / "org/maplibre/nativejni"
FFM_PACKAGE_ROOT = ROOT / "bindings/java-ffm/src/main/java/org/maplibre/nativeffi"
MODULE_INFO = JAVA_ROOT / "module-info.java"

PUBLIC_PACKAGES = [
    "camera",
    "error",
    "geo",
    "json",
    "log",
    "map",
    "offline",
    "query",
    "render",
    "resource",
    "runtime",
    "style",
]


def parse_inventory() -> list[Path]:
    text = SPEC.read_text(encoding="utf-8")
    start = text.index("## Java FFM source parity inventory")
    end = text.index(
        "## Java internal and package-private implementation inventory", start
    )
    section = text[start:end]

    current_package: str | None = None
    expected: list[Path] = []
    for line in section.splitlines():
        heading = re.fullmatch(r"### `?([^`]+?)`?", line.strip())
        if heading:
            name = heading.group(1)
            current_package = None if name == "Root" else name
            continue

        bullet = re.fullmatch(r"- `([^`]+\.java)`", line.strip())
        if bullet:
            filename = bullet.group(1)
            if current_package is None:
                expected.append(PACKAGE_ROOT / filename)
            else:
                expected.append(PACKAGE_ROOT / current_package / filename)
    return expected


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//.*", "", text)


def matching_brace(text: str, open_index: int) -> int:
    depth = 0
    quote: str | None = None
    escaped = False
    for index in range(open_index, len(text)):
        char = text[index]
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in {'"', "'"}:
            quote = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError("unmatched brace")


def normalize_java(text: str) -> str:
    return (
        text.replace("org.maplibre.nativeffi", "org.maplibre.nativejni")
        .replace("Java FFM binding", "Java JNI binding")
        .replace("Java FFM", "Java JNI")
        .replace("nativeffi", "nativejni")
    )


def class_signatures(text: str) -> list[str]:
    text = strip_comments(normalize_java(text))
    match = re.search(r"(?:public\\s+)?final\\s+class\\s+(\\w+)[^{]*\\{", text)
    if not match:
        return []
    body = text[match.end() : matching_brace(text, match.end() - 1)]
    signatures: list[str] = []
    member_start = 0
    index = 0
    while index < len(body):
        if body[index] == "{":
            candidate = body[member_start:index].strip()
            if ";" in candidate:
                candidate = candidate.split(";")[-1].strip()
            lines = [
                line.strip()
                for line in candidate.splitlines()
                if line.strip() and not line.strip().startswith("@")
            ]
            signature = re.sub(r"\\s+", " ", " ".join(lines)).strip()
            if (
                "(" in signature
                and ")" in signature
                and not signature.startswith("private ")
            ):
                signatures.append(signature)
            index = matching_brace(body, index)
            member_start = index + 1
        index += 1
    return signatures


def public_type_declaration(text: str) -> str | None:
    text = strip_comments(normalize_java(text))
    match = re.search(
        r"^(public\\s+)?(?:final\\s+)?(?:class|record|enum|interface)\\s+\\w+[^\\{;]*",
        text,
        re.M,
    )
    return re.sub(r"\\s+", " ", match.group(0)).strip() if match else None


def parity_mismatches(expected: list[Path]) -> list[tuple[Path, str]]:
    mismatches: list[tuple[Path, str]] = []
    for jni_path in expected:
        if not jni_path.exists():
            continue
        rel = jni_path.relative_to(PACKAGE_ROOT)
        ffm_path = FFM_PACKAGE_ROOT / rel
        if not ffm_path.exists():
            mismatches.append((jni_path, "missing Java FFM parity source"))
            continue
        ffm_text = ffm_path.read_text(encoding="utf-8")
        jni_text = jni_path.read_text(encoding="utf-8")
        if public_type_declaration(ffm_text) != public_type_declaration(jni_text):
            mismatches.append(
                (jni_path, "public type declaration differs from Java FFM")
            )
            continue
        ffm_signatures = class_signatures(ffm_text)
        jni_signatures = class_signatures(jni_text)
        if ffm_signatures != jni_signatures:
            mismatches.append(
                (jni_path, "non-private class member signatures differ from Java FFM")
            )
    return mismatches


def main() -> int:
    expected = parse_inventory()
    missing = [path for path in expected if not path.exists()]
    markers = sorted(PACKAGE_ROOT.glob("*/PackageMarker.java"))
    stale_imports = [
        path
        for path in expected
        if path.exists()
        and "org.maplibre.nativeffi" in path.read_text(encoding="utf-8")
    ]
    parity = parity_mismatches(expected)

    module_text = MODULE_INFO.read_text(encoding="utf-8")
    missing_exports = [
        f"org.maplibre.nativejni.{package}"
        for package in PUBLIC_PACKAGES
        if f"exports org.maplibre.nativejni.{package};" not in module_text
    ]
    if "exports org.maplibre.nativejni;" not in module_text:
        missing_exports.insert(0, "org.maplibre.nativejni")

    if missing or markers or stale_imports or parity or missing_exports:
        if missing:
            print("Missing public inventory files:", file=sys.stderr)
            for path in missing:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if markers:
            print("Package markers still present:", file=sys.stderr)
            for path in markers:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if stale_imports:
            print("Files still reference org.maplibre.nativeffi:", file=sys.stderr)
            for path in stale_imports:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if parity:
            print("Java FFM parity mismatches:", file=sys.stderr)
            for path, reason in parity:
                print(f"  {path.relative_to(ROOT)}: {reason}", file=sys.stderr)
        if missing_exports:
            print("Missing module exports:", file=sys.stderr)
            for package in missing_exports:
                print(f"  {package}", file=sys.stderr)
        return 1

    print(
        f"Verified {len(expected)} Java JNI public inventory files from bindings/java-jni/SPEC.md."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
