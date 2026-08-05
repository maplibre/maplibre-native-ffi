"""Reads the public C API as the pinned Emscripten clang sees it.

The Kotlin/Wasm binding has no jextract and no ffigen, so the two generators
beside this file take their input from clang directly. Clang has already applied
the wasm32 ABI by the time it answers, which is what makes the offsets and the
lowered signatures measured rather than modelled.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import subprocess

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent

#: Only this prefix is public. Everything else a header pulls in belongs to the
#: sysroot or to MapLibre Native itself.
PUBLIC_PREFIX = "mln_"

#: The adapter header sits outside the umbrella deliberately, so it is named.
UMBRELLA = (
    '#include "maplibre_native_c.h"\n#include "maplibre_native_c/callback_adapter.h"\n'
)

DEFAULT_INCLUDE = REPO_ROOT / "include"
DEFAULT_SOURCES = (
    REPO_ROOT / "bindings" / "kotlin" / "src" / "wasmJsMain" / "kotlin",
    REPO_ROOT / "bindings" / "kotlin" / "src" / "wasmJsTest" / "kotlin",
)
DEFAULT_GENERATED = (
    REPO_ROOT
    / "bindings"
    / "kotlin"
    / "src"
    / "wasmJsMain"
    / "generated"
    / "org"
    / "maplibre"
    / "nativeffi"
    / "internal"
    / "wasm"
    / "generated"
)


def add_clang_arguments(parser: argparse.ArgumentParser) -> None:
    """Adds the toolchain and input arguments both generators take."""
    parser.add_argument(
        "--clang",
        type=pathlib.Path,
        help="defaults to the clang inside $EMSDK",
    )
    parser.add_argument(
        "--sysroot",
        type=pathlib.Path,
        help="defaults to the sysroot inside $EMSDK",
    )
    parser.add_argument("--include", type=pathlib.Path, default=DEFAULT_INCLUDE)
    parser.add_argument(
        "--source",
        dest="sources",
        type=pathlib.Path,
        action="append",
        help="Kotlin source root to read references from; repeatable",
    )


def resolve_toolchain(
    arguments: argparse.Namespace,
) -> tuple[pathlib.Path, pathlib.Path]:
    """Locates the pinned Emscripten clang and its sysroot.

    Both are read from the emsdk this repository pins, because a host clang lays
    records out for the host target and would report offsets no shipped module
    uses.
    """
    if arguments.clang and arguments.sysroot:
        return arguments.clang, arguments.sysroot
    emsdk = os.environ.get("EMSDK")
    if not emsdk:
        raise SystemExit(
            "EMSDK is unset and --clang/--sysroot were not both given. Run this "
            "under `mise exec`, which puts the pinned emsdk in the environment."
        )
    root = pathlib.Path(emsdk) / "upstream"
    clang = arguments.clang or root / "bin" / "clang"
    sysroot = arguments.sysroot or root / "emscripten" / "cache" / "sysroot"
    return clang, sysroot


def run_clang(
    clang: pathlib.Path,
    sysroot: pathlib.Path,
    include: pathlib.Path,
    source: str,
    *flags: str,
) -> str:
    result = subprocess.run(
        [
            str(clang),
            "-target",
            "wasm32-unknown-emscripten",
            f"--sysroot={sysroot}",
            "-I",
            str(include),
            "-fsyntax-only",
            "-std=c23",
            "-x",
            "c",
            *flags,
            "-",
        ],
        input=source,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise SystemExit(f"clang failed:\n{result.stderr}")
    return result.stdout


class Declarations:
    """The public C API's functions, records, and enums, with typedefs resolved.

    A typedef's name says nothing about its width or its shape: `mln_map` is a
    64-bit handle and `mln_string_view` is a two-field struct passed by address,
    and only the resolved form says so.
    """

    def __init__(self, tree: dict) -> None:
        self.typedefs: dict[str, str] = {}
        self.enum_underlying: dict[str, str] = {}
        self.enums: dict[str, dict[str, int]] = {}
        self.records: dict[str, dict] = {}
        self.functions: dict[str, dict] = {}
        self._collect(tree)
        for node in tree.get("inner") or []:
            if node.get("kind") == "FunctionDecl" and self._public(node):
                self.functions[node["name"]] = {
                    "return": node["type"]["qualType"].split("(")[0].strip(),
                    "parameters": [
                        (child.get("name") or "", child["type"]["qualType"])
                        for child in node.get("inner") or []
                        if child.get("kind") == "ParmVarDecl"
                    ],
                }

    @staticmethod
    def _public(node: dict) -> bool:
        return node.get("name", "").startswith(PUBLIC_PREFIX)

    def _collect(self, node: dict) -> None:
        kind = node.get("kind")
        if kind == "TypedefDecl" and node.get("name"):
            declared = node["type"]
            self.typedefs[node["name"]] = declared.get(
                "desugaredQualType", declared["qualType"]
            )
        elif kind == "EnumDecl" and node.get("name"):
            fixed = node.get("fixedUnderlyingType", {}).get("qualType")
            if fixed:
                self.enum_underlying[f"enum {node['name']}"] = fixed
            if self._public(node):
                self.enums[node["name"]] = enum_values(node)
        elif (
            kind == "RecordDecl"
            and self._public(node)
            and node.get("completeDefinition")
        ):
            self.records[node["name"]] = {
                "tag": node.get("tagUsed", "struct"),
                # Unnamed members keep their slot, because the layout dump
                # reports offsets positionally.
                "fields": [
                    (child.get("name") or "", child["type"]["qualType"])
                    for child in node.get("inner") or []
                    if child.get("kind") == "FieldDecl"
                ],
            }
        for child in node.get("inner") or []:
            self._collect(child)

    def resolve(self, c_type: str) -> str:
        """Reduces a declared type to the spelling its ABI treatment follows.

        Enums resolve to their C23 fixed underlying type, so a mode field and a
        `uint32_t` field are placed the same way.
        """
        resolved = c_type.strip()
        for _ in range(16):
            if resolved in self.typedefs and self.typedefs[resolved] != resolved:
                resolved = self.typedefs[resolved].strip()
            elif resolved in self.enum_underlying:
                resolved = self.enum_underlying[resolved].strip()
            else:
                return resolved
        raise SystemExit(f"typedef chain for {c_type} does not terminate")


def enum_values(node: dict) -> dict[str, int]:
    """Reads one enum's constants, so no binding writes `1 shl 5` and hopes."""
    constants: dict[str, int] = {}
    value = 0
    for child in node.get("inner") or []:
        if child.get("kind") != "EnumConstantDecl":
            continue
        # An enumerator's children include attributes as well as its
        # initializer, and an attribute is not a value.
        initializers = [
            inner
            for inner in child.get("inner") or []
            if inner.get("kind", "").endswith(("Expr", "Literal", "Operator"))
        ]
        if initializers:
            folded = next(
                (
                    result
                    for result in (evaluate(inner) for inner in initializers)
                    if result is not None
                ),
                None,
            )
            if folded is None:
                # Falling through to the sequential value would emit a plausible
                # wrong constant that nothing downstream would notice.
                raise SystemExit(
                    f"cannot evaluate {node['name']}.{child['name']}; the AST "
                    "carries an initializer shape this does not fold"
                )
            value = folded
        constants[child["name"]] = value
        value += 1
    return constants


