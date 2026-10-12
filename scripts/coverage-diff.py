#!/usr/bin/env python3
"""List the src/ lines that one coverage report hits and another misses.

Each report is a suite name, which reads build/coverage/<suite>/lcov.info as
`mise run coverage <suite>` writes it, or a path to an lcov file. The output
groups the lines by file and by the function that contains them:

    mise run coverage-diff --only-in rust --not-in native

The exit status is 0 when no line differs and 1 when some do, as with diff(1).
"""

from __future__ import annotations

import argparse
import bisect
import pathlib
import shutil
import subprocess
import sys
from dataclasses import dataclass, field

ROOT = pathlib.Path(__file__).resolve().parents[1]


@dataclass
class SourceFile:
    # Line number to execution count, for every line with code.
    lines: dict[int, int] = field(default_factory=dict)
    # Function start lines, sorted, and the name that starts on each.
    starts: list[int] = field(default_factory=list)
    names: dict[int, str] = field(default_factory=dict)

    def function_at(self, line: int) -> str | None:
        index = bisect.bisect_right(self.starts, line) - 1
        return self.names[self.starts[index]] if index >= 0 else None


def report_path(name: str) -> pathlib.Path:
    path = pathlib.Path(name)
    if path.is_file():
        return path
    suite = ROOT / "build" / "coverage" / name / "lcov.info"
    if suite.is_file():
        return suite
    raise SystemExit(
        f"error: {name} is neither an lcov file nor a suite with a report at "
        f"{suite.relative_to(ROOT).as_posix()}; run `mise run coverage {name}`"
    )


def relative_source(path: str) -> str:
    source = pathlib.Path(path)
    if source.is_absolute():
        try:
            return source.relative_to(ROOT).as_posix()
        except ValueError:
            return source.as_posix()
    return source.as_posix()


def read_lcov(path: pathlib.Path) -> dict[str, SourceFile]:
    files: dict[str, SourceFile] = {}
    current: SourceFile | None = None
    for line in path.read_text(encoding="utf-8").splitlines():
        record, _, value = line.partition(":")
        if record == "SF":
            current = files.setdefault(relative_source(value), SourceFile())
        elif current is None:
            continue
        elif record == "FN":
            # FN:<start line>,<name>, or FN:<start>,<end>,<name> in newer lcov.
            parts = value.split(",")
            start, name = int(parts[0]), parts[-1]
            if start not in current.names:
                bisect.insort(current.starts, start)
            current.names.setdefault(start, name)
        elif record == "DA":
            number, count = value.split(",")[:2]
            current.lines[int(number)] = current.lines.get(int(number), 0) + int(count)
        elif record == "end_of_record":
            current = None
    return files


def demanglers() -> list[list[str]]:
    candidates = []
    if sys.platform == "darwin":
        candidates.append(["xcrun", "llvm-cxxfilt"])
    for name in ("llvm-cxxfilt", "c++filt"):
        path = shutil.which(name)
        if path is not None:
            candidates.append([path])
    # Keep the leading underscore: lcov names are already Itanium names.
    return [[*command, "-n"] for command in candidates]


def demangle(names: set[str]) -> dict[str, str]:
    ordered = sorted(names)
    # A function with internal linkage carries its file as a prefix, which the
    # file heading already shows.
    symbols = [name.rpartition(":")[2] for name in ordered]
    for command in demanglers() if ordered else []:
        try:
            result = subprocess.run(
                command,
                input="\n".join(symbols) + "\n",
                capture_output=True,
                text=True,
                check=False,
            )
        except OSError:
            continue
        readable = result.stdout.splitlines()
        if result.returncode == 0 and len(readable) == len(ordered):
            return dict(zip(ordered, readable, strict=True))
    return dict(zip(ordered, symbols, strict=True))


def ranges(numbers: list[int], instrumented: list[int]) -> list[tuple[int, int]]:
    """Merges lines that no other instrumented line separates."""
    position = {line: index for index, line in enumerate(instrumented)}
    merged: list[tuple[int, int]] = []
    for number in numbers:
        if merged and position[number] == position[merged[-1][1]] + 1:
            merged[-1] = (merged[-1][0], number)
        else:
            merged.append((number, number))
    return merged


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n\n", 1)[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("\n\n", 1)[1],
    )
    parser.add_argument("--only-in", required=True, metavar="REPORT")
    parser.add_argument("--not-in", required=True, metavar="REPORT")
    args = parser.parse_args()

    only = read_lcov(report_path(args.only_in))
    other = read_lcov(report_path(args.not_in))

    found: dict[str, dict[str | None, list[int]]] = {}
    for source, data in sorted(only.items()):
        if not source.startswith("src/"):
            continue
        missed = other.get(source, SourceFile()).lines
        for number in sorted(data.lines):
            if data.lines[number] > 0 and missed.get(number, 0) == 0:
                function = data.function_at(number)
                found.setdefault(source, {}).setdefault(function, []).append(number)

    if not found:
        print(f"No src/ line runs in {args.only_in} without running in {args.not_in}.")
        return 0

    readable = demangle(
        {name for functions in found.values() for name in functions if name}
    )
    total = 0
    for source, functions in found.items():
        print(source)
        instrumented = sorted(only[source].lines)
        for function, numbers in functions.items():
            total += len(numbers)
            spans = ", ".join(
                str(start) if start == end else f"{start}-{end}"
                for start, end in ranges(numbers, instrumented)
            )
            label = readable.get(function, function) if function else "(file scope)"
            print(f"  {label}: {spans}")
    print(
        f"\n{total} src/ lines in {len(found)} files run in {args.only_in} "
        f"and not in {args.not_in}."
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
