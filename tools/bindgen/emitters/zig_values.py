"""Generate allocator-free Zig values from resolved scalar and record plans."""

from os.path import commonprefix

from ..model import ModelError
from ..semantic import ValuePlan
from .zig import SCALARS, identifier, pascal


def field_default(values, field) -> str:
    """The Zig default of a required field: its annotated initial value, a
    record's own field defaults, or zero."""
    value, initial = field.value, field.initial
    public = values.public(value)
    if initial is None:
        return ".{}" if value.kind == "record" else f"std.mem.zeroes({public})"
    if value.kind != "enum":
        return initial.literal
    member = identifier(initial.member)
    if value.enum_kind != "bitmask":
        return f".{member}"
    if initial.value & (initial.value - 1) == 0:
        return f".{{ .{member} = true }}"
    return f"{public}.{member}"


class Values:
    def __init__(self, bound):
        self.bound = bound
        self.used = {}
        self.item_buffers = {}

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
            if value.item_buffer:
                self.item_buffers[value.element.native] = value.item_buffer
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
        for group in value.presence_groups:
            if len(group.fields) > 1 and group.type:
                self.add(self.bound.values[group.type])
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

    def render(self):
        return "\n".join(self.record(value) for _, value in sorted(self.used.items()))

    def record(self, value: ValuePlan):
        from .zig_dynamic_values import declaration, dynamic

        if value.response:
            return f"pub const {self.public(value)} = struct {{ native: *c.{value.native} }};\n"
        if value.kind == "union":
            fields = "\n".join(
                f"    {identifier(field.name)}: {self.public(field.value)},"
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
                writes.append(
                    f"        raw.{field.name} = {'false' if field.value.ctype.canonical in {'_Bool', 'bool'} else '0'};"
                )
        for flag in value.mask_flags:
            local = identifier(flag.member)
            fields.append(f"    {local}: bool = false,")
            copies.append(
                f"            .{local} = raw.{flag.mask} & c.{flag.name} != 0,"
            )
            writes.append(
                f"        if (self.{local}) raw.{flag.mask} |= c.{flag.name};"
            )
        grouped = {
            name
            for group in value.presence_groups
            if len(group.fields) > 1
            for name in group.fields
        }
        for group in value.presence_groups:
            if len(group.fields) == 1:
                continue
            child = self.bound.values[group.type]
            local = identifier(group.member)
            fields.append(f"    {local}: ?{self.public(child)} = null,")
            writes.append(
                f"        if (self.{local}) |item| {{ raw.{identifier(group.mask)} |= c.{group.bit}; "
                + " ".join(
                    f"raw.{identifier(name)} = {self.materialize(next(f.value for f in value.fields if f.name == name), 'item.' + identifier(name))};"
                    for name in group.fields
                )
                + " }"
            )
            copies.append(
                f"            .{local} = if (raw.{identifier(group.mask)} & c.{group.bit} != 0) .{{ "
                + ", ".join(
                    f".{identifier(name)} = {self.capture(next(f.value for f in value.fields if f.name == name), 'raw.' + identifier(name))}"
                    for name in group.fields
                )
                + " } else null,"
            )
        for field in value.fields:
            local = identifier(field.name)
            if field.role == "size":
                if not value.default:
                    writes.append(f"        raw.{local} = @sizeOf(c.{value.native});")
                continue
            if not field.public or field.name in grouped:
                continue
            optional = field.presence and field.presence.mask
            initial = "null" if optional else field_default(self, field)
            fields.append(
                f"    {local}: {'?' if optional else ''}{self.public(field.value)} = {initial},"
            )
            copy = self.capture(field.value, f"raw.{local}")
            if optional:
                mask, bit = identifier(field.presence.mask), field.presence.bit
                present = f"raw.{mask} & c.{bit} != 0" if bit else f"raw.{mask}"
                copies.append(
                    f"            .{local} = if ({present}) {copy} else null,"
                )
                writes.append(
                    f"        marshal.present(&raw.{mask}, {'c.' + bit if bit else 'true'}, &raw.{local}, self.{local});"
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
        return f"""pub const {public} = struct {{
{chr(10).join(fields)}
    pub fn toNative(self: {public}) c.{value.native} {{
        var raw = {initial};
{chr(10).join(writes)}
        return raw;
    }}
    pub fn fromNative(raw: c.{value.native}) {public} {{
        return .{{
{chr(10).join(copies)}
        }};
    }}
}};
"""

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
        prefix = (
            commonprefix([key for key, _ in value.enum_values]).rsplit("_", 1)[0] + "_"
        )
        if value.enum_kind == "bitmask":
            single = [
                (identifier(key.removeprefix(prefix).lower()), number)
                for key, number in value.enum_values
                if number > 0 and number & (number - 1) == 0
            ]
            fields = "\n".join(f"    {local}: bool = false," for local, _ in single)
            constants = "".join(
                f"    pub const {identifier(key.removeprefix(prefix).lower())} = fromNative({number});\n"
                for key, number in value.enum_values
                if number == 0 or number & (number - 1) != 0
            )
            bits = ", ".join(str(number) for _, number in single)
            methods = "".join(
                f"    pub const {method} = methods.{method};\n"
                for method in (
                    "fromNative",
                    "toNative",
                    "contains",
                    "isEmpty",
                    "unionWith",
                )
            )
            return f"pub const {public} = struct {{\n{fields}\n    unknown_bits: {raw} = 0,\n    pub const native_bits = [_]{raw}{{ {bits} }};\n    const methods = marshal.FlagMethods(@This());\n{methods}{constants}}};\n"
        members = "\n".join(
            f"    {identifier(key.removeprefix(prefix).lower())} = {number},"
            for number, key in {
                number: key for key, number in reversed(value.enum_values)
            }.items()
        )
        return f"pub const {public} = enum({raw}) {{\n{members}\n    _,\n    pub const fromNative = marshal.EnumMethods(@This()).fromNative;\n    pub const toNative = marshal.EnumMethods(@This()).toNative;\n}};\n"