def evaluate(node: dict) -> int | None:
    """Folds the constant expressions these headers use: literals and shifts."""
    kind = node.get("kind")
    if kind == "IntegerLiteral":
        return int(node["value"])
    if kind in ("ConstantExpr", "ImplicitCastExpr", "ParenExpr"):
        for child in node.get("inner") or []:
            folded = evaluate(child)
            if folded is not None:
                return folded
        return int(node["value"]) if "value" in node else None
    if kind == "BinaryOperator" and node.get("opcode") in ("<<", "|", "+"):
        operands = [evaluate(child) for child in node.get("inner") or []]
        if len(operands) == 2 and all(operand is not None for operand in operands):
            left, right = operands
            if node["opcode"] == "<<":
                return left << right
            if node["opcode"] == "|":
                return left | right
            return left + right
    return None


def read_declarations(
    clang: pathlib.Path, sysroot: pathlib.Path, include: pathlib.Path
) -> Declarations:
    dump = run_clang(clang, sysroot, include, UMBRELLA, "-Xclang", "-ast-dump=json")
    declarations = Declarations(json.loads(dump))
    if not declarations.functions:
        raise SystemExit(f"no {PUBLIC_PREFIX}* declarations under {include}")
    return declarations


#: `-fdump-record-layouts-simple` names each record, then reports its size in
#: bits and its field offsets in declaration order.
_LAYOUT_TYPE = re.compile(r"^Type: (?:struct|union) (\w+)$", re.MULTILINE)
_LAYOUT_SIZE = re.compile(r"^  Size:(\d+)$", re.MULTILINE)
_LAYOUT_OFFSETS = re.compile(r"^  FieldOffsets: \[([\d, ]*)\]>$", re.MULTILINE)


