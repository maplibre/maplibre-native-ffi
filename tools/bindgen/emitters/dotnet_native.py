"""Emit the raw C declarations that the .NET binding calls.

The output holds only the forms the binding uses: one `LibraryImport` per C
function, a blittable struct per record, a C# enum per C enum, and a handle
struct per handle typedef. Callback typedefs become unmanaged function pointer
types at their use sites. The handwritten runtime declares `mln_diagnostic`,
which the frontend strips from every signature it describes.
"""

from __future__ import annotations

from tools.bindgen.managed_contracts import KEYWORDS
from tools.bindgen.model import CType, Function, Record
from tools.bindgen.names import pascal
from tools.bindgen.semantic import BoundApi

from .dotnet_values import SCALARS

HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"


def raw_handle(native: str) -> str:
    """The raw struct that carries one handle typedef's issued id."""
    name = pascal(native.removeprefix("mln_").removesuffix("_handle"))
    return "Mln" + name.replace("Geojson", "GeoJson")


def identifier(name: str) -> str:
    return "@" + name if name in KEYWORDS["dotnet"] else name


class Declarations:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.api = bound.source
        self.typedefs = self.api.typedefs_by_name
        self.records = self.api.records_by_name
        self.enums = {enum.name: enum for enum in self.api.enums}
        self.used_records: set[str] = set()
        self.used_enums: set[str] = set()

    def functions(self) -> list[Function]:
        """Public functions, and the runtime adapters that generated code calls."""
        adapters = {
            name
            for handle in self.bound.handles.values()
            for name in (handle.view_begin, handle.view_end)
            if name
        }
        return [
            function
            for function in self.api.functions
            if function.name not in self.api.runtime_exports
            or function.name in adapters
        ]

    def type(self, ctype: CType, pointee: bool = False) -> str:
        """The C# spelling of one C type at a field, parameter, or result."""
        if ctype.kind == "pointer" and ctype.pointee is not None:
            return self.pointer(ctype.pointee)
        if ctype.kind == "array" and ctype.element is not None:
            return self.pointer(ctype.element)
        if ctype.kind == "void":
            return "void"
        name = ctype.declaration or ctype.spelling.removeprefix("const ")
        if name in self.bound.handles:
            return raw_handle(name)
        if name in self.records:
            self.use_record(name)
            return name
        if name in self.enums:
            self.used_enums.add(name)
            return name
        typedef = self.typedefs.get(name)
        if typedef is not None and ctype.kind == "typedef":
            return self.type(typedef.type, pointee)
        scalar = SCALARS.get(name) or SCALARS.get(ctype.canonical)
        if scalar == "bool":
            # A by-value C bool crosses as its byte; a pointer to one is a
            # pointer to C#'s one-byte bool.
            return "bool" if pointee else "byte"
        if scalar is not None:
            return scalar
        if ctype.kind in {"char_s", "schar"}:
            return "sbyte"
        if ctype.kind in {"char_u", "uchar"}:
            return "byte"
        if "(*)" in ctype.canonical or ctype.canonical.endswith("*"):
            # A function pointer that a header outside this API declares.
            return "void*"
        raise ValueError(f"{ctype.spelling}: no C# representation")

    def pointer(self, pointee: CType) -> str:
        target = pointee
        while target.kind == "typedef" and target.declaration in self.typedefs:
            underlying = self.typedefs[target.declaration].type
            if underlying.kind != "function":
                break
            target = underlying
        if target.kind == "function" and target.result is not None:
            arguments = [self.type(item) for item in target.parameters]
            return (
                "delegate* unmanaged[Cdecl]<"
                + ", ".join([*arguments, self.type(target.result)])
                + ">"
            )
        return self.type(pointee, pointee=True) + "*"

    def use_record(self, name: str) -> None:
        if name in self.used_records:
            return
        self.used_records.add(name)
        for field in self.records[name].fields:
            self.type(field.type)

    def function(self, function: Function) -> str:
        parameters = [
            f"{self.type(parameter.type)} {identifier(parameter.name)}"
            for parameter in function.parameters
        ]
        if function.diagnostic:
            parameters.append("mln_diagnostic* out_diagnostic")
        return (
            "    [LibraryImport(LibraryName)]\n"
            f"    internal static partial {self.type(function.return_type)} "
            f"{function.name}({', '.join(parameters)});\n"
        )

    def record(self, record: Record) -> str:
        explicit = record.kind == "union"
        fields = "".join(
            f"    {'[FieldOffset(0)] ' if explicit else ''}public {self.type(field.type)} {identifier(field.name)};\n"
            for field in record.fields
        )
        layout = "[StructLayout(LayoutKind.Explicit)]\n" if explicit else ""
        return f"{layout}internal unsafe struct {record.name}\n{{\n{fields}}}\n"

    def enum(self, name: str) -> str:
        enum = self.enums[name]
        underlying = SCALARS.get(enum.underlying_type.spelling) or SCALARS.get(
            enum.underlying_type.canonical, "int"
        )
        members = "".join(
            f"    {value.name} = {value.value},\n" for value in enum.values
        )
        return f"internal enum {name} : {underlying}\n{{\n{members}}}\n"


def generate(bound: BoundApi) -> dict[str, str]:
    declarations = Declarations(bound)
    functions = "".join(
        declarations.function(function) for function in declarations.functions()
    )
    # Completions deliver records through untyped pointers, and enum members
    # name the presence bits and variants that generated values read, so every
    # public record and enum is declared whether or not a signature names it.
    public = set(declarations.api.runtime_types)
    for record in declarations.api.records:
        if record.name not in public:
            declarations.use_record(record.name)
    declarations.used_enums.update(
        enum.name for enum in declarations.api.enums if enum.name not in public
    )
    records = "\n".join(
        declarations.record(declarations.records[name])
        for name in sorted(declarations.used_records)
    )
    enums = "\n".join(
        declarations.enum(name) for name in sorted(declarations.used_enums)
    )
    handles = "".join(
        f"internal readonly record struct {raw_handle(native)}(ulong Value) : IMlnHandle;\n"
        for native in sorted(bound.handles)
    )
    return {
        "Internal/C/NativeMethods.g.cs": (
            HEADER
            + "using System.Runtime.InteropServices;\n\n"
            + "namespace Maplibre.NativeFfi.Internal.C;\n\n"
            + "internal static unsafe partial class NativeMethods\n{\n"
            + functions
            + "}\n"
        ),
        "Internal/C/NativeTypes.g.cs": (
            HEADER
            + "using System.Runtime.InteropServices;\n\n"
            + "namespace Maplibre.NativeFfi.Internal.C;\n\n"
            + handles
            + "\n"
            + records
            + "\n"
            + enums
        ),
    }
