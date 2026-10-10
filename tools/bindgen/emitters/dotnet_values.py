"""Render recursive .NET values from the language-neutral binding plan."""

from __future__ import annotations

import os
import re
from dataclasses import replace

from .. import docs
from ..compiler import compile_api
from ..managed_contracts import KEYWORDS
from ..model import Api, ModelError
from ..names import pascal
from ..semantic import BoundApi, FieldPlan, ValuePlan

SCALARS = {
    "bool": "bool",
    "_Bool": "bool",
    "double": "double",
    "float": "float",
    "int8_t": "sbyte",
    "uint8_t": "byte",
    "int16_t": "short",
    "uint16_t": "ushort",
    "int32_t": "int",
    "uint32_t": "uint",
    "int64_t": "long",
    "uint64_t": "ulong",
    "size_t": "nuint",
    "intptr_t": "nint",
    "ptrdiff_t": "nint",
    "uintptr_t": "nuint",
    "int": "int",
    "unsigned int": "uint",
    "long long": "long",
    "unsigned long long": "ulong",
    "long": "long",
    "unsigned long": "ulong",
}


def raw_handle(handle) -> str:
    """The raw struct that carries one handle plan's issued id."""
    return "Mln" + pascal(handle.stem)


class Unsupported(ValueError):
    pass


def member(name: str) -> str:
    """A C field or parameter name, escaped only where it is a C# keyword."""
    return ".".join(
        "@" + part if part in KEYWORDS["dotnet"] else part for part in name.split(".")
    )


def width(ctype) -> str | None:
    return SCALARS.get(ctype.spelling) or SCALARS.get(ctype.canonical)


def typed_mask(field: FieldPlan) -> str | None:
    """The C enum that the raw declarations give a presence mask field.

    A mask carries the bits of one enum. Declaring it as that enum lets the
    generated values test and set bits by name, without casts.
    """
    value = field.value
    if (
        field.role != "presence_mask"
        or value.kind != "enum"
        or value.enum_underlying is None
        or width(value.ctype) is None
        or width(value.ctype) != width(value.enum_underlying)
    ):
        return None
    return value.native


def public_name(native: str) -> str:
    return pascal(native.removeprefix("mln_"))


