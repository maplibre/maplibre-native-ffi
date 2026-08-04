#!/usr/bin/env python3
"""Writes the browser module's ABI manifest from the linked module itself.

A browser host calls this C API through a foreign-function interface, so it has
to know each entry point's *lowered* WebAssembly signature rather than its C
declaration. The two differ in ways that silently corrupt memory if a binding
guesses:

  * A handle is a 64-bit value, so `mln_runtime_pump` is `(i64, i64) -> i32`
    and reaches JavaScript as a BigInt. A binding that narrows one keeps only
    the generation bits and every later call rejects it.
  * A function returning a struct by value is lowered to one taking a hidden
    out-pointer and returning nothing: `mln_map_options_default` is
    `(i32) -> ()`, not `() -> mln_map_options`. Thirty-one of this API's
    `*_default` constructors are of that shape. A binding generated from the C
    declaration would call them with no argument and let native write through a
    pointer that was never passed.

Reading the shipped module is what makes those exact rather than modelled. The
toolchain has already applied its own ABI by the time this runs, so there is no
second implementation of it here to disagree with the first.

The module is linked with `-lexports.js`, which is emcc's supported way to keep
wasm export names out of the optimizer's minifier. Without it the export table
reads `Qb`, `Rb`, and the signatures cannot be attributed to an entry point.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from browser_abi import headers_digest  # noqa: E402

# The manifest describes the module this project links with its own pinned
# emsdk, so the parser that reads it is the one that emsdk ships.
try:
    from tools import webassembly
    from tools.webassembly import ExternType, SecType, Type
except ImportError:  # pragma: no cover - reported through the CLI instead
    webassembly = None


#: Only this prefix is public. Everything else the link exports is either
#: emscripten runtime support or an implementation detail a host must not reach.
PUBLIC_PREFIX = "mln_"

#: Names outside the public prefix a host still needs to place a descriptor.
ALLOCATOR_EXPORTS = ("malloc", "free")

WASM_TYPE_NAMES = (
    {
        Type.I32: "i32",
        Type.I64: "i64",
        Type.F32: "f32",
        Type.F64: "f64",
    }
    if webassembly
    else {}
)


DISPATCH_PROTOCOL = re.compile(r"#define MLN_BROWSER_DISPATCH_PROTOCOL (\d+)")


def dispatch_protocol(source: pathlib.Path) -> int:
    """Reads the call protocol the module was built with.

    Recorded in the manifest so a host can refuse a module *before* instantiating
    it and starting its worker pool. The protocol can change without any public
    header changing, so the headers digest cannot stand in for it.
    """
    match = DISPATCH_PROTOCOL.search(source.read_text())
    if match is None:
        raise SystemExit(f"no MLN_BROWSER_DISPATCH_PROTOCOL in {source}")
    return int(match.group(1))


def function_type_indices(module) -> list[int]:
    """Reads the function section, which maps a defined function to its type.

    `Module.get_functions()` reports code bodies rather than types, so the
    section is read directly.
    """
    section = module.get_section(SecType.FUNCTION)
    if section is None:
        raise SystemExit("module has no function section")
    module.seek(section.offset)
    return [module.read_uleb() for _ in range(module.read_uleb())]


def collect(module_path: pathlib.Path) -> dict:
    with webassembly.Module(str(module_path)) as module:
        types = module.get_types()
        imported = sum(1 for i in module.get_imports() if i.kind == ExternType.FUNC)
        func_types = function_type_indices(module)
        entries = {}
        for export in module.get_exports():
            if export.kind != ExternType.FUNC:
                continue
            if not (
                export.name.startswith(PUBLIC_PREFIX)
                or export.name in ALLOCATOR_EXPORTS
            ):
                continue
            defined_index = export.index - imported
            if not 0 <= defined_index < len(func_types):
                raise SystemExit(
                    f"{export.name} resolves outside the defined functions; "
                    "the module exports an import, which this API never does"
                )
            signature = types[func_types[defined_index]]
            entries[export.name] = {
                "params": [WASM_TYPE_NAMES[Type(p)] for p in signature.params],
                "results": [WASM_TYPE_NAMES[Type(r)] for r in signature.returns],
            }
    if not entries:
        raise SystemExit(
            f"no {PUBLIC_PREFIX}* exports in {module_path}; the module was "
            "linked without -lexports.js and its export names are minified"
        )
    return entries


#: `*** Dumping AST Record Layout` introduces one record; the record's own name
#: line carries offset 0, its direct fields are indented one level further, and
#: a `[sizeof=..., align=...]` line closes it. Nested records repeat the shape at
#: deeper indentation, which is why only the first indentation level is taken.
RECORD_HEADER = re.compile(r"^\s*0 \| (?:struct|union) (\w+)\s*$")
RECORD_FIELD = re.compile(r"^\s*(\d+) \|(\s+)(.+?)\s*$")
RECORD_FOOTER = re.compile(r"^\s*\| \[sizeof=(\d+), align=(\d+)\]\s*$")


def parse_record_layouts(dump: str) -> dict:
    """Reads clang's record-layout dump for the pinned Emscripten target.

    A binding that hand-writes a field offset gets it wrong the first time a
    struct carries a leading size field or a double forces padding, and the
    result is a descriptor native reads at the wrong offsets rather than a
    compile error. Clang has already applied the target's ABI here, so these are
    measured rather than modelled.
    """
    records: dict[str, dict] = {}
    name: str | None = None
    field_indent: int | None = None
    fields: dict[str, int] = {}
    for line in dump.splitlines():
        header = RECORD_HEADER.match(line)
        if header:
            name, field_indent, fields = header.group(1), None, {}
            continue
        if name is None:
            continue
        footer = RECORD_FOOTER.match(line)
        if footer:
            records[name] = {
                "size": int(footer.group(1)),
                "align": int(footer.group(2)),
                "fields": fields,
            }
            name, field_indent, fields = None, None, {}
            continue
        field = RECORD_FIELD.match(line)
        if not field:
            continue
        indent = len(field.group(2))
        if field_indent is None:
            field_indent = indent
        elif indent > field_indent:
            # A nested record's own fields, already covered by its own entry.
            continue
        elif indent < field_indent:
            continue
        # The declaration is "<type> <name>". An anonymous aggregate member has
        # no name, and its last token is the tag rather than a field, so taking
        # it would invent a field the AST never describes.
        declaration = field.group(3).split()
        if len(declaration) < 2:
            continue
        if declaration[0] in ("struct", "union") and len(declaration) == 2:
            continue
        fields[declaration[-1]] = int(field.group(1))
    return records


#: `typedef struct <name> {`, which this API writes on one line for every record
#: it defines. Used only to name the records whose layout is wanted; the layout
#: itself still comes from clang.
STRUCT_DEFINITION = re.compile(r"^typedef struct (\w+) \{", re.MULTILINE)


def struct_names(include: pathlib.Path) -> list[str]:
    names: set[str] = set()
    for header in sorted(include.rglob("*.h")):
        names.update(STRUCT_DEFINITION.findall(header.read_text()))
    if not names:
        raise SystemExit(f"no struct definitions under {include}")
    return sorted(names)


def collect_field_types(
    clang: pathlib.Path, sysroot: pathlib.Path, include: pathlib.Path
) -> dict:
    """Reads each record's declared field types from clang's AST.

    The record-layout dump gives offsets but not types, and a binding needs both
    to read or write a field: an offset alone says where four bytes are, not
    whether they are an integer, an enum, or a pointer.
    """
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
        raise SystemExit(f"clang failed reading record fields: {result.stderr}")
    records: dict[str, dict] = {}

    def walk(node) -> None:
        if (
            node.get("kind") == "RecordDecl"
            and node.get("name", "").startswith(PUBLIC_PREFIX)
            and node.get("completeDefinition")
        ):
            fields = {}
            for field in node.get("inner", []) or []:
                if field.get("kind") != "FieldDecl" or not field.get("name"):
                    continue
                declared = field["type"]
                # A typedef's own name says nothing about its width, so the
                # desugared type travels beside it. `mln_offline_region_id` is a
                # `uint64_t`, and only the canonical form says so.
                fields[field["name"]] = {
                    "type": declared["qualType"],
                    "canonical": declared.get(
                        "desugaredQualType", declared["qualType"]
                    ),
                }
            if fields:
                records[node["name"]] = fields
        for child in node.get("inner", []) or []:
            walk(child)

    walk(json.loads(result.stdout))

    # An enum is a scalar the generated accessors can place, but its canonical
    # spelling is `enum mln_x`, which says nothing about width. C23 fixed
    # underlying types make that exact, so each enum resolves to the type it was
    # declared over -- without which a descriptor's mode or platform field gets
    # no accessor and a caller writes the offset by hand.
    enums: dict[str, str] = {}

    def walk_enums(node) -> None:
        if node.get("kind") == "EnumDecl" and node.get("name"):
            fixed = node.get("fixedUnderlyingType", {}).get("qualType")
            if fixed:
                enums[f"enum {node['name']}"] = fixed
        for child in node.get("inner", []) or []:
            walk_enums(child)

    walk_enums(json.loads(result.stdout))
    _ENUM_VALUES.update(collect_enum_values(json.loads(result.stdout)))
    for fields in records.values():
        for declared in fields.values():
            declared["canonical"] = enums.get(
                declared["canonical"], declared["canonical"]
            )
    return records


#: Filled while field types are collected, because both come from one AST.
_ENUM_VALUES: dict[str, dict[str, int]] = {}


def collect_enum_values(tree) -> dict:
    """Reads every public enum's constants and their values.

    A binding otherwise writes `1 shl 5` beside a field name and hopes. These are
    the values the headers declare, so a bit that moves moves here too.
    """
    enums: dict[str, dict[str, int]] = {}

    def walk(node) -> None:
        if node.get("kind") == "EnumDecl" and node.get("name", "").startswith(
            PUBLIC_PREFIX
        ):
            constants = {}
            value = 0
            for child in node.get("inner", []) or []:
                if child.get("kind") != "EnumConstantDecl":
                    continue
                # An enumerator's children can include attributes as well as its
                # initializer, and an attribute is not a value. Only an
                # expression counts, or a `VALUE [[deprecated]],` would look like
                # a value this cannot read and stop generation.
                initializer = [
                    node
                    for node in child.get("inner") or []
                    if node.get("kind", "").endswith(("Expr", "Literal", "Operator"))
                ]
                if initializer:
                    folded = next(
                        (
                            result
                            for result in (evaluate(e) for e in initializer)
                            if result is not None
                        ),
                        None,
                    )
                    if folded is None:
                        # An enumerator that states its value and whose value
                        # this cannot read must stop generation. Falling through
                        # to the sequential value would emit a plausible wrong
                        # constant, and nothing downstream would notice.
                        raise SystemExit(
                            f"cannot evaluate {node['name']}.{child['name']}; "
                            "the AST carries an initializer shape this does not "
                            "fold"
                        )
                    value = folded
                constants[child["name"]] = value
                value += 1
            if constants:
                enums[node["name"]] = constants
        for child in node.get("inner", []) or []:
            walk(child)

    walk(tree)
    return enums


def evaluate(node) -> int | None:
    """Folds the constant expressions these headers use: literals and shifts."""
    kind = node.get("kind")
    if kind == "IntegerLiteral":
        return int(node["value"])
    if kind in ("ConstantExpr", "ImplicitCastExpr", "ParenExpr"):
        for child in node.get("inner", []) or []:
            folded = evaluate(child)
            if folded is not None:
                return folded
        # A ConstantExpr carries its own folded value when its operand does not.
        return int(node["value"]) if "value" in node else None
    if kind == "BinaryOperator" and node.get("opcode") in ("<<", "|", "+"):
        operands = [evaluate(child) for child in node.get("inner", []) or []]
        if len(operands) == 2 and all(o is not None for o in operands):
            left, right = operands
            if node["opcode"] == "<<":
                return left << right
            if node["opcode"] == "|":
                return left | right
            return left + right
    return None


def collect_layouts(
    clang: pathlib.Path, sysroot: pathlib.Path, include: pathlib.Path
) -> dict:
    # Clang lays out only the records a translation unit actually uses, so a
    # bare include of the umbrella header reports a handful. Naming each record
    # in a `sizeof` is what forces all of them to be laid out. The adapter
    # header is deliberately outside the umbrella, so it is included by name.
    probe_lines = [
        '#include "maplibre_native_c.h"',
        '#include "maplibre_native_c/callback_adapter.h"',
    ]
    probe_lines += [
        f'_Static_assert(sizeof(struct {name}) > 0, "{name}");'
        for name in struct_names(include)
    ]
    probe = "\n".join(probe_lines) + "\n"
    result = subprocess.run(
        [
            str(clang),
            "-target",
            "wasm32-unknown-emscripten",
            f"--sysroot={sysroot}",
            "-I",
            str(include),
            "-Xclang",
            "-fdump-record-layouts",
            "-fsyntax-only",
            "-x",
            "c",
            "-std=c23",
            "-",
        ],
        input=probe,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise SystemExit(f"clang failed laying out records: {result.stderr}")
    layouts = parse_record_layouts(result.stdout)
    # Only this API's own records; the sysroot's are an implementation detail.
    public = {
        name: layout
        for name, layout in sorted(layouts.items())
        if name.startswith(PUBLIC_PREFIX)
    }
    # Fail closed. Every record this project defines must come back with a
    # layout and at least one field: a declaration clang laid out under a name
    # the discovery regex did not predict, or a dump format that changed shape,
    # would otherwise be dropped silently and the binding would write the
    # remaining descriptors at offsets nothing checked.
    expected = set(struct_names(include))
    missing = sorted(expected - public.keys())
    if missing:
        raise SystemExit(
            "clang reported no layout for: "
            + ", ".join(missing)
            + ". Either the record moved out of the headers this reads, or the "
            "dump format changed and the parser needs updating."
        )
    empty = sorted(name for name, layout in public.items() if not layout["fields"])
    if empty:
        raise SystemExit(
            "clang reported no fields for: "
            + ", ".join(empty)
            + ". A record with no parsed fields means the dump format changed."
        )
    return public


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("module", type=pathlib.Path, help="linked .wasm")
    parser.add_argument("output", type=pathlib.Path, help="manifest to write")
    parser.add_argument("--clang", type=pathlib.Path, required=True)
    parser.add_argument("--sysroot", type=pathlib.Path, required=True)
    parser.add_argument("--include", type=pathlib.Path, required=True)

    arguments = parser.parse_args(argv)

    if webassembly is None:
        parser.error(
            "emsdk's tools package is not importable; set PYTHONPATH to the "
            "emscripten directory of the pinned emsdk"
        )

    entries = collect(arguments.module)
    structs = collect_layouts(arguments.clang, arguments.sysroot, arguments.include)
    # Offsets say where a field is; types say how to read it. A binding needs
    # both, and taking them from the same clang invocation the layouts came from
    # keeps them describing the same declaration.
    field_types = collect_field_types(
        arguments.clang, arguments.sysroot, arguments.include
    )
    for name, layout in structs.items():
        types = field_types.get(name, {})
        described = {}
        for field, offset in layout["fields"].items():
            declared = types.get(field)
            if declared is None:
                # Fail closed. A field clang laid out but did not describe means
                # the two readings disagree, and a manifest that recorded the
                # offset without the type would silently lose that field's
                # accessor rather than stop.
                raise SystemExit(
                    f"clang reported no type for {name}.{field}; the record "
                    "layout and the AST disagree"
                )
            described[field] = {"offset": offset, **declared}
        # And the other way. A field clang described but the layout parser did
        # not report means the dump format changed under the parser, which would
        # otherwise drop that field's accessor without a word.
        unlaid = sorted(set(types) - set(layout["fields"]))
        if unlaid:
            raise SystemExit(
                f"clang described {name} fields the layout parser did not "
                f"report: {', '.join(unlaid)}"
            )
        layout["fields"] = described
    manifest = {
        # 2 added each field's declared and canonical type beside its offset.
        "version": 2,
        "headersDigest": headers_digest(arguments.include),
        "dispatchProtocol": dispatch_protocol(
            arguments.include.parent / "src" / "browser" / "dispatch_table.h"
        ),
        "functions": dict(sorted(entries.items())),
        "structs": structs,
        "enums": dict(sorted(_ENUM_VALUES.items())),
    }
    text = json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    manifest["manifestDigest"] = hashlib.sha256(text.encode()).hexdigest()
    text = json.dumps(manifest, indent=2, sort_keys=False) + "\n"

    # Rewritten only on change, so an unchanged manifest does not retrigger
    # every generator downstream of it.
    if arguments.output.exists() and arguments.output.read_text() == text:
        return 0
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
