"""Compile Python record copies and input storage from resolved value plans."""

from __future__ import annotations

import keyword
import os
import re
from dataclasses import replace
from typing import NoReturn

from .. import docs
from ..model import ModelError
from ..semantic import BoundApi, ValuePlan

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


def rust_field(name: str) -> str:
    from .rust import BINDGEN_RESERVED

    return name + "_" if name in BINDGEN_RESERVED else name


def optional(plan: ValuePlan) -> bool:
    return plan.nullable or plan.optional == "empty"


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
        self.defaults: set[str] = set()
        # Handle types whose generated disposer an operation calls.
        self.disposed: set[str] = set()
        self.enums: dict[str, ValuePlan] = {}
        self.item_buffers = {}

    def fail(self, plan: ValuePlan, reason: str) -> NoReturn:
        raise ModelError([f"Python: {plan.native}: {reason}"])

    def supported(self, plan: ValuePlan, *, input: bool = False) -> None:
        if plan.response:
            self.records[plan.native] = plan
            return
        if plan.ownership == "owned":
            self.fail(plan, "owned values require a handle adoption transaction")
        if plan.kind in {"scalar", "enum"} and (plan.nullable or plan.optional):
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
            if plan.item_buffer:
                self.item_buffers[plan.element.native] = plan.item_buffer
            self.supported(plan.element, input=input)
            return
        if plan.kind != "record":
            self.fail(plan, f"{plan.kind} requires a value adapter")
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

    def type(self, plan: ValuePlan) -> str:
        if plan.kind == "scalar":
            result = (
                "bool"
                if scalar_type(plan) == "bool"
                else "float"
                if scalar_type(plan) in {"f32", "f64"}
                else "int"
            )
        elif plan.kind in {"record", "enum"}:
            result = public_name(plan.native) + ("Scope" if plan.response else "")
        elif plan.kind == "native_pointer":
            result = "int"
        elif plan.kind == "union":
            result = (
                " | ".join(self.variant_name(plan, field) for field in plan.fields)
                + " | UnknownVariant"
            )
            if plan.empty_variant:
                result += " | None"
        elif plan.kind == "buffer":
            result = "str" if plan.encoding == "utf8" else "bytes"
        elif plan.kind == "reference" and plan.element:
            result = self.type(plan.element)
        elif plan.kind == "array" and plan.element:
            result = f"tuple[{self.type(plan.element)}, ...]"
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
        return public_name(parent.native) + public_name(field.name) + "Variant"

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
                else f"unsafe {{ generated_bytes(py, {argument}) }}?"
            )
            if optional(plan):
                present = (
                    f"{expr}.size != 0"
                    if plan.optional == "empty"
                    else f"!{expr}.data.is_null()"
                )
                return optional_copy(present, body)
            return body
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
            if plan.item_buffer:
                arena = plan.item_buffer
                content = f"generated_arena_string({scope}.{rust_field(arena.data)}, {scope}.{rust_field(arena.size)}, element.{rust_field(arena.offset)}, element.{rust_field(arena.length)})?"
                item = f'{{ let item = {item}; item.bind(py).cast::<PyDict>()?.set_item("{arena.field}", unsafe {{ {content} }})?; item }}'
            body = f"generated_list(py, {values}, |element| {ok(item)})?"
            if plan.nullable and plan.ctype.kind != "array":
                body = optional_copy(f"!{expr}.is_null()", body)
            return body
        self.fail(plan, "missing output copy")

    def facade_copy(self, plan: ValuePlan, expr: str) -> str:
        bare = replace(plan, nullable=False, optional=None)
        if optional(plan):
            copied = self.facade_copy(bare, expr)
            if copied == expr:
                return expr
            if bare.kind in {"record", "enum"} and not bare.response:
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
        if plan.kind == "record":
            return f"{public_name(plan.native)}._from_native({expr})"
        if plan.kind == "enum":
            return f"{public_name(plan.native)}({expr})"
        if plan.kind == "array" and plan.element:
            return f"tuple({self.facade_copy(plan.element, 'item')} for item in {expr})"
        if plan.kind == "reference" and plan.element:
            return self.facade_copy(plan.element, expr)
        return expr

    def input(self, plan: ValuePlan, expr: str, *, scope: str = "raw") -> str:
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
            return f"{expr}.extract::<sys::{plan.native}>()?"
        if plan.kind == "buffer":
            if plan.length == "nul":
                converted = f"storage.c_string({expr})?"
                return (
                    f"if {expr}.is_none() {{ std::ptr::null() }} else {{ {converted} }}"
                    if optional(plan)
                    else converted
                )
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
            # A record with a native default converts None to that default.
            return f"generated_input_{plan.native}(&{expr}, storage)?"
        if plan.kind == "reference" and plan.element:
            inner = self.input(plan.element, expr, scope=scope)
            body = f"{{ let value = {inner}; storage.keep_one(value) }}"
            return (
                f"if {expr}.is_none() {{ std::ptr::null() }} else {{ {body} }}"
                if plan.nullable
                else body
            )
        if plan.kind == "array" and plan.element:
            inner = self.input(plan.element, "item", scope=scope)
            if plan.ctype.kind == "array":
                return f'generated_items(&{expr}, |item| {ok(inner)})?.try_into().map_err(|_| invalid_argument_error("wrong fixed array length"))?'
            if plan.length and plan.length.isdigit():
                check = f'if items.len() != {plan.length} {{ return Err(invalid_argument_error("wrong fixed array length")); }}'
            else:
                check = f"{scope}.{rust_field(plan.length or 'count')} = generated_length(items.len())?;"
            body = f"{{ let items = generated_items(&{expr}, |item| {ok(inner)})?; {check} storage.keep_array(items) }}"
            if plan.nullable:
                return f"if {expr}.is_none() {{ std::ptr::null() }} else {{ {body} }}"
            return body
        self.fail(plan, "missing input conversion")

    def input_lines(self, plan):
        name = plan.native
        init = (
            f"unsafe {{ sys::{plan.default}() }}"
            if plan.default
            else "unsafe { std::mem::zeroed() }"
        )
        lines = [f"let mut raw: sys::{name} = {init};"]
        if plan.default:
            lines.insert(
                0,
                "if value.is_none() { return Ok(unsafe { sys::"
                + plan.default
                + "() }); }",
            )
        for field in plan.fields:
            if field.role == "size":
                lines.append(
                    f"raw.{rust_field(field.name)} = std::mem::size_of::<sys::{name}>() as _;"
                )
            elif field.role in {"reserved", "presence_mask"}:
                lines.append(
                    f"raw.{rust_field(field.name)} = {'false' if field.value.ctype.canonical in {'bool', '_Bool'} else '0'};"
                )
        for member, value, field in self.members(plan):
            lines.append(f'let field = value.getattr("{member}")?;')
            if value is not None and value.kind == "union":
                arms = []
                for variant in value.fields:
                    converted = self.input(variant.value, 'field.getattr("value")?')
                    arms.append(
                        f"sys::{variant.presence.variant} => {{ raw.{rust_field(field.name)}.{rust_field(variant.name)} = {converted}; }}"
                    )
                tag = 'field.getattr("_tag")?.extract::<u32>()?'
                if value.empty_variant:
                    tag = f"if field.is_none() {{ sys::{value.empty_variant[0]} }} else {{ {tag} }}"
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
                f"raw.{rust_field(field.name)} = {self.input(field.value, 'field')};"
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
                # A field read once reads its attribute in place.
                lines[-1] = re.sub(
                    r"\bfield\b", f'value.getattr("{member}")?', statements[0]
                )
            else:
                lines.extend(statements)
        return lines

    def input_sources(self) -> str:
        functions = []
        for name, plan in sorted(self.inputs.items()):
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
            '@dataclass(frozen=True, slots=True)\nclass UnknownVariant:\n    tag: int\n\n    @property\n    def _tag(self):\n        return self.tag\n\ndef _copy_variant(raw, variants, empty=None):\n    if raw["kind"] is None:\n        return None if raw["tag"] == empty else UnknownVariant(raw["tag"])\n    return variants[raw["kind"]]._from_native(raw["value"])\n\ndef _maybe(convert, raw):\n    return None if raw is None else convert(raw)\n'
        )
        for parent in self.records.values():
            for union in (
                field.value for field in parent.fields if field.value.kind == "union"
            ):
                for field in union.fields:
                    variant_name = self.variant_name(union, field)
                    tag = next(
                        number
                        for p in parent.fields
                        if p.name == union.tag
                        for key, number in p.value.enum_values
                        if key == field.presence.variant
                    )
                    python.append(
                        f"@dataclass(frozen=True, slots=True)\nclass {variant_name}:\n    value: {self.type(field.value)}\n    _tag = {tag}\n\n    @classmethod\n    def _from_native(cls, raw):\n        return cls({self.facade_copy(field.value, 'raw')})\n"
                    )
        for name, plan in sorted(self.enums.items()):
            prefix = (
                os.path.commonprefix([key for key, _ in plan.enum_values]).rsplit(
                    "_", 1
                )[0]
                + "_"
            )
            body = "\n".join(
                f"    {key.removeprefix(prefix)} = {value}"
                for key, value in plan.enum_values
            )
            python.append(
                f"class {public_name(name)}({'IntFlag' if plan.enum_kind == 'bitmask' else 'UnknownIntEnum'}):\n"
                + docs.docstring(self.api.doc(name), "    ")
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
        return "\n".join(rust) + "\n" + self.input_sources(), "\n".join(python)

    def record_sources(
        self,
        plan,
        *,
        extra_fields=(),
        extra_methods="",
        copied=True,
    ):
        """The Rust copy and the Python class of one record.

        A record that native never returns, `copied=False`, has neither copy.
        """
        name = plan.native
        rust, python = [], []
        fields, copies, public_copies = list(extra_fields), [], []
        for member, value, field in self.members(plan):
            arena = self.item_buffers.get(name)
            if arena and member in {arena.offset, arena.length}:
                continue
            if value is None:
                fields.append(f"    {member}: bool = False")
                copies.append(
                    f'dict.set_item("{member}", value.{rust_field(field.mask)} & sys::{field.name} != 0)?;'
                )
                public_copies.append(f'{member}=raw["{member}"]')
                continue
            public_type = self.type(value)
            if optional(value):
                default = " = None"
            elif value.registration:
                # A registration without presence defaults to a disabled one,
                # which is what the record's native default holds. The lambda
                # defers the name, whose class may come later in the module.
                default = (
                    f" = field(default_factory=lambda: {public_name(value.native)}())"
                )
            else:
                default = ""
            fields.append(f"    {member}: {public_type}{default}")
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
        if arena := self.item_buffers.get(name):
            fields.append(f"    {arena.field}: str")
            public_copies.append(f"{arena.field}=raw[{arena.field!r}]")
        python.append(
            f"@dataclass(frozen=True, slots=True)\nclass {public_name(name)}:\n"
            + docs.docstring(self.api.doc(name), "    ")
            + "\n".join(sorted(fields, key=lambda line: " = " in line) or ["    pass"])
            + "\n"
        )
        if copied:
            python[-1] += (
                "\n    @classmethod\n    def _from_native(cls, raw):\n        return cls("
                + ", ".join(public_copies)
                + ")\n"
            )
        python[-1] += extra_methods
        if name in self.defaults:
            python[-1] += (
                f"\n    @classmethod\n    def default(cls):\n        from . import _native\n        return cls._from_native(_native._default_{name.removeprefix('mln_')}())\n"
            )
        if not copied or name not in self.outputs:
            return "", "\n".join(python)
        rust.append(
            f"fn generated_copy_{name}(py: Python<'_>, value: &sys::{name}) -> PyResult<Py<PyAny>> {{\n    let dict = PyDict::new(py);\n    "
            + "\n    ".join(copies)
            + "\n    Ok(dict.into_any().unbind())\n}\n"
        )
        return "\n".join(rust), "\n".join(python)
