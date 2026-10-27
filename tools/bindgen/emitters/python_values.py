"""Compile Python record copies and input storage from resolved value plans."""

from __future__ import annotations

import keyword
import re
from dataclasses import replace
from typing import NoReturn

from .. import async_hosts, docs
from ..model import Unsupported
from ..semantic import BoundApi, ValuePlan, enum_member_prefix, required_member

SCALARS = {
    "bool": "bool",
    "_Bool": "bool",
    "double": "f64",
    "float": "f32",
    "int8_t": "i8",
    "uint8_t": "u8",
    "int16_t": "i16",
    "uint16_t": "u16",
    "int32_t": "i32",
    "uint32_t": "u32",
    "int64_t": "i64",
    "uint64_t": "u64",
    "size_t": "usize",
    "intptr_t": "isize",
    "ptrdiff_t": "isize",
    "uintptr_t": "usize",
    "int": "i32",
    "unsigned int": "u32",
    "char": "i8",
}


def enum_type(plan: ValuePlan) -> str:
    """The sys type of an enum value: its C enum, or the typedef that holds a
    set of a bitmask enum's flags."""
    return "sys::" + (plan.ctype.declaration or plan.native)


def scalar_type(plan: ValuePlan) -> str:
    canonical = {
        "unsigned char": "u8",
        "signed char": "i8",
        "unsigned short": "u16",
        "short": "i16",
        "unsigned int": "u32",
        "int": "i32",
        "unsigned long long": "u64",
        "long long": "i64",
        "unsigned long": "std::ffi::c_ulong",
        "long": "std::ffi::c_long",
        "double": "f64",
        "float": "f32",
        "bool": "bool",
        "_Bool": "bool",
    }
    return (
        SCALARS.get(plan.scalar_carrier or plan.native)
        or canonical[plan.scalar_carrier or plan.ctype.canonical]
    )


def field_name(name: str) -> str:
    return name + "_" if keyword.iskeyword(name) else name


def public_name(native: str) -> str:
    return "".join(x.capitalize() for x in native.removeprefix("mln_").split("_"))


def scope_name(native: str) -> str:
    """The class of a callback response, which is valid only during the
    callback."""
    return public_name(native) + "Scope"


def enum_member(plan: ValuePlan, key: str) -> str:
    """The Python name of an enum constant."""
    prefix = enum_member_prefix((known for known, _ in plan.enum_values), plan.native)
    return key.removeprefix(prefix)


def variant_name(parent: ValuePlan, field) -> str:
    """The class of a union member, named after the record that holds the
    union."""
    return public_name(parent.native) + public_name(field.name) + "Variant"


def native_name(plan: ValuePlan) -> str:
    """The class of a registration's native form."""
    return "Native" + public_name(plan.native)


def rust_field(name: str) -> str:
    from .rust import BINDGEN_RESERVED

    return name + "_" if name in BINDGEN_RESERVED else name


def optional(plan: ValuePlan) -> bool:
    return plan.nullable


def ok(copy: str) -> str:
    """The copy as a `PyResult`, which a copy that ends in `?` already is."""
    return copy[:-1] if copy.endswith("?") else f"Ok({copy})"


def optional_copy(present: str, copy: str) -> str:
    """Copies a value that is `None` unless `present` holds."""
    return f"generated_optional(py, {present}, || {ok(copy)})?"


