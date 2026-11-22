#!/usr/bin/env python3
"""Check that an installed native library exports exactly the public C API.

The shared library's dynamic symbol table has to hold every function that a
public header declares with MLN_API, and nothing else. No exported name may
belong to the test seams: a name containing `mln_test`, `testing`, or
`sync_point` fails the check even if a header declared it.

A preset without shared libraries installs only the static archive, where
hidden symbols stay visible to a static link. There the check covers the
archive's C symbols with the `mln_` prefix, which are the ones a host links
against by name. A web preset installs a WebAssembly module, whose exports
with the `mln_` prefix are the C API that a JavaScript host calls; its other
exports are wasm-bindgen's.

The library is read from `build/<preset>/install`, so run this after
`mise run build <preset>`. The symbols come from `nm` on macOS and ELF
targets, `llvm-readobj`, or `dumpbin` when it is missing, on Windows, and the
module's export section on the web.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
LIBRARY_STEM = "maplibre-native-c"
FORBIDDEN = re.compile(r"mln_test|testing|sync_point", re.IGNORECASE)
# Symbols a linker can define in a shared library's dynamic table on its own.
# The version script keeps them local with GNU ld, and the check ignores them in
# case another linker exports them anyway.
LINKER_DEFINED = frozenset({"_end", "_edata", "__bss_start", "_init", "_fini"})
PE_EXPORT = re.compile(r"^\s*Name: (?P<name>\S+)$")
DUMPBIN_EXPORT = re.compile(r"^\s+\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(?P<name>\S+)")


BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
LINE_COMMENT = re.compile(r"//[^\n]*")
# The function name is the last identifier before the parameter list. The
# return type may span lines, and nothing between MLN_API and the name holds a
# parenthesis, brace, or semicolon.
DECLARATION = re.compile(r"\bMLN_API\b[^;{}()]*?\b(mln_\w+)\s*\(")
API_MARKER = re.compile(r"\bMLN_API\b")


def declared_functions() -> set[str]:
    """The functions that a public header declares with MLN_API."""
    functions: set[str] = set()
    for header in sorted((ROOT / "include").rglob("*.h")):
        text = BLOCK_COMMENT.sub(" ", header.read_text(encoding="utf-8"))
        # The MLN_API definitions themselves are preprocessor lines.
        text = "\n".join(
            line
            for line in LINE_COMMENT.sub("", text).splitlines()
            if not line.lstrip().startswith("#")
        )
        names = DECLARATION.findall(text)
        # A declaration that the pattern misses would pass the check unseen, so
        # every MLN_API marker has to belong to a declaration it found.
        markers = len(API_MARKER.findall(text))
        if markers != len(names):
            raise SystemExit(
                f"error: {header.relative_to(ROOT).as_posix()} has {markers} "
                f"MLN_API markers but {len(names)} parsed declarations"
            )
        functions.update(names)
    if not functions:
        raise SystemExit("error: found no MLN_API declarations under include/")
    return functions


def run(command: list[str]) -> str:
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=False)
    except FileNotFoundError:
        raise SystemExit(f"error: {command[0]} is not installed") from None
    if result.returncode != 0:
        raise SystemExit(
            f"error: {command[0]} failed for {command[-1]}:\n{result.stderr.strip()}"
        )
    return result.stdout


WEB_MODULE = f"share/{LIBRARY_STEM}/web/maplibre_native_c_bg.wasm"


def find_library(install: pathlib.Path) -> tuple[pathlib.Path, bool]:
    """The installed library, and whether it is shared."""
    module = install / WEB_MODULE
    if module.is_file():
        return module, True
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


def leb128(data: bytes, offset: int) -> tuple[int, int]:
    """The unsigned LEB128 number at `offset`, and the offset after it."""
    value = shift = 0
    while True:
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if not byte & 0x80:
            return value, offset


def wasm_function_exports(module: pathlib.Path) -> set[str]:
    """The names of the functions that a WebAssembly module exports."""
    data = module.read_bytes()
    if data[:4] != b"\0asm":
        raise SystemExit(f"error: {module} is not a WebAssembly module")
    offset = 8
    while offset < len(data):
        section, offset = data[offset], offset + 1
        size, offset = leb128(data, offset)
        end = offset + size
        if section == 7:
            count, offset = leb128(data, offset)
            names = set()
            for _ in range(count):
                length, offset = leb128(data, offset)
                name = data[offset : offset + length].decode()
                kind = data[offset + length]
                _, offset = leb128(data, offset + length + 1)
                if kind == 0:
                    names.add(name)
            return names
        offset = end
    return set()


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
        # A versioned ELF symbol prints as `name@@VERSION`.
        names.add(fields[-1].split("@", 1)[0])
    if dynamic:
        names -= LINKER_DEFINED
    return names


def readobj_tool() -> str | None:
    found = shutil.which("llvm-readobj")
    if found:
        return found
    # The Windows presets compile with clang-cl from the LLVM installer, which
    # does not always put its tools on PATH.
    program_files = os.environ.get("ProgramFiles")
    if program_files:
        candidate = pathlib.Path(program_files) / "LLVM" / "bin" / "llvm-readobj.exe"
        if candidate.exists():
            return str(candidate)
    return None


def pe_symbols(library: pathlib.Path) -> set[str]:
    readobj = readobj_tool()
    if readobj:
        output = run([readobj, "--coff-exports", str(library)])
        pattern = PE_EXPORT
    elif shutil.which("dumpbin"):
        output = run(["dumpbin", "/nologo", "/exports", str(library)])
        pattern = DUMPBIN_EXPORT
    else:
        raise SystemExit(
            "error: reading a DLL's exports needs llvm-readobj, on PATH or in "
            "%ProgramFiles%\\LLVM\\bin, or dumpbin from a Visual Studio shell"
        )
    return {
        match.group("name")
        for line in output.splitlines()
        if (match := pattern.match(line))
    }


def library_symbols(preset: str, library: pathlib.Path, shared: bool) -> set[str]:
    if library.suffix == ".wasm":
        return {
            name for name in wasm_function_exports(library) if name.startswith("mln_")
        }
    if library.suffix == ".dll":
        return pe_symbols(library)
    macho = library.suffix == ".dylib" or (
        not shared
        and preset.startswith(("macos-", "ios-", "tvos-", "visionos-", "maccatalyst-"))
    )
    names = nm_symbols("nm", library, dynamic=library.suffix == ".so")
    if macho:
        # Mach-O prefixes every C symbol with an underscore.
        names = {name.removeprefix("_") for name in names}
    return names


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("preset", help="native CMake preset")
    args = parser.parse_args()
    build = ROOT / "build" / args.preset
    install = build / "install"
    library, shared = find_library(install)
    exported = library_symbols(args.preset, library, shared)
    if not shared:
        # C names only: mangled C++ names start with an underscore and a
        # capital, or with `?` on Windows.
        exported = {name for name in exported if name.startswith("mln_")}
    expected = declared_functions()

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
