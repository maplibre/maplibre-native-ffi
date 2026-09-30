#!/usr/bin/env python3
"""Check that the C test suite calls every exported function by name.

Every function that a public header declares with MLN_API has to appear by name
in the C test sources, outside comments and string literals. The functions that
no test calls yet are listed in tests/uncalled-exports.txt. The check fails when
an exported function is uncalled and not listed, and when a listed function is
called, no longer exported, or listed twice, so the list only shrinks.

`--prune` rewrites the list without the names that the check would reject. It
never adds a name.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADERS = ROOT / "include"
BASELINE = ROOT / "tests/uncalled-exports.txt"
# The first directory that exists holds the suite, so the check follows the
# suite when it moves.
TEST_DIRS = (ROOT / "tests/native", ROOT / "src/c_api/tests")
TEST_SUFFIXES = {".c", ".cpp", ".h", ".hpp", ".m", ".mm"}
# Adapter entry points serve generated bindings rather than hosts, so the
# report counts them separately.
ADAPTER_HEADER = "callback_adapter.h"

BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
LINE_COMMENT = re.compile(r"//[^\n]*")
STRING_LITERAL = re.compile(r'"(?:[^"\\\n]|\\.)*"')
# The function name is the last identifier before the parameter list. The
# return type may span lines, and nothing between MLN_API and the name holds a
# parenthesis, brace, or semicolon.
DECLARATION = re.compile(r"\bMLN_API\b[^;{}()]*?\b(mln_\w+)\s*\(")
IDENTIFIER = re.compile(r"\bmln_\w+\b")


def strip_comments(text: str) -> str:
    return LINE_COMMENT.sub("", BLOCK_COMMENT.sub(" ", text))


def exported_functions() -> dict[str, str]:
    """Maps each MLN_API function name to the header that declares it."""
    functions: dict[str, str] = {}
    for header in sorted(HEADERS.rglob("*.h")):
        text = strip_comments(header.read_text(encoding="utf-8"))
        # The MLN_API definitions themselves are preprocessor lines.
        text = "\n".join(
            line for line in text.splitlines() if not line.lstrip().startswith("#")
        )
        for match in DECLARATION.finditer(text):
            functions[match.group(1)] = header.name
    if not functions:
        raise SystemExit(f"error: found no MLN_API declarations under {HEADERS}")
    return functions


def test_dir() -> pathlib.Path:
    for directory in TEST_DIRS:
        if directory.is_dir():
            return directory
    raise SystemExit(
        "error: no C test directory: "
        + ", ".join(str(d.relative_to(ROOT)) for d in TEST_DIRS)
    )


def called_names(directory: pathlib.Path) -> set[str]:
    names: set[str] = set()
    for source in sorted(directory.rglob("*")):
        if source.suffix not in TEST_SUFFIXES or not source.is_file():
            continue
        text = strip_comments(source.read_text(encoding="utf-8"))
        names.update(IDENTIFIER.findall(STRING_LITERAL.sub('""', text)))
    return names


def read_baseline() -> list[str]:
    if not BASELINE.exists():
        return []
    names = []
    for line in BASELINE.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            names.append(line)
    return names


def write_baseline(names: list[str]) -> None:
    header = [
        line
        for line in BASELINE.read_text(encoding="utf-8").splitlines()
        if line.startswith("#")
    ]
    BASELINE.write_text("\n".join([*header, *names]) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    parser.add_argument(
        "--prune",
        action="store_true",
        help="remove called and unexported names from the baseline",
    )
    args = parser.parse_args()

    exported = exported_functions()
    directory = test_dir()
    called = called_names(directory)
    baseline = read_baseline()
    listed = set(baseline)

    uncalled = sorted(name for name in exported if name not in called)
    unlisted = [name for name in uncalled if name not in listed]
    stale = [name for name in dict.fromkeys(baseline) if name in called]
    removed = [name for name in dict.fromkeys(baseline) if name not in exported]
    duplicates = sorted({name for name in baseline if baseline.count(name) > 1})

    adapter = sum(1 for name in uncalled if exported[name] == ADAPTER_HEADER)
    total_adapter = sum(1 for header in exported.values() if header == ADAPTER_HEADER)
    print(
        f"{len(uncalled) - adapter} of {len(exported) - total_adapter} exported "
        f"non-adapter functions and {adapter} of {total_adapter} adapter "
        f"functions are uncalled in {directory.relative_to(ROOT).as_posix()}."
    )

    if args.prune:
        keep = sorted(set(uncalled) & listed)
        write_baseline(keep)
        print(f"Kept {len(keep)} names in {BASELINE.relative_to(ROOT).as_posix()}.")
        for name in unlisted:
            print(f"error: no C test calls {name}", file=sys.stderr)
        return 1 if unlisted else 0

    baseline_path = BASELINE.relative_to(ROOT).as_posix()
    problems = [
        *(
            f"no C test calls {name} ({exported[name]}); call it from a test"
            for name in unlisted
        ),
        *(f"{name} is called now; remove it from {baseline_path}" for name in stale),
        *(
            f"{name} is not exported; remove it from {baseline_path}"
            for name in removed
        ),
        *(f"{name} is listed twice in {baseline_path}" for name in duplicates),
    ]
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if stale or removed or duplicates:
        print(
            "Run `mise run check-export-calls --prune` to drop those names.",
            file=sys.stderr,
        )
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