class Values:
    def __init__(self, api: BoundApi):
        self.api = api
        self.records: dict[str, ValuePlan] = {}
        self.inputs: dict[str, ValuePlan] = {}
        self.outputs: set[str] = set()
        # The records that an operation lets its caller omit, which convert
        # None to their zero value.
        self.omittable: set[str] = set()
        self.disposed: set[str] = set()
        self.enums: dict[str, ValuePlan] = {}

    @property
    def symbol(self) -> docs.Symbol:
        """How Python spells the C declarations that its summaries name."""
        from .python import symbols

        return symbols(self.api)

    def fail(self, plan: ValuePlan, reason: str) -> NoReturn:
        raise Unsupported(f"Python: {plan.native}: {reason}")

    def supported(self, plan: ValuePlan, *, input: bool = False) -> None:
        if plan.response:
            self.records[plan.native] = plan
            return
        if plan.kind == "handle" and plan.ownership == "owned":
            # A callback adopts an owned handle into its generated owner.
            return
        if plan.ownership == "owned":
            self.fail(plan, "owned values require a handle adoption transaction")
        if plan.kind in {"scalar", "enum"} and plan.nullable:
            self.fail(plan, "scalar absence requires a C presence representation")
        if plan.kind == "native_pointer":
            return
        if plan.kind == "union":
            if not plan.tag or any(
                not f.presence or not f.presence.variant for f in plan.fields
            ):
                self.fail(plan, "union requires exhaustive tagged variants")
            for field in plan.fields:
                self.supported(field.value, input=input)
            return
        if plan.kind == "scalar":
            scalar_type(plan)
            return
        if plan.kind == "enum":
            self.enums[plan.native] = plan
            return
        if plan.kind == "buffer" and plan.encoding in {"utf8", "bytes", "json"}:
            return
        if plan.kind in {"reference", "array"} and plan.element:
            self.supported(plan.element, input=input)
            return
        if plan.kind != "record":
            self.fail(plan, f"{plan.kind} requires a value adapter")
        if plan.native in self.api.slices:
            # A slice record is presented as its array alone.
            self.supported(self.api.slices[plan.native].value, input=input)
            return
        if plan.registration:
            from . import python_callbacks

            python_callbacks.validate(self, plan)
            self.records[plan.native] = plan
            if input:
                self.inputs[plan.native] = plan
            elif plan.native in self.api.returned:
                self.outputs.add(plan.native)
            return
        names = [field_name(f.name) for f in plan.fields if f.public]
        if len(names) != len(set(names)) or any(n.startswith("_") for n in names):
            self.fail(plan, "record fields collide with generated identifiers")
        for field in plan.fields:
            if not field.public:
                continue
            self.supported(field.value, input=input)
        self.records[plan.native] = plan
        if input:
            self.inputs[plan.native] = plan
        else:
            self.outputs.add(plan.native)

    def type(self, plan: ValuePlan, *, input: bool = False) -> str:
        """The annotation of a value. An input takes any sequence or byte
        buffer, and an output is a tuple or bytes."""
        if plan.kind == "scalar":
            result = (
                "bool"
                if scalar_type(plan) == "bool"
                else "float"
                if scalar_type(plan) in {"f32", "f64"}
                else "int"
            )
        elif plan.native in self.api.slices:
            return self.type(self.api.slices[plan.native].value, input=input)
        elif plan.kind in {"record", "enum"}:
            result = (
                scope_name(plan.native) if plan.response else public_name(plan.native)
            )
            if plan.registration and async_hosts.accepts_native(self.api, plan):
                result += " | " + native_name(plan)
        elif plan.kind == "native_pointer":
            result = "int"
        elif plan.kind == "union":
            # Only a copy from native can hold a tag this binding predates.
            result = " | ".join(
                [self.variant_name(plan, field) for field in plan.fields]
                + (["UnknownVariant"] if plan.native in self.api.returned else [])
            )
            if plan.empty_variant:
                result += " | None"
        elif plan.kind == "buffer" and plan.encoding == "json":
            # Any value admits None, so a JSON value is never also optional.
            return "Any"
        elif plan.kind == "buffer":
            result = (
                "str"
                if plan.encoding == "utf8"
                else "Buffer"
                if input and plan.encoding == "bytes"
                else "bytes"
            )
        elif plan.kind == "reference" and plan.element:
            result = self.type(plan.element, input=input)
        elif plan.kind == "array" and plan.element:
            element = self.type(plan.element, input=input)
            result = f"Sequence[{element}]" if input else f"tuple[{element}, ...]"
        else:
            self.fail(plan, "missing public value type")
        return result + (" | None" if optional(plan) else "")

    def variant_name(self, plan, field):
        parent = next(
            (
                p
                for p in self.records.values()
                if any(f.value.native == plan.native for f in p.fields)
            ),
            None,
        )
        if parent is None:
            self.fail(plan, "union has no containing record")
        return variant_name(parent, field)

    def members(self, plan: ValuePlan):
        for field in plan.fields:
            if not field.public:
                continue
            value = (
                replace(field.value, nullable=True)
                if field.presence and field.presence.mask
                else field.value
            )
            yield field_name(field.name), value, field
        for flag in plan.mask_flags:
            yield flag.member, None, flag

    def copy(self, plan: ValuePlan, expr: str, *, scope: str = "value") -> str:
        """An expression that copies the C value `expr` into a Python object."""
        expr = f"({expr})" if expr.startswith(("*", "unsafe ")) else expr
        if plan.kind == "native_pointer":
            pointer = (
                f"{expr}.map_or(0, |function| function as usize)"
                if "(*)" in plan.ctype.canonical
                or plan.ctype.result
                or (plan.ctype.pointee and plan.ctype.pointee.result)
                else f"{expr} as usize"
            )
            return f"generated_value(py, {pointer})?"
        if plan.kind == "union":
            arms = []
            for field in plan.fields:
                value = self.copy(
                    field.value,
                    f"unsafe {{ {expr}.{rust_field(field.name)} }}",
                    scope=scope,
                )
                arms.append(
                    f'sys::{field.presence.variant} => generated_variant(py, "{field.name}", {value})?'
                )
            return (
                f"match {scope}.{rust_field(plan.tag)} {{ "
                + ", ".join(arms)
                + ", tag => generated_unknown_variant(py, tag)? }"
            )
        if plan.kind in {"scalar", "enum"}:
            return f"generated_value(py, {expr})?"
        if plan.kind == "buffer":
            if plan.ctype.pointee:
                if plan.length != "nul":
                    if not plan.length:
                        self.fail(
                            plan, "pointer buffer needs an explicit copied length"
                        )
                    count = (
                        plan.length
                        if plan.length.isdigit()
                        else f"{scope}.{rust_field(plan.length)}"
                    )
                    view = f"generated_view({expr}, {count})?"
                    return self.copy(
                        replace(plan, ctype=replace(plan.ctype, pointee=None)),
                        view,
                        scope=scope,
                    )
                return f"unsafe {{ generated_c_string(py, {expr}, {str(optional(plan)).lower()}) }}?"
            argument = (
                expr[1:-1] if expr.startswith("(*") and expr.endswith(")") else expr
            )
            body = (
                f"generated_text(py, {argument})?"
                if plan.encoding == "utf8"
                else f"generated_json(py, {argument})?"
                if plan.encoding == "json"
                else f"unsafe {{ generated_bytes(py, {argument}) }}?"
            )
            if optional(plan):
                return optional_copy(f"!{expr}.data.is_null()", body)
            return body
        if plan.native in self.api.slices:
            array = self.api.slices[plan.native]
            return self.copy(
                array.value, f"{expr}.{rust_field(array.name)}", scope=expr
            )
        if plan.kind == "record":
            reference = expr[2:-1] if expr.startswith("(*") else f"&{expr}"
            return f"generated_copy_{plan.native}(py, {reference})?"
        if plan.kind == "reference" and plan.element:
            child = (
                "{ let referenced = unsafe { *"
                + expr
                + " }; "
                + self.copy(plan.element, "referenced", scope=scope)
                + " }"
            )
            if plan.nullable:
                return optional_copy(f"!{expr}.is_null()", child)
            return f'{{ if {expr}.is_null() {{ return Err(native_error("null record pointer")); }} {child} }}'
        if plan.kind == "array" and plan.element:
            count = (
                plan.length
                if (plan.length or "").isdigit()
                else f"{scope}.{rust_field(plan.length or 'count')}"
            )
            values = (
                f"&{expr}"
                if plan.ctype.kind == "array"
                else f"unsafe {{ generated_slice({expr}, {count})? }}"
            )
            if plan.stride:
                values = f"unsafe {{ generated_strided_values({expr}, {count}, {scope}.{rust_field(plan.stride)})? }}"
            item = self.copy(
                plan.element, "element" if plan.stride else "*element", scope=scope
            )
            body = f"generated_list(py, {values}, |element| {ok(item)})?"
            if plan.nullable and plan.ctype.kind != "array":
                body = optional_copy(f"!{expr}.is_null()", body)
            return body
        self.fail(plan, "missing output copy")

    def facade_copy(self, plan: ValuePlan, expr: str) -> str:
        bare = replace(plan, nullable=False)
        if optional(plan):
            copied = self.facade_copy(bare, expr)
            if copied == expr:
                return expr
            # A record that presents its union copies through _copy_variant,
            # which takes more than the value, so it can't pass as a callable.
            if (
                bare.kind in {"record", "enum"}
                and not bare.response
                and not bare.union_member
            ):
                return f"_maybe({copied.removesuffix(f'({expr})')}, {expr})"
            return f"None if {expr} is None else ({copied})"
        if plan.kind == "union":
            variants = ", ".join(
                f"{field.name!r}: {self.variant_name(plan, field)}"
                for field in plan.fields
            )
            return f"_copy_variant({expr}, {{{variants}}}, {plan.empty_variant[1] if plan.empty_variant else None!r})"
        if plan.response:
            return f"_wrap_response({expr}, {public_name(plan.native) + 'Scope'!r})"
        if plan.native in self.api.slices:
            return self.facade_copy(self.api.slices[plan.native].value, expr)
        if plan.union_member:
            return self.facade_copy(plan.union_member.value, expr)
        if plan.kind == "record":
            return f"{public_name(plan.native)}._from_native({expr})"
        if plan.kind == "enum":
            return f"{public_name(plan.native)}({expr})"
        if plan.kind == "array" and plan.element:
            return f"tuple({self.facade_copy(plan.element, 'item')} for item in {expr})"
        if plan.kind == "reference" and plan.element:
            return self.facade_copy(plan.element, expr)
        return expr

    def input(
        self, plan: ValuePlan, expr: str, *, scope: str = "raw", member: bool = False
    ) -> str:
        """Converts the Python value `expr` to C. A JSON value serializes
        through `json.dumps`, except that None in a record member, `member`,
        is absent and converts to an empty view."""
        if plan.kind == "native_pointer":
            pointer = f"{expr}.extract::<usize>()?"
            if (
                "(*)" in plan.ctype.canonical
                or plan.ctype.result
                or (plan.ctype.pointee and plan.ctype.pointee.result)
            ):
                return f"{{ let address = {pointer}; if address == 0 {{ None }} else {{ Some(unsafe {{ std::mem::transmute::<usize, _>(address) }}) }} }}"
            return f"{pointer} as _"
        if plan.kind == "scalar":
            return f"{expr}.extract::<{scalar_type(plan)}>()?"
        if plan.kind == "enum":
            return f"{expr}.extract::<{enum_type(plan)}>()?"
        if plan.kind == "buffer" and plan.encoding == "json":
            if plan.ctype.pointee:
                self.fail(plan, "JSON requires a view")
            return f"storage.json({expr}, {str(member).lower()})?"
        if plan.kind == "buffer":
            if plan.length == "nul":
                self.fail(plan, "terminated text needs a view")
            converted = (
                f"storage.buffer({expr}, {str(plan.encoding == 'utf8').lower()})?"
            )
            if plan.ctype.pointee:
                count = (
                    f"{scope}.{rust_field(plan.length)} = generated_length(buffer.size)?;"
                    if plan.length and not plan.length.isdigit()
                    else ""
                )
                return f"{{ let buffer = {converted}; {count} buffer.data.cast() }}"
            return converted
        if plan.kind == "record":
            return f"generated_input_{plan.native}(&{expr}, storage)?"
        if plan.kind == "array" and plan.element:
            inner = self.input(plan.element, "item", scope=scope)
            if plan.ctype.kind == "array" or (plan.length and plan.length.isdigit()):
                self.fail(plan, "fixed-length array input")
            check = f"{scope}.{rust_field(plan.length or 'count')} = generated_length(items.len())?;"
            body = f"{{ let items = generated_items(&{expr}, |item| {ok(inner)})?; {check} storage.keep_array(items) }}"
            if plan.nullable:
                return f"if {expr}.is_none() {{ std::ptr::null() }} else {{ {body} }}"
            return body
        self.fail(plan, "missing input conversion")

    def input_lines(self, plan):
        name = plan.native
        lines = [f"let mut raw: sys::{name} = unsafe {{ std::mem::zeroed() }};"]
        for field in plan.fields:
            if field.role == "size":
                lines.append(
                    f"raw.{rust_field(field.name)} = std::mem::size_of::<sys::{name}>() as _;"
                )
            elif field.role in {"reserved", "presence_mask"}:
                lines.append(
                    f"raw.{rust_field(field.name)} = {'false' if field.value.ctype.canonical in {'bool', '_Bool'} else '0'};"
                )
        if name in self.omittable:
            lines.append("if value.is_none() { return Ok(raw); }")
        for member, value, field in self.members(plan):
            # A record of only a union is presented as the selected variant.
            lines.append(
                "let field = value;"
                if plan.union_member
                else f'let field = value.getattr("{member}")?;'
            )
            if value is not None and value.kind == "union":
                arms = []
                for variant in value.fields:
                    converted = self.input(variant.value, 'field.getattr("value")?')
                    arms.append(
                        f"sys::{variant.presence.variant} => {{ raw.{rust_field(field.name)}.{rust_field(variant.name)} = {converted}; }}"
                    )
                tag = 'field.getattr("_tag")?.extract::<u32>()?'
                lines.append(f"let tag = {tag};")
                lines.append("match tag { " + ", ".join(arms) + ", _ => {} }")
                lines.append(f"raw.{rust_field(value.tag)} = tag;")
                continue
            if value is None:
                lines.append(
                    f"if field.extract::<bool>()? {{ raw.{rust_field(field.mask)} |= sys::{field.name}; }}"
                )
                continue
            statements = [
                f"raw.{rust_field(field.name)} = {self.input(field.value, 'field', member=True)};"
            ]
            if field.presence and field.presence.mask:
                statements.append(
                    f"raw.{rust_field(field.presence.mask)} |= sys::{field.presence.bit};"
                )
                lines[-1] = (
                    f'if let Some(field) = generated_present(value, "{member}")? {{ '
                    + " ".join(statements)
                    + " }"
                )
            elif (
                len(statements) == 1
                and len(re.findall(r"\bfield\b", statements[0])) == 1
            ):
                lines[-1] = re.sub(
                    r"\bfield\b", f'value.getattr("{member}")?', statements[0]
                )
            else:
                lines.extend(statements)
        return lines

    def input_sources(self) -> str:
        functions = []
        for name, plan in sorted(self.inputs.items()):
            if name not in self.api.inputs:
                continue
            if plan.registration:
                from . import python_callbacks

                functions.append(python_callbacks.input_source(plan, self))
                continue
            lines = self.input_lines(plan)
            if not any("storage" in line for line in lines):
                lines.insert(0, "let _ = storage;")
            functions.append(
                f"fn generated_input_{name}<'py>(value: &Bound<'py, PyAny>, storage: &mut GeneratedInputStorage<'py>) -> PyResult<sys::{name}> {{\n    "
                + "\n    ".join(lines)
                + "\n    Ok(raw)\n}\n"
            )
        return "\n".join(functions)

    def sources(self) -> tuple[str, str]:
        rust, python = [], []
        python.append(
            '@dataclass(frozen=True, slots=True)\nclass UnknownVariant:\n    tag: int\n\n    @property\n    def _tag(self):\n        return self.tag\n\ndef _copy_variant(raw, variants, empty=None):\n    if raw["kind"] is None:\n        return None if raw["tag"] == empty else UnknownVariant(raw["tag"])\n    return variants[raw["kind"]]._from_native(raw["value"])\n\ndef _maybe(convert, raw):\n    return None if raw is None else convert(raw)\n\ndef _sequence(value):\n    if value is None or type(value) is tuple:\n        return value\n    if isinstance(value, str):\n        raise TypeError("expected a sequence, not str")\n    return tuple(value)\n\ndef _bytes(value):\n    return value if value is None or type(value) is bytes else memoryview(value).tobytes()\n'
        )
        for parent in self.records.values():
            for union in (
                field.value for field in parent.fields if field.value.kind == "union"
            ):
                for field in union.fields:
                    variant_name = self.variant_name(union, field)
                    tag = self.api.enum_number(field.presence.variant)
                    python.append(
                        f"@dataclass(frozen=True, slots=True)\nclass {variant_name}:\n"
                        + docs.docstring(
                            self.api.doc(f"{union.native}.{field.name}"),
                            self.symbol,
                            "    ",
                        )
                        + f"    value: {self.type(field.value)}\n    _tag = {tag}\n\n    @classmethod\n    def _from_native(cls, raw):\n        return cls({self.facade_copy(field.value, 'raw')})\n"
                    )
        for name, plan in sorted(self.enums.items()):
            body = "\n".join(
                f"    {enum_member(plan, key)} = {value}"
                for key, value in plan.enum_values
            )
            python.append(
                f"class {public_name(name)}({'IntFlag' if plan.enum_kind == 'bitmask' else 'UnknownIntEnum'}):\n"
                + docs.docstring(self.api.doc(name), self.symbol, "    ")
                + f"{body or '    pass'}\n"
            )
        for name, plan in sorted(self.records.items()):
            if plan.response:
                continue
            if plan.registration:
                from . import python_callbacks

                callback_rust, callback_python = python_callbacks.sources(self, plan)
                rust.append(callback_rust)
                python.append(callback_python)
                continue
            record_rust, record_python = self.record_sources(plan)
            rust.append(record_rust)
            python.append(record_python)
        from . import python_callbacks

        python.append(python_callbacks.protocol_sources(self))
        return "\n".join(rust) + "\n" + self.input_sources(), "\n".join(python)

    def union_member_sources(self, plan, *, copied=True):
        """The Rust copy and the Python alias of a record of only a union,
        which is presented as the union's variants under the record's name."""
        name = plan.native
        python = (
            f"type {public_name(name)} = {self.type(plan.union_member.value)}\n"
            + docs.docstring(self.api.doc(name), self.symbol, "")
        )
        if not copied or name not in self.outputs:
            return "", python
        copy = self.copy(
            plan.union_member.value,
            "value." + rust_field(plan.union_member.name),
        )
        rust = (
            f"fn generated_copy_{name}(py: Python<'_>, value: &sys::{name}) -> PyResult<Py<PyAny>> {{\n"
            f"    Ok({copy})\n}}\n"
        )
        return rust, python

    def zero(self, plan: ValuePlan, *, input: bool) -> str:
        """The default of a member that a host may omit: its zero value."""
        if optional(plan) or plan.kind == "buffer" and plan.encoding == "json":
            return " = None"
        if plan.kind == "scalar":
            return {"bool": " = False", "float": " = 0.0"}.get(self.type(plan), " = 0")
        if plan.kind == "native_pointer":
            return " = 0"
        if plan.kind == "enum":
            zero = next((key for key, value in plan.enum_values if value == 0), None)
            if zero is None:
                # A set of flags with none of them set.
                return (
                    f" = field(default_factory=lambda: {public_name(plan.native)}(0))"
                )
            return f" = {public_name(plan.native)}.{enum_member(plan, zero)}"
        if plan.kind == "buffer":
            return ' = b""' if plan.encoding == "bytes" else ' = ""'
        if plan.kind == "array":
            return " = ()"
        if plan.kind == "union" and plan.empty_variant:
            return " = None"
        if plan.kind == "record":
            # The lambda defers the name, whose class may come later in the
            # module.
            return f" = field(default_factory=lambda: {public_name(plan.native)}())"
        self.fail(plan, "missing zero value")

    def record_sources(
        self,
        plan,
        *,
        extra_fields=(),
        extra_methods="",
        copied=True,
        registration=False,
        required=(),
    ):
        """The Rust copy and the Python class of one record.

        A record that native never returns, `copied=False`, has neither copy.
        A copy holds no callbacks, so the copy of a registration takes its
        `required` callbacks.
        """
        name = plan.native
        if plan.union_member:
            return self.union_member_sources(plan, copied=copied)
        rust, python = [], []
        fields, copies, public_copies = list(extra_fields), [], []
        # An input record copies a sequence into a tuple and a byte buffer into
        # bytes, so it stays immutable and hashable.
        input = name in self.api.inputs
        normalized = []
        for member, value, field in self.members(plan):
            if value is None:
                fields.append(f"    {member}: bool = False")
                copies.append(
                    f'dict.set_item("{member}", value.{rust_field(field.mask)} & sys::{field.name} != 0)?;'
                )
                public_copies.append(f'{member}=raw["{member}"]')
                continue
            public_type = self.type(value, input=input)
            if input and value.kind == "array":
                normalized.append((member, "_sequence"))
            elif input and value.kind == "buffer" and value.encoding == "bytes":
                normalized.append((member, "_bytes"))
            default = (
                self.zero(value, input=input)
                if (input or registration) and not required_member(field)
                else ""
            )
            fields.append(
                f"    {member}: {public_type}{default}\n"
                + docs.docstring(
                    self.api.doc(f"{name}.{field.name}"), self.symbol, "    "
                )
            )
            if value.registration:
                # Native never returns callbacks, so a copy leaves them unset.
                unset = "None" if optional(value) else public_name(value.native) + "()"
                public_copies.append(f"{member}={unset}")
                continue
            copy = self.copy(field.value, "value." + rust_field(field.name))
            if field.presence and field.presence.mask:
                present = f"value.{rust_field(field.presence.mask)} & sys::{field.presence.bit} != 0"
                copy = optional_copy(present, copy)
            copies.append(f'dict.set_item("{member}", {copy})?;')
            public_copies.append(
                f"{member}={self.facade_copy(value, f'raw[{member!r}]')}"
            )
        python.append(
            f"@dataclass(frozen=True, slots=True)\nclass {public_name(name)}:\n"
            + docs.docstring(self.api.doc(name), self.symbol, "    ")
            + "\n".join(sorted(fields, key=lambda line: " = " in line) or ["    pass"])
            + "\n"
        )
        if normalized:
            python[-1] += "\n    def __post_init__(self):\n" + "".join(
                f'        object.__setattr__(self, "{member}", {helper}(self.{member}))\n'
                for member, helper in normalized
            )
        taken = "".join(f", {member}" for member in required)
        if copied:
            python[-1] += (
                f"\n    @classmethod\n    def _from_native(cls, raw{taken}):\n        return cls("
                + ", ".join(
                    [*(f"{member}={member}" for member in required), *public_copies]
                )
                + ")\n"
            )
        python[-1] += extra_methods
        if not copied or name not in self.outputs:
            return "", "\n".join(python)
        rust.append(
            f"fn generated_copy_{name}(py: Python<'_>, value: &sys::{name}) -> PyResult<Py<PyAny>> {{\n    let dict = PyDict::new(py);\n    "
            + "\n    ".join(copies)
            + "\n    Ok(dict.into_any().unbind())\n}\n"
        )
        return "\n".join(rust), "\n".join(python)
