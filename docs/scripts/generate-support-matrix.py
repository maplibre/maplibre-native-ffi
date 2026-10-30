#!/usr/bin/env python3
import json
import pathlib
import sys
from collections import defaultdict
from typing import Any

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "support-matrix.json"
sys.path.insert(0, str(REPO_ROOT))

from tools.ci import plan
from tools.ci.gaps import (
    BACKEND_LABELS,
    BINDING_LABELS,
    GAP_STATUSES,
    OPERATIONS,
    PLATFORM_LABELS,
    STATUS_LABELS,
    Entry,
    Target,
    gap,
    load_gaps,
    target_from_preset,
)
from tools.ci.suites import command_support, target_suites


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
        if gap("c", target, "build"):
            continue
        native_tested = not gap("c", target, "test")
        record("c", target, "tested" if native_tested else "build-only")
        commands = [" ".join(suite.command) for suite in target_suites(preset)]
        for command in commands:
            for binding, status in command_support(command):
                record(binding, target, status)

    issue, entries = load_gaps()
    built = {target_from_preset(preset) for preset in plan.targets(presets)}
    named = {target for entry in entries for target in entry.rows()}
    targets = sorted(built | named, key=Target.sort_key)

    # Keep test and publication gaps visible even when CI builds the binding.
    explained: dict[tuple[str, Target, str], Entry] = {}
    for entry in entries:
        cells = [
            (name, target, operation)
            for name in BINDING_LABELS
            for target in targets
            for operation in OPERATIONS
            if entry.matches(name, target, operation)
        ]
        if not cells:
            raise SystemExit(f"error: {entry.describe()} matches no cell")
        for name, target, operation in cells:
            status = coverage.get((name, target))
            if (operation == "build" and status) or (
                operation == "test" and status == "tested"
            ):
                raise SystemExit(
                    f"error: CI reaches {name} {operation} on {target}, which {entry.describe()} explains; narrow or remove the entry"
                )
            cell = (name, target, operation)
            if cell in explained:
                raise SystemExit(
                    f"error: {explained[cell].describe()} and {entry.describe()} both explain {cell}"
                )
            explained[cell] = entry

    def explanation(name: str, target: Target, operation: str) -> Entry | None:
        return explained.get((name, target, operation)) or explained.get(
            ("c", target, operation)
        )

    def cell(name: str, target: Target) -> dict[str, str]:
        entry = (
            explanation(name, target, "test")
            or explanation(name, target, "build")
            or explanation(name, target, "package")
        )
        status = coverage.get((name, target))
        if status is None:
            entry = explanation(name, target, "test") or explanation(
                name, target, "build"
            )
            if entry is None:
                raise SystemExit(
                    f"error: CI does not reach {name} on {target}; add its gap to docs/support-gaps.toml"
                )
            status = entry.status
        result = {"status": status}
        if entry and entry.reason:
            # Tooltips show plain text, so code spans drop their backticks.
            result["detail"] = entry.reason.replace("`", "")
        return result

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
                    "include" if name == "c" else f"bindings/{name.split('-', 1)[0]}"
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
