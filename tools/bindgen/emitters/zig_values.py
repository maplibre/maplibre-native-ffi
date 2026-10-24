"""Generate allocator-free Zig values from resolved scalar and record plans."""

from ..model import ModelError
from ..semantic import ValuePlan, enum_member_prefix
from .zig import SCALARS, doc, documented, identifier, pascal


def field_default(values, field) -> str | None:
    value = field.value
    if value.default:
        return None
    if value.kind == "enum":
        if value.enum_kind == "bitmask":
            return ".{}"
        prefix = enum_member_prefix((key for key, _ in value.enum_values), value.native)
        zero = next((key for key, number in value.enum_values if number == 0), None)
        return (
            "." + identifier(zero.removeprefix(prefix).lower())
            if zero
            else "@enumFromInt(0)"
        )
    if value.kind != "record":
        public = values.public(value)
        if public.startswith("?"):
            return "null"
        if public == "bool":
            return "false"
        if public in SCALARS.values():
            return "0"
        return f"std.mem.zeroes({public})"
    # `.{}` builds a record only when each of its public members has a default
    # too, which a member record with a default function lacks.
    members = (
        member
        for member in value.fields
        if member.public
        and member.value.kind == "record"
        and not (member.presence and member.presence.mask)
    )
    if any(field_default(values, member) is None for member in members):
        return None
    return ".{}"


def first_names(enum_values):
    """Each distinct value under its first name, in header order."""
    seen = set()
    for key, number in enum_values:
        if number not in seen:
            seen.add(number)
            yield key, number


def flag_literal(single, number) -> str:
    """A flag set's literal naming the single bits that `number` sets."""
    members = [f".{local} = true" for local, bit, _ in single if number & bit]
    unknown = number & ~sum(bit for _, bit, _ in single)
    if unknown:
        members.append(f".unknown_bits = {unknown}")
    return ".{ " + ", ".join(members) + " }" if members else ".{}"


