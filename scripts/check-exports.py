#!/usr/bin/env python3
"""Check that an installed native library exports exactly the public C API.

The shared library's dynamic symbol table has to hold every function that a
public header declares with MLN_API, plus the MapLibre Native plugin entry
point the plugin header documents, and nothing else. No exported name may
belong to the test seams: a name containing `mln_test`, `testing`, or
`sync_point` fails the check even if a header declared it.

A preset without shared libraries installs only the static archive, where
hidden symbols stay visible to a static link. There the check covers the
archive's C symbols with the `mln_` prefix, which are the ones a host links
against by name.

The library is read from `build/<preset>/install`, so run this after
`mise run build <preset>`. The symbols come from `nm` on macOS and ELF
targets, `llvm-nm` for Emscripten archives, and `llvm-readobj`, or `dumpbin`
when it is missing, on Windows.
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
LIBRARY_STEM = "maplibre-native-c"
# Upstream defines the plugin registration function, and the library exports
# it for include/maplibre_native_c/plugin.h; see cmake/mln_ffi_c_api.cmake.
EXTRA_EXPORTS = frozenset({"mln_plugin_register_v1"})
FORBIDDEN = re.compile(r"mln_test|testing|sync_point", re.IGNORECASE)
PE_EXPORT = re.compile(r"^\s*Name: (?P<name>\S+)$")
DUMPBIN_EXPORT = re.compile(r"^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(?P<name>\S+)")


def declared_functions() -> set[str]:
    """The MLN_API functions, parsed the way check-export-calls parses them."""
    spec = importlib.util.spec_from_file_location(
        "check_export_calls", ROOT / "scripts" / "check-export-calls.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return set(module.exported_functions())


def run(command: list[str]) -> str:
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise SystemExit(
            f"error: {command[0]} failed for {command[-1]}:\n{result.stderr.strip()}"
        )
    return result.stdout


def find_library(install: pathlib.Path) -> tuple[pathlib.Path, bool]:
    """The installed library, and whether it is shared."""
    shared = [
        path
        for pattern in (
            f"lib/lib{LIBRARY_STEM}.dylib",
            f"lib/lib{LIBRARY_STEM}.so",
            f"bin/{LIBRARY_STEM}.dll",
        )
        for path in install.glob(pattern)
    ]
    if shared:
        return shared[0], True
    static = sorted(install.glob(f"lib/lib{LIBRARY_STEM}.a"))
    if static:
        return static[0], False
    raise SystemExit(
        f"error: no {LIBRARY_STEM} library under {install}; build the preset first"
    )


def nm_tool(preset: str) -> str:
    if preset.startswith("emscripten-"):
        emsdk = os.environ.get("EMSDK")
        if emsdk:
            candidate = pathlib.Path(emsdk) / "upstream" / "bin" / "llvm-nm"
            if candidate.exists():
                return str(candidate)
        return "llvm-nm"
    return "nm"


def nm_symbols(tool: str, library: pathlib.Path, dynamic: bool) -> set[str]:
    command = [tool, "-g", "--defined-only"]
    if dynamic:
        command.append("-D")
    names = set()
    for line in run([*command, str(library)]).splitlines():
        fields = line.split()
        # An archive lists each member as `member.o:` before its symbols.
        if len(fields) < 2 or line.endswith(":"):
            continue
        names.add(fields[-1])
    return names


def pe_symbols(library: pathlib.Path) -> set[str]:
    if shutil.which("llvm-readobj"):
        output = run(["llvm-readobj", "--coff-exports", str(library)])
        pattern = PE_EXPORT
    else:
        output = run(["dumpbin", "/nologo", "/exports", str(library)])
        pattern = DUMPBIN_EXPORT
    return {
        match.group("name")
        for line in output.splitlines()
        if (match := pattern.match(line))
    }


def library_symbols(preset: str, library: pathlib.Path, shared: bool) -> set[str]:
    if library.suffix == ".dll":
        return pe_symbols(library)
    macho = library.suffix == ".dylib" or (
        not shared and preset.startswith(("macos-", "ios-", "tvos-", "maccatalyst-"))
    )
    names = nm_symbols(nm_tool(preset), library, dynamic=library.suffix == ".so")
    if macho:
        # Mach-O prefixes every C symbol with an underscore.
        names = {name.removeprefix("_") for name in names}
    return names


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("preset", help="native CMake preset")
    args = parser.parse_args()
    install = ROOT / "build" / args.preset / "install"
    library, shared = find_library(install)
    exported = library_symbols(args.preset, library, shared)
    if not shared:
        # C names only: mangled C++ names start with an underscore and a
        # capital, or with `?` on Windows.
        exported = {name for name in exported if name.startswith("mln_")}
    expected = declared_functions() | EXTRA_EXPORTS

    errors = []
    for name in sorted(exported - expected):
        errors.append(f"exports {name}, which no public header declares")
    for name in sorted(expected - exported):
        errors.append(f"does not export {name}, which a public header declares")
    for name in sorted(exported):
        if FORBIDDEN.search(name):
            errors.append(f"exports {name}, which names a test seam")
    relative = library.relative_to(ROOT)
    for error in errors:
        print(f"error: {relative} {error}", file=sys.stderr)
    if errors:
        return 1
    kind = "exports" if shared else "defines"
    print(f"{relative} {kind} the {len(exported)} public C API symbols.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
