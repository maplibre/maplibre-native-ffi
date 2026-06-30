"""Native extension loader setup."""

from __future__ import annotations

import json
import os
from pathlib import Path
import sys
from typing import Any

_DLL_DIRECTORY_HANDLES: list[Any] = []


def configure_native_loader() -> None:
    """Register native dependency directories before importing the extension."""
    if sys.platform != "win32":
        return

    add_dll_directory = getattr(os, "add_dll_directory", None)
    if add_dll_directory is None:
        return

    for directory in _native_loader_dirs():
        try:
            handle = add_dll_directory(str(directory))
        except OSError:
            continue
        _DLL_DIRECTORY_HANDLES.append(handle)


def _native_loader_dirs() -> list[Path]:
    result: list[Path] = []
    seen: set[str] = set()
    for directory in _candidate_loader_dirs():
        if not directory.is_dir():
            continue
        resolved = str(directory.resolve())
        if resolved in seen:
            continue
        result.append(directory)
        seen.add(resolved)
    return result


def _candidate_loader_dirs() -> list[Path]:
    package_dir = Path(__file__).resolve().parent
    result = [package_dir]

    build_dir = os.environ.get("MLN_FFI_BUILD_DIR")
    if build_dir:
        build_path = Path(build_dir)
        result.append(build_path)
        result.extend(_metadata_loader_dirs(build_path / "maplibre-native-c.dev.json"))

    dependency_library_dir = os.environ.get("MLN_FFI_DEPENDENCY_LIBRARY_DIR")
    if dependency_library_dir:
        result.extend(_library_dir_loader_dirs(Path(dependency_library_dir)))

    return result


def _metadata_loader_dirs(metadata_path: Path) -> list[Path]:
    try:
        with metadata_path.open(encoding="utf-8") as metadata_file:
            metadata = json.load(metadata_file)
    except OSError, json.JSONDecodeError:
        return []

    result = []
    library_path = metadata.get("library_path")
    if isinstance(library_path, str) and library_path:
        result.append(Path(library_path).parent)

    for key in ("library_dirs", "rpaths"):
        values = metadata.get(key, [])
        if not isinstance(values, list):
            continue
        for value in values:
            if isinstance(value, str) and value:
                result.extend(_library_dir_loader_dirs(Path(value)))

    return result


def _library_dir_loader_dirs(library_dir: Path) -> list[Path]:
    return [library_dir, library_dir.parent / "bin"]


configure_native_loader()
