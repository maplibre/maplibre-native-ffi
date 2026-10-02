"""Raw `repr(C)` declarations for the Rust -sys crate.

The crate mirrors the C boundary: one type alias and its constants per enum,
one `repr(C)` item per record, one transparent newtype per handle, and one
`extern "C"` declaration per function. Rust code above the crate and the
Python extension call the C API only through these declarations.

Only the C forms that the headers use are translated. A function whose
signature reaches any other form is left out and reported unsupported, since
an approximate declaration would break the ABI.
"""

from __future__ import annotations

from ..model import Api, CType, Function, Record
from ..semantic import BoundApi

PATH = "crates/maplibre-native-ffi-sys/src/generated.rs"

# Rust spellings of the C scalars, by typedef or builtin kind.
SCALARS = {
    "int8_t": "i8",
    "uint8_t": "u8",
    "int16_t": "i16",
    "uint16_t": "u16",
    "int32_t": "i32",
    "uint32_t": "u32",
    "int64_t": "i64",
    "uint64_t": "u64",
    "intptr_t": "isize",
    "uintptr_t": "usize",
    "ptrdiff_t": "isize",
    "size_t": "usize",
    "bool": "bool",
    "float": "f32",
    "double": "f64",
    "char_s": "std::ffi::c_char",
    "char_u": "std::ffi::c_char",
    "schar": "std::ffi::c_schar",
    "uchar": "std::ffi::c_uchar",
    "short": "std::ffi::c_short",
    "ushort": "std::ffi::c_ushort",
    "int": "std::ffi::c_int",
    "uint": "std::ffi::c_uint",
    "long": "std::ffi::c_long",
    "ulong": "std::ffi::c_ulong",
    "longlong": "std::ffi::c_longlong",
    "ulonglong": "std::ffi::c_ulonglong",
}


class Unsupported(ValueError):
    pass


def identifier(name: str) -> str:
    from .rust import native_identifier

    return native_identifier(name)


