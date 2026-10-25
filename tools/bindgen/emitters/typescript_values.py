"""Render TypeScript value types from resolved semantic plans.

Records are plain object interfaces, open enums are `const` objects with an
integer type that admits unknown values, bitmasks are integers with named flag
constants, and unions are discriminated by a `kind` member. A union copied
from native keeps an unknown tag, and a record of only a union and its tag is
that union. Integers of 32 bits or fewer and floating-point values are
`number`; 64-bit integers and native pointers are `bigint`. A JSON value is
`unknown`, any value that `JSON.stringify` takes or `JSON.parse` returns. A callback
registration takes the form that `async_hosts.py` chooses for a host that runs
no code on native threads: its callback's function type, an object of its
callbacks and members, or its members alone when native rules answer. A
registration that accepts a native registration also has a `native()` factory
of the same name, which takes a native function pointer for each callback, a
context, and a release.
"""

from __future__ import annotations

from dataclasses import dataclass

from .. import async_hosts, docs
from ..names import camel, pascal
from ..semantic import BoundApi, FieldPlan, ValuePlan, enum_member_prefix


class Unsupported(ValueError):
    pass


@dataclass(frozen=True)
class Registration:
    value: ValuePlan
    # "queue", "wake", "rules", or "native"; see async_hosts.py.
    form: str
    # The callbacks that host code supplies.
    callbacks: tuple[FieldPlan, ...]
    # The registration's public fields beside its callbacks.
    members: tuple[FieldPlan, ...]
    # The fields of a native registration in C order: each callback, then the
    # context and its release. Empty when the registration accepts none.
    native: tuple[FieldPlan, ...] = ()

    @property
    def function(self) -> bool:
        """Whether the TypeScript form of the registration is its callback."""
        return len(self.callbacks) == 1 and not self.members


def registration(bound: BoundApi, value: ValuePlan) -> Registration:
    form = async_hosts.form(bound, value)
    if form is None:
        raise Unsupported(
            f"{value.native}: registration requires queue delivery, a wake, "
            "or native rules"
        )
    plan = value.registration
    callbacks = (
        tuple(async_hosts.queue_callbacks(bound, value))
        if form == "queue"
        else tuple(async_hosts.callback_fields(value))
        if form == "wake"
        else ()
    )
    masks = {f.presence.mask for f in value.fields if f.presence and f.presence.mask}
    members = tuple(
        f
        for f in value.fields
        if f.name not in plan.callbacks
        and f.name not in {plan.user_data, plan.release}
        and f.public
        and f.name not in masks
    )
    native = ()
    if async_hosts.accepts_native(bound, value):
        if members:
            raise Unsupported(
                f"{value.native}: a native registration has no members beside "
                "its callbacks"
            )
        by_name = {f.name: f for f in value.fields}
        native = (
            *async_hosts.callback_fields(value),
            by_name[plan.user_data],
            by_name[plan.release],
        )
    return Registration(value, form, callbacks, members, native)


# Words that a TypeScript identifier cannot be, as a binding or a parameter.
RESERVED = {
    "arguments",
    "await",
    "break",
    "case",
    "catch",
    "class",
    "const",
    "continue",
    "debugger",
    "default",
    "delete",
    "do",
    "else",
    "enum",
    "eval",
    "export",
    "extends",
    "false",
    "finally",
    "for",
    "function",
    "if",
    "implements",
    "import",
    "in",
    "instanceof",
    "interface",
    "let",
    "new",
    "null",
    "package",
    "private",
    "protected",
    "public",
    "return",
    "static",
    "super",
    "switch",
    "this",
    "throw",
    "true",
    "try",
    "typeof",
    "var",
    "void",
    "while",
    "with",
    "yield",
}

# Names that generated method bodies bind, which a parameter cannot take.
LOCALS = {"native", "handle", "registrations", "completion", "disposed", "result"}

# Members of the owner base class that an operation cannot replace.
# Members that every owner class defines: its handle, its state, its release,
# and the constructor.
OWNER_MEMBERS = {"id", "isClosed", "close", "constructor"}


