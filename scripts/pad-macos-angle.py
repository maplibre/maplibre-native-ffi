#!/usr/bin/env python3
"""Reserves load-command space in the prebuilt macOS ANGLE libraries.

Dart's asset bundler replaces install names with absolute paths. The upstream
binaries have only a few spare bytes after their load commands. LIEF shifts the
payload by one arm64 page and updates Mach-O offsets, relocations, and exports.
CMake rewrites the install names and signs the modified libraries afterwards.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import lief


def pad_library(path: Path) -> None:
    parsed = lief.MachO.parse(str(path))
    if parsed is None or len(parsed) != 1:
        raise ValueError(f"Expected one Mach-O architecture in {path}")
    binary = parsed.at(0)
    exports = {symbol.name for symbol in binary.exported_symbols}
    dependencies = [library.name for library in binary.libraries]
    result = binary.shift(0x4000)
    if isinstance(result, lief.lief_errors):
        raise RuntimeError(  # noqa: TRY004 - LIEF returns an error enum.
            f"Could not reserve load-command space in {path}: {result}"
        )
    binary.write(str(path))

    updated = lief.MachO.parse(str(path)).at(0)
    valid, diagnostic = lief.MachO.check_layout(updated)
    if not valid:
        raise ValueError(f"Invalid Mach-O layout in {path}: {diagnostic}")
    if {symbol.name for symbol in updated.exported_symbols} != exports:
        raise ValueError(f"Mach-O exports changed while padding {path}")
    if [library.name for library in updated.libraries] != dependencies:
        raise ValueError(f"Mach-O dependencies changed while padding {path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("libraries", nargs="+", type=Path)
    for library in parser.parse_args().libraries:
        pad_library(library)
