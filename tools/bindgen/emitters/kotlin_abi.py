"""Lay out C types and lower C signatures to the Kotlin native-call carriers.

The Kotlin binding marshals every value in common code, so each platform shim
passes only primitives: integers, floats, and addresses carried as `Long`. A
record that C passes by value crosses as the address of a copy, and a record
that C returns by value is written to an address the caller supplies.

Common code reads and writes records at offsets computed here for LP64 and
two ILP32 layouts: Android ARM32 and WebAssembly align 64-bit scalars to eight
bytes, while Android x86 aligns them to four. A generator test compiles these
offsets as static assertions against each Android ABI.
"""

from __future__ import annotations

from dataclasses import dataclass

from ..model import Api, CType


class LayoutError(ValueError):
    pass


@dataclass(frozen=True)
class Primitive:
    name: str
    size32: int
    size64: int
    carrier: str

    def size(self, width: int) -> int:
        return self.size64 if width == 64 else self.size32


BOOL = Primitive("bool", 1, 1, "Boolean")
I8 = Primitive("i8", 1, 1, "Byte")
U8 = Primitive("u8", 1, 1, "Byte")
I16 = Primitive("i16", 2, 2, "Short")
U16 = Primitive("u16", 2, 2, "Short")
I32 = Primitive("i32", 4, 4, "Int")
U32 = Primitive("u32", 4, 4, "Int")
I64 = Primitive("i64", 8, 8, "Long")
U64 = Primitive("u64", 8, 8, "Long")
SIZE = Primitive("size", 4, 8, "Long")
SSIZE = Primitive("ssize", 4, 8, "Long")
F32 = Primitive("f32", 4, 4, "Float")
F64 = Primitive("f64", 8, 8, "Double")
POINTER = Primitive("ptr", 4, 8, "Long")

# Portable typedef spellings, which keep their width whatever the host's
# canonical expansion is.
PORTABLE = {
    "bool": BOOL,
    "_Bool": BOOL,
    "int8_t": I8,
    "uint8_t": U8,
    "int16_t": I16,
    "uint16_t": U16,
    "int32_t": I32,
    "uint32_t": U32,
    "int64_t": I64,
    "uint64_t": U64,
    "size_t": SIZE,
    "uintptr_t": SIZE,
    "ptrdiff_t": SSIZE,
    "intptr_t": SSIZE,
    "float": F32,
    "double": F64,
}

# Builtin canonical spellings. `long` is pointer-sized on every data model a
# Kotlin target uses except LLP64, and the C API declares no `long`.
BUILTIN = {
    "bool": BOOL,
    "_Bool": BOOL,
    "char": I8,
    "signed char": I8,
    "unsigned char": U8,
    "short": I16,
    "unsigned short": U16,
    "int": I32,
    "unsigned int": U32,
    "long": SSIZE,
    "unsigned long": SIZE,
    "long long": I64,
    "unsigned long long": U64,
    "float": F32,
    "double": F64,
}


@dataclass(frozen=True)
class Record:
    """A struct or union, laid out per data model."""

    name: str
    union: bool
    fields: tuple[tuple[str, object], ...]


@dataclass(frozen=True)
class Array:
    element: object
    length: int


@dataclass(frozen=True)
class Layout:
    size: int
    align: int
    offsets: dict[str, int]


# Pointer width and 64-bit scalar alignment, in Kotlin's w() argument order.
DATA_MODELS = ((32, 8), (64, 8), (32, 4))


class Abi:
    """Classify C types and compute their layouts for each supported ABI."""

    def __init__(self, api: Api):
        self.api = api
        self.records = api.records_by_name
        self.typedefs = api.typedefs_by_name
        self.enums = {enum.name: enum for enum in api.enums}
        self._layouts: dict[tuple[str, int, int], Layout] = {}

    def classify(self, ctype: CType):
        """Resolve a C type to a Primitive, Record, or Array."""
        if ctype.kind == "pointer":
            return POINTER
        if ctype.kind == "array":
            if ctype.element is None or ctype.length is None:
                raise LayoutError(f"{ctype.spelling}: array needs a fixed extent")
            return Array(self.classify(ctype.element), ctype.length)
        spelling = ctype.spelling.removeprefix("const ").strip()
        if spelling in PORTABLE:
            return PORTABLE[spelling]
        name = ctype.declaration or spelling
        if ctype.kind == "typedef" and name in PORTABLE:
            return PORTABLE[name]
        if ctype.kind == "typedef" and name in self.typedefs:
            return self.classify(self.typedefs[name].type)
        if ctype.kind == "enum" or name in self.enums and ctype.kind != "record":
            enum = self.enums.get(name)
            if enum is None:
                raise LayoutError(f"{name}: enum has no declaration")
            return self.classify(enum.underlying_type)
        if ctype.kind == "record" or name in self.records:
            record = self.records.get(name)
            if record is None or not record.complete:
                raise LayoutError(f"{name}: record has no complete layout")
            return Record(
                name,
                record.kind == "union",
                tuple((f.name, self.classify(f.type)) for f in record.fields),
            )
        canonical = ctype.canonical.removeprefix("const ").strip()
        if canonical in BUILTIN:
            return BUILTIN[canonical]
        # A typedef declared outside the parsed headers, such as a plugin
        # entry point, still has a canonical pointer type.
        if canonical.endswith("*") or "(*)" in canonical:
            return POINTER
        raise LayoutError(f"{ctype.spelling}: no C layout rule")

    def size_align(self, kind, width: int, scalar64_align: int = 8) -> tuple[int, int]:
        if isinstance(kind, Primitive):
            size = kind.size(width)
            return size, min(size, scalar64_align)
        if isinstance(kind, Array):
            size, align = self.size_align(kind.element, width, scalar64_align)
            return size * kind.length, align
        layout = self.record(kind, width, scalar64_align)
        return layout.size, layout.align

    def record(self, kind: Record, width: int, scalar64_align: int = 8) -> Layout:
        key = (kind.name, width, scalar64_align)
        if key not in self._layouts:
            offsets, size, align = {}, 0, 1
            for name, field in kind.fields:
                field_size, field_align = self.size_align(field, width, scalar64_align)
                align = max(align, field_align)
                if kind.union:
                    offsets[name] = 0
                    size = max(size, field_size)
                else:
                    size = -(-size // field_align) * field_align
                    offsets[name] = size
                    size += field_size
            self._layouts[key] = Layout(-(-size // align) * align, align, offsets)
        return self._layouts[key]

    def record_named(self, name: str, width: int, scalar64_align: int = 8) -> Layout:
        kind = self.classify(CType("record", name, "struct " + name, name))
        assert isinstance(kind, Record)
        return self.record(kind, width, scalar64_align)

    def offset(self, record: str, field: str) -> tuple[int, ...]:
        """The byte offset of a field in each supported data model."""
        return tuple(
            self.record_named(record, width, align).offsets[field]
            for width, align in DATA_MODELS
        )

    def size(self, ctype: CType) -> tuple[int, ...]:
        kind = self.classify(ctype)
        return tuple(
            self.size_align(kind, width, align)[0] for width, align in DATA_MODELS
        )

    def align(self, ctype: CType) -> tuple[int, ...]:
        kind = self.classify(ctype)
        return tuple(
            self.size_align(kind, width, align)[1] for width, align in DATA_MODELS
        )


def width_expression(values: tuple[int, ...]) -> str:
    """A Kotlin expression for a value that may differ between ABI layouts."""
    narrow, wide, x86 = values
    if narrow != x86:
        return f"w({narrow}, {wide}, {x86})"
    return str(wide) if narrow == wide else f"w({narrow}, {wide})"
