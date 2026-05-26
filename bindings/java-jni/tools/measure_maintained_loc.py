#!/usr/bin/env python3
"""Measure project-maintained Java JNI bridge code.

The count intentionally excludes generated JavaCPP declarations and build output.
It includes handwritten Java JNI internal adapters/support, the workspace marker
crate, and binding-local build glue.
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
from collections.abc import Iterable

ROOT = pathlib.Path(__file__).resolve().parents[3]

INCLUDE_PREFIXES = (
    "bindings/java-jni/src/main/java/org/maplibre/nativejni/internal/",
    "bindings/java-jni/src/main/java/module-info.java",
    "bindings/java-jni/native/Cargo.toml",
    "bindings/java-jni/native/build.rs",
    "bindings/java-jni/native/src/",
    "bindings/java-jni/build.gradle.kts",
    "bindings/java-jni/mise.toml",
)

EXCLUDE_PREFIXES = (
    "bindings/java-jni/src/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeC.java",
    "bindings/java-jni/native/vendor/",
    "bindings/java-jni/build/",
)

TEXT_SUFFIXES = {".java", ".rs", ".toml", ".kts"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--baseline",
        default="1d53075",
        help="git revision to compare against (default: %(default)s)",
    )
    args = parser.parse_args()

    baseline = count_revision(args.baseline)
    current = count_worktree()
    reduction = baseline - current
    percent = reduction / baseline * 100 if baseline else 0

    print(f"baseline_revision={args.baseline}")
    print(f"baseline_loc={baseline}")
    print(f"current_loc={current}")
    print(f"reduction_loc={reduction}")
    print(f"reduction_percent={percent:.1f}")


def count_worktree() -> int:
    files = [path for path in ROOT.rglob("bindings/java-jni/**") if path.is_file()]
    total = 0
    for path in selected(path.relative_to(ROOT) for path in files):
        total += count_text((ROOT / path).read_text(encoding="utf-8"))
    return total


def count_revision(revision: str) -> int:
    output = git("ls-tree", "-r", "--name-only", revision, "bindings/java-jni")
    total = 0
    for name in selected(pathlib.PurePosixPath(line) for line in output.splitlines()):
        try:
            text = git("show", f"{revision}:{name.as_posix()}")
        except subprocess.CalledProcessError:
            continue
        total += count_text(text)
    return total


def selected(paths: Iterable[pathlib.PurePath]) -> list[pathlib.PurePath]:
    result = []
    for path in paths:
        normalized = path.as_posix()
        if pathlib.PurePosixPath(normalized).suffix not in TEXT_SUFFIXES:
            continue
        if not normalized.startswith(INCLUDE_PREFIXES):
            continue
        if normalized.startswith(EXCLUDE_PREFIXES):
            continue
        result.append(path)
    return result


def count_text(text: str) -> int:
    total = 0
    in_block_comment = False
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if in_block_comment:
            if "*/" in stripped:
                stripped = stripped.split("*/", 1)[1].strip()
                in_block_comment = False
            else:
                continue
        while "/*" in stripped:
            before, after = stripped.split("/*", 1)
            if "*/" in after:
                after = after.split("*/", 1)[1]
                stripped = (before + after).strip()
            else:
                stripped = before.strip()
                in_block_comment = True
                break
        if not stripped:
            continue
        if stripped.startswith(("//", "#", "*")):
            continue
        total += 1
    return total


def git(*args: str) -> str:
    return subprocess.check_output(("git", *args), cwd=ROOT, text=True)


if __name__ == "__main__":
    main()
