#!/usr/bin/env python3
"""Derives and checks execution classifications from the public C headers.

Every public function's execution form follows from its name and signature,
so the headers are the single source of truth:

- ``_start`` suffix: operation. Declares ``mln_operation* out_operation`` and
  carries a documentation comment.
- ``_take_result`` suffix: immediate result accessor taking ``mln_operation``.
- ``_destroy`` or ``_release`` suffix: immediate.
- ``_drain_`` in the name: event batch.
- ``_service_driver_work`` suffix: render-driver call. Takes a session and
  documents its graphics-thread contract.
- ``snapshot`` in the name with a live ``mln_map`` or ``mln_render_session``
  parameter: published snapshot.
- ``uint64_t* out_command_id`` parameter: command.
- Anything else: immediate.

EXCEPTIONS lists the functions whose form is not derivable; each entry says
why. Growth of that table is a design smell, not a checker gap.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

# Irregular forms, with the reason each one cannot follow the conventions.
EXCEPTIONS = {
    # A frame demand is a command in the render-session domain, which has no
    # command channel: its terminal report drains through frame results
    # instead of COMMAND_FINISHED, so it carries no out_command_id.
    "mln_render_session_request_frame": "command",
}

FUNCTION = re.compile(
    r"MLN_API\s+[^;]*?\b(mln_[A-Za-z0-9_]+)\s*\([^;]*?\)\s*MLN_NOEXCEPT\s*;",
    re.DOTALL,
)


def header_files(paths: list[pathlib.Path]) -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for path in paths:
        if path.is_dir():
            files.extend(sorted(path.glob("*.h")))
        else:
            files.append(path)
    return files


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
    if name in EXCEPTIONS:
        return EXCEPTIONS[name]
    if name.endswith("_take_result"):
        return "immediate"
    if name.endswith("_start"):
        return "operation"
    if name.endswith(("_destroy", "_release")):
        return "immediate"
    if "_drain_" in name:
        return "event_batch"
    if name.endswith("_service_driver_work"):
        return "render_driver_call"
    if "snapshot" in name and re.search(
        r"\b(mln_map|mln_render_session)\s+\w+", declaration
    ):
        return "published_snapshot"
    if re.search(r"\buint64_t\s*\*\s*out_command_id\b", declaration):
        return "command"
    return "immediate"


def convention_errors(
    name: str, category: str, declaration: str, documentation: str
) -> list[str]:
    errors: list[str] = []
    if name.endswith("_start"):
        if not re.search(r"\bmln_operation\s*\*\s*out_operation\b", declaration):
            errors.append(f"operation starter has no out_operation boundary: {name}")
        if not documentation:
            errors.append(f"operation starter has no boundary documentation: {name}")
        if re.search(r"\bout_command_id\b", declaration):
            errors.append(f"operation starter also takes out_command_id: {name}")
    elif name.endswith("_take_result"):
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
    elif category == "command" and name not in EXCEPTIONS:
        if not documentation:
            errors.append(f"command has no boundary documentation: {name}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("headers", nargs="+", type=pathlib.Path)
    arguments = parser.parse_args()

    errors: list[str] = []
    try:
        declarations, documentation = exported_functions(
            header_files(arguments.headers)
        )
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    for name in sorted(EXCEPTIONS.keys() - declarations.keys()):
        errors.append(f"stale exception: {name}")

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
