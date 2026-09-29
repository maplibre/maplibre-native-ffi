"""Compile Python record copies and input storage from resolved value plans."""

from __future__ import annotations

import keyword
import os
from dataclasses import replace
from typing import NoReturn

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

    return ".".join(
        part + "_" if part in BINDGEN_RESERVED else part for part in name.split(".")
    )


def optional(plan: ValuePlan) -> bool:
    return plan.nullable or plan.optional == "empty"


class Values:
    def __init__(self, api: BoundApi):
        self.api = api
        self.records: dict[str, ValuePlan] = {}
        self.inputs: dict[str, ValuePlan] = {}
        self.outputs: set[str] = set()
        self.defaults: set[str] = set()
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
            else:
                self.outputs.add(plan.native)
            return
        names = [field_name(f.name) for f in plan.fields if f.role == "value"]
        if len(names) != len(set(names)) or any(n.startswith("_") for n in names):
            self.fail(plan, "record fields collide with generated identifiers")
        for field in plan.fields:
            if field.role in {
                "size",
                "reserved",
                "count",
                "stride",
                "arena",
                "presence_mask",
            }:
                continue
            self.supported(field.value, input=input)
        for group in plan.presence_groups:
            if len(group.fields) > 1:
                grouped = self.group_value(plan, group)
                self.records[grouped.native] = grouped
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

    def group_value(self, plan, group):
        if group.type:
            return self.api.values[group.type]
        prefix = os.path.commonprefix(group.fields).rsplit("_", 1)[0] + "_"
        fields = tuple(
            replace(
                next(f for f in plan.fields if f.name == name),
                name=name.removeprefix(prefix),
                presence=None,
            )
            for name in group.fields
        )
        return ValuePlan(
            kind="record",
            native=plan.native + "_" + group.mask.removeprefix("has_"),
            ctype=plan.ctype,
            fields=fields,
        )

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
        grouped = {
            name: group
            for group in plan.presence_groups
            if len(group.fields) > 1
            for name in group.fields
        }
        for field in plan.fields:
            if field.role in {
                "size",
                "reserved",
                "count",
                "stride",
                "arena",
                "presence_mask",
                "tag",
            }:
                continue
            if field.name in grouped:
                group = grouped[field.name]
                if field.name != group.fields[0]:
                    continue
                # The group bit is the semantic name; common prefix belongs to the mask domain.
                bits = [g.bit for g in plan.presence_groups if g.bit]
                prefix = os.path.commonprefix(bits).rsplit("_", 1)[0] + "_"
                name = (
                    group.bit.removeprefix(prefix).lower()
                    if group.bit
                    else group.mask.removeprefix("has_")
                )
                yield (
                    name,
                    replace(self.group_value(plan, group), nullable=True),
                    field,
                    group,
                )
            else:
                value = (
                    replace(field.value, nullable=True)
                    if field.presence and field.presence.mask
                    else field.value
                )
                yield field_name(field.name), value, field, None
        for flag in plan.mask_flags:
            bits = [g.bit for g in plan.presence_groups if g.bit] + [
                f.name for f in plan.mask_flags
            ]
            prefix = os.path.commonprefix(bits).rsplit("_", 1)[0] + "_"
            yield flag.name.removeprefix(prefix).lower(), None, flag, None

    def copy(self, plan: ValuePlan, expr: str, *, scope: str = "value") -> str:
        expr = f"({expr})" if expr.startswith(("*", "unsafe ")) else expr
        if plan.kind == "native_pointer":
            pointer = (
                f"{expr}.map_or(0, |function| function as usize)"
                if "(*)" in plan.ctype.canonical
                or plan.ctype.result
                or (plan.ctype.pointee and plan.ctype.pointee.result)
                else f"{expr} as usize"
            )
            return (
                f"pyo3::BoundObject::unbind(({pointer}).into_pyobject(py)?).into_any()"
            )
        if plan.kind == "union":
            arms = []
            for field in plan.fields:
                value = self.copy(
                    field.value,
                    f"unsafe {{ {expr}.{rust_field(field.name)} }}",
                    scope=scope,
                )
                arms.append(
                    f'sys::{field.presence.variant} => {{ let variant = PyDict::new(py); variant.set_item("kind", "{field.name}")?; variant.set_item("value", {value})?; variant.into_any().unbind() }}'
                )
            return (
                f"match {scope}.{rust_field(plan.tag)} {{ "
                + ", ".join(arms)
                + ', tag => { let variant = PyDict::new(py); variant.set_item("kind", py.None())?; variant.set_item("tag", tag)?; variant.into_any().unbind() } }'
            )
        if plan.kind in {"scalar", "enum"}:
            return f"pyo3::BoundObject::unbind(({expr}).into_pyobject(py)?).into_any()"
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
                    view = f"sys::mln_buffer_view {{ data: {expr}.cast(), size: {count} as usize }}"
                    return self.copy(
                        replace(plan, ctype=replace(plan.ctype, pointee=None)),
                        view,
                        scope=scope,
                    )
                text = f'unsafe {{ std::ffi::CStr::from_ptr({expr}) }}.to_str().map_err(|_| native_error("native string is not UTF-8"))?.into_pyobject(py)?.into_any().unbind()'
                if optional(plan):
                    return f"if {expr}.is_null() {{ py.None() }} else {{ {text} }}"
                return f'{{ if {expr}.is_null() {{ return Err(native_error("null native string")); }} {text} }}'
            argument = (
                expr[1:-1] if expr.startswith("(*") and expr.endswith(")") else expr
            )
            if plan.encoding == "utf8":
                body = f"copied_string_view({argument})?.into_pyobject(py)?.into_any().unbind()"
            else:
                body = f"PyBytes::new(py, unsafe {{ generated_slice({expr}.data.cast::<u8>(), {expr}.size)? }}).into_any().unbind()"
            if optional(plan):
                absent = (
                    f"{expr}.size == 0"
                    if plan.optional == "empty"
                    else f"{expr}.data.is_null()"
                )
                return f"if {absent} {{ py.None() }} else {{ {body} }}"
            return body
        if plan.kind == "record":
            return f"generated_copy_{plan.native}(py, &{expr})?"
        if plan.kind == "reference" and plan.element:
            child = (
                "{ let referenced = unsafe { *"
                + expr
                + " }; "
                + self.copy(plan.element, "referenced", scope=scope)
                + " }"
            )
            if plan.nullable:
                return f"if {expr}.is_null() {{ py.None() }} else {{ {child} }}"
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
                else f"unsafe {{ generated_slice({expr}, {count} as usize)? }}"
            )
            if plan.stride:
                values = f"unsafe {{ generated_strided_values({expr}, {count} as usize, {scope}.{rust_field(plan.stride)} as usize)? }}"
            item = self.copy(
                plan.element, "element" if plan.stride else "*element", scope=scope
            )
            enrichment = ""
            if plan.item_buffer:
                arena = plan.item_buffer
                content = f"generated_arena_string({scope}.{rust_field(arena.data)}.cast(), {scope}.{rust_field(arena.size)} as usize, element.{rust_field(arena.offset)} as usize, element.{rust_field(arena.length)} as usize)?"
                enrichment = f'item.bind(py).cast::<PyDict>()?.set_item("{arena.field}", unsafe {{ {content} }})?;'
            body = f"{{ let items = PyList::empty(py); for element in {values} {{ let item = {item}; {enrichment} items.append(item)?; }} items.into_any().unbind() }}"
            if plan.nullable and plan.ctype.kind != "array":
                body = f"if {expr}.is_null() {{ py.None() }} else {{ {body} }}"
            return body
        self.fail(plan, "missing output copy")

    def facade_copy(self, plan: ValuePlan, expr: str) -> str:
        bare = replace(plan, nullable=False, optional=None)
        if optional(plan):
            return f"None if {expr} is None else ({self.facade_copy(bare, expr)})"
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
                    f'{scope}.{rust_field(plan.length)} = buffer.size.try_into().map_err(|_| pyo3::exceptions::PyOverflowError::new_err("buffer length exceeds native count"))?;'
                    if plan.length and not plan.length.isdigit()
                    else ""
                )
                return f"{{ let buffer = {converted}; {count} buffer.data.cast() }}"
            return converted
        if plan.kind == "record":
            conversion = f"generated_input_{plan.native}(&{expr}, storage)?"
            if plan.default:
                return f"if {expr}.is_none() {{ unsafe {{ sys::{plan.default}() }} }} else {{ {conversion} }}"
            return conversion
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
                return f'{{ let mut items = Vec::new(); for item in {expr}.try_iter()? {{ let item = item?; items.push({inner}); }} items.try_into().map_err(|_| invalid_argument_error("wrong fixed array length"))? }}'
            if plan.length and plan.length.isdigit():
                check = f'if items.len() != {plan.length} {{ return Err(invalid_argument_error("wrong fixed array length")); }}'
            else:
                check = f'{scope}.{rust_field(plan.length or "count")} = items.len().try_into().map_err(|_| pyo3::exceptions::PyOverflowError::new_err("array length exceeds native count"))?;'
            body = f"{{ let mut items = Vec::new(); for item in {expr}.try_iter()? {{ let item = item?; items.push({inner}); }} {check} storage.keep_array(items) }}"
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
        for field in plan.fields:
            if field.role == "size":
                lines.append(
                    f"raw.{rust_field(field.name)} = std::mem::size_of::<sys::{name}>() as _;"
                )
            elif field.role in {"reserved", "presence_mask"}:
                lines.append(
                    f"raw.{rust_field(field.name)} = {'false' if field.value.ctype.canonical in {'bool', '_Bool'} else '0'};"
                )
        for member, value, field, group in self.members(plan):
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
            statements = []
            if group:
                gp = self.group_value(plan, group)
                for child, dest in zip(
                    [f for f in gp.fields if f.role == "value"],
                    group.fields,
                    strict=True,
                ):
                    statements.append(
                        f"raw.{rust_field(dest)} = {self.input(child.value, f'field.getattr({child.name!r})?')};".replace(
                            "'", '"'
                        )
                    )
            else:
                statements.append(
                    f"raw.{rust_field(field.name)} = {self.input(field.value, 'field')};"
                )
            if field.presence and field.presence.mask:
                statements.append(
                    f"raw.{rust_field(field.presence.mask)} |= sys::{field.presence.bit};"
                    if field.presence.bit
                    else f"raw.{rust_field(field.presence.mask)} = true;"
                )
                lines.append("if !field.is_none() { " + " ".join(statements) + " }")
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
            '@dataclass(frozen=True, slots=True)\nclass UnknownVariant:\n    tag: int\n\n    @property\n    def _tag(self):\n        return self.tag\n\ndef _copy_variant(raw, variants, empty=None):\n    if raw["kind"] is None:\n        return None if raw["tag"] == empty else UnknownVariant(raw["tag"])\n    return variants[raw["kind"]]._from_native(raw["value"])\n'
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
                f"class {public_name(name)}({'IntFlag' if plan.enum_kind == 'bitmask' else 'UnknownIntEnum'}):\n{body or '    pass'}\n"
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
        extra_copies=(),
        extra_public_copies=(),
        extra_methods="",
    ):
        name = plan.native
        rust, python = [], []
        fields, copies, public_copies = (
            list(extra_fields),
            list(extra_copies),
            list(extra_public_copies),
        )
        for member, value, field, group in self.members(plan):
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
            fields.append(
                f"    {member}: {public_type}" + (" = None" if optional(value) else "")
            )
            if group:
                group_value = self.group_value(plan, group)
                inner = " ".join(
                    f'inner.set_item("{f.name}", {self.copy(f.value, "value." + rust_field(source))})?;'
                    for f, source in zip(
                        [f for f in group_value.fields if f.role == "value"],
                        group.fields,
                        strict=True,
                    )
                )
                copy = f"{{ let inner = PyDict::new(py); {inner} inner.into_any().unbind() }}"
            else:
                copy = self.copy(field.value, "value." + rust_field(field.name))
            if field.presence and field.presence.mask:
                absent = (
                    f"value.{rust_field(field.presence.mask)} & sys::{field.presence.bit} == 0"
                    if field.presence.bit
                    else f"!value.{rust_field(field.presence.mask)}"
                )
                copy = f"if {absent} {{ py.None() }} else {{ {copy} }}"
            copies.append(f'dict.set_item("{member}", {copy})?;')
            public_copies.append(
                f"{member}={self.facade_copy(value, f'raw[{member!r}]')}"
            )
        if arena := self.item_buffers.get(name):
            fields.append(f"    {arena.field}: str")
            public_copies.append(f"{arena.field}=raw[{arena.field!r}]")
        python.append(
            f"@dataclass(frozen=True, slots=True)\nclass {public_name(name)}:\n"
            + "\n".join(sorted(fields, key=lambda line: " = " in line) or ["    pass"])
            + "\n\n    @classmethod\n    def _from_native(cls, raw):\n        return cls("
            + ", ".join(public_copies)
            + ")\n"
        )
        python[-1] += extra_methods
        if name in self.defaults:
            python[-1] += (
                f"\n    @classmethod\n    def default(cls):\n        from . import _native\n        return cls._from_native(_native._default_{name.removeprefix('mln_')}())\n"
            )
        if name not in self.outputs:
            return "", "\n".join(python)
        rust.append(
            f"fn generated_copy_{name}(py: Python<'_>, value: &sys::{name}) -> PyResult<Py<PyAny>> {{\n    let dict = PyDict::new(py);\n    "
            + "\n    ".join(copies)
            + "\n    Ok(dict.into_any().unbind())\n}\n"
        )
        return "\n".join(rust), "\n".join(python)
