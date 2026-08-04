#!/usr/bin/env python3
"""Emits the browser dispatch table from the C API's own declarations.

A browser host cannot call this C API on the thread it lives on. Its calls have
to run on the pthread that owns the runtime, because MapLibre blocks --
`waitForEmpty` drains a queue, `Thread<>` makes synchronous cross-thread calls,
and teardown joins -- and the page thread may not block. So the page packs a call
into a buffer and the owner thread performs it.

Every entry point is reached **by name**, not through a cast function pointer.
That is the whole point of generating this: a cast compiles whatever it is given,
so a generator that got an argument wrong would produce a corrupted stack at run
time, while a direct call is checked against the real declaration and a mistake
is a build failure. It also means this file cannot drift from the headers without
the build saying so.

The declared parameter types come from clang's own AST for the pinned Emscripten
target, because the lowered WebAssembly signature cannot supply them: `i32`
covers a pointer, a handle, and an enum alike, and casting a slot to the wrong
one of those is exactly the mistake worth preventing.

Arguments travel as eight-byte slots whatever their width, which is what lets one
buffer layout serve every entry point. A function returning a struct by value
takes the destination as its first slot, matching the hidden out-pointer the
target's ABI gives it anyway.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from browser_abi import headers_digest  # noqa: E402

#: Types that occupy the slot as themselves rather than as an integer.
FLOAT_TYPES = {"double": "f64", "float": "f32"}


def declarations(
    clang: pathlib.Path, sysroot: pathlib.Path, include: pathlib.Path
) -> dict:
    """Returns `{name: (return type, [parameter types])}` for every entry point."""
    source = (
        '#include "maplibre_native_c.h"\n'
        '#include "maplibre_native_c/callback_adapter.h"\n'
    )
    result = subprocess.run(
        [
            str(clang),
            "-target",
            "wasm32-unknown-emscripten",
            f"--sysroot={sysroot}",
            "-I",
            str(include),
            "-Xclang",
            "-ast-dump=json",
            "-fsyntax-only",
            "-std=c23",
            "-x",
            "c",
            "-",
        ],
        input=source,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise SystemExit(f"clang failed reading the headers: {result.stderr}")
    tree = json.loads(result.stdout)
    functions = {}
    for node in tree.get("inner", []):
        if node.get("kind") != "FunctionDecl":
            continue
        name = node.get("name", "")
        if not name.startswith("mln_"):
            continue
        parameters = [
            child["type"]["qualType"]
            for child in node.get("inner", [])
            if child.get("kind") == "ParmVarDecl"
        ]
        # `qualType` for a function is "<return> (<params>)"; the return type is
        # everything before the parameter list.
        returns = node["type"]["qualType"].split("(")[0].strip()
        functions[name] = (returns, parameters)
    if not functions:
        raise SystemExit("clang reported no mln_ declarations")
    return functions


def is_pointer(c_type: str) -> bool:
    return c_type.rstrip().endswith("*")


def struct_of(c_type: str, structs: set[str]) -> str | None:
    """Returns the struct name when [c_type] is one of this API's structs by value."""
    bare = c_type.strip().removeprefix("const ").strip()
    return bare if bare in structs else None


def read_slot(c_type: str, index: int, structs: set[str]) -> str:
    """Reads slot [index] back as [c_type]."""
    bare = c_type.strip()
    if bare in FLOAT_TYPES:
        return f"slots[{index}].{FLOAT_TYPES[bare]}"
    if is_pointer(bare):
        # Through uintptr_t, because a conversion straight from a 64-bit integer
        # to a pointer is not one C performs.
        return f"({bare})(uintptr_t)slots[{index}].u"
    name = struct_of(bare, structs)
    if name is not None:
        # A struct passed by value in C is passed indirectly once lowered, so the
        # slot already holds its address and the wrapper reads through it. Casting
        # the slot to the struct instead would not compile, which is the check
        # this file exists for.
        return f"*(const {name}*)(uintptr_t)slots[{index}].u"
    return f"({bare})slots[{index}].u"


