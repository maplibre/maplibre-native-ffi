"""Read C declarations with libclang; preserve binding metadata from attributes."""

from __future__ import annotations

import ctypes
import functools
import os
import re
import shutil
import subprocess
import sys
import tomllib
from collections.abc import Iterable
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Any

from clang import cindex

from .model import (
    Api,
    Constant,
    CType,
    Enum,
    EnumValue,
    Field,
    Function,
    Location,
    ModelError,
    Parameter,
    Record,
    Typedef,
)
from .names import type_name
from .protocol import C_VERSION, DELIVERY_QUEUE, DIAGNOSTIC
from .schema import apply_defaults

# libclang creates its enum members after class construction. Its Python
# bindings expose these objects dynamically, without static type declarations.
CursorKind: Any = cindex.CursorKind
TypeKind: Any = cindex.TypeKind

ANNOTATION_PREFIX = "mln:"
# Source that may separate a doc comment from the start of its declaration,
# spelled backward: whitespace, an annotation macro, which takes only string
# literals, or the `typedef` before a tag. Matching the reversed source at a
# declaration reads only as far back as its comment.
_DECLARATION_PREFIX_REVERSED = re.compile(
    rb'(?:\s|\)\s*(?:"[^"]*"\s*)*\([A-Z0-9_]*[A-Z]\b|fedepyt\b)*'
)
# The directive before a macro definition's name, where its extent starts.
_DEFINE = re.compile(rb"[ \t]*#[ \t]*define[ \t]+")
# The fixed-width integer types that a constant's expansion can have, by
# signedness and size in bytes.
_FIXED_WIDTH = {
    (signed, size): f"{'' if signed else 'u'}int{8 * size}_t"
    for signed in (True, False)
    for size in (1, 2, 4, 8)
}
_SIGNED_KINDS = frozenset(
    {"CHAR_S", "SCHAR", "SHORT", "INT", "LONG", "LONGLONG", "INT128"}
)
_UNSIGNED_KINDS = frozenset(
    {"CHAR_U", "UCHAR", "USHORT", "UINT", "ULONG", "ULONGLONG", "UINT128"}
)


@functools.cache
def _is_function_like():
    """libclang's test for a function-like macro, which its Python bindings
    leave unwrapped."""
    function = cindex.conf.lib.clang_Cursor_isMacroFunctionLike
    function.argtypes = [cindex.Cursor]
    function.restype = ctypes.c_uint
    return function


def is_diagnostic_type(native: cindex.Type) -> bool:
    if native.kind != TypeKind.POINTER:
        return False
    pointee = native.get_pointee()
    return (
        not pointee.is_const_qualified()
        and pointee.get_canonical().get_declaration().spelling == DIAGNOSTIC
    )


def enum_value(cursor, underlying) -> int:
    """Preserve the integer domain after resolving an enum's underlying typedef."""
    canonical = underlying.get_canonical()
    value = cursor.enum_value
    if canonical.kind in {
        TypeKind.CHAR_U,
        TypeKind.UCHAR,
        TypeKind.USHORT,
        TypeKind.UINT,
        TypeKind.ULONG,
        TypeKind.ULONGLONG,
        TypeKind.UINT128,
    }:
        value %= 1 << (8 * canonical.get_size())
    return value


def compiler_arguments(clang: str = "clang") -> list[str]:
    """Use the configured compiler's system headers with the pinned parser."""
    return list(_compiler_arguments(clang, os.environ.get("SDKROOT")))


@functools.cache
def _compiler_arguments(clang: str, sdk: str | None) -> tuple[str, ...]:
    """Discover once per process; each discovery starts the compiler."""
    executable = shutil.which(clang)
    if executable is None:
        raise ModelError([f"compiler not found: {clang}"])
    try:
        resource = subprocess.check_output(
            [executable, "-print-resource-dir"], text=True
        ).strip()
        arguments = ["-resource-dir", resource]
        if sys.platform == "darwin":
            sdk = (
                sdk
                or subprocess.check_output(
                    ["xcrun", "--show-sdk-path"], text=True
                ).strip()
            )
            arguments += ["-isysroot", sdk]
        return tuple(arguments)
    except (OSError, subprocess.CalledProcessError) as error:
        raise ModelError([f"cannot discover compiler headers: {error}"]) from error


