#!/usr/bin/env python3
"""Check that every binding maps every conformance case to a real test.

tests/conformance/cases.toml lists the cases. Each binding's file maps every
case ID to "path::test_name", where the path is relative to the repository and
the file mentions the test name, or to { na = "reason" }. A case still marked
{ todo = true } passes unless --strict is given.
"""

import argparse
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFORMANCE = ROOT / "tests" / "conformance"
BINDINGS = ("rust", "python", "go", "zig", "kotlin", "swift", "dotnet", "dart")


def check_binding(binding: str, ids: list[str], strict: bool) -> list[str]:
    path = CONFORMANCE / f"{binding}.toml"
    if not path.exists():
        return [f"{path.relative_to(ROOT)} is missing"]
    cases = tomllib.loads(path.read_text()).get("cases", {})
    errors = [
        f"{binding}: {case} is not a case in cases.toml"
        for case in cases
        if case not in ids
    ]
    for case in ids:
        entry = cases.get(case)
        if entry is None:
            errors.append(f"{binding}: {case} has no entry")
        elif isinstance(entry, dict):
            if entry.get("todo"):
                if strict:
                    errors.append(f"{binding}: {case} is still todo")
            elif not str(entry.get("na", "")).strip():
                errors.append(f"{binding}: {case} needs a test or an na reason")
        elif isinstance(entry, str):
            file, separator, name = entry.partition("::")
            test_file = ROOT / file
            if not separator or not name:
                errors.append(f"{binding}: {case} must be 'path::test_name'")
            elif not test_file.is_file():
                errors.append(f"{binding}: {case} names missing file {file}")
            elif name not in test_file.read_text(errors="replace"):
                errors.append(f"{binding}: {case} names {name}, absent from {file}")
        else:
            errors.append(f"{binding}: {case} must be a string or a table")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--strict", action="store_true", help="fail on todo entries")
    arguments = parser.parse_args()
    ids = list(tomllib.loads((CONFORMANCE / "cases.toml").read_text())["cases"])
    errors = [
        error
        for binding in BINDINGS
        for error in check_binding(binding, ids, arguments.strict)
    ]
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