def write_result(c_type: str) -> str:
    bare = c_type.strip()
    if bare in FLOAT_TYPES:
        return f"result->{FLOAT_TYPES[bare]} = value;"
    if is_pointer(bare):
        return "result->u = (uint64_t)(uintptr_t)value;"
    return "result->u = (uint64_t)value;"


def wrapper(name: str, returns: str, parameters: list[str], structs: set[str]) -> str:
    lines = [
        f"static void mln_browser_wrap_{name}(",
        "  const mln_browser_slot* slots, mln_browser_slot* result",
        ") {",
    ]
    arguments = ", ".join(
        read_slot(parameter, index, structs)
        for index, parameter in enumerate(parameters)
    )
    if returns.strip() in structs:
        # Returned by value in C, and by hidden out-pointer once lowered. The
        # destination is the first slot, so the packed call says what the ABI
        # already does rather than leaving it implicit.
        destination = f"({returns.strip()}*)(uintptr_t)slots[0].u"
        shifted = ", ".join(
            read_slot(parameter, index + 1, structs)
            for index, parameter in enumerate(parameters)
        )
        lines.append(f"  *({destination}) = {name}({shifted});")
        lines.append("  result->u = 0;")
    elif returns.strip() == "void":
        lines.append(f"  {name}({arguments});")
        lines.append("  result->u = 0;")
    else:
        lines.append(f"  {returns.strip()} value = {name}({arguments});")
        lines.append(f"  {write_result(returns)}")
    lines.append("  (void)slots;")
    lines.append("}")
    return "\n".join(lines)


PROLOGUE = """// Generated by scripts/generate-browser-dispatch.py from the C API's headers.
// Edit neither; change the generator.
//
// Every entry point is called by name so the compiler checks each argument
// against its real declaration. See the generator for why that matters.

#include "browser/dispatch_table.h"

#include <stdint.h>

#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"

"""

STRUCT_DEFINITION = re.compile(r"^typedef struct (\w+) \{", re.MULTILINE)


def struct_names(include: pathlib.Path) -> set[str]:
    names: set[str] = set()
    for header in sorted(include.rglob("*.h")):
        names.update(STRUCT_DEFINITION.findall(header.read_text()))
    return names


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", type=pathlib.Path, required=True)
    parser.add_argument("--sysroot", type=pathlib.Path, required=True)
    parser.add_argument("--include", type=pathlib.Path, required=True)
    parser.add_argument("output", type=pathlib.Path)
    arguments = parser.parse_args(argv)

    functions = declarations(arguments.clang, arguments.sysroot, arguments.include)
    structs = struct_names(arguments.include)

    parts = [PROLOGUE]
    parts.append(
        "// Digest of the public headers this module was built from, so a host can\n"
        "// ask the module rather than a file beside it.\n"
        "MLN_API const char* mln_browser_headers_digest(void) MLN_NOEXCEPT {\n"
        f'  return "{headers_digest(arguments.include)}";\n'
        "}\n"
    )
    for name in sorted(functions):
        returns, parameters = functions[name]
        parts.append(wrapper(name, returns, parameters, structs))
        parts.append("")

    parts.append("const mln_browser_entry mln_browser_entries[] = {")
    for name in sorted(functions):
        returns, parameters = functions[name]
        # A struct-returning entry reads its destination from slot 0, so it needs
        # one more slot than it has declared parameters.
        slots = len(parameters) + (1 if returns.strip() in structs else 0)
        parts.append(f'  {{ "{name}", {slots}, mln_browser_wrap_{name} }},')
    parts.append("};")
    parts.append("")
    parts.append(
        "const uint32_t mln_browser_entry_count =\n"
        "  (uint32_t)(sizeof(mln_browser_entries) / sizeof(mln_browser_entries[0]));"
    )
    parts.append("")

    text = "\n".join(parts)
    if arguments.output.exists() and arguments.output.read_text() == text:
        return 0
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
