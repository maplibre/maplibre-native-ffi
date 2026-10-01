"""Emit the `@Native` declarations that the Dart binding calls.

The output holds only the forms the binding uses: one `@Native` external per C
function, a `Struct` or `Union` per record, a top-level integer constant per
C enum constant, and a `NativeFunction` typedef per callback typedef. Handle
typedefs alias their integer carrier. The handwritten `native_abi.dart`
declares `mln_diagnostic`, which the frontend strips from every signature it
describes, and the library re-exports it.
"""

from __future__ import annotations

from tools.bindgen.model import CType, Function, Record
from tools.bindgen.semantic import BoundApi

PATH = "bindings/dart/lib/src/internal/c/maplibre_native_c.g.dart"

# C scalar spellings and their `dart:ffi` native types.
NATIVE = {
    "bool": "Bool",
    "_Bool": "Bool",
    "char": "Char",
    "signed char": "Int8",
    "unsigned char": "Uint8",
    "int8_t": "Int8",
    "uint8_t": "Uint8",
    "int16_t": "Int16",
    "uint16_t": "Uint16",
    "int32_t": "Int32",
    "uint32_t": "Uint32",
    "int64_t": "Int64",
    "uint64_t": "Uint64",
    "size_t": "Size",
    "intptr_t": "IntPtr",
    "uintptr_t": "UintPtr",
    "int": "Int",
    "unsigned int": "UnsignedInt",
    "float": "Float",
    "double": "Double",
}


def dart_type(native: str, handles=()) -> str:
    """The Dart type that one native type crosses as."""
    if native in handles:
        return "int"
    if native == "Void":
        return "void"
    if native == "Bool":
        return "bool"
    if native in {"Float", "Double"}:
        return "double"
    if native in NATIVE.values():
        return "int"
    return native


class Declarations:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.api = bound.source
        self.typedefs = self.api.typedefs_by_name
        self.records = self.api.records_by_name
        self.enums = {enum.name: enum for enum in self.api.enums}

    def native(self, ctype: CType) -> str:
        """The `dart:ffi` native type of one C type."""
        if ctype.kind == "pointer" and ctype.pointee is not None:
            if ctype.pointee.kind == "function":
                raise ValueError(f"{ctype.spelling}: name the callback typedef")
            return f"Pointer<{self.native(ctype.pointee)}>"
        if ctype.kind == "array" and ctype.element is not None:
            return f"Pointer<{self.native(ctype.element)}>"
        if ctype.kind == "void":
            return "Void"
        name = ctype.declaration or ctype.spelling.removeprefix("const ")
        if name in self.bound.handles or name in self.records:
            return name
        if name in self.enums:
            return self.native(self.enums[name].underlying_type)
        typedef = self.typedefs.get(name)
        if typedef is not None:
            if self.callback(typedef.type):
                return name
            return self.native(typedef.type)
        if name in NATIVE:
            return NATIVE[name]
        if ctype.canonical in NATIVE:
            return NATIVE[ctype.canonical]
        if "(*)" in ctype.canonical or ctype.canonical.endswith("*"):
            # A function pointer that a header outside this API declares.
            return "Pointer<Void>"
        raise ValueError(f"{ctype.spelling}: no dart:ffi representation")

    @staticmethod
    def callback(ctype: CType) -> bool:
        return (
            ctype.kind == "pointer"
            and ctype.pointee is not None
            and ctype.pointee.kind == "function"
        )

    def function(self, function: Function) -> str:
        parameters = [
            (self.native(parameter.type), parameter.name)
            for parameter in function.parameters
        ]
        if function.diagnostic:
            parameters.append(("Pointer<mln_diagnostic>", "out_diagnostic"))
        result = self.native(function.return_type)
        native = ", ".join(native for native, _ in parameters)
        dart = ", ".join(
            f"{dart_type(native, self.bound.handles)} {identifier(name)}"
            for native, name in parameters
        )
        return (
            f"@Native<{result} Function({native})>()\n"
            f"external {dart_type(result, self.bound.handles)} {function.name}({dart});\n"
        )

    def field(self, name: str, ctype: CType) -> str:
        native = self.native(ctype)
        if native in self.bound.handles:
            native = "Uint64"
        dart = dart_type(native)
        annotation = f"@{native}() " if dart != native else ""
        return f"  {annotation}external {dart} {identifier(name)};\n"

    def record(self, record: Record) -> str:
        base = "Union" if record.kind == "union" else "Struct"
        fields = "".join(self.field(field.name, field.type) for field in record.fields)
        return f"final class {record.name} extends {base} {{\n{fields}}}\n"

    def enum(self, name: str) -> str:
        members = "".join(
            f"const {value.name} = {value.value};\n"
            for value in self.enums[name].values
        )
        return f"// {name}\n{members}"

    def callback_typedef(self, name: str) -> str:
        typedef = self.typedefs[name]
        signature = typedef.type.pointee
        assert signature is not None and signature.result is not None
        names = [parameter.name for parameter in typedef.parameters]
        if len(names) != len(signature.parameters):
            names = [""] * len(signature.parameters)
        parameters = ", ".join(
            f"{self.native(ctype)} {parameter}".rstrip()
            for ctype, parameter in zip(signature.parameters, names, strict=True)
        )
        return (
            f"typedef {name} = Pointer<NativeFunction<{name}Function>>;\n"
            f"typedef {name}Function = {self.native(signature.result)} Function({parameters});\n"
        )


# Words that no Dart declaration can use as a name. Built-in identifiers such as
# `extension` remain valid field and parameter names.
RESERVED = {
    "assert",
    "break",
    "case",
    "catch",
    "class",
    "const",
    "continue",
    "default",
    "do",
    "else",
    "enum",
    "extends",
    "false",
    "final",
    "finally",
    "for",
    "if",
    "in",
    "is",
    "new",
    "null",
    "rethrow",
    "return",
    "super",
    "switch",
    "this",
    "throw",
    "true",
    "try",
    "var",
    "void",
    "while",
    "with",
}


def identifier(name: str) -> str:
    if name in RESERVED:
        raise ValueError(f"{name}: C identifier is a Dart reserved word")
    return name


def generate(bound: BoundApi) -> str:
    declarations = Declarations(bound)
    api = declarations.api
    handles = "".join(
        f"typedef {native} = Uint64;\n" for native in sorted(bound.handles)
    )
    callbacks = "".join(
        declarations.callback_typedef(typedef.name)
        for typedef in api.typedefs
        if declarations.callback(typedef.type)
    )
    records = "\n".join(declarations.record(record) for record in api.records)
    enums = "\n".join(declarations.enum(enum.name) for enum in api.enums)
    functions = "\n".join(declarations.function(function) for function in api.functions)
    return (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n"
        "// ignore_for_file: camel_case_types, constant_identifier_names, non_constant_identifier_names\n"
        "@DefaultAsset(nativeAssetId)\n"
        "library;\n\n"
        "import 'dart:ffi';\n\n"
        "import 'native_abi.dart';\n"
        "import 'native_asset.dart';\n\n"
        "export 'native_abi.dart';\n\n"
        + handles
        + "\n"
        + callbacks
        + "\n"
        + records
        + "\n"
        + enums
        + "\n"
        + functions
    )