@dataclass(frozen=True)
class Macro:
    """An object-like macro with an expansion, which may be a constant."""

    name: str
    location: Location
    documentation: str


class Extractor:
    def __init__(self, include_directory: Path):
        self.include_directory = include_directory.resolve()
        self.errors: list[str] = []
        self.records: dict[str, Record] = {}
        self.sources: dict[str, tuple[bytes, bytes]] = {}
        # The object-like macros with an expansion, by name, which
        # `constants` evaluates.
        self.macros: dict[str, Macro] = {}

    def location(self, cursor: cindex.Cursor) -> Location:
        source = cursor.location
        path = Path(source.file.name).resolve() if source.file else None
        if path is None:
            filename = "<unknown>"
        elif path.is_relative_to(self.include_directory):
            filename = path.relative_to(self.include_directory).as_posix()
        else:
            filename = path.name
        return Location(filename, source.line, source.column)

    def comment(self, cursor: cindex.Cursor) -> str:
        """The doc comment that immediately precedes a declaration.

        Clang's own attachment misses a comment when an `MLN_BINDING` contract
        with a semicolon separates it from the declaration, and it gives an
        undocumented enum constant the comment of the constant before it. This
        reads the source instead: only whitespace, annotations, and the
        `typedef` that encloses a tag may separate the comment from the start
        of the declaration.
        """
        start = cursor.extent.start
        if not start.file:
            return ""
        name = start.file.name
        if name not in self.sources:
            source = Path(name).read_bytes()
            self.sources[name] = (source, source[::-1])
        source, reversed_source = self.sources[name]
        offset = start.offset
        if cursor.kind == CursorKind.MACRO_DEFINITION:
            # A macro's extent starts at its name, after `#define`.
            line = source.rfind(b"\n", 0, offset) + 1
            if _DEFINE.fullmatch(source, line, offset):
                offset = line
        prefix = _DECLARATION_PREFIX_REVERSED.match(
            reversed_source, len(source) - offset
        )
        # The pattern matches the empty string, so it always matches.
        assert prefix is not None
        end = len(source) - prefix.end()
        if source.endswith(b"*/", 0, end):
            comment = source[source.rfind(b"/*", 0, end) : end]
            if comment.startswith((b"/**", b"/*!")) and not comment.startswith(b"/**<"):
                return comment.decode()
            return ""
        lines: list[str] = []
        while end > 0:
            line_start = source.rfind(b"\n", 0, end) + 1
            line = source[line_start:end].strip()
            if not line.startswith((b"///", b"//!")) or line.startswith(b"///<"):
                break
            lines.append(line.decode())
            end = line_start - 1
        return "\n".join(reversed(lines))

    def owned(self, cursor: cindex.Cursor) -> bool:
        return bool(
            cursor.location.file
            and Path(cursor.location.file.name)
            .resolve()
            .is_relative_to(self.include_directory)
        )

    def metadata(self, cursor: cindex.Cursor) -> dict[str, str]:
        result: dict[str, str] = {}
        for child in cursor.get_children():
            if child.kind != CursorKind.ANNOTATE_ATTR:
                continue
            annotation = child.spelling
            if not annotation.startswith(ANNOTATION_PREFIX):
                continue
            for item in annotation.removeprefix(ANNOTATION_PREFIX).split(";"):
                key, separator, value = item.partition("=")
                key, value = key.strip(), value.strip()
                if not separator or not key or not value:
                    self.errors.append(
                        f"{self.location(cursor)}: malformed binding annotation {item!r}"
                    )
                elif key in result:
                    self.errors.append(
                        f"{self.location(cursor)}: duplicate binding metadata {key!r}"
                    )
                else:
                    result[key] = value
        return dict(sorted(result.items()))

    def record_name(self, cursor: cindex.Cursor) -> str:
        if cursor.spelling and not cursor.is_anonymous():
            return cursor.spelling
        location = self.location(cursor)
        # File/line identities are stable for a given input and contain no Clang
        # address or checkout path. Emitters can name anonymous members locally.
        return f"@{location.path}:{location.line}:{location.column}"

    def type(self, native: cindex.Type) -> CType:
        kind = native.kind
        if kind == TypeKind.ELABORATED:
            return replace(
                self.type(native.get_named_type()),
                const=native.is_const_qualified(),
                spelling=self.type_spelling(native.spelling),
                canonical=self.type_spelling(native.get_canonical().spelling),
            )
        declaration = native.get_declaration()
        name = declaration.spelling or None
        if declaration.kind in (
            CursorKind.STRUCT_DECL,
            CursorKind.UNION_DECL,
        ):
            name = self.record_name(declaration)
        common = CType(
            kind=kind.name.lower(),
            spelling=self.type_spelling(native.spelling),
            canonical=self.type_spelling(native.get_canonical().spelling),
            declaration=name,
            const=native.is_const_qualified(),
        )
        if kind == TypeKind.POINTER:
            return replace(
                common, kind="pointer", pointee=self.type(native.get_pointee())
            )
        if kind in (TypeKind.CONSTANTARRAY, TypeKind.INCOMPLETEARRAY):
            return replace(
                common,
                kind="array",
                element=self.type(native.element_type),
                length=native.element_count if kind == TypeKind.CONSTANTARRAY else None,
            )
        if kind == TypeKind.FUNCTIONPROTO:
            return replace(
                common,
                kind="function",
                result=self.type(native.get_result()),
                parameters=tuple(
                    self.type(argument) for argument in native.argument_types()
                ),
                variadic=native.is_function_variadic(),
            )
        supported = {
            TypeKind.VOID,
            TypeKind.BOOL,
            TypeKind.CHAR_U,
            TypeKind.UCHAR,
            TypeKind.USHORT,
            TypeKind.UINT,
            TypeKind.ULONG,
            TypeKind.ULONGLONG,
            TypeKind.CHAR_S,
            TypeKind.SCHAR,
            TypeKind.SHORT,
            TypeKind.INT,
            TypeKind.LONG,
            TypeKind.LONGLONG,
            TypeKind.FLOAT,
            TypeKind.DOUBLE,
            TypeKind.LONGDOUBLE,
            TypeKind.TYPEDEF,
            TypeKind.RECORD,
            TypeKind.ENUM,
        }
        if kind not in supported:
            self.errors.append(f"unsupported C type {native.spelling!r}: {kind.name}")
        return common

    def type_spelling(self, spelling: str) -> str:
        return spelling.replace(str(self.include_directory) + os.sep, "")

    def record(self, cursor: cindex.Cursor) -> None:
        name = self.record_name(cursor)
        fields: list[Field] = []
        for child in cursor.get_children():
            if child.kind in (
                CursorKind.STRUCT_DECL,
                CursorKind.UNION_DECL,
            ):
                self.record(child)
                if child.is_anonymous() and not any(
                    sibling.kind == CursorKind.FIELD_DECL
                    and sibling.type.get_declaration() == child
                    for sibling in cursor.get_children()
                ):
                    fields.append(
                        Field(
                            "",
                            self.type(child.type),
                            self.metadata(child),
                            self.location(child),
                        )
                    )
            elif child.kind == CursorKind.FIELD_DECL:
                if child.is_bitfield():
                    raise ModelError(
                        [
                            f"{self.location(child)}: bitfield has no portable ABI conversion"
                        ]
                    )
                fields.append(
                    Field(
                        name=child.spelling,
                        type=self.type(child.type),
                        metadata=self.metadata(child),
                        location=self.location(child),
                        documentation=self.comment(child),
                    )
                )
        value = Record(
            name=name,
            kind="union" if cursor.kind == CursorKind.UNION_DECL else "struct",
            fields=tuple(fields),
            location=self.location(cursor),
            documentation=self.comment(cursor),
            complete=cursor.is_definition(),
        )
        previous = self.records.get(name)
        if previous is None or not previous.complete:
            self.records[name] = value

    def extract(self, unit: cindex.TranslationUnit) -> Api:
        functions: dict[str, Function] = {}
        enums: dict[str, Enum] = {}
        typedefs: dict[str, Typedef] = {}
        for cursor in unit.cursor.get_children():
            if not self.owned(cursor):
                continue
            kind = cursor.kind
            name = cursor.spelling
            if kind == CursorKind.MACRO_DEFINITION:
                self.macro(cursor)
            elif kind == CursorKind.FUNCTION_DECL:
                arguments = list(cursor.get_arguments())
                diagnostic = bool(arguments) and is_diagnostic_type(arguments[-1].type)
                if diagnostic:
                    arguments.pop()
                if any(is_diagnostic_type(argument.type) for argument in arguments):
                    self.errors.append(
                        f"{self.location(cursor)}: {name}: mln_diagnostic* must be the last parameter"
                    )
                function = Function(
                    name=name,
                    return_type=self.type(cursor.result_type),
                    parameters=tuple(
                        Parameter(
                            argument.spelling,
                            self.type(argument.type),
                            self.metadata(argument),
                        )
                        for argument in arguments
                    ),
                    metadata=self.metadata(cursor),
                    location=self.location(cursor),
                    documentation=self.comment(cursor),
                    variadic=cursor.type.is_function_variadic(),
                    diagnostic=diagnostic,
                )
                if name in functions:
                    self.errors.append(
                        f"{self.location(cursor)}: duplicate function declaration {name}"
                    )
                functions[name] = function
            elif kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
                self.record(cursor)
            elif kind == CursorKind.ENUM_DECL:
                enums[name] = Enum(
                    name=name,
                    underlying_type=self.type(cursor.enum_type),
                    values=tuple(
                        EnumValue(
                            child.spelling,
                            enum_value(child, cursor.enum_type),
                            self.comment(child),
                        )
                        for child in cursor.get_children()
                        if child.kind == CursorKind.ENUM_CONSTANT_DECL
                    ),
                    metadata=self.metadata(cursor),
                    location=self.location(cursor),
                    documentation=self.comment(cursor),
                )
            elif kind == CursorKind.TYPEDEF_DECL:
                typedefs[name] = Typedef(
                    name=name,
                    type=self.type(cursor.underlying_typedef_type),
                    metadata=self.metadata(cursor),
                    location=self.location(cursor),
                    documentation=self.comment(cursor),
                    parameters=tuple(
                        Parameter(
                            child.spelling, self.type(child.type), self.metadata(child)
                        )
                        for child in cursor.get_children()
                        if child.kind == CursorKind.PARM_DECL
                    ),
                )
            elif kind not in (
                CursorKind.MACRO_INSTANTIATION,
                CursorKind.INCLUSION_DIRECTIVE,
            ):
                self.errors.append(
                    f"{self.location(cursor)}: unsupported declaration {kind.name} {name}"
                )
        if self.errors:
            raise ModelError(self.errors)
        return Api(
            functions=tuple(functions[name] for name in sorted(functions)),
            records=tuple(self.records[name] for name in sorted(self.records)),
            enums=tuple(enums[name] for name in sorted(enums)),
            typedefs=tuple(typedefs[name] for name in sorted(typedefs)),
        )

    def macro(self, cursor: cindex.Cursor) -> None:
        """Keep an object-like macro that expands to something.

        A function-like macro, such as MLN_BINDING, and an empty one, such as
        an include guard, declare no constant.
        """
        if _is_function_like()(cursor):
            return
        if len(list(cursor.get_tokens())) < 2:
            return
        self.macros[cursor.spelling] = Macro(
            cursor.spelling, self.location(cursor), self.comment(cursor)
        )

    def constants(self, unit, filename: str, source: str) -> list[Constant]:
        """Evaluate each kept macro as an integer constant.

        Each macro initializes an enum constant whose fixed underlying type is
        the macro's own type, in a parse of the headers that appends one such
        enum per macro. The compiler checks that the expansion is an integer
        constant expression, and gives its type and value. A documented macro
        must evaluate, and one that evaluates must be documented. An
        undocumented macro that doesn't evaluate, such as MLN_API, declares
        no constant.
        """
        if not self.macros:
            return []
        macros = list(self.macros.values())
        first = source.count("\n") + 1
        probes = source + "".join(
            f"enum mln_bindgen_constant_{index} : __typeof__({macro.name}) "
            f"{{ mln_bindgen_constant_{index}_value = ({macro.name}) }};\n"
            for index, macro in enumerate(macros)
        )
        unit.reparse(unsaved_files=[(filename, probes)])
        failures: dict[int, str] = {}
        for diagnostic in unit.diagnostics:
            location = diagnostic.location
            if (
                diagnostic.severity >= cindex.Diagnostic.Error
                and location.file
                and location.file.name == filename
                and location.line >= first
            ):
                failures.setdefault(location.line - first, diagnostic.spelling)
        found: dict[int, cindex.Cursor] = {}
        for cursor in unit.cursor.get_children():
            if cursor.kind == CursorKind.ENUM_DECL and cursor.spelling.startswith(
                "mln_bindgen_constant_"
            ):
                found[int(cursor.spelling.rsplit("_", 1)[1])] = cursor
        constants = []
        for index, macro in enumerate(macros):
            probe = found.get(index)
            values = [
                child
                for child in (probe.get_children() if probe else ())
                if child.kind == CursorKind.ENUM_CONSTANT_DECL
            ]
            underlying = probe.enum_type.get_canonical() if probe else None
            kind = underlying.kind.name if underlying else ""
            signed = kind in _SIGNED_KINDS
            type_ = (
                _FIXED_WIDTH.get((signed, underlying.get_size()))
                if underlying and (signed or kind in _UNSIGNED_KINDS)
                else None
            )
            failure = failures.get(index)
            if failure is None and (len(values) != 1 or type_ is None):
                spelling = underlying.spelling if underlying else "its type"
                failure = f"{spelling} is not a supported integer type"
            if failure is not None:
                if macro.documentation:
                    self.errors.append(
                        f"{macro.location}: {macro.name}: a documented macro must "
                        f"be an integer constant: {failure}"
                    )
                continue
            if not macro.documentation:
                self.errors.append(
                    f"{macro.location}: {macro.name}: constant has no documentation"
                )
                continue
            assert type_ is not None and probe is not None
            constants.append(
                Constant(
                    macro.name,
                    enum_value(values[0], probe.enum_type),
                    type_,
                    macro.location,
                    macro.documentation,
                )
            )
        if self.errors:
            raise ModelError(self.errors)
        return sorted(constants, key=lambda constant: constant.name)


