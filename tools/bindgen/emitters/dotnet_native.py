"""Emit the raw C declarations that the .NET binding calls.

The output holds only the forms the binding uses: one `LibraryImport` per C
function, a blittable struct per record, a C# enum per C enum, and a handle
struct per handle typedef. Callback typedefs become unmanaged function pointer
types at their use sites. The handwritten runtime declares `mln_diagnostic`,
which the frontend strips from every signature it describes. A presence mask
takes the type of the enum whose bits it carries. A fixed-size array field
fails generation, because no current header needs its inline layout.
"""

from __future__ import annotations

from tools.bindgen.managed_contracts import KEYWORDS
from tools.bindgen.model import CType, Function, Record
from tools.bindgen.names import pascal
from tools.bindgen.semantic import BoundApi

from .dotnet_values import SCALARS, raw_handle, typed_mask

# C's long is 32 bits on Windows and pointer-sized elsewhere.
C_LONG = {"long": "CLong", "unsigned long": "CULong"}

HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"


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
        """The public functions; the runtime header serves other bindings."""
        return list(self.api.public_functions)

    def type(self, ctype: CType, pointee: bool = False, field: bool = False) -> str:
        """The C# spelling of one C type at a field, parameter, or result."""
        if ctype.kind == "pointer" and ctype.pointee is not None:
            return self.pointer(ctype.pointee)
        if ctype.kind == "array" and ctype.element is not None:
            if field:
                # A parameter array decays to a pointer, but a field array
                # sits inline as the record's nested inline array type.
                raise ValueError(f"{ctype.spelling}: an inline array needs its field")
            return self.pointer(ctype.element)
        if ctype.kind == "void":
            return "void"
        name = ctype.declaration or ctype.spelling.removeprefix("const ")
        if name in self.bound.handles:
            return raw_handle(self.bound.handles[name])
        if name in self.records:
            self.use_record(name)
            return name
        if name in self.enums:
            self.used_enums.add(name)
            return name
        # A fixed-width name such as int64_t keeps its width whatever it
        # expands to on the host.
        scalar = None if name in C_LONG else SCALARS.get(name)
        typedef = self.typedefs.get(name)
        if scalar is None and typedef is not None and ctype.kind == "typedef":
            return self.type(typedef.type, pointee, field)
        if scalar is None:
            if width := C_LONG.get(name) or C_LONG.get(ctype.canonical):
                return width
            scalar = SCALARS.get(ctype.canonical)
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
            self.field_type(field)

    def inline_array(self, ctype: CType) -> CType | None:
        """The fixed-size array that a field type names, through typedefs."""
        while ctype.kind == "typedef" and ctype.declaration in self.typedefs:
            ctype = self.typedefs[ctype.declaration].type
        return ctype if ctype.kind == "array" and ctype.element is not None else None

    def field_type(self, field) -> str:
        """A field's C# type; a fixed-size array is a nested inline array."""
        ctype = self.inline_array(field.type) or field.type
        if ctype.kind == "array" and ctype.element is not None:
            if ctype.length is None:
                raise ValueError(f"{ctype.spelling}: a flexible array has no layout")
            self.type(ctype.element, field=True)
            return pascal(field.name) + "Array"
        return self.type(ctype, field=True)

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
        plan = self.bound.values.get(record.name)
        masks = {field.name: typed_mask(field) for field in plan.fields} if plan else {}
        fields = "".join(
            f"    {'[FieldOffset(0)] ' if explicit else ''}public {masks.get(field.name) or self.field_type(field)} {identifier(field.name)};\n"
            for field in record.fields
        )
        # A fixed-size array field sits inline as an InlineArray of its length.
        arrays = "".join(
            f"\n    [System.Runtime.CompilerServices.InlineArray({array.length})]\n"
            f"    public struct {pascal(field.name)}Array\n    {{\n"
            f"        private {self.type(array.element, field=True)} element;\n    }}\n"
            for field in record.fields
            if (array := self.inline_array(field.type))
        )
        layout = "[StructLayout(LayoutKind.Explicit)]\n" if explicit else ""
        return f"{layout}internal unsafe struct {record.name}\n{{\n{fields}{arrays}}}\n"

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
        f"internal readonly record struct {raw_handle(handle)}(ulong Value) : IMlnHandle;\n"
        for _, handle in sorted(bound.handles.items())
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
