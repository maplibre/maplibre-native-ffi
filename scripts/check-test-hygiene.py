#!/usr/bin/env python3
"""Fail on sleeps and network hosts in test code beyond the recorded baseline.

A test that sleeps decides its outcome by elapsed time, and a test that names a
public or reserved host depends on the network or on DNS. Both rules are errors.

`scripts/test-hygiene-baseline.toml` records the violations that predate these
rules, as a count per rule and file. A file absent from the baseline allows no
violations, and a count above its baseline fails. A count below its baseline
also fails until `--update` lowers the baseline, so the baseline only shrinks.
`--update` never raises a count or adds a file. When a file moves, rename its
key by hand.

A host in an input that the test only rewrites or rejects, and never fetches, is
allowed when the line or the line above it carries the marker `lint: not-fetched`.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
import tomllib

ROOT = pathlib.Path(__file__).resolve().parents[1]
BASELINE = ROOT / "scripts" / "test-hygiene-baseline.toml"
NOT_FETCHED = "lint: not-fetched"

# A path is test code when a directory or file name marks it as one: `tests/`
# and `test/` trees, Swift `Tests/`, Kotlin `*Test` source sets, Go `_test.go`
# files and test support packages, and test support sources.
TEST_DIRECTORY = re.compile(r"^(?:tests?|Tests|testsupport|[a-z][A-Za-z]*Test)$")
TEST_FILE = re.compile(r"(?:_test\.go|^test_.*\.py|_test\.dart|test_support\.\w+)$")

# Any call whose name contains "sleep" is a sleep, including test helpers
# that wrap one.
SLEEP = re.compile(
    r"\w*[sS]leep\w*\s*\("
    r"|\bsleepMillis\b"
    r"|\bTask\.Delay\b"
    r"|\bFuture(?:<[^>]*>)?\.delayed\b"
)
# Kotlin coroutines sleep through `delay`.
KOTLIN_SLEEP = re.compile(r"\bdelay\s*\(")

# Documentation domains and reserved top-level domains never serve a test, and
# real tile hosts would make a test depend on a public service.
URL_HOST = re.compile(r"\b[a-z][a-z0-9+.-]*://([A-Za-z0-9.-]+)")
BARE_HOST = re.compile(
    r"(?<![@\w-])example\.(?:com|org|net|invalid|test)\b", re.IGNORECASE
)
BLOCKED_HOST = re.compile(
    r"(?:^|\.)(?:"
    r"example\.(?:com|org|net)|example|invalid|test"
    r"|demotiles\.maplibre\.org|tile\.openstreetmap\.org|tiles\.openfreemap\.org"
    r"|api\.maptiler\.com|api\.mapbox\.com|basemaps\.cartocdn\.com"
    r"|tiles\.stadiamaps\.com|api\.protomaps\.com"
    r")$",
    re.IGNORECASE,
)
RUST_TEST_ATTRIBUTE = re.compile(r"#\[cfg\((?:all\()?test\b[^\]]*\]\s*")

RULES = ("sleep", "network-host")


def is_test_path(path: str) -> bool:
    *directories, name = path.split("/")
    return any(TEST_DIRECTORY.match(part) for part in directories) or bool(
        TEST_FILE.search(name)
    )


def block_end(lines: list[str], start: int) -> int:
    """Index past the item at `start`, by counting braces from its first line."""
    depth = 0
    for index in range(start, len(lines)):
        line = lines[index]
        if depth == 0 and ";" in line and "{" not in line.split(";", 1)[0]:
            return index + 1
        depth += line.count("{") - line.count("}")
        if depth <= 0 and "{" in "".join(lines[start : index + 1]):
            return index + 1
    return len(lines)


def test_lines(path: str, lines: list[str]) -> list[int]:
    """The indices of the lines in a file that belong to test code."""
    if is_test_path(path):
        return list(range(len(lines)))
    selected: list[int] = []
    index = 0
    while index < len(lines):
        stripped = lines[index].strip()
        start = None
        attribute = RUST_TEST_ATTRIBUTE.match(stripped)
        if path.endswith(".rs") and attribute:
            # The gated item follows the attribute on its line or below any
            # further attributes.
            start = index if stripped[attribute.end() :] else index + 1
            while start < len(lines) and lines[start].strip().startswith("#["):
                start += 1
        elif path.endswith(".zig") and re.match(r"test\b", lines[index]):
            start = index
        if start is None or start >= len(lines):
            index += 1
            continue
        end = block_end(lines, start)
        selected.extend(range(index, end))
        index = end
    return selected


def violations(path: str, lines: list[str]) -> dict[str, int]:
    counts = dict.fromkeys(RULES, 0)
    for index in test_lines(path, lines):
        line = lines[index]
        if SLEEP.search(line) or (path.endswith(".kt") and KOTLIN_SLEEP.search(line)):
            counts["sleep"] += 1
        if NOT_FETCHED in line or (index > 0 and NOT_FETCHED in lines[index - 1]):
            continue
        if BARE_HOST.search(line) or any(
            BLOCKED_HOST.search(host.rstrip(".")) for host in URL_HOST.findall(line)
        ):
            counts["network-host"] += 1
    return counts


def scan() -> dict[str, dict[str, int]]:
    files = subprocess.run(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
        cwd=ROOT,
        capture_output=True,
        check=True,
        text=True,
    ).stdout.split("\0")
    result: dict[str, dict[str, int]] = {rule: {} for rule in RULES}
    for path in files:
        if not path or path.startswith("third_party/"):
            continue
        if not (is_test_path(path) or path.endswith((".rs", ".zig"))):
            continue
        try:
            text = (ROOT / path).read_text(encoding="utf-8")
        except FileNotFoundError, IsADirectoryError, UnicodeDecodeError:
            continue
        for rule, count in violations(path, text.splitlines()).items():
            if count:
                result[rule][path] = count
    return result


def write_baseline(baseline: dict[str, dict[str, int]]) -> None:
    lines = [
        "# Violations of scripts/check-test-hygiene.py that predate its rules.",
        "# Counts only fall. Run the script with --update after removing some.",
    ]
    for rule in RULES:
        lines.append("")
        lines.append(f"[{rule}]")
        for path, count in sorted(baseline.get(rule, {}).items()):
            lines.append(f'"{path}" = {count}')
    BASELINE.write_text("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--update", action="store_true", help="lower baseline counts to the current"
    )
    args = parser.parse_args()
    with BASELINE.open("rb") as file:
        baseline = tomllib.load(file)
    current = scan()
    errors = []
    lowered: dict[str, dict[str, int]] = {}
    for rule in RULES:
        allowed = baseline.get(rule, {})
        found = current[rule]
        lowered[rule] = {
            path: min(count, found.get(path, 0))
            for path, count in allowed.items()
            if found.get(path, 0)
        }
        for path in sorted(allowed.keys() | found.keys()):
            have, limit = found.get(path, 0), allowed.get(path, 0)
            if have > limit:
                errors.append(f"{path}: {have} {rule} violations, baseline {limit}")
            elif have < limit and not args.update:
                errors.append(
                    f"{path}: {have} {rule} violations, baseline {limit}; "
                    "run scripts/check-test-hygiene.py --update"
                )
    if args.update:
        write_baseline(lowered)
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    if errors:
        print(
            "Tests wait on signals rather than sleeping, and serve every request "
            "from a local fixture.",
            file=sys.stderr,
        )
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