def parse_headers(
    include_directory: Path,
    *,
    headers: tuple[str, ...] | None = None,
    clang_args: tuple[str, ...] = (),
    clang: str = "clang",
) -> Api:
    """Parse every public header in one C23 translation unit.

    `clang_args` supplies target defines and include paths. Pass `headers` for
    fixture tests.
    Compiler errors are fatal;
    partial ASTs never reach an emitter. The result carries each declaration's
    complete metadata: its annotations together with the defaults that its C
    shape implies.
    """
    return apply_defaults(
        extract_headers(
            include_directory,
            headers=headers,
            clang_args=clang_args,
            clang=clang,
        )
    )


def extract_headers(
    include_directory: Path,
    *,
    headers: tuple[str, ...] | None = None,
    clang_args: tuple[str, ...] = (),
    clang: str = "clang",
) -> Api:
    """Parse the headers into declarations that carry only their annotations."""
    include_directory = include_directory.resolve()
    classify_interfaces = (
        headers is None and (include_directory / "binding-interfaces.toml").is_file()
    )
    if headers is None:
        found = {
            path.relative_to(include_directory)
            for path in include_directory.rglob("*.h")
        }
        headers = tuple(path.as_posix() for path in sorted(found))
    if not headers:
        raise ModelError([f"no public headers in {include_directory}"])
    source = "".join(f'#include "{header}"\n' for header in headers)
    filename = str(include_directory / "__bindgen__.c")
    arguments = [
        "-x",
        "c",
        "-std=c2x",
        "-DMLN_BINDGEN=1",
        "-fparse-all-comments",
        f"-I{include_directory}",
        *compiler_arguments(clang),
        *clang_args,
    ]
    unit = cindex.Index.create().parse(
        filename,
        args=arguments,
        unsaved_files=[(filename, source)],
        # Keeps macro definitions in the AST, for the constants.
        options=cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD,
    )
    diagnostics = [
        str(diagnostic).replace(str(include_directory) + os.sep, "")
        for diagnostic in unit.diagnostics
        if diagnostic.severity >= cindex.Diagnostic.Error
    ]
    if diagnostics:
        raise ModelError(diagnostics)
    extractor = Extractor(include_directory)
    api = extractor.extract(unit)
    api = replace(api, constants=tuple(extractor.constants(unit, filename, source)))
    if classify_interfaces:
        if C_VERSION not in api.constants_by_name:
            raise ModelError([f"{include_directory}: no header defines {C_VERSION}"])
        manifest_path = include_directory / "binding-interfaces.toml"
        try:
            manifest = tomllib.loads(manifest_path.read_text())
        except (OSError, tomllib.TOMLDecodeError) as error:
            raise ModelError([f"{manifest_path}: {error}"]) from error
        if set(manifest) != {"public", "runtime"} or any(
            not isinstance(entries, list)
            or not entries
            or not all(isinstance(entry, str) for entry in entries)
            for entries in manifest.values()
        ):
            raise ModelError(
                [
                    f"{manifest_path}: requires public and runtime header entrypoint lists"
                ]
            )
        interfaces = {
            name: extract_headers(
                include_directory,
                headers=tuple(entries),
                clang_args=clang_args,
                clang=clang,
            )
            for name, entries in manifest.items()
        }
        # Every exported declaration belongs to an explicit entrypoint closure,
        # so a header that the manifest omits fails generation.
        for kind in ("functions", "records", "enums", "typedefs", "constants"):
            reachable = {
                item.name
                for interface in interfaces.values()
                for item in getattr(interface, kind)
            }
            missing = [
                item for item in getattr(api, kind) if item.name not in reachable
            ]
            if missing:
                raise ModelError(
                    [
                        f"{item.location}: {item.name}: declaration is outside all interface entrypoints"
                        for item in missing
                    ]
                )
        public = {function.name for function in interfaces["public"].functions}
        runtime = {function.name for function in interfaces["runtime"].functions}
        public_types = {
            item.name
            for kind in ("records", "enums", "typedefs", "constants")
            for item in getattr(interfaces["public"], kind)
        }
        runtime_types = {
            item.name
            for kind in ("records", "enums", "typedefs", "constants")
            for item in getattr(interfaces["runtime"], kind)
        }
        api = replace(
            api,
            runtime_exports=tuple(sorted(runtime - public)),
            runtime_types=tuple(sorted(runtime_types - public_types)),
        )
    return with_delivery_types(api)


