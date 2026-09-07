#!/usr/bin/env python3
"""Checks execution categories derived from public C declarations.

A ``const mln_completion* completion`` parameter identifies one-shot
asynchronous work. Drains transfer queued stream records, driver service keeps
its graphics-thread contract, published snapshots are synchronous copies, and
everything else is immediate. Public operation handles, result-taking
accessors, and command IDs are forbidden, and every status-returning
declaration carries a ``Returns:`` list.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

FUNCTION = re.compile(
    r"MLN_API\s+[^;]*?\b(mln_[A-Za-z0-9_]+)\s*\([^;]*?\)\s*MLN_NOEXCEPT\s*;",
    re.DOTALL,
)


def exported_functions(
    headers: list[pathlib.Path],
) -> tuple[dict[str, str], dict[str, str]]:
    declarations: dict[str, str] = {}
    documentation: dict[str, str] = {}
    for header in headers:
        contents = header.read_text()
        for match in FUNCTION.finditer(contents):
            name = match.group(1)
            if name in declarations:
                raise ValueError(f"{name} is declared more than once")
            declarations[name] = match.group(0)
            preceding = contents[: match.start()]
            comments = list(re.finditer(r"/\*\*.*?\*/", preceding, re.DOTALL))
            comment = comments[-1] if comments else None
            documentation[name] = (
                comment.group(0)
                if comment and not preceding[comment.end() :].strip()
                else ""
            )
    return declarations, documentation


def derive_category(name: str, declaration: str) -> str:
    if re.search(r"\bconst\s+mln_completion\s*\*\s*completion\b", declaration):
        return "completion"
    if "_drain_" in name:
        return "event_batch"
    if name.endswith("_service_driver_work"):
        return "render_driver_call"
    if "snapshot" in name and re.search(
        r"\b(mln_map|mln_render_session)\s+\w+", declaration
    ):
        return "published_snapshot"
    return "immediate"


def convention_errors(
    name: str, category: str, declaration: str, documentation: str
) -> list[str]:
    errors: list[str] = []
    legacy = re.search(
        r"\bmln_operation\b|\bout_(?:command_id|operation)\b", declaration
    )
    if legacy or name.endswith(("_start", "_take_result")):
        errors.append(f"legacy asynchronous shape remains public: {name}")
    if (
        re.match(r"MLN_API\s+mln_status\b", declaration.strip())
        and "Returns:" not in documentation
    ):
        errors.append(f"status-returning function has no Returns list: {name}")
    if category == "completion":
        if not documentation:
            errors.append(f"completion function has no boundary documentation: {name}")
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
    parser.add_argument("include_directory", type=pathlib.Path)
    arguments = parser.parse_args()

    errors: list[str] = []
    try:
        declarations, documentation = exported_functions(
            sorted(arguments.include_directory.glob("*.h"))
        )
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    counts: dict[str, int] = {}
    for name in sorted(declarations):
        category = derive_category(name, declarations[name])
        counts[category] = counts.get(category, 0) + 1
        errors.extend(
            convention_errors(name, category, declarations[name], documentation[name])
        )

    if errors:
        print("\n".join(f"error: {error}" for error in errors), file=sys.stderr)
        return 1
    summary = ", ".join(f"{count} {name}" for name, count in sorted(counts.items()))
    print(f"classified {len(declarations)} exports by convention: {summary}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