@dataclass(frozen=True)
class Scalar:
    """How one C scalar crosses: its TypeScript type and its C reader."""

    ts: str
    kind: str  # bool, int, double, u64, or i64
    c: str
    minimum: int = 0
    maximum: int = 0


def _int(c: str, bits: int, signed: bool) -> Scalar:
    if signed:
        return Scalar("number", "int", c, -(1 << (bits - 1)), (1 << (bits - 1)) - 1)
    return Scalar("number", "int", c, 0, (1 << bits) - 1)


# Keyed by the portable carrier the semantic layer names, so a host whose
# headers spell int64_t as long or long long generates the same code.
SCALARS = {
    "bool": Scalar("boolean", "bool", "bool"),
    "_Bool": Scalar("boolean", "bool", "bool"),
    "float": Scalar("number", "double", "float"),
    "double": Scalar("number", "double", "double"),
    "int8_t": _int("int8_t", 8, True),
    "uint8_t": _int("uint8_t", 8, False),
    "int16_t": _int("int16_t", 16, True),
    "uint16_t": _int("uint16_t", 16, False),
    "int32_t": _int("int32_t", 32, True),
    "uint32_t": _int("uint32_t", 32, False),
    "int64_t": Scalar("bigint", "i64", "int64_t"),
    "uint64_t": Scalar("bigint", "u64", "uint64_t"),
    "size_t": Scalar("bigint", "u64", "size_t"),
    # The base types whose width every supported target shares. `long` is
    # 32 bits on Windows and 64 elsewhere, so it has no portable form.
    "char": _int("char", 8, True),
    "signed char": _int("signed char", 8, True),
    "unsigned char": _int("unsigned char", 8, False),
    "short": _int("short", 16, True),
    "unsigned short": _int("unsigned short", 16, False),
    "int": _int("int", 32, True),
    "unsigned int": _int("unsigned int", 32, False),
    "unsigned": _int("unsigned int", 32, False),
    "long long": Scalar("bigint", "i64", "long long"),
    "unsigned long long": Scalar("bigint", "u64", "unsigned long long"),
}
U64 = SCALARS["uint64_t"]


def public_name(native: str) -> str:
    return pascal(native.removeprefix("mln_"))


def identifier(name: str) -> str:
    """A camelCase member or local name that is not a reserved word."""
    result = camel(name)
    return result + "Value" if result in RESERVED else result


def parameter_name(name: str) -> str:
    """A parameter name that no generated local shadows."""
    result = identifier(name)
    return result + "Value" if result in LOCALS else result


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    return docs.block_comment(bound.doc(native), indent)


def enum_members(value: ValuePlan) -> list[tuple[str, str, int]]:
    """(member, constant, number) for each enum constant."""
    names = [name for name, _ in value.enum_values]
    prefix = enum_member_prefix(names, value.native)
    return [
        (identifier(name.removeprefix(prefix).lower()), name, number)
        for name, number in value.enum_values
    ]


def is_counted(value: ValuePlan) -> bool:
    """A pointer buffer whose size another field or parameter holds."""
    return (
        value.kind == "buffer"
        and value.buffer_form != "view"
        and value.length not in {None, "1"}
    )


def owner_class(handle) -> str:
    stem = identifier(handle.stem)
    return stem[:1].upper() + stem[1:] + "Handle"


def union_kinds(value: ValuePlan) -> list[tuple[str | None, str, str | None]]:
    """(variant constant, kind, arm field) for each arm, then the empty one."""
    result = [
        (arm.presence.variant, identifier(arm.name), arm.name) for arm in value.fields
    ]
    if value.empty_variant:
        constant = value.empty_variant[0]
        prefix = enum_member_prefix(
            [constant, *(a.presence.variant for a in value.fields)]
        )
        result.append(
            (constant, identifier(constant.removeprefix(prefix).lower()), None)
        )
    return result


