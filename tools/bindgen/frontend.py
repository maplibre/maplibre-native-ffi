"""Read C declarations with libclang; preserve binding metadata from attributes."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tomllib
from dataclasses import replace
from pathlib import Path
from typing import Any

from clang import cindex

from .model import (
    Api,
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

# libclang creates its enum members after class construction. Its Python
# bindings expose these objects dynamically, without static type declarations.
CursorKind: Any = cindex.CursorKind
TypeKind: Any = cindex.TypeKind

ANNOTATION_PREFIX = "mln:"


DIAGNOSTIC_RECORD = "mln_diagnostic"


def is_diagnostic_type(native: cindex.Type) -> bool:
    if native.kind != cindex.TypeKind.POINTER:
        return False
    pointee = native.get_pointee()
    return (
        not pointee.is_const_qualified()
        and pointee.get_canonical().get_declaration().spelling == DIAGNOSTIC_RECORD
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
                os.environ.get("SDKROOT")
                or subprocess.check_output(
                    ["xcrun", "--show-sdk-path"], text=True
                ).strip()
            )
            arguments += ["-isysroot", sdk]
        return arguments
    except (OSError, subprocess.CalledProcessError) as error:
        raise ModelError([f"cannot discover compiler headers: {error}"]) from error


class Extractor:
    def __init__(self, include_directory: Path):
        self.include_directory = include_directory.resolve()
        self.errors: list[str] = []
        self.records: dict[str, Record] = {}

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
                volatile=native.is_volatile_qualified(),
                restrict=native.is_restrict_qualified(),
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
            volatile=native.is_volatile_qualified(),
            restrict=native.is_restrict_qualified(),
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
                fields.append(
                    Field(
                        name=child.spelling,
                        type=self.type(child.type),
                        metadata=self.metadata(child),
                        location=self.location(child),
                        documentation=child.raw_comment or "",
                        bit_width=child.get_bitfield_width()
                        if child.is_bitfield()
                        else None,
                    )
                )
        value = Record(
            name=name,
            kind="union" if cursor.kind == CursorKind.UNION_DECL else "struct",
            fields=tuple(fields),
            metadata=self.metadata(cursor),
            location=self.location(cursor),
            documentation=cursor.raw_comment or "",
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
            if kind == CursorKind.FUNCTION_DECL:
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
                    documentation=cursor.raw_comment or "",
                    variadic=cursor.type.is_function_variadic(),
                    diagnostic=diagnostic,
                )
                if name in functions:
                    self.errors.append(
                        f"{self.location(cursor)}: duplicate function declaration {name}"
                    )
                functions[name] = function
            elif kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
                if self.record_name(cursor) != DIAGNOSTIC_RECORD:
                    self.record(cursor)
            elif kind == CursorKind.ENUM_DECL:
                enums[name] = Enum(
                    name=name,
                    underlying_type=self.type(cursor.enum_type),
                    values=tuple(
                        EnumValue(
                            child.spelling,
                            enum_value(child, cursor.enum_type),
                            child.raw_comment or "",
                        )
                        for child in cursor.get_children()
                        if child.kind == CursorKind.ENUM_CONSTANT_DECL
                    ),
                    metadata=self.metadata(cursor),
                    location=self.location(cursor),
                    documentation=cursor.raw_comment or "",
                )
            elif kind == CursorKind.TYPEDEF_DECL and name == DIAGNOSTIC_RECORD:
                continue
            elif kind == CursorKind.TYPEDEF_DECL:
                typedefs[name] = Typedef(
                    name=name,
                    type=self.type(cursor.underlying_typedef_type),
                    metadata=self.metadata(cursor),
                    location=self.location(cursor),
                    documentation=cursor.raw_comment or "",
                    parameters=tuple(
                        Parameter(
                            child.spelling, self.type(child.type), self.metadata(child)
                        )
                        for child in cursor.get_children()
                        if child.kind == CursorKind.PARM_DECL
                    ),
                )
            elif kind not in (
                CursorKind.MACRO_DEFINITION,
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


def parse_headers(
    include_directory: Path,
    *,
    headers: tuple[str, ...] | None = None,
    clang_args: tuple[str, ...] = (),
    clang: str = "clang",
) -> Api:
    """Parse every public header in one C23 translation unit.

    `clang_args` supplies target defines and include paths, including the
    installed MapLibre Native plugin headers. Pass `headers` for fixture tests.
    Compiler errors are fatal; partial ASTs never reach an emitter.
    """
    include_directory = include_directory.resolve()
    classify_interfaces = (
        headers is None and (include_directory / "binding-interfaces.toml").is_file()
    )
    if headers is None:
        headers = tuple(
            path.relative_to(include_directory).as_posix()
            for path in sorted(include_directory.rglob("*.h"))
        )
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
        filename, args=arguments, unsaved_files=[(filename, source)]
    )
    diagnostics = [
        str(diagnostic).replace(str(include_directory) + os.sep, "")
        for diagnostic in unit.diagnostics
        if diagnostic.severity >= cindex.Diagnostic.Error
    ]
    if diagnostics:
        raise ModelError(diagnostics)
    api = Extractor(include_directory).extract(unit)
    if classify_interfaces:
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
            name: parse_headers(
                include_directory,
                headers=tuple(entries),
                clang_args=clang_args,
                clang=clang,
            )
            for name, entries in manifest.items()
        }
        # Every exported declaration belongs to an explicit entrypoint closure.
        # An accidentally omitted new header cannot silently become private.
        for kind in ("functions", "records", "enums", "typedefs"):
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
            for kind in ("records", "enums", "typedefs")
            for item in getattr(interfaces["public"], kind)
        }
        runtime_types = {
            item.name
            for kind in ("records", "enums", "typedefs")
            for item in getattr(interfaces["runtime"], kind)
        }
        api = replace(
            api,
            runtime_exports=tuple(sorted(runtime - public)),
            runtime_types=tuple(sorted(runtime_types - public_types)),
        )
    return api
