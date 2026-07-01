"""Native extension loader setup."""

from __future__ import annotations

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
    install_runtime_dir = Path(os.environ["MLN_FFI_NATIVE_INSTALL_DIR"]) / "bin"
    host_library_dirs = [
        Path(directory)
        for directory in os.environ["MLN_FFI_HOST_LIBRARY_DIRS"].split(os.pathsep)
        if directory
    ]
    return [package_dir, install_runtime_dir, *host_library_dirs]


configure_native_loader()
