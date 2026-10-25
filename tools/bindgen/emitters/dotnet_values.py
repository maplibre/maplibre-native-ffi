"""Render recursive .NET values from the language-neutral binding plan."""

from __future__ import annotations

import re

from .. import async_hosts, docs
from ..compiler import compile_api
from ..managed_contracts import KEYWORDS
from ..model import Api, ModelError
from ..names import camel, pascal
from ..semantic import (
    BoundApi,
    FieldPlan,
    ValuePlan,
    enum_member_prefix,
    required_callbacks,
)

# JSON is text, so it takes and returns a string as UTF-8 text does.
TEXT_ENCODINGS = frozenset({"utf8", "json"})

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
    return "@" + name if name in KEYWORDS["dotnet"] else name


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
        self.mask_enums: set[str] = set()
        self.strided_records = {
            field.value.element.native
            for value in self.bound.values.values()
            for field in value.fields
            if field.value.stride and field.value.element
        }

    def record(self, name: str) -> ValuePlan:
        try:
            plan = self.bound.values[name]
            self.supported(plan)
        except (ModelError, KeyError) as error:
            raise Unsupported(f"{name}: no resolved record value: {error}") from error
        return plan

    def supported(self, plan: ValuePlan) -> None:
        if plan.response:
            self.plans[plan.native] = plan
            return
        if plan.kind in {"scalar", "enum"} and plan.nullable:
            raise Unsupported(
                f"{plan.native}: scalar absence requires a value representation"
            )
        if plan.kind == "handle" and plan.handle:
            return
        if plan.ownership == "owned":
            raise Unsupported(
                f"{plan.native}: owned value requires an adoption transaction"
            )
        if plan.kind == "callback":
            callback = self.bound.callbacks[plan.native]
            if any(
                public_name(name) == public_name(plan.native)
                for name in self.bound.public_values
            ):
                raise Unsupported(f"{plan.native}: the delegate collides with a value")
            if callback.result.kind == "enum":
                self.supported(callback.result)
            for parameter in callback.arguments:
                self.supported(parameter.value)
            return
        if plan.kind == "enum":
            self.enum_names.add(plan.native)
            prefix = enum_member_prefix(
                (name for name, _ in plan.enum_values), plan.native
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
        if plan.native in self.bound.slices:
            # A slice record is presented as its array alone, which a copier
            # reads from the record.
            self.supported(self.bound.slices[plan.native].value)
            self.plans[plan.native] = plan
            return
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
        self.plans[plan.native] = plan

    def fields(self, plan: ValuePlan) -> tuple[FieldPlan, ...]:
        controls = {field.name for field in plan.fields if not field.public} | {
            field.presence.mask
            for field in plan.fields
            if field.presence and field.presence.mask
        }
        fields = tuple(field for field in plan.fields if field.name not in controls)
        return fields

    def members(self, plan: ValuePlan) -> tuple[tuple[str, FieldPlan], ...]:
        return tuple((pascal(field.name), field) for field in self.fields(plan))

    def member_type(self, plan: ValuePlan, name: str, field: FieldPlan) -> str:
        if field.value.kind == "union":
            return (
                public_name(plan.native)
                if plan.union_member is not None
                else f"{public_name(plan.native)}.{name}Value"
            )
        return self.public_type(field.value)

    def public_type(self, plan: ValuePlan) -> str:
        if plan.kind == "handle":
            return pascal(plan.handle.stem) + "Handle"
        if plan.kind == "callback":
            return public_name(plan.native) + ("?" if plan.nullable else "")
        if plan.kind == "native_pointer":
            return "NativePointer"
        if plan.kind == "scalar":
            return "ulong" if plan.native == "size_t" else self.native_type(plan)
        if plan.kind == "reference" and plan.element:
            base = self.public_type(plan.element)
            return base if not plan.nullable or base.endswith("?") else base + "?"
        if plan.kind == "buffer":
            base = "string" if plan.encoding in TEXT_ENCODINGS else "byte[]"
            return base + ("?" if plan.nullable else "")
        if plan.kind == "array" and plan.element:
            return self.public_type(plan.element) + "[]"
        if plan.native in self.bound.slices:
            return self.public_type(self.bound.slices[plan.native].value)
        return public_name(plan.native)

    def delegate(self, native: str) -> str:
        """The named delegate of a callback type, whose parameters keep the C
        names. A status callback returns nothing and throws to fail."""
        callback = self.bound.callbacks[native]
        result = (
            "void"
            if callback.result.ctype.kind == "void" or callback.status
            else self.public_type(callback.result)
        )
        parameters = ", ".join(
            f"{self.public_type(parameter.value)} {member(camel(parameter.name))}"
            for parameter in callback.arguments
        )
        return f"public delegate {result} {public_name(native)}({parameters});\n"

    def delegate_note(self, native: str) -> str | None:
        """How a callback type's delegate reports failure, or None when it
        returns nothing."""
        callback = self.bound.callbacks[native]
        if callback.status:
            fallback = pascal(callback.failure.removeprefix("MLN_STATUS_").lower())
            return (
                "Returning succeeds. Throwing a `MaplibreException` whose status "
                "is not `Status.Ok` fails the call with that status. Any other "
                f"exception fails it with `Status.{fallback}` and is reported to "
                "`Maplibre.CallbackException`."
            )
        if callback.result.ctype.kind == "void":
            return None
        return (
            f"A thrown exception answers `{callback.failure}` and is reported to "
            "`Maplibre.CallbackException`."
        )

    def raw_failure(self, callback) -> str:
        """The failure result as the trampoline returns it."""
        if callback.failure in {"true", "false"}:
            return f"(byte){int(callback.failure == 'true')}"
        if callback.failure.startswith("MLN_"):
            return self.constant(callback.failure)
        return callback.failure

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
        if plan.native in self.bound.slices:
            array = self.bound.slices[plan.native]
            if array.value.stride:
                return (
                    f"Copy{public_name(plan.native)}{pascal(array.name)}({expression})"
                )
            return self.copy(
                array.value,
                f"{expression}.{member(array.name)}",
                f"{expression}.{member(array.value.length)}",
            )
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
                    if plan.encoding in TEXT_ENCODINGS
                    else f"NativeCallScope.CopyArray<byte>((byte*){expression}, (nuint)({length}))"
                )
                return (
                    f"{expression} == null ? null : {copied}"
                    if plan.nullable
                    else copied
                )
            copy = (
                f"RuntimeStructs.CopyUtf8((sbyte*){expression}.data, {expression}.size)"
                if plan.encoding in TEXT_ENCODINGS
                else f"ValueStructs.CopyBufferView({expression})"
            )
            return (
                f"{expression}.data == null ? null : {copy}" if plan.nullable else copy
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
            method = "Utf8" if plan.encoding in TEXT_ENCODINGS else "Buffer"
            return (
                f"{expression} is null ? default : scope.{method}({expression})"
                if plan.nullable
                else f"scope.{method}({expression})"
            )
        if plan.kind == "array" and plan.element:
            if plan.element.kind == "scalar" and self.raw_type(
                plan.element
            ) == self.public_type(plan.element):
                return f"scope.Array<{self.raw_type(plan.element)}>({expression})"
            element = self.encode(plan.element, "item")
            return f"scope.Array<{self.raw_type(plan.element)}, {self.public_type(plan.element)}>({expression}, static (item, scope) => {element})"
        raise Unsupported(f"{plan.native}: input requires an allocation scope")

    def is_reference_type(self, plan: ValuePlan) -> bool:
        return (
            bool(plan.registration)
            or plan.kind in {"buffer", "array", "reference"}
            or (
                plan.kind == "record"
                and (
                    plan.union_member is not None
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
        control = next(field for field in plan.fields if field.name == name)
        return self.native_type(control.value)

    def mask_enum(self, plan: ValuePlan, name: str) -> str | None:
        control = next(field for field in plan.fields if field.name == name)
        enum = typed_mask(control)
        if enum:
            self.mask_enums.add(enum)
        return enum

    def has_bit(self, plan: ValuePlan, mask: str, bit: str) -> str:
        if self.mask_enum(plan, mask):
            return f"value.{member(mask)}.HasFlag({bit})"
        return f"(value.{member(mask)} & {self.typed_constant(self.mask_type(plan, mask), bit)}) != 0"

    def bit(self, plan: ValuePlan, mask: str, bit: str) -> str:
        if self.mask_enum(plan, mask):
            return bit
        return self.typed_constant(self.mask_type(plan, mask), bit)

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
        if any(field.value.stride for field in plan.fields):
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

    def typed_constant(self, native: str, name: str) -> str:
        """An enum constant as the raw type `native`, cast unless it is
        already that enum's member."""
        constant = self.constant(name)
        return (
            constant if constant.startswith(native + ".") else f"({native}){constant}"
        )

    def flag_name(self, flag) -> str:
        return pascal(flag.member)

    def decoder(self, plan: ValuePlan) -> str:
        if plan.response:
            return ""
        if plan.native in self.bound.slices:
            return self.array_copiers(plan)
        values = []
        members = self.members(plan)
        for name, field in members:
            if field.value.stride:
                expression = (
                    f"Copy{public_name(plan.native)}{pascal(field.name)}(value)"
                )
            elif plan.registration and field in required_callbacks(plan):
                # A copy holds no callbacks, so its caller passes the
                # required ones.
                expression = member(camel(field.name))
            elif field.value.registration or (
                plan.registration and field.value.kind == "callback"
            ):
                # Native never returns callbacks, so a copy leaves them
                # unset.
                expression = "default"
            elif field.value.kind == "union":
                union = field.value
                tag = next(item for item in plan.fields if item.name == union.tag)
                arms = [
                    f"{self.typed_constant(self.native_type(tag.value), variant.presence.variant)} => new {self.member_type(plan, name, field)}.{pascal(variant.name)}({self.copy(variant.value, f'value.{member(field.name)}.{member(variant.name)}')})"
                    for variant in union.fields
                ]
                if union.empty_variant:
                    arms.append(
                        f"{self.typed_constant(self.native_type(tag.value), union.empty_variant[0])} => new {self.member_type(plan, name, field)}.None()"
                    )
                raw_bytes = (
                    f"NativeCallScope.CopyValueBytes(value.{member(field.name)})"
                )
                if plan.native in self.strided_records:
                    offset = f"(nuint)Marshal.OffsetOf<{plan.native}>(nameof({plan.native}.{member(field.name)}))"
                    raw_bytes = f"record == null ? {raw_bytes} : NativeCallScope.CopyArray<byte>(record + {offset}, recordSize - {offset})"
                arms.append(
                    f"_ => new {self.member_type(plan, name, field)}.Unknown(({self.unknown_tag_type(tag.value)})value.{member(union.tag)}, {raw_bytes})"
                )
                expression = (
                    f"value.{member(union.tag)} switch {{ " + ", ".join(arms) + " }"
                )
            else:
                expression = self.copy(
                    field.value,
                    f"value.{member(field.name)}",
                    f"value.{member(field.value.length)}"
                    if field.value.length
                    else None,
                )
            presence = field.presence
            if presence and presence.mask:
                condition = self.has_bit(plan, presence.mask, presence.bit)
                expression = f"{condition} ? {expression} : null"
            values.append(expression)
        arrays = {name: self.array_member(plan, name, field) for name, field in members}
        masked = any(
            field.presence and field.presence.mask for field in self.fields(plan)
        )
        if plan.union_member is not None:
            copied = values[0]
        elif masked or any(array for array, _ in arrays.values()):
            copied = (
                (
                    "new() { "
                    if masked
                    else f"default({public_name(plan.native)}) with {{ "
                )
                + ", ".join(
                    (
                        f"{name}Storage = {'ValueArray.Optional(' if arrays[name][1] else 'new('}{value})"
                        if arrays[name][0]
                        else f"{name} = {value}"
                    )
                    for (name, _), value in zip(members, values, strict=True)
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
            ", byte* record = null, nuint recordSize = 0"
            if plan.native in self.strided_records
            else ""
        )
        extra += "".join(
            f", {self.public_type(field.value)} {member(camel(field.name))}"
            for field in required_callbacks(plan)
        )
        return (
            f"    private static {public_name(plan.native)} Copy{public_name(plan.native)}({plan.native} value{extra}) => {copied};\n"
            + self.array_copiers(plan)
        )

    def array_copiers(self, plan: ValuePlan) -> str:
        """Copies each strided array of a record, stepping by its stride."""
        declaration = ""
        for field in self.fields(plan):
            array = field.value
            if not array.stride or not array.element:
                continue
            native = self.raw_type(array.element)
            public = self.public_type(array.element)
            declaration += (
                f"    private static {public}[] Copy{public_name(plan.native)}{pascal(field.name)}({plan.native} value)\n    {{\n"
                f"        var count = checked((int)value.{member(array.length)});\n"
                f'        if (value.{member(array.stride)} < sizeof({native}) || (count != 0 && value.{member(field.name)} == null)) throw new InvalidOperationException("Invalid native array storage.");\n'
                f"        var copied = new {public}[count];\n"
                "        for (var index = 0; index < count; index++)\n        {\n"
                f"            var record = (byte*)value.{member(field.name)} + checked((nuint)index * value.{member(array.stride)});\n"
                f"            var item = *({native}*)record;\n"
            )
            copied = (
                f"Copy{public_name(array.element.native)}(item, record, value.{member(array.stride)})"
                if array.element.native in self.strided_records
                else self.copy(array.element, "item")
            )
            declaration += f"            copied[index] = {copied};\n"
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
        for name, field in self.members(plan):
            value = field.value
            if value.kind in {"callback", "union"} or (
                field.presence and field.presence.mask
            ):
                continue
            type_ = self.member_type(plan, name, field)
            reference = type_ == "string" or (
                value.kind == "record" and self.declares_class(value)
            )
            if not reference or plan.union_member is not None:
                continue
            message = f"{public_name(plan.native)}.{name} must not be null."
            checks.append(f'        Required(value.{name}, "{message}");')
        return checks

    def declares_class(self, plan: ValuePlan) -> bool:
        """Whether declaration() emits the record as a class rather than a
        record struct."""
        return bool(plan.response) or (
            plan.union_member is not None
            or any(
                field.presence and field.presence.mask for field in self.fields(plan)
            )
        )

    def encoder(self, plan: ValuePlan) -> str:
        if plan.native not in self.bound.inputs:
            return ""
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
            if self.native_callbacks(plan):
                lines += self.native_registration_encoding(plan, condition)
                lines.append(f"        else if ({condition})")
            else:
                lines.append(f"        if ({condition})")
            lines.append("        {")
            rooted = "value with { }"
            lines.append(
                f"            native.{member(plan.registration.user_data)} = scope.Register({rooted});"
            )
            lines.append(
                f"            native.{member(plan.registration.release)} = &NativeCallbackRoot.Release;"
            )
            for field in callbacks:
                lines.append(
                    f"            native.{member(field.name)} = value.{pascal(field.name)} is null ? null : &Invoke{public_name(plan.native)}{pascal(field.name)};"
                )
            lines.append("        }")
        for name, field in self.members(plan):
            if field.value.kind == "callback":
                continue
            array, optional = self.array_member(plan, name, field)
            expression = (
                "value"
                if plan.union_member is not None
                else f"value.{name}Storage{'?' if optional else ''}.Items"
                if array
                else f"value.{name}"
            )
            presence = field.presence
            indent = "        "
            if presence and presence.mask:
                if put := self.put(plan, presence, field, expression):
                    lines.append(put)
                    continue
                local = f"field{name}"
                lines.extend(
                    [
                        f"        if ({expression} is {{ }} {local})",
                        "        {",
                        f"            native.{member(presence.mask)} |= {self.bit(plan, presence.mask, presence.bit)};",
                    ]
                )
                expression = local
                indent = "            "
            elif array and optional:
                # A local lets nullable analysis see the absence check.
                local = f"field{name}"
                lines.append(f"        var {local} = {expression};")
                expression = local
            lines += self.field_encoding(plan, name, field, expression, indent)
            if presence and presence.mask:
                lines.append("        }")
        for flag in plan.mask_flags:
            lines.append(
                f"        if (value.{self.flag_name(flag)}) native.{member(flag.mask)} |= {self.bit(plan, flag.mask, flag.name)};"
            )
        return "\n".join([*lines, "        return native;", "    }", ""])

    def native_callbacks(self, plan: ValuePlan) -> tuple[FieldPlan, ...]:
        """The callback fields of a registration that also accepts a native
        registration, in registration order, or none."""
        if not async_hosts.accepts_native(self.bound, plan):
            return ()
        return tuple(
            next(field for field in plan.fields if field.name == name)
            for name in plan.registration.callbacks
        )

    def function_pointer(self, native: str) -> str:
        """The unmanaged function pointer type of a callback typedef."""
        callback = self.bound.callbacks[native]
        types = [self.raw_type(parameter.value) for parameter in callback.parameters]
        void = callback.result.ctype.kind == "void"
        types.append("void" if void else self.raw_type(callback.result))
        return f"delegate* unmanaged[Cdecl]<{', '.join(types)}>"

    def native_registration_encoding(
        self, plan: ValuePlan, delegates: str
    ) -> list[str]:
        """The branch that consumes a native registration and copies it
        unchanged into the C struct.

        The scope releases a consumed context unless native code accepts the
        call, so a value that also sets a delegate throws after the claim.
        """
        registration = plan.registration
        release = next(f for f in plan.fields if f.name == registration.release)
        lines = [
            "        if (value.NativeRegistration is { } registration)",
            "        {",
            "            scope.Consume(registration);",
            f"            if ({delegates})",
            '                throw new ArgumentException("A native registration carries no delegates.", nameof(value));',
        ]
        lines += [
            f"            native.{member(field.name)} = ({self.function_pointer(field.value.native)})registration.Function({index});"
            for index, field in enumerate(self.native_callbacks(plan))
        ]
        lines += [
            f"            native.{member(registration.user_data)} = registration.UserData;",
            f"            native.{member(registration.release)} = ({self.function_pointer(release.value.native)})registration.ReleaseUserData;",
            "        }",
        ]
        return lines

    def native_members(self, plan: ValuePlan) -> str:
        """The factory of a registration's native form, and the reference that
        every copy of the record shares, so that one claim covers all copies."""
        callbacks = self.native_callbacks(plan)
        if not callbacks:
            return ""
        registration = plan.registration
        name = public_name(plan.native)
        functions = [member(camel(field.name)) for field in callbacks]
        user_data = member(camel(registration.user_data))
        release = member(camel(registration.release))
        parameters = ", ".join(
            f"NativePointer {item}" for item in [*functions, user_data, release]
        )
        addresses = ", ".join(f"{item}.Address" for item in functions)
        lines = [
            "<summary>",
            "Creates a native registration from C functions and a context that the",
            "host supplies. The binding copies them unchanged, so native code calls",
            "each function without entering the .NET runtime.",
            "</summary>",
            "<remarks>",
            "The first call that carries this value or a copy of it consumes the",
            "registration. Native code releases the context of a call that it accepts,",
            "and the binding calls the release for a call that native code does not",
            "accept. A later call that carries the registration throws an",
            'invalid-state <see cref="Error.MaplibreException"/> before it reaches',
            "native code.",
            "</remarks>",
            *(
                f'<param name="{item.removeprefix("@")}">The C function that native code calls as <c>{field.name}</c>.</param>'
                for item, field in zip(functions, callbacks)
            ),
            f'<param name="{user_data}">The context that native code passes to each function.</param>',
            f'<param name="{release}">The C function that releases <paramref name="{user_data}"/>, or <see cref="NativePointer.Null"/>.</param>',
        ]
        return (
            "".join(f"    /// {line}\n" for line in lines)
            + f"    public static {name} Native({parameters}) =>\n"
            + f"        default({name}) with {{ NativeRegistration = new([{addresses}], {user_data}.Address, {release}.Address) }};\n\n"
            + "    internal NativeRegistration? NativeRegistration { get; init; }\n"
        )

    def field_encoding(
        self, plan: ValuePlan, name: str, field: FieldPlan, expression: str, indent: str
    ) -> list[str]:
        """The statements that store one member's value."""
        encoded = []
        if field.value.kind == "union":
            tag = next(item for item in plan.fields if item.name == field.value.tag)
            encoded.append(f"{indent}switch ({expression})")
            encoded.append(f"{indent}{{")
            for variant in field.value.fields:
                encoded.extend(
                    [
                        f"{indent}    case {self.member_type(plan, name, field)}.{pascal(variant.name)} selected:",
                        f"{indent}        native.{member(field.value.tag)} = {self.typed_constant(self.native_type(tag.value), variant.presence.variant)};",
                        f"{indent}        native.{member(field.name)}.{member(variant.name)} = {self.encode(variant.value, 'selected.Value')};",
                        f"{indent}        break;",
                    ]
                )
            encoded.append(
                f'{indent}    default: throw new ArgumentException("A union variant is required.", nameof(value));'
            )
            encoded.append(f"{indent}}}")
            return encoded
        if (
            field.value.kind == "buffer"
            and field.value.ctype.pointee
            and field.value.length != "nul"
        ):
            local = "buffer" + pascal(field.name)
            method = "Utf8" if field.value.encoding in TEXT_ENCODINGS else "Buffer"
            count = next(
                item for item in plan.fields if item.name == field.value.length
            )
            encoded.append(f"{indent}var {local} = scope.{method}({expression});")
            encoded.append(
                f"{indent}native.{member(field.name)} = ({self.raw_type(field.value)}){local}.data;"
            )
            encoded.append(
                f"{indent}native.{member(field.value.length)} = checked(({self.native_type(count.value)}){local}.size);"
            )
            return encoded
        encoded.append(
            f"{indent}native.{member(field.name)} = {self.encode(field.value, expression)};"
        )
        if field.value.kind == "array" and field.value.length:
            count_field = next(
                item for item in plan.fields if item.name == field.value.length
            )
            encoded.append(
                f"{indent}native.{member(field.value.length)} = checked(({self.native_type(count_field.value)}){expression}.Length);"
            )
        return encoded

    def put(self, plan: ValuePlan, presence, field, expression: str) -> str | None:
        """One line that stores a present member and marks its bit.

        A union or a counted buffer keeps its explicit branch.
        """
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
            # A scope is a ref struct, so the encoder receives it rather than
            # capturing it.
            arguments += (
                [group[1]]
                if group
                else ["scope", f"static (present, scope) => {encoded}"]
                if "scope" in encoded
                else [f"static present => {encoded}"]
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
            args = ", ".join(
                f"{self.raw_type(parameter.value)} {member(parameter.name)}"
                for parameter in callback.parameters
            )
            responses = [
                parameter
                for parameter in callback.parameters
                if parameter.value.kind == "reference"
                and parameter.value.element
                and parameter.value.element.response
            ]
            # A handle the callback owns is adopted before the host runs and
            # disposed when the host throws, so native never waits on it.
            owned = [
                parameter
                for parameter in callback.arguments
                if parameter.value.kind == "handle"
                and parameter.value.ownership == "owned"
            ]
            converted = ", ".join(
                "response" + pascal(parameter.name)
                if parameter in responses
                else "owned" + pascal(parameter.name)
                if parameter in owned
                else self.copy(parameter.value, f"{member(parameter.name)}")
                for parameter in callback.arguments
            )
            setup = "".join(
                f"            var response{pascal(parameter.name)} = new {public_name(parameter.value.element.native)}({member(parameter.name)});\n"
                for parameter in responses
            )
            declared = "".join(
                f"        {self.public_type(parameter.value)}? owned{pascal(parameter.name)} = null;\n"
                for parameter in owned
            )
            setup += "".join(
                f"            owned{pascal(parameter.name)} = {self.public_type(parameter.value)}.Adopt({member(parameter.name)});\n"
                for parameter in owned
            )
            disposals = "".join(
                f"            owned{pascal(parameter.name)}?.Dispose();\n"
                for parameter in owned
            )
            cleanup = " ".join(
                f"response{pascal(parameter.name)}.Expire();" for parameter in responses
            )
            root = f"NativeCallbackRoot.Value({member(callback.context)})"
            invoke = f"(({public_name(plan.native)}){root}).{pascal(name)}?.Invoke({converted});"
            if responses:
                invoke = f"try {{ {invoke} }} finally {{ {cleanup} }}"
            result_type = (
                "void"
                if callback.result.ctype.kind == "void"
                else callback.result.native
                if callback.status
                else self.raw_type(callback.result)
            )
            success = (
                "            return mln_status.MLN_STATUS_OK;\n"
                if callback.status
                else ""
            )
            failure_value = "" if result_type == "void" else self.raw_failure(callback)
            failure = "" if result_type == "void" else f"return {failure_value};"
            # A status callback fails by throwing. A thrown MaplibreException
            # with a failing status is a deliberate failure, which native
            # receives without a report.
            deliberate = (
                "        catch (MaplibreException error)\n"
                f"            when (({result_type})error.Status != {self.constant('MLN_STATUS_OK')})\n"
                f"        {{\n{disposals}            return ({result_type})error.Status;\n        }}\n"
                if callback.status
                else ""
            )
            if result_type != "void" and not callback.status:
                # A callback that answers with a value returns its failure value
                # when the record leaves it unset.
                answer = self.encode(callback.result, f"answer({converted})")
                invoke = (
                    f"var answer = (({public_name(plan.native)}){root}).{pascal(name)}; "
                    f"return answer is null ? {failure_value} : {answer};"
                )
            methods.append(
                "    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]\n"
                f"    private static {result_type} Invoke{public_name(plan.native)}{pascal(name)}({args})\n    {{\n"
                f"{declared}        try\n        {{\n{setup}{restriction}            {invoke}\n{success}        }}\n"
                f"{deliberate}"
                f'        catch (Exception error)\n        {{\n            NativeCallbackFailure.Report("{callback.native}", error);\n{disposals}            {failure}\n        }}\n    }}\n'
            )
        return "\n".join(methods)

    def array_member(
        self, plan: ValuePlan, name: str, field: FieldPlan
    ) -> tuple[bool, bool]:
        """Whether a member is an array, and whether that array is optional."""
        type_ = self.member_type(plan, name, field)
        optional = bool(field.presence and field.presence.mask) or type_.endswith("?")
        return type_.removesuffix("?").endswith("[]"), optional

    def property_type(self, plan: ValuePlan, name: str, field: FieldPlan) -> str:
        """The type of a member's property, where an array is an ImmutableArray
        that shares the value's storage."""
        type_ = self.member_type(plan, name, field)
        array, optional = self.array_member(plan, name, field)
        if not array:
            return type_
        element = type_.removesuffix("?").removesuffix("[]")
        return f"ImmutableArray<{element}>{'?' if optional else ''}"

    def array_declaration(self, plan: ValuePlan) -> str:
        """A value with array members, which keep their elements in ValueArray
        storage so that record synthesis compares them element by element."""
        members = self.members(plan)
        masked = any(
            field.presence and field.presence.mask for field in self.fields(plan)
        )
        name = public_name(plan.native)
        properties, parameters, assignments = [], [], []
        for member, field in members:
            type_ = self.property_type(plan, member, field)
            array, optional = self.array_member(plan, member, field)
            if optional and not type_.endswith("?"):
                type_ += "?"
            parameters.append(f"{type_} {member}")
            assignments.append(f"        this.{member} = {member};")
            if array:
                element = type_.removesuffix("?").removeprefix("ImmutableArray<")[:-1]
                storage = f"ValueArray<{element}>{'?' if optional else ''}"
                adopt = "ValueArray.FromOptional" if optional else "ValueArray.From"
                properties.extend(
                    [
                        self.member_doc(plan, field)
                        + f"    public {type_} {member} {{ get => {member}Storage{'?' if optional else ''}.AsImmutable(); init => {member}Storage = {adopt}(value); }}",
                        f"    internal {storage} {member}Storage {{ get; init; }}",
                    ]
                )
                continue
            required = (
                "required "
                if masked
                and not optional
                and (type_ == "string" or self.is_reference_type(field.value))
                else ""
            )
            properties.append(
                self.member_doc(plan, field)
                + f"    public {required}{type_} {member} {{ get; init; }}"
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
                f"    private readonly {plan.native}* pointer;\n"
                "    private readonly NativeViewScope scope = new();\n"
                f"    internal {name}({plan.native}* pointer) => this.pointer = pointer;\n"
                f"    internal {plan.native}* Pointer {{ get {{ scope.EnsureActive(); return pointer; }} }}\n"
                "    internal void Expire() => scope.Expire();\n}\n"
            )
        if plan.union_member is not None:
            name = public_name(plan.native)
            union = plan.union_member.value
            return (
                f"public abstract record {name}\n{{\n    private {name}() {{ }}\n"
                + "".join(
                    docs.xml_param(
                        self.bound.doc(f"{union.native}.{field.name}"), "Value", "    "
                    )
                    + f"    public sealed record {pascal(field.name)}({self.public_type(field.value)} Value) : {name};\n"
                    for field in union.fields
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
                    plan,
                )
                + "}\n"
            )
        declaration = self.value_declaration(plan)
        unions = []
        for name, field in self.members(plan):
            union = field.value
            if union.kind != "union":
                continue
            unions.append(
                f"    public abstract record {name}Value\n    {{\n        private {name}Value() {{ }}\n"
                + "".join(
                    docs.xml_param(
                        self.bound.doc(f"{union.native}.{variant.name}"),
                        "Value",
                        "        ",
                    )
                    + f"        public sealed record {pascal(variant.name)}({self.public_type(variant.value)} Value) : {name}Value;\n"
                    for variant in union.fields
                )
                + (
                    f"        public sealed record None : {name}Value;\n"
                    if union.empty_variant
                    else ""
                )
                + self.unknown_variant(
                    name + "Value",
                    self.unknown_tag_type(
                        next(
                            item.value for item in plan.fields if item.name == union.tag
                        )
                    ),
                    union,
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

    def unknown_variant(self, base: str, tag: str, value: ValuePlan) -> str:
        """The arm that carries a tag this binding does not know. Only a value
        copied from native can hold one, so an input-only union has none."""
        if value.native not in self.bound.returned:
            return ""
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

    def parameterless(self, plan: ValuePlan, *, primary: bool = False) -> str:
        if not plan.default:
            return ""
        initializer = (
            " : this(" + ", ".join("default!" for _ in self.members(plan)) + ")"
            if primary
            else ""
        )
        return f"    public {public_name(plan.native)}(){initializer} {{ this = Default; }}\n"

    def member_doc(self, plan: ValuePlan, field: FieldPlan) -> str:
        """The XML doc comment of a member."""
        return docs.xml_comment(self.bound.doc(f"{plan.native}.{field.name}"), "    ")

    def value_declaration(self, plan: ValuePlan) -> str:
        fields = self.fields(plan)
        if any(
            self.public_type(field.value).removesuffix("?").endswith("[]")
            for field in fields
        ):
            return self.array_declaration(plan)
        if any(field.presence and field.presence.mask for field in fields):
            properties = []
            for name, field in self.members(plan):
                type_ = self.member_type(plan, name, field)
                if field.presence and field.presence.mask and not type_.endswith("?"):
                    type_ += "?"
                required = (
                    "required "
                    if not field.presence
                    and (
                        type_ in {"string", "byte[]"}
                        or (field.value.kind == "callback" and not type_.endswith("?"))
                        or (
                            field.value.kind == "record"
                            and any(
                                member.presence for member in self.fields(field.value)
                            )
                        )
                    )
                    else ""
                )
                properties.append(
                    self.member_doc(plan, field)
                    + f"    public {required}{type_} {name} {{ get; init; }}"
                )
            properties.extend(
                docs.xml_comment(self.bound.doc(flag.name), "    ")
                + f"    public bool {self.flag_name(flag)} {{ get; init; }}"
                for flag in plan.mask_flags
            )
            return (
                f"public sealed record {public_name(plan.native)}\n{{\n"
                + "\n".join(properties)
                + "\n}\n"
            )
        params = "".join(
            docs.xml_param(self.bound.doc(f"{plan.native}.{field.name}"), name)
            for name, field in self.members(plan)
        )
        declaration = params + (
            f"public readonly partial record struct {public_name(plan.native)}("
            + ", ".join(
                f"{self.member_type(plan, name, field)} {name}"
                for name, field in self.members(plan)
            )
            + ")"
        )
        body = self.parameterless(plan, primary=True) + self.native_members(plan)
        return f"{declaration}\n{{\n{body}}}\n" if body else declaration + ";\n"
