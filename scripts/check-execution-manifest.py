#!/usr/bin/env python3
"""Checks execution classifications for the runtime, map, and render API."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

SUPPORTED_CATEGORIES = (
    "immediate",
    "command",
    "published_snapshot",
    "operation",
    "event_batch",
    "render_driver_call",
)


FUNCTION = re.compile(
    r"MLN_API\s+[^;]*?\b(mln_[A-Za-z0-9_]+)\s*\([^;]*?\)\s*MLN_NOEXCEPT\s*;",
    re.DOTALL,
)

PHASE3_HEADERS = (
    "base.h",
    "render_target.h",
    "render_session.h",
    "surface.h",
    "texture.h",
)


def complete_header_set(headers: list[pathlib.Path]) -> list[pathlib.Path]:
    """Adds the render contract headers beside the explicitly selected headers."""
    completed = list(headers)
    seen = {header.resolve() for header in completed}
    include_directories = {header.parent for header in headers}
    for directory in sorted(include_directories):
        for name in PHASE3_HEADERS:
            candidate = directory / name
            if candidate.exists() and candidate.resolve() not in seen:
                completed.append(candidate)
                seen.add(candidate.resolve())
    return completed


def exported_functions(
    headers: list[pathlib.Path],
) -> tuple[dict[str, str], dict[str, str], dict[str, str]]:
    exports: dict[str, str] = {}
    declarations: dict[str, str] = {}
    documentation: dict[str, str] = {}
    for header in headers:
        contents = header.read_text()
        for match in FUNCTION.finditer(contents):
            name = match.group(1)
            if name in exports:
                raise ValueError(f"{name} is declared more than once")
            exports[name] = header.name
            declarations[name] = match.group(0)
            preceding = contents[: match.start()]
            comments = list(re.finditer(r"/\*\*.*?\*/", preceding, re.DOTALL))
            comment = comments[-1] if comments else None
            documentation[name] = (
                comment.group(0)
                if comment and not preceding[comment.end() :].strip()
                else ""
            )
    return exports, declarations, documentation


def object_without_duplicate_keys(
    pairs: list[tuple[str, object]],
) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def boundary_convention_errors(
    name: str, category: str, declaration: str, documentation: str
) -> list[str]:
    errors: list[str] = []
    if name.endswith("_start"):
        if category != "operation":
            errors.append(f"operation starter has category {category}: {name}")
        if not re.search(r"\bmln_operation\s*\*\s*out_operation\b", declaration):
            errors.append(f"operation starter has no out_operation boundary: {name}")
        if not documentation:
            errors.append(f"operation starter has no boundary documentation: {name}")
    elif name.endswith("_take_result"):
        if category != "immediate":
            errors.append(f"operation result accessor has category {category}: {name}")
        if not re.search(r"\bmln_operation\s+operation\b", declaration):
            errors.append(f"operation result accessor has no operation input: {name}")
    elif category == "render_driver_call":
        if not re.search(r"\bmln_render_session\s+session\b", declaration):
            errors.append(f"render-driver call has no session boundary: {name}")
        thread_contract = documentation.lower()
        if not (
            "graphics thread" in thread_contract
            or "graphics-thread" in thread_contract
            or "context must be current" in thread_contract
        ):
            errors.append(f"render-driver call does not document its thread: {name}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=pathlib.Path)
    parser.add_argument("headers", nargs="+", type=pathlib.Path)
    arguments = parser.parse_args()

    errors: list[str] = []
    try:
        data = json.loads(
            arguments.manifest.read_text(),
            object_pairs_hook=object_without_duplicate_keys,
        )
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"error: cannot read manifest: {error}", file=sys.stderr)
        return 1

    if not isinstance(data, dict):
        print("error: manifest root must be an object", file=sys.stderr)
        return 1
    if set(data) != {"schema_version", "functions"}:
        errors.append("manifest must contain schema_version and functions")
    if data.get("schema_version") != 2:
        errors.append("schema_version must be 2")

    entries = data.get("functions")
    if not isinstance(entries, dict):
        errors.append("functions must be an object")
        entries = {}

    classified: dict[str, str] = {}
    for name, category in entries.items():
        if not isinstance(name, str) or not isinstance(category, str):
            errors.append("function names and categories must be strings")
            continue
        classified[name] = category
        if category not in SUPPORTED_CATEGORIES:
            errors.append(f"unsupported category for {name}: {category}")
    if list(entries) != sorted(entries):
        errors.append("function classifications must be sorted by function name")

    try:
        exports, declarations, documentation = exported_functions(
            complete_header_set(arguments.headers)
        )
    except (OSError, ValueError) as error:
        errors.append(str(error))
        exports, declarations, documentation = {}, {}, {}
    for name in sorted(exports.keys() - classified.keys()):
        errors.append(f"unclassified export: {name}")
    for name in sorted(classified.keys() - exports.keys()):
        errors.append(f"stale classification: {name}")
    for name in sorted(exports.keys() & classified.keys()):
        category = classified[name]
        errors.extend(
            boundary_convention_errors(
                name, category, declarations[name], documentation[name]
            )
        )
    if errors:
        print("\n".join(f"error: {error}" for error in errors), file=sys.stderr)
        return 1
    print(f"classified {len(exports)} execution-boundary functions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