class Values:
    """The value plans an emitted operation reaches, by direction."""

    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.used: dict[str, ValuePlan] = {
            name: value
            for name, value in bound.public_values.items()
            if value.kind == "enum"
        }
        self.inputs: set[str] = set()
        self.outputs: set[str] = set()
        # Owner classes that a value type names, such as a delivered request.
        self.owners: set[str] = set()

    def merge(self, other: Values) -> None:
        self.used.update(other.used)
        self.inputs |= other.inputs
        self.outputs |= other.outputs
        self.owners |= other.owners

    def scalar(self, value: ValuePlan) -> Scalar | None:
        return SCALARS.get(value.scalar_carrier or value.native) or SCALARS.get(
            value.ctype.canonical.removeprefix("const ")
        )

    def enum_scalar(self, value: ValuePlan) -> Scalar:
        carrier = SCALARS.get(value.scalar_carrier or "") or SCALARS.get(
            value.enum_underlying.canonical if value.enum_underlying else ""
        )
        if carrier is None or carrier.kind not in {"int", "u64", "i64"}:
            raise Unsupported(f"{value.native}: unsupported enum carrier")
        return carrier

    def carrier(self, value: ValuePlan) -> Scalar:
        """The scalar form of a scalar, enum, handle, or native pointer."""
        if value.kind == "enum":
            return self.enum_scalar(value)
        if value.kind == "scalar":
            return self.scalar(value)
        return U64

    def registration(self, value: ValuePlan) -> Registration:
        return registration(self.bound, value)

    def fields(self, value: ValuePlan) -> tuple[FieldPlan, ...]:
        """The public members of a record, without its control fields."""
        if value.registration:
            entry = self.registration(value)
            return entry.callbacks + entry.members
        # Beside the control roles, TypeScript reads a mask as optional
        # members, and a counted value's length as its array's.
        controls = {f.name for f in value.fields if not f.public}
        controls |= {
            f.presence.mask for f in value.fields if f.presence and f.presence.mask
        }
        controls |= {
            f.value.length
            for f in value.fields
            if f.value.kind == "array" or is_counted(f.value)
        }
        return tuple(f for f in value.fields if f.name not in controls)

    def check(self, value: ValuePlan, direction: str) -> None:
        """Accept a value that this binding can carry in `direction`."""
        if value.ownership == "owned" and value.kind != "handle":
            raise Unsupported(f"{value.native}: owned value needs adoption")
        if value.kind == "scalar":
            if self.scalar(value) is None:
                raise Unsupported(f"{value.native}: unsupported scalar")
            return
        if value.kind == "native_pointer":
            return
        if value.kind == "enum":
            self.enum_scalar(value)
            if value.native not in self.bound.control_enums:
                self.used[value.native] = value
            return
        if value.kind == "handle":
            if value.native not in self.bound.public_handles:
                raise Unsupported(f"{value.native}: handle without an owner")
            if direction == "out" and value.ownership != "owned":
                raise Unsupported(f"{value.native}: borrowed handle result")
            return
        if value.kind == "buffer":
            if value.encoding not in {"utf8", "json", "bytes"}:
                raise Unsupported(f"{value.native}: unsupported buffer encoding")
            if value.encoding == "json" and value.buffer_form != "view":
                raise Unsupported(f"{value.native}: JSON outside a view")
            if value.buffer_form != "view" and not is_counted(value):
                raise Unsupported(f"{value.native}: buffer without a length")
            return
        if value.kind == "reference":
            if value.element is None:
                raise Unsupported(f"{value.native}: reference without element")
            if value.length not in {None, "1"}:
                raise Unsupported(f"{value.native}: counted reference")
            self.check(value.element, direction)
            return
        if value.kind == "array":
            element = value.element
            if element is None:
                raise Unsupported(f"{value.native}: array without element")
            if element.kind not in {"record", "buffer"} or (
                element.kind == "buffer" and element.buffer_form != "view"
            ):
                raise Unsupported(f"{value.native}: array of {element.kind}")
            self.check(element, direction)
            return
        if value.kind == "callback":
            self.check_callback(value)
            self.used[value.native] = value
            return
        if value.kind == "record" and value.native in self.bound.slices:
            # A slice record reaches host code as its array alone.
            self.check(self.bound.slices[value.native].value, direction)
            return
        if value.kind == "record" and value.registration:
            entry = self.registration(value)
            if entry.form != "native":
                for field in entry.callbacks:
                    self.check_callback(field.value)
                for field in entry.members:
                    self.check(field.value, direction)
            (self.inputs if direction == "in" else self.outputs).add(value.native)
            self.used[value.native] = value
            return
        if value.kind == "record":
            if value.response:
                raise Unsupported(f"{value.native}: callback response record")
            for field in self.fields(value):
                if field.value.kind == "union":
                    self.check_union(value, field, direction)
                    continue
                if field.value.kind == "callback":
                    raise Unsupported(f"{value.native}: callback field")
                self.check(field.value, direction)
            (self.inputs if direction == "in" else self.outputs).add(value.native)
            self.used[value.native] = value
            return
        raise Unsupported(f"{value.native}: {value.kind} needs a TypeScript rule")

    def delivered_owner(self, callback, parameter) -> bool:
        """Whether a delivered argument is a handle that the host takes from
        the batch: one the callback owns."""
        return parameter.value.kind == "handle" and parameter.value.ownership == "owned"

    def delivered_handle(self, parameter):
        return parameter.value.handle or self.bound.handles[parameter.value.native]

    def check_callback(self, value: ValuePlan) -> None:
        """Accept a callback that a queue delivers or a wake posts, whose
        copied arguments this binding can decode."""
        callback = self.bound.callbacks[value.native]
        if not callback.delivery and not async_hosts.wake_callback(callback):
            raise Unsupported(
                "callback must answer on its native thread, which TypeScript cannot run"
            )
        for parameter in callback.arguments:
            if self.delivered_owner(callback, parameter):
                handle = self.delivered_handle(parameter)
                if handle.native not in self.bound.public_handles:
                    raise Unsupported("delivered handle requires a generated owner")
                self.owners.add(owner_class(handle))
                continue
            if parameter.value.kind == "handle":
                raise Unsupported("callback handle argument needs an owner rule")
            if parameter.value.kind == "reference" and (
                not parameter.value.element or parameter.value.element.kind != "record"
            ):
                raise Unsupported("delivered reference argument requires a record")
            self.check(parameter.value, "out")

    def check_union(self, record: ValuePlan, field: FieldPlan, direction: str) -> None:
        if not field.presence or not field.presence.tag:
            raise Unsupported(f"{record.native}: union needs a tag")
        tag = next(f for f in record.fields if f.name == field.presence.tag)
        self.check(tag.value, direction)
        for arm in field.value.fields:
            if not arm.presence or not arm.presence.variant:
                raise Unsupported(f"{record.native}: union arm needs a variant")
            self.check(arm.value, direction)
        if not record.union_member:
            # A record of only a union presents the union under its own name.
            self.used[field.value.native] = field.value

    # -- TypeScript types ---------------------------------------------------

    def ts(self, value: ValuePlan) -> str:
        """The TypeScript type of a value, without its absence."""
        if value.kind in {"scalar", "enum", "native_pointer"}:
            return (
                public_name(value.native)
                if value.kind == "enum"
                else self.carrier(value).ts
            )
        if value.kind == "handle":
            return owner_class(value.handle)
        if value.kind == "buffer":
            if value.encoding == "json":
                return "unknown"
            return "Uint8Array" if value.encoding == "bytes" else "string"
        if value.kind == "reference":
            return self.ts(value.element)
        if value.kind == "array":
            return f"readonly {self.ts(value.element)}[]"
        if value.native in self.bound.slices:
            return self.ts(self.bound.slices[value.native].value)
        if value.kind in {"record", "union", "callback"}:
            return public_name(value.native)
        raise Unsupported(f"{value.native}: no TypeScript type")

    def callback_type(self, callback) -> str:
        """The function type of a host callback."""
        parameters = []
        for parameter in callback.arguments:
            if self.delivered_owner(callback, parameter):
                ts = owner_class(self.delivered_handle(parameter))
            else:
                ts = self.ts(parameter.value)
                if parameter.value.nullable:
                    ts += " | undefined"
            parameters.append(f"{identifier(parameter.name)}: {ts}")
        return f"({', '.join(parameters)}) => void"

    def absent(self, value: ValuePlan) -> bool:
        """Whether a value may be absent, as `undefined`."""
        if value.kind == "record" and value.registration:
            entry = self.registration(value)
            return entry.form in {"queue", "wake", "native"} and all(
                f.value.nullable for f in entry.callbacks
            )
        return bool(value.nullable)

    def omittable(self, record: ValuePlan, field: FieldPlan) -> bool:
        """Whether an input may leave a member out and keep its default."""
        if field.presence and field.presence.mask:
            return True
        if self.absent(field.value):
            return True
        if record.default:
            return True
        return self.zero_valid(field.value)

    def zero_valid(self, value: ValuePlan) -> bool:
        # An empty counted buffer is no bytes, which native code validates.
        if value.kind in {"scalar", "enum", "native_pointer"} or is_counted(value):
            return True
        if value.kind == "record" and not value.registration:
            return value.default is not None or all(
                self.zero_valid(f.value) or self.absent(f.value)
                for f in self.fields(value)
            )
        return False

    def all_omittable(self, value: ValuePlan) -> bool:
        return value.kind == "record" and all(
            self.omittable(value, f) for f in self.fields(value)
        )

    def member_optional(self, record: ValuePlan, field: FieldPlan) -> bool:
        if field.presence and field.presence.mask:
            return True
        if self.absent(field.value):
            return True
        if field.value.registration and record.native in self.outputs:
            # A copy leaves a registration unset.
            return True
        return record.native in self.inputs and self.omittable(record, field)

    # -- Declarations -------------------------------------------------------

    def render(self) -> str:
        chunks = []
        for native in sorted(self.used):
            value = self.used[native]
            if value.kind == "enum":
                chunks.append(self.render_enum(value))
            elif value.kind == "union":
                chunks.append(self.render_union(value, value))
            elif value.kind == "record" and value.union_member:
                chunks.append(self.render_union(value.union_member.value, value))
            elif value.kind == "callback":
                callback = self.bound.callbacks[native]
                chunks.append(
                    doc(self.bound, native)
                    + f"export type {public_name(native)} = {self.callback_type(callback)};\n"
                )
            elif value.kind == "record" and value.registration:
                chunks.append(self.render_registration(value))
            elif value.kind == "record":
                chunks.append(self.render_record(value))
        imports = ""
        if self.owners:
            imports = (
                f"import type {{ {', '.join(sorted(self.owners))} }} "
                'from "./generated_operations.ts";\n'
            )
        if any(
            value.registration and self.registration(value).native
            for value in self.used.values()
        ):
            imports += (
                "import {\n  type NativeRegistration,\n  nativeRegistration,\n} "
                'from "./internal/native_registrations.ts";\n'
            )
        return imports + ("\n" if imports else "") + "\n".join(chunks)

    def render_enum(self, value: ValuePlan) -> str:
        public = public_name(value.native)
        scalar = self.enum_scalar(value)
        suffix = "n" if scalar.ts == "bigint" else ""
        members = "".join(
            doc(self.bound, constant, "  ") + f"  {member}: {number}{suffix},\n"
            for member, constant, number in enum_members(value)
        )
        if value.enum_kind == "bitmask":
            kind = f"A bitwise OR of {public} flags."
            alias = scalar.ts
        else:
            kind = f"One of the {public} values, or a value this binding does not know."
            alias = f"(typeof {public})[keyof typeof {public}] | ({scalar.ts} & {{}})"
        return (
            doc(self.bound, value.native)
            + f"export const {public} = {{\n{members}}} as const;\n"
            + f"/** {kind} */\n"
            + f"export type {public} = {alias};\n"
        )

    def member(self, record: ValuePlan, field: FieldPlan) -> str:
        name = identifier(field.name)
        optional = "?" if self.member_optional(record, field) else ""
        readonly = "" if record.native in self.inputs else "readonly "
        return (
            doc(self.bound, f"{record.native}.{field.name}", "  ")
            + f"  {readonly}{name}{optional}: {self.ts(field.value)};\n"
        )

    def render_record(self, value: ValuePlan) -> str:
        public = public_name(value.native)
        members = [self.member(value, field) for field in self.fields(value)]
        readonly = "" if value.native in self.inputs else "readonly "
        for flag in value.mask_flags:
            members.append(
                doc(self.bound, flag.name, "  ")
                + f"  {readonly}{identifier(flag.member)}?: boolean;\n"
            )
        return (
            doc(self.bound, value.native)
            + f"export interface {public} {{\n"
            + "".join(members)
            + "}\n"
        )

    def render_registration(self, value: ValuePlan) -> str:
        public = public_name(value.native)
        entry = self.registration(value)
        if entry.native:
            return self.render_native_registration(value, entry)
        if entry.function:
            callback = self.bound.callbacks[entry.callbacks[0].value.native]
            return (
                doc(self.bound, value.native)
                + f"export type {public} = {self.callback_type(callback)};\n"
            )
        members = []
        for field in entry.callbacks:
            callback = self.bound.callbacks[field.value.native]
            optional = "?" if field.value.nullable else ""
            members.append(
                doc(self.bound, f"{value.native}.{field.name}", "  ")
                + f"  {identifier(field.name)}{optional}: {self.callback_type(callback)};\n"
            )
        members += [self.member(value, field) for field in entry.members]
        return (
            doc(self.bound, value.native)
            + f"export interface {public} {{\n"
            + "".join(members)
            + "}\n"
        )

    def render_native_registration(self, value: ValuePlan, entry: Registration) -> str:
        """A registration type that admits a native registration, a value of
        the same name whose `native()` creates one, and the fields that
        `native()` takes."""
        public = public_name(value.native)
        fields = f"Native{public}Fields"
        native = f'NativeRegistration<"{public}">'
        if entry.form == "native":
            alias = native
        elif entry.function:
            callback = self.bound.callbacks[entry.callbacks[0].value.native]
            alias = f"({self.callback_type(callback)}) | {native}"
        else:
            raise Unsupported(f"{value.native}: no TypeScript form with a native one")
        context = {value.registration.user_data, value.registration.release}
        members = "".join(
            doc(self.bound, f"{value.native}.{field.name}", "  ")
            + f"  readonly {identifier(field.name)}"
            + f"{'?' if field.name in context else ''}: bigint;\n"
            for field in entry.native
        )
        arguments = "".join(
            f"      {identifier(field.name)}: fields.{identifier(field.name)}"
            + (" ?? 0n" if field.name in context else "")
            + ",\n"
            for field in entry.native
        )
        return (
            doc(self.bound, value.native)
            + f"export type {public} = {alias};\n"
            + f"export const {public} = {{\n"
            + "  /**\n"
            + "   * Creates a native registration from a native function pointer for\n"
            + "   * each callback, and an optional context and release. The binding\n"
            + "   * copies them into the C descriptor unchanged. The first call that\n"
            + "   * carries the registration consumes it: native code releases the\n"
            + "   * context of a call that it accepts, and the binding releases it for\n"
            + "   * any other call. A later call throws a `MaplibreError` with the\n"
            + "   * invalid-state status.\n"
            + "   */\n"
            + f"  native(fields: {fields}): {native} {{\n"
            + f'    return nativeRegistration("{public}", {{\n'
            + arguments
            + "    });\n"
            + "  },\n"
            + "};\n\n"
            + f"/** The fields of a native {{@link {public}}}, as native addresses. */\n"
            + f"export interface {fields} {{\n"
            + members
            + "}\n"
        )

    def render_union(self, value: ValuePlan, owner: ValuePlan) -> str:
        """The union `value` as a type named after `owner`: the union itself, or
        the record that holds only it and its tag."""
        public = public_name(owner.native)
        arms = []
        for _, kind, arm in union_kinds(value):
            if arm is None:
                arms.append(f'  | {{ readonly kind: "{kind}" }}\n')
                continue
            field = next(f for f in value.fields if f.name == arm)
            arms.append(
                f'  | {{ readonly kind: "{kind}"; readonly value: {self.ts(field.value)} }}\n'
            )
        if owner.native in self.bound.returned:
            # Only a copy from native can hold a tag this binding predates.
            arms.append(
                '  | { readonly kind: "unknown"; readonly tag: number; readonly bytes: Uint8Array }'
            )
        return (
            doc(self.bound, owner.native)
            + f"export type {public} =\n"
            + "".join(arms).rstrip("\n")
            + ";\n"
        )
