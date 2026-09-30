#!/usr/bin/env python3
"""Measure the src/ line coverage of one test suite.

Runs the C tests (`native`) or one binding's tests against a coverage preset
with LLVM_PROFILE_FILE set, merges the raw profiles, and writes an lcov file and
an HTML report for src/ to build/coverage/<suite>/. Test sources and
third-party code stay out of the report.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BINDINGS = ("dart", "dotnet", "go", "kotlin", "python", "rust", "swift", "zig")
# Test sources under src/, such as the C suite before it moves to tests/native.
IGNORED = r"/tests/"


def tool(name: str) -> str:
    """Finds an LLVM tool that matches the compiler's profile format."""
    if sys.platform == "darwin":
        found = subprocess.run(
            ["xcrun", "--find", name], capture_output=True, text=True, check=False
        )
        if found.returncode == 0:
            return found.stdout.strip()
    path = shutil.which(name)
    if path is None:
        raise SystemExit(f"error: {name} not found; install the LLVM tools")
    return path


def library(preset: str) -> pathlib.Path:
    lib_dir = ROOT / "build" / preset / "install" / "lib"
    for name in ("libmaplibre-native-c.dylib", "libmaplibre-native-c.so"):
        if (lib_dir / name).exists():
            return lib_dir / name
    raise SystemExit(f"error: no shared library under {lib_dir}")


def line_totals(lcov: pathlib.Path) -> tuple[int, int]:
    found = hit = 0
    for line in lcov.read_text(encoding="utf-8").splitlines():
        if line.startswith("LF:"):
            found += int(line[3:])
        elif line.startswith("LH:"):
            hit += int(line[3:])
    return hit, found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    parser.add_argument(
        "suite",
        nargs="?",
        default="native",
        choices=("native", *BINDINGS),
        help="`native` for the C tests, or a binding name",
    )
    parser.add_argument("--preset", default="macos-arm64-metal-coverage")
    args = parser.parse_args()

    output = ROOT / "build" / "coverage" / args.suite
    profiles = output / "profiles"
    shutil.rmtree(output, ignore_errors=True)
    profiles.mkdir(parents=True)

    task = "//:test" if args.suite == "native" else f"//bindings/{args.suite}:test"
    # %p keeps one file per process, and %m one per instrumented binary, so
    # parallel test processes never write the same file.
    env = {**os.environ, "LLVM_PROFILE_FILE": str(profiles / "%m-%p.profraw")}
    print(f"Running {task} {args.preset} with coverage", flush=True)
    tests = subprocess.run(["mise", "run", task, args.preset], env=env, check=False)

    raw = sorted(profiles.glob("*.profraw"))
    if not raw:
        print(
            f"error: {task} wrote no profiles; it may not load the library "
            f"that {args.preset} built",
            file=sys.stderr,
        )
        return 1

    profdata = output / f"{args.suite}.profdata"
    subprocess.run(
        [tool("llvm-profdata"), "merge", "-sparse", *map(str, raw), "-o", profdata],
        check=True,
    )

    # The shared library carries the coverage mapping for every instrumented
    # function. A suite that links the static archive runs the same objects,
    # so its profiles match that mapping too.
    report_args = [
        f"-instr-profile={profdata}",
        f"-compilation-dir={ROOT}",
        f"-ignore-filename-regex={IGNORED}",
        str(library(args.preset)),
        str(ROOT / "src"),
    ]
    lcov = output / "lcov.info"
    with lcov.open("w", encoding="utf-8") as file:
        subprocess.run(
            [tool("llvm-cov"), "export", "-format=lcov", *report_args],
            stdout=file,
            check=True,
        )
    subprocess.run(
        [
            tool("llvm-cov"),
            "show",
            "-format=html",
            f"-output-dir={output / 'html'}",
            "-show-line-counts-or-regions",
            *report_args,
        ],
        check=True,
    )

    hit, found = line_totals(lcov)
    percent = 100 * hit / found if found else 0
    relative = output.relative_to(ROOT).as_posix()
    print(
        f"src/ line coverage for {args.suite}: {hit} of {found} lines ({percent:.1f}%)"
    )
    print(f"lcov: {relative}/lcov.info")
    print(f"HTML: {relative}/html/index.html")
    if tests.returncode != 0:
        print(
            f"error: {task} failed; the report covers the tests that ran",
            file=sys.stderr,
        )
    return tests.returncode


if __name__ == "__main__":
    raise SystemExit(main())