def delivery_functions(api: Api) -> set[str]:
    """The functions of the queue delivery protocol: those that take or return
    a delivery queue handle."""
    return {
        function.name
        for function in api.functions
        if any(
            type_name(parameter.type.pointee or parameter.type) == DELIVERY_QUEUE
            for parameter in function.parameters
        )
    }


def reachable_types(api: Api, functions: Iterable[Function]) -> set[str]:
    """The records, enums, and typedefs that the functions reach, through their
    parameters, results, record members, and callback signatures, along with
    the bitmask enum of each flag set they reach."""
    typedefs = api.typedefs_by_name
    records = api.records_by_name
    seen: set[str] = set()

    def visit(type_: CType | None):
        while type_ is not None and type_.kind in {"pointer", "array"}:
            type_ = type_.pointee or type_.element
        if type_ is None:
            return
        name = type_.declaration or ""
        if not name or name in seen:
            return
        seen.add(name)
        enum = api.enum_for(name)
        if enum is not None:
            seen.add(enum.name)
        if name in typedefs:
            typedef = typedefs[name]
            for parameter in typedef.parameters:
                visit(parameter.type)
            signature = typedef.type.pointee
            if signature is not None and signature.result is not None:
                visit(signature.result)
            visit(typedef.type)
        if name in records:
            for field in records[name].fields:
                visit(field.type)

    for function in functions:
        visit(function.return_type)
        for parameter in function.parameters:
            visit(parameter.type)
    return seen


def with_delivery_types(api: Api) -> Api:
    """Mark the delivery protocol's functions as runtime exports, and the types
    that only they reach as runtime types.

    A binding runtime drives the protocol itself, so its functions and types
    stay out of every public API, as the runtime header's do.
    """
    delivery = delivery_functions(api)
    if not delivery:
        return api
    functions = api.functions_by_name
    internal = reachable_types(api, (functions[name] for name in delivery))
    public = reachable_types(
        api,
        (
            function
            for function in api.functions
            if function.name not in delivery
            and function.name not in api.runtime_exports
        ),
    )
    names = {
        item.name for kind in (api.records, api.enums, api.typedefs) for item in kind
    }
    added = (internal - public) & names
    return replace(
        api,
        runtime_exports=tuple(sorted(set(api.runtime_exports) | delivery)),
        runtime_types=tuple(sorted(set(api.runtime_types) | added)),
    )
