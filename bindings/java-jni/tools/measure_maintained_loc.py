#!/usr/bin/env python3
"""Measure project-maintained Java JNI bridge code.

The maintained-code count intentionally excludes generated JavaCPP declarations
and build output. It includes handwritten Java JNI internal adapters/support,
binding-local build glue, and historical native bridge paths when they exist in
the measured revision.
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
from collections.abc import Iterable

ROOT = pathlib.Path(__file__).resolve().parents[3]

JAVA_JNI = "bindings/java-jni"
HISTORICAL_NATIVE = JAVA_JNI + "/" + "native"

INCLUDE_PREFIXES = (
    f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/",
    f"{JAVA_JNI}/src/main/java/module-info.java",
    f"{HISTORICAL_NATIVE}/Cargo.toml",
    f"{HISTORICAL_NATIVE}/build.rs",
    f"{HISTORICAL_NATIVE}/src/",
    f"{JAVA_JNI}/build.gradle.kts",
    f"{JAVA_JNI}/mise.toml",
)

EXCLUDE_PREFIXES = (
    f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeC.java",
    f"{HISTORICAL_NATIVE}/vendor/",
    f"{JAVA_JNI}/build/",
)

TEXT_SUFFIXES = {".java", ".rs", ".toml", ".kts", ".md", ".h"}
COUNTED_SUFFIXES = {".java", ".rs", ".toml", ".kts"}
ROOT_BUILD_METADATA = (
    "Cargo.toml",
    "Cargo.lock",
    "settings.gradle.kts",
    "mise.toml",
    "hk.pkl",
)

CATEGORY_PATHS = {
    "checked_in_generated_javacpp": [
        f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeC.java"
    ],
    "generated_at_build_javacpp": [
        f"{JAVA_JNI}/build/generated/sources/javacpp/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeC.java"
    ],
    "javacpp_preset_config_support": [
        f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/javacpp"
    ],
    "handwritten_internal_bridge": [
        f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/bridge"
    ],
    "other_internal_support": [
        f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal"
    ],
    "public_api": [f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni"],
    "tests": [f"{JAVA_JNI}/src/test"],
    "build_docs_tools": [
        f"{JAVA_JNI}/build.gradle.kts",
        f"{JAVA_JNI}/mise.toml",
        f"{JAVA_JNI}/tools",
        "docs/src/content/docs/development/bindings-java-jni.md",
    ],
    "deleted_prior_stack_files": [
        f"{HISTORICAL_NATIVE}/Cargo.toml",
        f"{HISTORICAL_NATIVE}/build.rs",
        f"{HISTORICAL_NATIVE}/src",
        f"{HISTORICAL_NATIVE}/JAVA_BINDGEN.md",
        f"{HISTORICAL_NATIVE}/JAVA_BINDGEN_SPIKE.md",
        f"{HISTORICAL_NATIVE}/UNIFFI_JNI_DECISION.md",
        f"{JAVA_JNI}/JAVACPP_SPIKE.md",
    ],
}


class DiffTotals:
    def __init__(self) -> None:
        self.added = 0
        self.deleted = 0
        self.files = 0


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--baseline",
        default="1d53075",
        help="git revision to compare against (default: %(default)s)",
    )
    parser.add_argument(
        "--inventory",
        action="store_true",
        help="also print a current Java JNI LoC inventory by category",
    )
    parser.add_argument(
        "--diff-base",
        default="main",
        help="revision used for the inventory diff size (default: %(default)s)",
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
    if args.inventory:
        print_inventory(args.diff_base)


def count_worktree() -> int:
    files = [path for path in ROOT.rglob("bindings/java-jni/**") if path.is_file()]
    total = 0
    for path in selected(path.relative_to(ROOT) for path in files):
        total += count_text((ROOT / path).read_text(encoding="utf-8"))
    return total


def print_inventory(diff_base: str) -> None:
    print("inventory_loc:")
    for name in CATEGORY_PATHS:
        print(f"  {name}={count_category(name)}")
    print(f"inventory_diff_base={diff_base}")
    print("inventory_diff:")
    for name, totals in diff_inventory(diff_base).items():
        print(
            f"  {name}=files:{totals.files},added:{totals.added},deleted:{totals.deleted}"
        )


def count_category(name: str) -> int:
    files: set[pathlib.Path] = set()
    for path in CATEGORY_PATHS[name]:
        candidate = ROOT / path
        if candidate.is_file():
            files.add(candidate)
        elif candidate.exists():
            files.update(file for file in candidate.rglob("*") if file.is_file())
    files = filter_category_files(name, files)
    return sum(
        count_text(file.read_text(encoding="utf-8"))
        for file in files
        if is_counted(file)
    )


def diff_inventory(diff_base: str) -> dict[str, DiffTotals]:
    totals = {name: DiffTotals() for name in CATEGORY_PATHS}
    for added, deleted, path in diff_numstat(diff_base):
        category = category_for_path(path)
        if category is None:
            continue
        totals[category].files += 1
        totals[category].added += added
        totals[category].deleted += deleted
    return totals


def diff_numstat(diff_base: str) -> list[tuple[int, int, str]]:
    output = git(
        "diff",
        "--numstat",
        f"{diff_base}...HEAD",
        "--",
        JAVA_JNI,
        "docs/src/content/docs/development/bindings-java-jni.md",
        *ROOT_BUILD_METADATA,
    )
    rows = []
    for line in output.splitlines():
        added, deleted, path = line.split("\t", 2)
        if added == "-" or deleted == "-":
            continue
        rows.append((int(added), int(deleted), path))
    return rows


def category_for_path(path: str) -> str | None:
    if (
        path.startswith(f"{HISTORICAL_NATIVE}/")
        or path == f"{JAVA_JNI}/JAVACPP_SPIKE.md"
    ):
        return "deleted_prior_stack_files"
    if path == CATEGORY_PATHS["checked_in_generated_javacpp"][0]:
        return "checked_in_generated_javacpp"
    if path.startswith(f"{JAVA_JNI}/build/generated/sources/javacpp/"):
        return "generated_at_build_javacpp"
    if path.startswith(
        f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/javacpp/"
    ):
        return "javacpp_preset_config_support"
    if path.startswith(
        f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/bridge/"
    ):
        return "handwritten_internal_bridge"
    if path.startswith(f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/"):
        return "other_internal_support"
    if path.startswith(f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/"):
        return "public_api"
    if path.startswith(f"{JAVA_JNI}/src/test/"):
        return "tests"
    if path in {
        f"{JAVA_JNI}/build.gradle.kts",
        f"{JAVA_JNI}/mise.toml",
        "docs/src/content/docs/development/bindings-java-jni.md",
        *ROOT_BUILD_METADATA,
    } or path.startswith(f"{JAVA_JNI}/tools/"):
        return "build_docs_tools"
    return None


def filter_category_files(name: str, files: set[pathlib.Path]) -> set[pathlib.Path]:
    if name == "javacpp_preset_config_support":
        return {
            file
            for file in files
            if file.as_posix()
            != str(
                ROOT
                / f"{JAVA_JNI}/src/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeC.java"
            )
        }
    if name == "other_internal_support":
        return {
            file
            for file in files
            if "/bridge/" not in file.as_posix() and "/javacpp/" not in file.as_posix()
        }
    if name == "public_api":
        return {file for file in files if "/internal/" not in file.as_posix()}
    return files


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
        if pathlib.PurePosixPath(normalized).suffix not in COUNTED_SUFFIXES:
            continue
        if not normalized.startswith(INCLUDE_PREFIXES):
            continue
        if normalized.startswith(EXCLUDE_PREFIXES):
            continue
        result.append(path)
    return result


def is_counted(path: pathlib.Path) -> bool:
    return path.suffix in TEXT_SUFFIXES


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
