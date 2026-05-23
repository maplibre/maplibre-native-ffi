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

TYPE_DECLARATION_RE = re.compile(
    r"^(?:public\s+)?(?:(?:abstract|final|sealed|non-sealed)\s+)*"
    r"(class|record|enum|interface)\s+\w+[^\{;]*",
    re.M,
)


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


def normalize_signature(signature: str) -> str:
    signature = re.sub(r"\s+", " ", signature).strip()
    signature = signature.replace("public synchronized ", "public ")
    signature = signature.replace("protected synchronized ", "protected ")
    return signature


def include_signature(signature: str, type_kind: str) -> bool:
    if (
        not signature
        or "InternalAccess" in signature
        or signature.startswith("private ")
    ):
        return False
    if signature.startswith("public ") or signature.startswith("protected "):
        return True
    # Interface fields and methods are public even without an explicit modifier.
    return type_kind == "interface"


def type_body_and_kind(text: str) -> tuple[str, str] | None:
    text = strip_comments(normalize_java(text))
    match = TYPE_DECLARATION_RE.search(text)
    if not match:
        return None
    open_brace = text.find("{", match.end() - 1)
    if open_brace < 0:
        return None
    return match.group(1), text[open_brace + 1 : matching_brace(text, open_brace)]


def class_signatures(text: str) -> list[str]:
    parsed = type_body_and_kind(text)
    if not parsed:
        return []
    type_kind, body = parsed
    signatures: list[str] = []
    member_start = 0
    index = 0
    while index < len(body):
        char = body[index]
        if char == ";":
            candidate = body[member_start:index].strip()
            if ";" in candidate:
                candidate = candidate.split(";")[-1].strip()
            lines = [
                line.strip()
                for line in candidate.splitlines()
                if line.strip() and not line.strip().startswith("@")
            ]
            signature = normalize_signature(" ".join(lines))
            if include_signature(signature, type_kind):
                signatures.append(signature)
            member_start = index + 1
        elif char == "{":
            candidate = body[member_start:index].strip()
            if ";" in candidate:
                candidate = candidate.split(";")[-1].strip()
            lines = [
                line.strip()
                for line in candidate.splitlines()
                if line.strip() and not line.strip().startswith("@")
            ]
            signature = normalize_signature(" ".join(lines))
            if (
                "(" in signature
                and ")" in signature
                and include_signature(signature, type_kind)
            ):
                signatures.append(signature)
            index = matching_brace(body, index)
            member_start = index + 1
        index += 1
    return signatures


def enum_constants(text: str) -> list[str]:
    parsed = type_body_and_kind(text)
    if not parsed:
        return []
    type_kind, body = parsed
    if type_kind != "enum":
        return []
    constants_end = len(body)
    depth = 0
    for index, char in enumerate(body):
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        elif char == ";" and depth == 0:
            constants_end = index
            break
    constants_text = body[:constants_end]
    constants: list[str] = []
    member_start = 0
    depth = 0
    for index, char in enumerate(constants_text + ","):
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
        elif char == "," and depth == 0:
            candidate = constants_text[member_start:index].strip()
            member_start = index + 1
            if not candidate:
                continue
            lines = [
                line.strip()
                for line in candidate.splitlines()
                if line.strip() and not line.strip().startswith("@")
            ]
            constant = normalize_signature(" ".join(lines))
            constant = re.split(r"[\s({]", constant, maxsplit=1)[0]
            if constant:
                constants.append(constant)
    return constants


def public_type_declaration(text: str) -> str | None:
    text = strip_comments(normalize_java(text))
    match = TYPE_DECLARATION_RE.search(text)
    return re.sub(r"\s+", " ", match.group(0)).strip() if match else None


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
        ffm_declaration = public_type_declaration(ffm_text)
        jni_declaration = public_type_declaration(jni_text)
        if ffm_declaration is None or jni_declaration is None:
            mismatches.append((jni_path, "public type declaration was not parsed"))
            continue
        if ffm_declaration != jni_declaration:
            mismatches.append(
                (jni_path, "public type declaration differs from Java FFM")
            )
            continue
        ffm_constants = enum_constants(ffm_text)
        jni_constants = enum_constants(jni_text)
        if ffm_constants != jni_constants:
            mismatches.append((jni_path, "public enum constants differ from Java FFM"))
            continue
        ffm_signatures = class_signatures(ffm_text)
        jni_signatures = class_signatures(jni_text)
        if ffm_signatures != jni_signatures:
            mismatches.append(
                (
                    jni_path,
                    "public/protected class member signatures differ from Java FFM",
                )
            )
    return mismatches


def actual_public_package_sources() -> list[Path]:
    sources: list[Path] = []
    for path in PACKAGE_ROOT.rglob("*.java"):
        relative = path.relative_to(PACKAGE_ROOT)
        if relative.name == "package-info.java" or relative.parts[0] == "internal":
            continue
        sources.append(path)
    return sorted(sources)


def public_internal_api_leaks(expected: list[Path]) -> list[Path]:
    pattern = re.compile(
        r"\b(?:public|protected)\b[^;{]*(?:InternalAccess|nativeAddress\s*\()"
    )
    return [
        path
        for path in expected
        if path.exists()
        and pattern.search(strip_comments(path.read_text(encoding="utf-8")))
    ]


def main() -> int:
    expected = parse_inventory()
    expected_set = set(expected)
    missing = [path for path in expected if not path.exists()]
    extra_sources = [
        path for path in actual_public_package_sources() if path not in expected_set
    ]
    markers = sorted(PACKAGE_ROOT.glob("*/PackageMarker.java"))
    stale_imports = [
        path
        for path in expected
        if path.exists()
        and (
            "org.maplibre.nativeffi" in path.read_text(encoding="utf-8")
            or "java.lang.foreign" in path.read_text(encoding="utf-8")
        )
    ]
    parity = parity_mismatches(expected)
    internal_api_leaks = public_internal_api_leaks(expected)

    module_text = MODULE_INFO.read_text(encoding="utf-8")
    missing_exports = [
        f"org.maplibre.nativejni.{package}"
        for package in PUBLIC_PACKAGES
        if f"exports org.maplibre.nativejni.{package};" not in module_text
    ]
    if "exports org.maplibre.nativejni;" not in module_text:
        missing_exports.insert(0, "org.maplibre.nativejni")

    if (
        missing
        or extra_sources
        or markers
        or stale_imports
        or parity
        or internal_api_leaks
        or missing_exports
    ):
        if missing:
            print("Missing public inventory files:", file=sys.stderr)
            for path in missing:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if extra_sources:
            print(
                "Public package sources missing from SPEC inventory:", file=sys.stderr
            )
            for path in extra_sources:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if markers:
            print("Package markers still present:", file=sys.stderr)
            for path in markers:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if stale_imports:
            print("Files still reference Java FFM APIs:", file=sys.stderr)
            for path in stale_imports:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        if parity:
            print("Java FFM parity mismatches:", file=sys.stderr)
            for path, reason in parity:
                print(f"  {path.relative_to(ROOT)}: {reason}", file=sys.stderr)
        if internal_api_leaks:
            print(
                "Public/protected JNI APIs leak internal native access:",
                file=sys.stderr,
            )
            for path in internal_api_leaks:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
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