class Declarations:
    def __init__(self, api: Api, functions: list[Function]):
        self.api = api
        self.functions = functions
        self.records = api.records_by_name
        self.enums = {enum.name: enum for enum in api.enums}
        self.typedefs = api.typedefs_by_name
        self.used: set[str] = set()
        self.externs: list[str] = []
        self.unsupported: dict[str, str] = {}
        for function in functions:
            used = set(self.used)
            try:
                self.externs.append(self.extern(function))
            except Unsupported as error:
                self.used = used
                self.unsupported[function.name] = f"{function.location}: {error}"
        # Public types stay declared even when no function names them, since
        # they are part of the C contract that Rust code builds values for.
        for kind in (api.enums, api.records, api.typedefs):
            for item in kind:
                if item.name in api.runtime_types or item.name.startswith("@"):
                    continue
                used = set(self.used)
                try:
                    self.use(item.name)
                except Unsupported:
                    self.used = used

    def type(self, value: CType) -> str:
        """The Rust spelling of a C type, recording every declaration it names."""
        if value.kind == "pointer":
            pointee = value.pointee
            if pointee.kind == "function":
                return self.function_pointer(pointee, ())
            target = (
                "std::ffi::c_void" if pointee.kind == "void" else self.type(pointee)
            )
            return ("*const " if pointee.const else "*mut ") + target
        if value.kind == "array":
            if value.length is None:
                raise Unsupported(f"{value.spelling}: flexible arrays have no layout")
            return f"[{self.type(value.element)}; {value.length}]"
        if value.kind in {"typedef", "record", "enum"}:
            name = value.declaration
            if name in SCALARS:
                return SCALARS[name]
            if name is None or name.startswith("@"):
                raise Unsupported(f"{value.spelling}: anonymous types need a name")
            self.use(name)
            return name
        if value.kind in SCALARS:
            return SCALARS[value.kind]
        raise Unsupported(f"{value.spelling}: {value.kind} has no Rust declaration")

    def use(self, name: str) -> None:
        """Record a declaration and everything its definition names."""
        if name in self.used or name in SCALARS:
            return
        self.used.add(name)
        if name in self.typedefs:
            typedef = self.typedefs[name]
            if typedef.type.kind in {"record", "enum"}:
                self.use(typedef.type.declaration)
            else:
                self.typedef_target(typedef)
        elif name in self.records:
            record = self.records[name]
            for field in record.fields:
                if field.bit_width is not None:
                    raise Unsupported(f"{name}: bit-fields have no Rust layout")
                if not field.name:
                    raise Unsupported(f"{name}: anonymous members need a name")
                self.type(field.type)
        # Any other name is a typedef that another header declares, such as the
        # plugin registration function type, which the crate declares by hand.

    def function_pointer(self, function: CType, names) -> str:
        names = names or [""] * len(function.parameters)
        parameters = [
            f"{identifier(name)}: {self.type(parameter)}"
            if name
            else self.type(parameter)
            for parameter, name in zip(function.parameters, names, strict=True)
        ]
        if function.variadic:
            raise Unsupported("variadic callbacks have no Rust declaration")
        result = function.result
        returns = "" if result.kind == "void" else f" -> {self.type(result)}"
        return f'Option<unsafe extern "C" fn({", ".join(parameters)}){returns}>'

    def typedef_target(self, typedef) -> str:
        pointee = typedef.type.pointee
        if pointee is None or pointee.kind != "function":
            return self.type(typedef.type)
        names = [parameter.name for parameter in typedef.parameters]
        if len(names) != len(pointee.parameters):
            names = []
        return self.function_pointer(pointee, names)

    def extern(self, function: Function) -> str:
        if function.variadic:
            raise Unsupported(f"{function.name}: variadic functions")
        parameters = [
            f"{identifier(parameter.name)}: {self.type(parameter.type)}"
            for parameter in function.parameters
        ]
        if function.diagnostic:
            parameters.append("out_diagnostic: *mut mln_diagnostic")
        result = function.return_type
        returns = "" if result.kind == "void" else f" -> {self.type(result)}"
        return f"    pub fn {function.name}({', '.join(parameters)}){returns};"

    def handle(self, name: str) -> bool:
        typedef = self.typedefs.get(name)
        return bool(typedef and typedef.metadata.get("kind") == "handle")

    def debuggable(self, record: Record) -> bool:
        """Unions have no derived Debug, so neither do records that hold one."""
        if record.kind == "union":
            return False
        for field in record.fields:
            value = field.type
            while value.kind == "array":
                value = value.element
            name = value.declaration
            if name in self.typedefs and self.typedefs[name].type.kind == "record":
                name = self.typedefs[name].type.declaration
            if name in self.records and not self.debuggable(self.records[name]):
                return False
        return True

    def render(self) -> str:
        items = []
        for enum in self.api.enums:
            if enum.name not in self.used:
                continue
            items.append(f"pub type {enum.name} = {self.type(enum.underlying_type)};")
            items.extend(
                f"pub const {value.name}: {enum.name} = {value.value};"
                for value in enum.values
            )
        for record in self.api.records:
            if record.name not in self.used:
                continue
            if not record.complete:
                items.append(
                    f"#[repr(C)]\npub struct {record.name} {{\n    _opaque: [u8; 0],\n}}"
                )
                continue
            fields = "\n".join(
                f"    pub {identifier(field.name)}: {self.type(field.type)},"
                for field in record.fields
            )
            derive = "Debug, Clone, Copy" if self.debuggable(record) else "Clone, Copy"
            keyword = "union" if record.kind == "union" else "struct"
            items.append(
                f"#[repr(C)]\n#[derive({derive})]\npub {keyword} {record.name} {{\n{fields}\n}}"
            )
        for typedef in self.api.typedefs:
            if typedef.name not in self.used:
                continue
            if typedef.type.kind in {"record", "enum"}:
                if typedef.type.declaration != typedef.name:
                    items.append(
                        f"pub type {typedef.name} = {typedef.type.declaration};"
                    )
            elif self.handle(typedef.name):
                items.append(
                    "#[repr(transparent)]\n#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]\n"
                    f"pub struct {typedef.name}(pub {self.type(typedef.type)});"
                )
            else:
                items.append(
                    f"pub type {typedef.name} = {self.typedef_target(typedef)};"
                )
        handles = [
            typedef.name
            for typedef in self.api.typedefs
            if typedef.name in self.used and self.handle(typedef.name)
        ]
        if handles:
            items.append(f"native_handles!({', '.join(handles)});")
        return (
            "// Generated from C headers by tools/bindgen. Do not edit.\n"
            "use super::*;\n\n"
            + "\n".join(items)
            + '\n\nunsafe extern "C" {\n'
            + "\n".join(self.externs)
            + "\n}\n"
        )

    def layout(self) -> dict[str, tuple[str, ...]]:
        """Each emitted record, handle, and enum by its C name, with the field
        names whose offsets the C layout fixes."""
        result = {
            record.name: ()
            if record.kind == "union"
            else tuple(field.name for field in record.fields)
            for record in self.api.records
            if record.name in self.used and record.complete
        }
        result.update(
            (name, ())
            for name in sorted(self.used)
            if name in self.enums or self.handle(name)
        )
        return result


def declarations(api: Api | BoundApi) -> Declarations:
    bound = api if isinstance(api, BoundApi) else None
    source = bound.source if bound else api
    # The runtime header serves bindings that cannot run code on MapLibre
    # threads. Rust calls only the adapters that the handle plans name.
    adapters = (
        {
            name
            for handle in bound.handles.values()
            for name in (handle.view_begin, handle.view_end)
            if name
        }
        if bound
        else set()
    )
    functions = [
        function
        for function in source.functions
        if function.name not in source.runtime_exports or function.name in adapters
    ]
    return Declarations(source, functions)