class Values:
    def __init__(self, api: Api | BoundApi):
        self.bound = compile_api(api)
        self.api = self.bound.source
        self.plans: dict[str, ValuePlan] = {}
        self.enum_names: set[str] = set()
        # The mask enums whose members generated values name unqualified.
        self.mask_enums: set[str] = set()
        self.item_buffers = {
            field.value.element.native: field.value.item_buffer
            for value in self.bound.values.values()
            for field in value.fields
            if field.value.item_buffer and field.value.element
        }

    def record(self, name: str) -> ValuePlan:
        try:
            plan = self.bound.values[name]
            # A record typedef's underlying CType names its struct declaration.
            self.supported(plan)
        except (ModelError, KeyError) as error:
            raise Unsupported(f"{name}: no resolved record value: {error}") from error
        return plan

    def supported(self, plan: ValuePlan) -> None:
        if plan.response:
            self.plans[plan.native] = plan
            return
        if plan.kind in {"scalar", "enum"} and (plan.nullable or plan.optional):
            raise Unsupported(
                f"{plan.native}: scalar absence requires a value representation"
            )
        if plan.ownership == "owned":
            raise Unsupported(
                f"{plan.native}: owned value requires an adoption transaction"
            )
        if plan.kind == "handle" and plan.handle:
            return
        if plan.kind == "callback":
            callback = self.bound.callbacks[plan.native]
            if callback.result.kind == "enum":
                self.supported(callback.result)
            for parameter in callback.parameters:
                if parameter.name != callback.context:
                    self.supported(parameter.value)
            return
        if plan.kind == "enum":
            self.enum_names.add(plan.native)
            prefix = (
                os.path.commonprefix([name for name, _ in plan.enum_values]).rsplit(
                    "_", 1
                )[0]
                + "_"
            )
            members = [
                pascal(name.removeprefix(prefix).lower())
                for name, _ in plan.enum_values
            ]
            if len(members) != len(set(members)) or any(
                not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) for name in members
            ):
                raise Unsupported(
                    f"{plan.native}: enum members collide or require identifier escaping"
                )
            return
        if plan.kind == "native_pointer":
            return
        if plan.kind == "scalar":
            self.native_type(plan)
            return
        if plan.kind == "buffer" and plan.encoding in {"utf8", "bytes", "json"}:
            if plan.optional not in (
                None,
                "empty",
            ):
                raise Unsupported("buffer presence needs a supported representation")
            return
        if plan.kind in {"array", "reference"} and plan.element:
            self.supported(plan.element)
            return
        if plan.kind == "union":
            for field in plan.fields:
                self.supported(field.value)
            return
        if plan.kind != "record":
            raise Unsupported(
                f"{plan.native}: {plan.kind} requires another value renderer"
            )
        names = [name for name, _ in self.members(plan)]
        if (
            len(names) != len(set(names))
            or set(names)
            & {
                public_name(plan.native),
                "Equals",
                "GetHashCode",
                "ToString",
                "Deconstruct",
            }
            or any(not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) for name in names)
        ):
            raise Unsupported(
                f"{plan.native}: record fields collide after public-name conversion"
            )
        for field in self.fields(plan):
            if field.presence:
                presence = field.presence
                if not presence.mask and not (
                    presence.tag and field.value.kind == "union"
                ):
                    raise Unsupported(
                        f"{plan.native}.{field.name}: unsupported presence expression"
                    )
            self.supported(field.value)
        for group in plan.presence_groups:
            if group.type:
                self.record(group.type)
        self.plans[plan.native] = plan

    def fields(self, plan: ValuePlan) -> tuple[FieldPlan, ...]:
        controls = {field.name for field in plan.fields if not field.public} | {
            field.presence.mask
            for field in plan.fields
            if field.presence and field.presence.mask
        }
        item_buffer = self.item_buffers.get(plan.native)
        if item_buffer:
            controls.update((item_buffer.offset, item_buffer.length))
        fields = tuple(field for field in plan.fields if field.name not in controls)
        if item_buffer:
            prototype = next(
                field for field in plan.fields if field.name == item_buffer.offset
            )
            fields += (
                replace(
                    prototype,
                    name=item_buffer.field,
                    value=replace(
                        prototype.value,
                        kind="buffer",
                        native="mln_buffer_view",
                        buffer_form="view",
                        encoding=item_buffer.encoding,
                    ),
                ),
            )
        return fields

    def members(self, plan: ValuePlan) -> tuple[tuple[str, tuple[FieldPlan, ...]], ...]:
        fields = self.fields(plan)
        groups = {
            (group.mask, group.bit): group.fields
            for group in plan.presence_groups
            if len(group.fields) > 1
        }
        emitted = set()
        members = []
        for field in fields:
            key = (
                (field.presence.mask, field.presence.bit)
                if field.presence and field.presence.mask
                else None
            )
            if key in groups:
                if key in emitted:
                    continue
                emitted.add(key)
                name = pascal(
                    next(
                        group.member
                        for group in plan.presence_groups
                        if (group.mask, group.bit) == key
                    )
                )
                members.append(
                    (name, tuple(item for item in fields if item.name in groups[key]))
                )
            else:
                members.append((pascal(field.name), (field,)))
        return tuple(members)

    def member_type(
        self, plan: ValuePlan, name: str, fields: tuple[FieldPlan, ...]
    ) -> str:
        group = next(
            (
                group
                for group in plan.presence_groups
                if group.fields == tuple(field.name for field in fields)
            ),
            None,
        )
        if fields[0].value.kind == "union":
            return (
                public_name(plan.native)
                if self.union_only(plan)
                else f"{public_name(plan.native)}.{name}Value"
            )
        if group and group.type:
            return public_name(group.type)
        return (
            f"{public_name(plan.native)}.{name}Value"
            if len(fields) > 1
            else self.public_type(fields[0].value)
        )

    def public_type(self, plan: ValuePlan) -> str:
        if plan.kind == "handle":
            return pascal(plan.handle.stem) + "Handle"
        if plan.kind == "callback":
            callback = self.bound.callbacks[plan.native]
            arguments = [
                self.public_type(parameter.value)
                for parameter in callback.parameters
                if parameter.name != callback.context
            ]
            if callback.result.ctype.kind != "void" and not callback.status:
                arguments.append(self.public_type(callback.result))
                return "Func<" + ", ".join(arguments) + ">?"
            return (
                "Action" + ("<" + ", ".join(arguments) + ">" if arguments else "") + "?"
            )
        if plan.kind == "native_pointer":
            return "NativePointer"
        if plan.kind == "scalar":
            return "ulong" if plan.native == "size_t" else self.native_type(plan)
        if plan.kind == "reference" and plan.element:
            base = self.public_type(plan.element)
            return base if not plan.nullable or base.endswith("?") else base + "?"
        if plan.kind == "buffer":
            base = "string" if plan.encoding == "utf8" else "byte[]"
            return base + ("?" if plan.nullable or plan.optional else "")
        if plan.kind == "array" and plan.element:
            return self.public_type(plan.element) + "[]"
        return public_name(plan.native)

    def copy(self, plan: ValuePlan, expression: str, length: str | None = None) -> str:
        if plan.kind == "reference" and plan.element:
            copied = self.copy(plan.element, f"NativeCallScope.Read({expression})")
            return (
                f"{expression} == null ? null : {copied}" if plan.nullable else copied
            )
        if plan.kind == "callback":
            return f'{expression} == null ? null : throw new InvalidOperationException("Native callback cannot be copied into a managed delegate.")'
        if plan.kind == "native_pointer":
            return f"NativePointer.FromNativeAddress((nint){expression})"
        if plan.kind == "enum":
            return f"({public_name(plan.native)}){expression}"
        if plan.kind == "scalar":
            if plan.native == "size_t":
                return f"(ulong){expression}"
            return (
                f"{expression} != 0" if self.public_type(plan) == "bool" else expression
            )
        if plan.kind == "array" and plan.element:
            if length is None:
                raise Unsupported(f"{plan.native}: array needs a count expression")
            native = self.raw_type(plan.element)
            public = self.public_type(plan.element)
            if plan.element.kind == "scalar" and native == public:
                return f"NativeCallScope.CopyArray<{native}>({expression}, (nuint)({length}))"
            element = self.copy(plan.element, "item")
            return f"NativeCallScope.CopyArray<{native}, {public}>({expression}, (nuint)({length}), static item => {element})"
        if plan.kind == "buffer":
            if plan.length == "nul":
                return (
                    f"{expression} == null ? null : NativeCallScope.CopyCString({expression})"
                    if plan.nullable
                    else f"NativeCallScope.CopyCString({expression})"
                )
            if plan.ctype.pointee:
                if length is None:
                    raise Unsupported("counted buffer requires a count expression")
                copied = (
                    f"RuntimeStructs.CopyUtf8((sbyte*){expression}, (nuint)({length}))"
                    if plan.encoding == "utf8"
                    else f"NativeCallScope.CopyArray<byte>((byte*){expression}, (nuint)({length}))"
                )
                return (
                    f"{length} == 0 ? null : {copied}"
                    if plan.optional == "empty"
                    else f"{expression} == null ? null : {copied}"
                    if plan.nullable
                    else copied
                )
            copy = (
                f"RuntimeStructs.CopyUtf8((sbyte*){expression}.data, {expression}.size)"
                if plan.encoding == "utf8"
                else f"ValueStructs.CopyBufferView({expression})"
            )
            return (
                f"{expression}.size == 0 ? null : {copy}"
                if plan.optional == "empty"
                else f"{expression}.data == null ? null : {copy}"
                if plan.nullable
                else copy
            )
        return f"Copy{public_name(plan.native)}({expression})"

    def encode(self, plan: ValuePlan, expression: str) -> str:
        if plan.kind == "reference" and plan.element:
            source = expression
            if plan.nullable and not self.is_reference_type(plan.element):
                source = f"{expression}.Value"
            encoded = f"scope.Value({self.encode(plan.element, source)})"
            return (
                f"{expression} is null ? null : {encoded}" if plan.nullable else encoded
            )
        if plan.kind == "native_pointer":
            return f"(void*){expression}.Address"
        if plan.kind == "enum":
            return f"({self.native_type(plan)}){expression}"
        if plan.kind == "scalar":
            if plan.native == "size_t":
                return f"checked((nuint){expression})"
            return (
                f"(byte)({expression} ? 1 : 0)"
                if self.public_type(plan) == "bool"
                else expression
            )
        if plan.kind == "record":
            scope = ", scope" if self.needs_scope(plan) else ""
            return f"Native{public_name(plan.native)}({expression}{scope})"
        if plan.kind == "buffer":
            if plan.length == "nul":
                return (
                    f"{expression} is null ? null : scope.CString({expression})"
                    if plan.nullable
                    else f"scope.CString({expression})"
                )
            method = "Utf8" if plan.encoding == "utf8" else "Buffer"
            default = '""' if plan.encoding == "utf8" else "[]"
            value = (
                f"{expression} ?? {default}"
                if plan.optional or plan.nullable
                else expression
            )
            return (
                f"{expression} is null ? default : scope.{method}({expression})"
                if plan.nullable
                else f"scope.{method}({value})"
            )
        if plan.kind == "array" and plan.element:
            if plan.element.kind == "scalar" and self.raw_type(
                plan.element
            ) == self.public_type(plan.element):
                return f"scope.Array<{self.raw_type(plan.element)}>({expression})"
            element = self.encode(plan.element, "item")
            return f"scope.Array<{self.raw_type(plan.element)}, {self.public_type(plan.element)}>({expression}, item => {element})"
        raise Unsupported(f"{plan.native}: input requires an allocation scope")

    def union_only(self, plan: ValuePlan) -> bool:
        fields = self.fields(plan)
        return len(fields) == 1 and fields[0].value.kind == "union"

    def is_reference_type(self, plan: ValuePlan) -> bool:
        return (
            bool(plan.registration)
            or plan.kind in {"buffer", "array", "reference"}
            or (
                plan.kind == "record"
                and (
                    self.union_only(plan)
                    or any(
                        field.presence and field.presence.mask
                        for field in self.fields(plan)
                    )
                )
            )
        )

    def raw_type(self, plan: ValuePlan) -> str:
        if plan.kind == "enum":
            return self.native_type(plan)
        if plan.kind == "handle":
            return raw_handle(plan.handle)
        if plan.kind == "buffer" and plan.ctype.pointee:
            return "sbyte*" if plan.encoding == "utf8" else "byte*"
        if plan.kind == "native_pointer":
            return "void*"
        if plan.kind == "reference" and plan.element:
            return self.raw_type(plan.element) + "*"
        if plan.kind == "scalar":
            return (
                "byte" if plan.native in {"bool", "_Bool"} else self.native_type(plan)
            )
        return plan.native

    def native_type(self, plan: ValuePlan) -> str:
        if plan.ctype.declaration in {enum.name for enum in self.api.enums}:
            return plan.ctype.declaration
        for name in (plan.scalar_carrier, plan.ctype.spelling, plan.ctype.declaration):
            if name in SCALARS:
                return SCALARS[name]
        if plan.ctype.canonical in SCALARS:
            return SCALARS[plan.ctype.canonical]
        raise Unsupported(
            f"{plan.native}: native representation requires a scalar type"
        )

    def mask_type(self, plan: ValuePlan, name: str) -> str:
        current = plan
        for segment in name.split("."):
            control = next(field for field in current.fields if field.name == segment)
            current = control.value
        return self.native_type(current)

    def mask_enum(self, plan: ValuePlan, name: str) -> str | None:
        current, control = plan, None
        for segment in name.split("."):
            control = next(field for field in current.fields if field.name == segment)
            current = control.value
        assert control is not None
        enum = typed_mask(control)
        if enum:
            self.mask_enums.add(enum)
        return enum

    def has_bit(self, plan: ValuePlan, mask: str, bit: str) -> str:
        if self.mask_enum(plan, mask):
            return f"value.{member(mask)}.HasFlag({bit})"
        return f"(value.{member(mask)} & ({self.mask_type(plan, mask)}){self.constant(bit)}) != 0"

    def bit(self, plan: ValuePlan, mask: str, bit: str) -> str:
        if self.mask_enum(plan, mask):
            return bit
        return f"({self.mask_type(plan, mask)}){self.constant(bit)}"

    def needs_scope(self, plan: ValuePlan) -> bool:
        return (
            bool(plan.registration)
            or plan.kind in {"buffer", "array", "reference"}
            or (
                plan.kind in {"record", "union"}
                and any(self.needs_scope(field.value) for field in self.fields(plan))
            )
        )

    def can_encode(self, plan: ValuePlan) -> bool:
        if plan.native in self.item_buffers or any(
            field.value.stride or field.value.item_buffer for field in plan.fields
        ):
            return False
        if plan.response:
            return False
        return (
            plan.kind in {"scalar", "enum", "buffer", "native_pointer", "callback"}
            or (
                plan.kind in {"array", "reference"}
                and plan.element is not None
                and self.can_encode(plan.element)
            )
            or (
                plan.kind in {"record", "union"}
                and all(self.can_encode(field.value) for field in self.fields(plan))
            )
        )

    def constant(self, name: str) -> str:
        for enum in self.api.enums:
            if any(value.name == name for value in enum.values):
                return f"{enum.name}.{name}"
        raise Unsupported(f"unknown enum constant {name}")

    def flag_name(self, flag) -> str:
        return pascal(flag.member)

    def decoder(self, plan: ValuePlan) -> str:
        if plan.response:
            return ""
        values = []
        members = self.members(plan)
        for name, fields in members:
            copied = []
            for field in fields:
                if (
                    item_buffer := self.item_buffers.get(plan.native)
                ) and field.name == item_buffer.field:
                    copied.append("message")
                elif field.value.stride:
                    copied.append(
                        f"Copy{public_name(plan.native)}{pascal(field.name)}(value)"
                    )
                elif field.value.registration or (
                    plan.registration and field.value.kind == "callback"
                ):
                    # Native never returns callbacks, so a copy leaves them
                    # unset.
                    copied.append("default")
                elif field.value.kind == "union":
                    union = field.value
                    tag = next(item for item in plan.fields if item.name == union.tag)
                    arms = [
                        f"({self.native_type(tag.value)}){self.constant(variant.presence.variant)} => new {self.member_type(plan, name, fields)}.{pascal(variant.name)}({self.copy(variant.value, f'value.{member(field.name)}.{member(variant.name)}')})"
                        for variant in union.fields
                    ]
                    if union.empty_variant:
                        arms.append(
                            f"({self.native_type(tag.value)}){self.constant(union.empty_variant[0])} => new {self.member_type(plan, name, fields)}.None()"
                        )
                    raw_bytes = (
                        f"NativeCallScope.CopyValueBytes(value.{member(field.name)})"
                    )
                    if plan.native in self.item_buffers:
                        offset = f"(nuint)Marshal.OffsetOf<{plan.native}>(nameof({plan.native}.{member(field.name)}))"
                        raw_bytes = f"record == null ? {raw_bytes} : NativeCallScope.CopyArray<byte>(record + {offset}, recordSize - {offset})"
                    arms.append(
                        f"_ => new {self.member_type(plan, name, fields)}.Unknown(({self.unknown_tag_type(tag.value)})value.{member(union.tag)}, {raw_bytes})"
                    )
                    copied.append(
                        f"value.{member(union.tag)} switch {{ " + ", ".join(arms) + " }"
                    )
                else:
                    copied.append(
                        self.copy(
                            field.value,
                            f"value.{member(field.name)}",
                            f"value.{member(field.value.length)}"
                            if field.value.length
                            else None,
                        )
                    )
            expression = (
                f"new {self.member_type(plan, name, fields)}({', '.join(copied)})"
                if len(fields) > 1
                else copied[0]
            )
            presence = fields[0].presence
            if presence and presence.mask:
                assert presence.mask
                condition = (
                    self.has_bit(plan, presence.mask, presence.bit)
                    if presence.bit
                    else f"value.{member(presence.mask)} != 0"
                )
                expression = f"{condition} ? {expression} : null"
            values.append(expression)
        arrays = {
            name: self.array_member(plan, name, fields) for name, fields in members
        }
        if self.union_only(plan):
            copied = values[0]
        elif any(
            field.presence and field.presence.mask for field in self.fields(plan)
        ) or any(array for array, _ in arrays.values()):
            copied = (
                "new() { "
                + ", ".join(
                    (
                        f"{name}Storage = {'ValueArray.Optional(' if arrays[name][1] else 'new('}{value})"
                        if arrays[name][0]
                        else f"{name} = {value}"
                    )
                    for (name, fields), value in zip(members, values, strict=True)
                )
                + "".join(
                    f", {self.flag_name(flag)} = {self.has_bit(plan, flag.mask, flag.name)}"
                    for flag in plan.mask_flags
                )
                + " }"
            )
        else:
            copied = "new(" + ", ".join(values) + ")"
        extra = (
            ', string message = "", byte* record = null, nuint recordSize = 0'
            if plan.native in self.item_buffers
            else ""
        )
        declaration = f"    private static {public_name(plan.native)} Copy{public_name(plan.native)}({plan.native} value{extra}) => {copied};\n"
        for field in self.fields(plan):
            array = field.value
            if not array.stride or not array.element:
                continue
            native = self.raw_type(array.element)
            public = self.public_type(array.element)
            item_buffer = array.item_buffer
            declaration += (
                f"    private static {public}[] Copy{public_name(plan.native)}{pascal(field.name)}({plan.native} value)\n    {{\n"
                f"        var count = checked((int)value.{member(array.length)});\n"
                f'        if (value.{member(array.stride)} < sizeof({native}) || (count != 0 && value.{member(field.name)} == null)) throw new InvalidOperationException("Invalid native array storage.");\n'
                f"        var copied = new {public}[count];\n"
                "        for (var index = 0; index < count; index++)\n        {\n"
                f"            var record = (byte*)value.{member(field.name)} + checked((nuint)index * value.{member(array.stride)});\n"
                f"            var item = *({native}*)record;\n"
            )
            if item_buffer:
                declaration += (
                    f'            if (item.{member(item_buffer.offset)} > value.{member(item_buffer.size)} || item.{member(item_buffer.length)} > value.{member(item_buffer.size)} - item.{member(item_buffer.offset)} || (item.{member(item_buffer.length)} != 0 && value.{member(item_buffer.data)} == null)) throw new InvalidOperationException("Invalid native item buffer.");\n'
                    f"            var message = RuntimeStructs.CopyUtf8((byte*)value.{member(item_buffer.data)} + item.{member(item_buffer.offset)}, item.{member(item_buffer.length)});\n"
                    f"            copied[index] = Copy{public_name(array.element.native)}(item, message, record, value.{member(array.stride)});\n"
                )
            else:
                declaration += (
                    f"            copied[index] = {self.copy(array.element, 'item')};\n"
                )
            declaration += "        }\n        return copied;\n    }\n"
        return declaration

    def required_reference_checks(self, plan: ValuePlan) -> list[str]:
        """Rejects a null in a member whose type does not admit one.

        A record struct's default value, or an initializer that skips a
        member, leaves a reference-typed member null although its type is
        non-nullable. Encoding it would dereference the null. Array members
        need no check, because their storage reads back an empty array.
        """
        checks = []
        for name, fields in self.members(plan):
            value = fields[0].value
            if (
                len(fields) != 1
                or value.kind in {"callback", "union"}
                or (fields[0].presence and fields[0].presence.mask)
            ):
                continue
            type_ = self.member_type(plan, name, fields)
            reference = type_ == "string" or (
                value.kind == "record" and self.declares_class(value)
            )
            if not reference or self.union_only(plan):
                continue
            message = f"{public_name(plan.native)}.{name} must not be null."
            checks.append(f'        Required(value.{name}, "{message}");')
        return checks

    def declares_class(self, plan: ValuePlan) -> bool:
        """Whether declaration() emits the record as a class rather than a
        record struct."""
        return bool(plan.response) or (
            self.union_only(plan)
            or any(
                field.presence and field.presence.mask for field in self.fields(plan)
            )
        )

    def encoder(self, plan: ValuePlan) -> str:
        if not self.can_encode(plan):
            raise Unsupported(f"{plan.native}: input requires an allocation scope")
        lines = [
            f"    private static {plan.native} Native{public_name(plan.native)}({public_name(plan.native)} value{', NativeCallScope scope' if self.needs_scope(plan) else ''})",
            "    {",
        ]
        lines.extend(self.required_reference_checks(plan))
        initial = (
            f"NativeMethods.{plan.default}()"
            if plan.default
            else f"new {plan.native}()"
        )
        lines.append(f"        var native = {initial};")
        for control in plan.fields:
            if control.role == "presence_mask" or any(
                field.presence and field.presence.mask == control.name
                for field in plan.fields
            ):
                lines.append(f"        native.{member(control.name)} = 0;")
        for control in plan.fields:
            if control.role == "size":
                lines.append(
                    f"        native.{member(control.name)} = ({self.native_type(control.value)})sizeof({plan.native});"
                )
        if plan.registration:
            callbacks = [
                next(field for field in plan.fields if field.name == name)
                for name in plan.registration.callbacks
            ]
            condition = " || ".join(
                f"value.{pascal(field.name)} is not null" for field in callbacks
            )
            lines.append(f"        if ({condition})")
            lines.append("        {")
            lines.append(
                f"            native.{member(plan.registration.user_data)} = scope.Register(value with {{ }});"
            )
            lines.append(
                f"            native.{member(plan.registration.release)} = &NativeCallbackRoot.Release;"
            )
            for field in callbacks:
                lines.append(
                    f"            native.{member(field.name)} = value.{pascal(field.name)} is null ? null : &Invoke{public_name(plan.native)}{pascal(field.name)};"
                )
            lines.append("        }")
        for name, fields in self.members(plan):
            if fields[0].value.kind == "callback":
                continue
            array, optional = self.array_member(plan, name, fields)
            expression = (
                "value"
                if self.union_only(plan)
                else f"value.{name}Storage{'?' if optional else ''}.Items"
                if array and len(fields) == 1
                else f"value.{name}"
            )
            presence = fields[0].presence
            indent = "        "
            if presence and presence.mask:
                if "." in presence.mask:
                    reset = (
                        f" &= ~{self.bit(plan, presence.mask, presence.bit)}"
                        if presence.bit
                        else " = 0"
                    )
                    lines.append(f"        native.{member(presence.mask)}{reset};")
                if put := self.put(plan, presence, fields, expression):
                    lines.append(put)
                    continue
                local = f"field{name}"
                lines.extend(
                    [
                        f"        if ({expression} is {{ }} {local})",
                        "        {",
                        f"            native.{member(presence.mask)} |= {self.bit(plan, presence.mask, presence.bit)};"
                        if presence.bit
                        else f"            native.{member(presence.mask)} = 1;",
                    ]
                )
                expression = local
                indent = "            "
            elif array and optional:
                # A local lets nullable analysis see the absence check.
                local = f"field{name}"
                lines.append(f"        var {local} = {expression};")
                expression = local
            for field in fields:
                if field.value.kind == "union":
                    tag = next(
                        item for item in plan.fields if item.name == field.value.tag
                    )
                    lines.append(f"{indent}switch ({expression})")
                    lines.append(f"{indent}{{")
                    for variant in field.value.fields:
                        lines.extend(
                            [
                                f"{indent}    case {self.member_type(plan, name, fields)}.{pascal(variant.name)} selected:",
                                f"{indent}        native.{member(field.value.tag)} = ({self.native_type(tag.value)}){self.constant(variant.presence.variant)};",
                                f"{indent}        native.{member(field.name)}.{member(variant.name)} = {self.encode(variant.value, 'selected.Value')};",
                                f"{indent}        break;",
                            ]
                        )
                    lines.append(
                        f'{indent}    default: throw new ArgumentException("A union variant is required.", nameof(value));'
                    )
                    lines.append(f"{indent}}}")
                    continue
                source = (
                    f"{expression}.{pascal(field.name)}"
                    if len(fields) > 1
                    else expression
                )
                if (
                    field.value.kind == "buffer"
                    and field.value.ctype.pointee
                    and field.value.length != "nul"
                ):
                    local = "buffer" + pascal(field.name)
                    method = "Utf8" if field.value.encoding == "utf8" else "Buffer"
                    count = next(
                        item for item in plan.fields if item.name == field.value.length
                    )
                    lines.append(f"{indent}var {local} = scope.{method}({source});")
                    lines.append(
                        f"{indent}native.{member(field.name)} = ({self.raw_type(field.value)}){local}.data;"
                    )
                    lines.append(
                        f"{indent}native.{member(field.value.length)} = checked(({self.native_type(count.value)}){local}.size);"
                    )
                    continue
                lines.append(
                    f"{indent}native.{member(field.name)} = {self.encode(field.value, source)};"
                )
                if field.value.kind == "array" and field.value.length:
                    count_field = next(
                        item for item in plan.fields if item.name == field.value.length
                    )
                    lines.append(
                        f"{indent}native.{member(field.value.length)} = checked(({self.native_type(count_field.value)}){source}.Length);"
                    )
            if presence and presence.mask:
                lines.append("        }")
        for flag in plan.mask_flags:
            lines.append(
                f"        if (value.{self.flag_name(flag)}) native.{member(flag.mask)} |= {self.bit(plan, flag.mask, flag.name)};"
            )
        return "\n".join([*lines, "        return native;", "    }", ""])

    def put(self, plan: ValuePlan, presence, fields, expression: str) -> str | None:
        """One line that stores a present member and marks its bit.

        A member that spans several fields, a union, or a counted buffer keeps
        its explicit branch.
        """
        if len(fields) != 1 or not presence.bit:
            return None
        field = fields[0]
        value = field.value
        if (
            not self.mask_enum(plan, presence.mask)
            or value.kind in {"union", "array", "callback"}
            or (value.kind == "buffer" and value.ctype.pointee)
        ):
            return None
        encoded = self.encode(value, "present")
        arguments = [expression, f"ref native.{member(field.name)}", presence.bit]
        if encoded != "present":
            group = re.fullmatch(r"(\w+)\(present\)", encoded)
            arguments.append(
                group[1]
                if group
                else f"{'' if 'scope' in encoded else 'static '}present => {encoded}"
            )
        return f"        native.{member(presence.mask)} |= Put({', '.join(arguments)});"

    def callback_methods(self, plan: ValuePlan) -> str:
        if not plan.registration:
            return ""
        methods = []
        for name in plan.registration.callbacks:
            field = next(field for field in plan.fields if field.name == name)
            callback = self.bound.callbacks[field.value.native]
            restriction = ""
            if callback.reentry_policy:
                policy = callback.reentry_policy
                table = "Allowed" + public_name(plan.native) + pascal(name)
                operations = ", ".join(f'"{item}"' for item in policy.operations)
                methods.append(
                    f"    private static readonly string[] {table} = [{operations}];\n"
                )
                owner_expression = (
                    "owned"
                    if callback.decision
                    else "response" + pascal(policy.owner_parameter)
                )
                restriction = f"            using var restriction = NativeCallbackGuard.Restrict({owner_expression}, {table});\n"
            elif callback.reentry == "forbid":
                restriction = "            using var restriction = NativeCallbackGuard.ForbidReentry();\n"
            args = ", ".join(
                f"{self.raw_type(parameter.value)} {member(parameter.name)}"
                for parameter in callback.parameters
            )
            if callback.decision:
                decision = callback.decision
                input_handle = next(
                    parameter
                    for parameter in callback.parameters
                    if parameter.name == decision.parameter
                )
                owner = self.public_type(input_handle.value)
                converted = ", ".join(
                    "owned"
                    if parameter.name == decision.parameter
                    else self.copy(parameter.value, member(parameter.name))
                    for parameter in callback.parameters
                    if parameter.name != callback.context
                )
                accept = self.constant(decision.accept)
                raw = self.raw_type(callback.result)
                failure = self.constant(callback.failure)
                methods.append(
                    "    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]\n"
                    f"    private static {raw} Invoke{public_name(plan.native)}{pascal(name)}({args})\n    {{\n"
                    f"        {owner}? owned = null;\n        try\n        {{\n"
                    f"            owned = {owner}.BorrowDecision({member(decision.parameter)});\n"
                    f"{restriction}"
                    f"            var decision = (({public_name(plan.native)})NativeCallbackRoot.Value({member(callback.context)})).{pascal(name)}?.Invoke({converted}) ?? ({self.public_type(callback.result)}){failure};\n"
                    f"            return owned.FinishDecision(decision == ({self.public_type(callback.result)}){accept}) ? ({raw}){accept} : ({raw})decision;\n"
                    f'        }}\n        catch (Exception error) {{ NativeCallbackFailure.Report("{callback.native}", error); try {{ return owned is not null && owned.FinishDecision(false) ? ({raw}){accept} : ({raw}){failure}; }} catch {{ return ({raw}){accept}; }} }}\n    }}\n'
                )
                continue
            responses = [
                parameter
                for parameter in callback.parameters
                if parameter.value.kind == "reference"
                and parameter.value.element
                and parameter.value.element.response
            ]
            converted = ", ".join(
                "response" + pascal(parameter.name)
                if parameter in responses
                else self.copy(parameter.value, f"{member(parameter.name)}")
                for parameter in callback.parameters
                if parameter.name != callback.context
            )
            setup = "".join(
                f"            var response{pascal(parameter.name)} = new {public_name(parameter.value.element.native)}({member(parameter.name)});\n"
                for parameter in responses
            )
            cleanup = " ".join(
                f"response{pascal(parameter.name)}.Expire();" for parameter in responses
            )
            invoke = f"(({public_name(plan.native)})NativeCallbackRoot.Value({member(callback.context)})).{pascal(name)}?.Invoke({converted});"
            if responses:
                invoke = f"try {{ {invoke} }} finally {{ {cleanup} }}"
            result_type = (
                "void"
                if callback.result.ctype.kind == "void"
                else callback.result.native
            )
            success = (
                ""
                if result_type == "void"
                else "            return mln_status.MLN_STATUS_OK;\n"
            )
            failure = (
                ""
                if result_type == "void"
                else f"return {self.constant(callback.failure)};"
            )
            methods.append(
                "    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]\n"
                f"    private static {result_type} Invoke{public_name(plan.native)}{pascal(name)}({args})\n    {{\n"
                f"        try\n        {{\n{setup}{restriction}            {invoke}\n{success}        }}\n"
                f'        catch (Exception error)\n        {{\n            NativeCallbackFailure.Report("{callback.native}", error);\n            {failure}\n        }}\n    }}\n'
            )
        return "\n".join(methods)

    def owned_direct_callback(self, plan: ValuePlan) -> bool:
        policy = self.bound.callbacks[plan.native].reentry_policy
        if policy and not policy.registration_owner:
            raise Unsupported(
                f"{plan.native}: direct callback requires a registration owner"
            )
        return policy is not None

    def direct_callback_method(self, plan: ValuePlan) -> str:
        callback = self.bound.callbacks[plan.native]
        self.supported(plan)
        parameters = ", ".join(
            f"{self.raw_type(parameter.value)} {member(parameter.name)}"
            for parameter in callback.parameters
        )
        arguments = ", ".join(
            self.copy(parameter.value, member(parameter.name))
            for parameter in callback.parameters
            if parameter.name != callback.context
        )
        result = (
            self.raw_type(callback.result)
            if callback.result.native != "void"
            else "void"
        )
        delegate = self.public_type(plan).removesuffix("?")
        call = f"(({delegate})NativeCallbackRoot.Value({member(callback.context)}))({arguments})"
        failure = (
            ""
            if result == "void"
            else f"return {self.constant(callback.failure) if callback.failure and callback.failure.startswith('MLN_') else callback.failure};"
        )
        guard = (
            "using var restriction = NativeCallbackGuard.ForbidReentry(); "
            if callback.reentry == "forbid"
            else ""
        )
        table_declaration = ""
        if self.owned_direct_callback(plan):
            policy = callback.reentry_policy
            table = "Allowed" + public_name(plan.native)
            operations = ", ".join(f'"{item}"' for item in policy.operations)
            table_declaration = (
                f"    private static readonly string[] {table} = [{operations}];\n"
            )
            guard = (
                f"var owned = (NativeOwnedCallback)NativeCallbackRoot.Value({member(callback.context)}); "
                f"using var restriction = NativeCallbackGuard.Restrict(owned.Owner, {table}); "
            )
            call = f"(({delegate})owned.Callback)({arguments})"
        statement = call + ";" if result == "void" else f"return {call};"
        return (
            table_declaration
            + "    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]\n"
            f"    internal static {result} Invoke{public_name(plan.native)}({parameters})\n    {{\n"
            f'        try {{ {guard}{statement} }}\n        catch (Exception error) {{ NativeCallbackFailure.Report("{callback.native}", error); {failure} }}\n    }}\n'
        )

    def array_member(
        self, plan: ValuePlan, name: str, fields: tuple[FieldPlan, ...]
    ) -> tuple[bool, bool]:
        """Whether a member is an array, and whether that array is optional."""
        type_ = self.member_type(plan, name, fields)
        optional = bool(
            fields[0].presence and fields[0].presence.mask
        ) or type_.endswith("?")
        return type_.removesuffix("?").endswith("[]"), optional

    def array_declaration(self, plan: ValuePlan) -> str:
        """A value with array members, which keep their elements in ValueArray
        storage so that record synthesis compares them element by element."""
        members = self.members(plan)
        masked = any(
            field.presence and field.presence.mask for field in self.fields(plan)
        )
        name = public_name(plan.native)
        accessor = "set" if masked else "init"
        properties, parameters, assignments = [], [], []
        for member, fields in members:
            type_ = self.member_type(plan, member, fields)
            array, optional = self.array_member(plan, member, fields)
            if optional and not type_.endswith("?"):
                type_ += "?"
            parameters.append(f"{type_} {member}")
            assignments.append(f"        this.{member} = {member};")
            if array:
                element = type_.removesuffix("?").removesuffix("[]")
                storage = f"ValueArray<{element}>{'?' if optional else ''}"
                copy = "ValueArray.CopyOptional" if optional else "ValueArray.Copy"
                properties.extend(
                    [
                        f"    public {type_} {member} {{ get => {member}Storage{'?' if optional else ''}.ToArray(); {accessor} => {member}Storage = {copy}(value); }}",
                        f"    internal {storage} {member}Storage {{ get; {accessor}; }}",
                    ]
                )
                continue
            required = (
                "required "
                if masked
                and not optional
                and (type_ == "string" or self.is_reference_type(fields[0].value))
                else ""
            )
            properties.append(
                self.initialized(
                    f"    public {required}{type_} {member} {{ get; {accessor}; }}",
                    fields,
                )
                if masked
                else f"    public {required}{type_} {member} {{ get; {accessor}; }}"
            )
        declaration = f"public {'sealed record' if masked else 'readonly record struct'} {name}\n{{\n"
        if not masked:
            declaration += (
                self.parameterless(plan)
                + f"    public {name}({', '.join(parameters)})\n    {{\n"
                + "\n".join(assignments)
                + "\n    }\n"
            )
        return declaration + "\n".join(properties) + "\n}\n"

    def declaration(self, plan: ValuePlan) -> str:
        if plan.response:
            name = public_name(plan.native)
            return (
                f"public sealed unsafe partial class {name}\n{{\n"
                f"    private readonly global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackScope<{plan.native}> scope;\n"
                f"    internal {name}({plan.native}* pointer) => scope = new(pointer);\n"
                f"    internal {plan.native}* Pointer => scope.Pointer;\n"
                "    internal void Expire() => scope.Expire();\n}\n"
            )
        if self.union_only(plan):
            name = public_name(plan.native)
            union = self.fields(plan)[0].value
            return (
                f"public abstract record {name}\n{{\n    private {name}() {{ }}\n"
                + "".join(
                    f"    public sealed record {pascal(field.name)}({self.public_type(field.value)} Value) : {name};\n"
                    for field in self.fields(plan)[0].value.fields
                )
                + (
                    f"    public sealed record None : {name};\n"
                    if union.empty_variant
                    else ""
                )
                + self.unknown_variant(
                    name,
                    self.unknown_tag_type(
                        next(
                            field.value
                            for field in plan.fields
                            if field.name == union.tag
                        )
                    ),
                )
                + "}\n"
            )
        declaration = self.value_declaration(plan)
        unions = []
        for name, fields in self.members(plan):
            if len(fields) > 1 and self.member_type(plan, name, fields).removesuffix(
                "?"
            ).startswith(public_name(plan.native) + "."):
                parameters = ", ".join(
                    f"{self.public_type(field.value)} {pascal(field.name)}"
                    for field in fields
                )
                unions.append(
                    f"    public readonly record struct {name}Value({parameters});\n"
                )
            if fields[0].value.kind != "union":
                continue
            unions.append(
                f"    public abstract record {name}Value\n    {{\n        private {name}Value() {{ }}\n"
                + "".join(
                    f"        public sealed record {pascal(field.name)}({self.public_type(field.value)} Value) : {name}Value;\n"
                    for field in fields[0].value.fields
                )
                + (
                    f"        public sealed record None : {name}Value;\n"
                    if fields[0].value.empty_variant
                    else ""
                )
                + self.unknown_variant(
                    name + "Value",
                    self.unknown_tag_type(
                        next(
                            field.value
                            for field in plan.fields
                            if field.name == fields[0].value.tag
                        )
                    ),
                )
                + "    }\n"
            )
        if unions:
            declaration = declaration.rstrip()
            if declaration.endswith(";"):
                declaration = declaration[:-1] + "\n{\n" + "".join(unions) + "}\n"
            else:
                declaration = declaration[:-1] + "".join(unions) + "}\n"
        return declaration

    def unknown_tag_type(self, value: ValuePlan) -> str:
        native = value.enum_underlying or value.ctype
        return (
            SCALARS.get(value.scalar_carrier)
            or SCALARS.get(native.spelling)
            or SCALARS.get(native.canonical)
            or self.native_type(value)
        )

    @staticmethod
    def unknown_variant(base: str, tag: str) -> str:
        return (
            f"        public sealed record Unknown : {base}\n        {{\n"
            "            private readonly byte[] bytes;\n"
            f"            public {tag} Tag {{ get; }}\n"
            "            public byte[] PayloadBytes => (byte[])bytes.Clone();\n"
            f"            public Unknown({tag} tag, byte[] payloadBytes) {{ Tag = tag; bytes = (byte[])payloadBytes.Clone(); }}\n"
            "            public bool Equals(Unknown? other) => other is not null && Tag == other.Tag && bytes.AsSpan().SequenceEqual(other.bytes);\n"
            "            public override int GetHashCode() { var hash = new HashCode(); hash.Add(Tag); foreach (var value in bytes) hash.Add(value); return hash.ToHashCode(); }\n"
            "        }\n"
        )

    def member_initial(self, fields: tuple[FieldPlan, ...]) -> str | None:
        """A member's annotated default, or None when its type's zero value is
        the native default. A record member takes its type's own defaults."""
        if len(fields) != 1 or fields[0].presence:
            return None
        field, value = fields[0], fields[0].value
        initial = field.initial
        if initial is None:
            if value.kind == "record" and self.has_initials(value):
                return f"new {self.public_type(value)}()"
            return None
        type_ = self.public_type(value)
        if value.kind == "enum":
            return f"{type_}.{pascal(initial.member)}"
        if type_ == "float":
            return f"{initial.literal}f"
        if type_ == "bool":
            return initial.literal
        return initial.literal if type_ == "double" else str(initial.value)

    def has_initials(self, plan: ValuePlan) -> bool:
        """Whether a record's native default holds a nonzero member."""
        return any(
            self.member_initial(fields) is not None for _, fields in self.members(plan)
        )

    def initialized(self, property_: str, fields: tuple[FieldPlan, ...]) -> str:
        """A property declaration that starts at the member's native default."""
        initial = self.member_initial(fields)
        return property_ if initial is None else f"{property_} = {initial};"

    def parameterless(self, plan: ValuePlan) -> str:
        """A constructor that starts every member at its native default.

        A struct's implicit parameterless constructor zeroes every member, so
        a record whose native default holds a nonzero member declares its own.
        """
        if not self.has_initials(plan):
            return ""
        arguments = []
        for member, fields in self.members(plan):
            initial = self.member_initial(fields)
            type_ = self.member_type(plan, member, fields)
            if initial is not None:
                arguments.append(initial)
            elif type_.endswith("?") or not (
                type_ in {"string", "byte[]"} or self.is_reference_type(fields[0].value)
            ):
                arguments.append("default")
            else:
                arguments.append("default!")
        return (
            f"    public {public_name(plan.native)}()\n"
            f"        : this({', '.join(arguments)}) {{ }}\n"
        )

    def member_doc(self, plan: ValuePlan, fields) -> str:
        """The XML doc comment of a member that one field backs."""
        if len(fields) != 1:
            return ""
        return docs.xml_comment(
            self.bound.doc(f"{plan.native}.{fields[0].name}"), "    "
        )

    def value_declaration(self, plan: ValuePlan) -> str:
        fields = self.fields(plan)
        if any(
            self.public_type(field.value).removesuffix("?").endswith("[]")
            for field in fields
        ):
            return self.array_declaration(plan)
        if any(field.presence and field.presence.mask for field in fields):
            properties = []
            for name, members in self.members(plan):
                type_ = self.member_type(plan, name, members)
                if (
                    members[0].presence
                    and members[0].presence.mask
                    and not type_.endswith("?")
                ):
                    type_ += "?"
                required = (
                    "required "
                    if not members[0].presence
                    and (
                        type_ in {"string", "byte[]"}
                        or (
                            members[0].value.kind == "record"
                            and any(
                                field.presence
                                for field in self.fields(members[0].value)
                            )
                        )
                    )
                    else ""
                )
                properties.append(
                    self.member_doc(plan, members)
                    + self.initialized(
                        f"    public {required}{type_} {name} {{ get; set; }}", members
                    )
                )
            properties.extend(
                docs.xml_comment(self.bound.doc(flag.name), "    ")
                + f"    public bool {self.flag_name(flag)} {{ get; set; }}"
                for flag in plan.mask_flags
            )
            return (
                f"public sealed record {public_name(plan.native)}\n{{\n"
                + "\n".join(properties)
                + "\n}\n"
            )
        # A positional record documents each property as a parameter, in the
        # comment that the record's own summary begins.
        params = "".join(
            docs.xml_param(self.bound.doc(f"{plan.native}.{members[0].name}"), name)
            for name, members in self.members(plan)
            if len(members) == 1
        )
        declaration = params + (
            f"public readonly partial record struct {public_name(plan.native)}("
            + ", ".join(
                f"{self.member_type(plan, name, members)} {name}"
                for name, members in self.members(plan)
            )
            + ")"
        )
        constructor = self.parameterless(plan)
        return (
            f"{declaration}\n{{\n{constructor}}}\n"
            if constructor
            else declaration + ";\n"
        )