class Values:
    def __init__(self, bound):
        self.bound = bound
        self.used = {}
        self.results = {}
        # The methods of each response type, by native name.
        self.methods = {}

    def native_type(self, value):
        return (
            self.public(value)
            if value.kind in {"scalar", "native_pointer"}
            else "c." + value.native
        )

    def public(self, value):
        from .zig_dynamic_values import public

        specialized = public(self, value)
        if specialized:
            return specialized
        if value.kind == "handle":
            return pascal(value.native.removeprefix("mln_"))
        if value.kind == "native_pointer":
            return (
                "c." + value.native
                if value.ctype.kind == "typedef"
                else "?*"
                + (
                    "const "
                    if value.ctype.pointee and value.ctype.pointee.const
                    else ""
                )
                + "anyopaque"
            )
        if value.kind == "scalar":
            canonical = value.scalar_carrier or value.ctype.canonical
            public = SCALARS.get(
                canonical,
                SCALARS.get(
                    canonical,
                    {
                        "unsigned char": "u8",
                        "long": "i64",
                        "long long": "i64",
                        "unsigned long": "u64",
                        "unsigned long long": "u64",
                        "int": "i32",
                        "unsigned int": "u32",
                    }.get(canonical),
                ),
            )
            if public:
                return public
        if value.kind == "union" and value.native.startswith("@"):
            parent, field = next(
                (parent, field)
                for parent in self.bound.values.values()
                for field in parent.fields
                if field.value.native == value.native
            )
            return pascal(parent.native.removeprefix("mln_")) + pascal(field.name)
        if value.kind in {"record", "enum", "union"}:
            return pascal(value.native.removeprefix("mln_"))
        raise ModelError(
            [f"Zig: {value.native}: {value.kind} needs a value conversion"]
        )

    def add(self, value):
        self.public(value)
        if value.registration:
            from .zig_callbacks import add

            return add(self, value)
        if value.response:
            self.used[value.native] = value
            return
        if value.kind == "enum":
            self.used[value.native] = value
            return
        if value.kind in {"array", "reference"}:
            self.add(value.element)
        if value.kind == "union":
            if not value.tag or any(
                not f.presence or not f.presence.variant for f in value.fields
            ):
                raise ModelError(
                    [f"Zig: {value.native}: union requires tagged variants"]
                )
            for field in value.fields:
                self.add(field.value)
            self.used[value.native] = value
            return
        if value.kind != "record":
            return
        for field in value.fields:
            if not field.public:
                continue
            if field.presence and field.presence.variant:
                raise ModelError(
                    [f"Zig: {value.native}: variant requires tag decoding"]
                )
            self.add(field.value)
        self.used[value.native] = value

    def capture(self, value, source):
        return (
            source
            if value.kind in {"scalar", "native_pointer"}
            else f"{self.public(value)}.fromNative({source})"
        )

    def materialize(self, value, source):
        return (
            source
            if value.kind in {"scalar", "native_pointer"}
            else f"{source}.toNative()"
        )

    def result(self, name, fields, summary):
        """Names the struct that a call with several results returns."""
        members = "".join(f"    {label}: {typ},\n" for label, typ in fields)
        self.results[name] = (
            f"/// {summary}\npub const {name} = struct {{\n{members}}};\n"
        )
        return name

    def render(self):
        return "\n".join(
            [
                *(
                    documented(
                        self.bound,
                        value.native,
                        self.record(value),
                        f"pub const {self.public(value)} = ",
                    )
                    for _, value in sorted(self.used.items())
                ),
                *(code for _, code in sorted(self.results.items())),
            ]
        )

    def record(self, value: ValuePlan):
        from .zig_dynamic_values import declaration, dynamic

        if value.response:
            methods = "\n".join(self.methods.get(value.native, []))
            return f"pub const {self.public(value)} = struct {{ native: *c.{value.native},\n{methods}\n}};\n"
        if value.kind == "union":
            fields = "\n".join(
                doc(self.bound, f"{value.native}.{field.name}", "    ")
                + f"    {identifier(field.name)}: {self.public(field.value)},"
                for field in value.fields
            )
            empty = "    empty,\n" if value.empty_variant else ""
            return f"pub const {self.public(value)} = union(enum) {{\n{fields}\n{empty}    unknown: u32,\n}};\n"
        if dynamic(value):
            return declaration(self, value)
        public = self.public(value)
        if value.kind == "enum":
            return self.enumeration(value)
        fields, copies, writes = [], [], []
        for field in value.fields:
            if field.role == "presence_mask":
                writes.append(f"        raw.{field.name} = 0;")
        for flag in value.mask_flags:
            local = identifier(flag.member)
            fields.append(
                f"{doc(self.bound, flag.name, '    ')}    {local}: bool = false,"
            )
            copies.append(
                f"            .{local} = raw.{flag.mask} & c.{flag.name} != 0,"
            )
            writes.append(
                f"        if (self.{local}) raw.{flag.mask} |= c.{flag.name};"
            )
        for field in value.fields:
            local = identifier(field.name)
            if field.role == "size":
                if not value.default:
                    writes.append(f"        raw.{local} = @sizeOf(c.{value.native});")
                continue
            if not field.public:
                continue
            optional = field.presence and field.presence.mask
            initial = "null" if optional else field_default(self, field)
            fields.append(
                doc(self.bound, f"{value.native}.{field.name}", "    ")
                + f"    {local}: {'?' if optional else ''}{self.public(field.value)}"
                + (
                    f" = {initial}"
                    if initial is not None and (not value.default or optional)
                    else ""
                )
                + ","
            )
            copy = self.capture(field.value, f"raw.{local}")
            if optional:
                mask, bit = identifier(field.presence.mask), field.presence.bit
                present = f"raw.{mask} & c.{bit} != 0"
                copies.append(
                    f"            .{local} = if ({present}) {copy} else null,"
                )
                writes.append(
                    f"        marshal.present(&raw.{mask}, c.{bit}, &raw.{local}, self.{local});"
                )
            else:
                copies.append(f"            .{local} = {copy},")
                writes.append(
                    f"        raw.{local} = {self.materialize(field.value, 'self.' + local)};"
                )
        initial = (
            f"c.{value.default}()"
            if value.default
            else f"std.mem.zeroes(c.{value.native})"
        )
        encoder = (
            f"""    pub fn toNative(self: {public}) c.{value.native} {{
        var raw = {initial};
{chr(10).join(writes)}
        return raw;
    }}
"""
            if value.native in self.bound.inputs
            else ""
        )
        rendered = f"""pub const {public} = struct {{
{chr(10).join(fields)}
{encoder}    pub fn fromNative(raw: c.{value.native}) {public} {{
        return .{{
{chr(10).join(copies)}
        }};
    }}
}};
"""

        return rendered

    def enumeration(self, value):
        public = self.public(value)
        canonical = (
            value.enum_underlying.canonical
            if value.enum_underlying
            else value.ctype.canonical
        )
        canonical = value.scalar_carrier or canonical
        raw = SCALARS.get(
            canonical,
            {
                "int": "i32",
                "unsigned int": "u32",
                "long": "i64",
                "long long": "i64",
                "unsigned long": "u64",
                "unsigned long long": "u64",
            }.get(canonical, "u32"),
        )
        prefix = enum_member_prefix((key for key, _ in value.enum_values), value.native)
        if value.enum_kind == "bitmask":
            single = [
                (identifier(key.removeprefix(prefix).lower()), number, key)
                for key, number in value.enum_values
                if number > 0 and number & (number - 1) == 0
            ]
            fields = "\n".join(
                f"{doc(self.bound, key, '    ')}    {local}: bool = false,"
                for local, _, key in single
            )
            constants = "".join(
                f"    pub const {identifier(key.removeprefix(prefix).lower())}: {public} = {flag_literal(single, number)};\n"
                for key, number in value.enum_values
                if number == 0 or number & (number - 1) != 0
            )
            bits = ", ".join(str(number) for _, number, _ in single)
            methods = "".join(
                f"    pub const {method} = methods.{method};\n"
                for method in (
                    "fromNative",
                    "toNative",
                )
            )
            return f"pub const {public} = struct {{\n{fields}\n    unknown_bits: {raw} = 0,\n    pub const native_bits = [_]{raw}{{ {bits} }};\n    const methods = marshal.FlagMethods(@This());\n{methods}{constants}}};\n"
        members = "\n".join(
            f"{doc(self.bound, key, '    ')}    {identifier(key.removeprefix(prefix).lower())} = {number},"
            for key, number in first_names(value.enum_values)
        )
        return f"pub const {public} = enum({raw}) {{\n{members}\n    _,\n    pub const fromNative = marshal.EnumMethods(@This()).fromNative;\n    pub const toNative = marshal.EnumMethods(@This()).toNative;\n}};\n"