def read_layouts(
    clang: pathlib.Path,
    sysroot: pathlib.Path,
    include: pathlib.Path,
    declarations: Declarations,
) -> dict[str, dict]:
    """Measures every public record, in bytes.

    Clang lays out only the records a translation unit uses, so naming each one
    in a `sizeof` is what forces all of them into the dump.
    """
    probe = [UMBRELLA]
    probe += [
        f'_Static_assert(sizeof({record["tag"]} {name}) > 0, "{name}");'
        for name, record in sorted(declarations.records.items())
    ]
    dump = run_clang(
        clang,
        sysroot,
        include,
        "\n".join(probe) + "\n",
        "-Xclang",
        "-fdump-record-layouts-simple",
    )

    layouts: dict[str, dict] = {}
    blocks = _LAYOUT_TYPE.split(dump)
    for name, block in zip(blocks[1::2], blocks[2::2], strict=True):
        if name not in declarations.records:
            continue
        size = _LAYOUT_SIZE.search(block)
        offsets = _LAYOUT_OFFSETS.search(block)
        if not size or not offsets:
            raise SystemExit(
                f"clang's layout for {name} has a shape this does not read; "
                "the dump format changed and the parser needs updating"
            )
        listed = [
            int(offset) // 8 for offset in offsets.group(1).split(",") if offset.strip()
        ]
        fields = declarations.records[name]["fields"]
        if len(listed) != len(fields):
            raise SystemExit(
                f"clang reported {len(listed)} offsets for {name} and "
                f"{len(fields)} fields; the two readings disagree"
            )
        layouts[name] = {
            "size": int(size.group(1)) // 8,
            "fields": [
                (field, declared, offset)
                for (field, declared), offset in zip(fields, listed, strict=True)
            ],
        }

    # Fail closed. A record that clang laid out under a name this did not expect
    # would otherwise be dropped in silence, and the binding would write that
    # descriptor at offsets nothing measured.
    missing = sorted(set(declarations.records) - set(layouts))
    if missing:
        raise SystemExit("clang reported no layout for: " + ", ".join(missing))
    return layouts


_IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def referenced_identifiers(sources: list[pathlib.Path]) -> set[str]:
    """Collects every identifier the hand-written Kotlin names.

    Both generators emit only what the binding names, so an entry point or a
    descriptor the browser binding never touches costs nothing. Generated
    sources are excluded, or a declaration would keep itself alive.
    """
    found: set[str] = set()
    for root in sources:
        for path in sorted(root.rglob("*.kt")):
            if "generated" in path.parts:
                continue
            found.update(_IDENTIFIER.findall(path.read_text()))
    if not found:
        raise SystemExit(f"no Kotlin sources under {', '.join(map(str, sources))}")
    return found


def object_name(c_name: str) -> str:
    """`mln_render_target_extent` becomes `MlnRenderTargetExtent`."""
    return "".join(part.capitalize() for part in c_name.split("_"))


def member_name(c_name: str) -> str:
    """`scale_factor` becomes `scaleFactor`."""
    head, *rest = c_name.split("_")
    return head + "".join(part.capitalize() for part in rest)


def write_if_changed(path: pathlib.Path, text: str) -> None:
    """Leaves an unchanged file alone, so nothing downstream rebuilds."""
    if path.exists() and path.read_text() == text:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
