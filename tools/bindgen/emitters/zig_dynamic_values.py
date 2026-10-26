"""Zig borrowed input records and arena-owned result snapshots."""

from dataclasses import replace

from ..model import ModelError
from ..semantic import required_callbacks
from .rust_dynamic_values import dynamic
from .zig import doc, identifier
from .zig_values import field_default


def slice_of(values, value):
    """The array field of a `kind=slice` record, which Zig presents as a slice
    in place of the record."""
    return values.bound.slices.get(value.native) if value.kind == "record" else None


def public(values, value):
    if array := slice_of(values, value):
        return values.public(array.value)
    if value.kind == "buffer":
        typ = "[]const u8"
    elif value.kind == "array":
        typ = (
            f"[{value.length}]{values.public(value.element)}"
            if value.ctype.kind == "array"
            else f"[]const {values.public(value.element)}"
        )
    elif value.kind == "reference":
        typ = values.public(value.element)
    else:
        return None
    return "?" + typ if value.nullable else typ


def encode(values, value, source, depth=0):
    item = f"array_item_{depth}"
    optional = value.nullable
    if value.kind in {"buffer", "array", "reference"} and optional:
        fallback = (
            "std.mem.zeroes(c.mln_buffer_view)"
            if value.buffer_form == "view"
            else "null"
        )
        return f"if ({source}) |{item}| {encode(values, replace(value, nullable=False), item, depth + 1)} else {fallback}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"try marshal.cString(allocator, {source})"
        return (
            f"marshal.view({source})"
            if value.buffer_form == "view"
            else f"@ptrCast({source}.ptr)"
        )
    if value.kind == "reference":
        return f"try marshal.store(allocator, {encode(values, value.element, source)})"
    if value.kind == "array":
        if value.element.kind == "scalar" and value.ctype.kind != "array":
            return f"{source}.ptr"
        native = values.native_type(value.element)
        fixed = value.ctype.kind == "array"
        storage = (
            f"var items: [{value.length}]{native} = undefined;"
            if fixed
            else f"const items = try allocator.alloc({native}, {source}.len);"
        )
        return f"blk: {{ {storage} for ({source}, 0..) |{item}, index| items[index] = {encode(values, value.element, item, depth + 1)}; break :blk items{'' if fixed else '.ptr'}; }}"
    if value.kind in {"scalar", "native_pointer"}:
        return source
    return to_native(value, source)


def encoding(value) -> tuple[bool, bool]:
    """Whether encoding `value` allocates, and whether it retains callback roots."""
    if value.registration:
        allocates, roots = fields_encoding(value)
        return allocates, True
    if value.kind == "buffer":
        return value.length == "nul", False
    if value.kind == "reference":
        return True, encoding(value.element)[1]
    if value.kind == "array":
        allocates, roots = encoding(value.element)
        if value.ctype.kind == "array":
            return allocates, roots
        return value.element.kind != "scalar" or allocates, roots
    if value.kind in {"record", "union"}:
        return fields_encoding(value)
    return False, False


def fields_encoding(value) -> tuple[bool, bool]:
    hidden = (
        {
            *value.registration.callbacks,
            value.registration.user_data,
            value.registration.release,
        }
        if value.registration
        else set()
    )
    parts = [
        encoding(field.value)
        for field in value.fields
        if field.name not in hidden and (field.public or value.kind == "union")
    ]
    return any(a for a, _ in parts), any(r for _, r in parts)


def to_native_parameters(value) -> list[str]:
    """The arguments that a record's `toNative` takes after `self`."""
    allocates, roots = encoding(value)
    return [*(["allocator"] if allocates else []), *(["roots"] if roots else [])]


def to_native(value, source):
    if not dynamic(value):
        return f"{source}.toNative()"
    return f"try {source}.toNative({', '.join(to_native_parameters(value))})"


def borrows(values, value):
    """Whether a copy of `value` can borrow native's text and needs no
    allocator: none of its arrays has a length that varies."""
    if slice_of(values, value):
        return False
    if value.kind == "array":
        return value.ctype.kind == "array" and borrows(values, value.element)
    if value.kind == "reference":
        return borrows(values, value.element)
    if value.kind in {"record", "union"}:
        return all(
            borrows(values, field.value)
            for field in value.fields
            if field.public or value.kind == "union"
        )
    return True


def lends(values, value):
    """Whether a record's `fromNative` borrows native's text, which a callback
    argument alone reaches and native lends until the callback returns."""
    return (
        value.kind == "record" and dynamic(value) and value.native in values.bound.lent
    )


def copies_with_allocator(values, value):
    """Whether a record's `fromNative` takes an allocator: a copy owns its
    text, or a borrowed copy still allocates an array whose length varies."""
    return not lends(values, value) or not borrows(values, value)


def decode(values, value, source, context="raw", borrowed=False):
    """Copies `value` from `source`. A `borrowed` copy points into native's
    text instead of copying it."""
    if value.registration and value.native not in values.bound.returned:
        # Native never returns callbacks, so a copy leaves a registration
        # unset.
        return "null" if value.nullable else ".{}"
    if array := slice_of(values, value):
        return decode(
            values, array.value, f"{source}.{identifier(array.name)}", source, borrowed
        )
    optional = value.nullable
    if value.kind in {"buffer", "array", "reference"} and optional:
        absent = (
            (f"{source}.data == null" if value.nullable else f"{source}.size == 0")
            if value.buffer_form == "view"
            else f"{source} == null"
        )
        return f"if ({absent}) null else {decode(values, replace(value, nullable=False), source, context, borrowed)}"
    if value.kind == "buffer":
        if value.length == "nul":
            span = f"std.mem.span({source} orelse return error.NativeError)"
            return span if borrowed else f"try allocator.dupe(u8, {span})"
        if value.buffer_form != "view":
            count = (
                value.length
                if value.length.isdigit()
                else f"{context}.{identifier(value.length)}"
            )
            source = f".{{ .data = {source}, .size = {count} }}"
        if borrowed:
            return f"try marshal.borrowView({source})"
        return f"try marshal.copyView(allocator, {source})"
    if value.kind == "reference" and slice_of(values, value.element):
        view = decode(values, value.element, "slice_view", borrowed=borrowed)
        return f"slice: {{ const slice_view = ({source} orelse return error.NativeError).*; break :slice {view}; }}"
    if value.kind == "reference":
        return decode(
            values,
            value.element,
            f"({source} orelse return error.NativeError).*",
            context,
            borrowed,
        )
    if value.kind == "array":
        fixed = value.ctype.kind == "array"
        count = (
            value.length
            if value.length.isdigit()
            else context + "." + identifier(value.length)
        )
        items = (
            source
            if fixed
            else f"try marshal.nativeSlice({values.native_type(value.element)}, {source}, {count})"
        )
        storage = (
            f"var copied: [{value.length}]{values.public(value.element)} = undefined;"
            if fixed
            else f"const copied = try allocator.alloc({values.public(value.element)}, {count});"
        )
        copied = decode(values, value.element, "item", context, borrowed)
        if value.stride:
            return f"blk: {{ {storage} for (0..{count}) |index| {{ const item = try marshal.stridedAt({values.native_type(value.element)}, {source}, {count}, {context}.{identifier(value.stride)}, index); copied[index] = {copied}; }} break :blk copied; }}"
        return f"blk: {{ {storage} for ({items}, 0..) |item, index| copied[index] = {copied}; break :blk copied; }}"
    if value.kind == "union":
        return variant(values, value, value, source, context, borrowed)
    if value.kind in {"scalar", "native_pointer"}:
        return source
    if not dynamic(value):
        return f"{values.public(value)}.fromNative({source})"
    if copies_with_allocator(values, value):
        return f"try {values.public(value)}.fromNative(allocator, {source})"
    return f"try {values.public(value)}.fromNative({source})"


def variant(values, union, owner, source, context, borrowed=False):
    """Copies the variant of the union at `source` that the tag in `context`
    selects. Only a returned `owner` keeps a tag it predates as `unknown`."""
    cases = ", ".join(
        f"c.{field.presence.variant} => .{{ .{identifier(field.name)} = {decode(values, field.value, source + '.' + identifier(field.name), context, borrowed)} }}"
        for field in union.fields
    )
    if union.empty_variant:
        cases += f", c.{union.empty_variant[0]} => .empty"
    unknown = (
        "else => |tag| .{ .unknown = @intCast(tag) }"
        if owner.native in values.bound.returned
        else "else => return error.NativeError"
    )
    return f"switch ({context}.{identifier(union.tag)}) {{ {cases}, {unknown} }}"


def submit(values, union, owner, source, place):
    """Writes the variant `source` and its tag into the union storage `place`."""
    tag = f"raw.{identifier(union.tag)}"
    cases = [
        f".{identifier(member.name)} => |item| {{ {tag} = c.{member.presence.variant}; {place}.{identifier(member.name)} = {encode(values, member.value, 'item')}; }},"
        for member in union.fields
    ]
    if union.empty_variant:
        cases.append(f".empty => {tag} = c.{union.empty_variant[0]},")
    if owner.native in values.bound.returned:
        cases.append(".unknown => return error.InvalidArgument,")
    return f"        switch ({source}) {{ {' '.join(cases)} }}"


def declaration(values, value):
    typ = values.public(value)
    borrowed = lends(values, value)
    fields, writes, captures = [], [], []
    trampolines = []
    hidden = set()
    if value.registration:
        from .zig_callbacks import parts

        fields, callback_writes, captures, trampolines = parts(values, value)
        hidden = {
            *value.registration.callbacks,
            value.registration.user_data,
            value.registration.release,
        }
    for field in value.fields:
        if field.name in hidden:
            continue
        raw = f"raw.{identifier(field.name)}"
        local = identifier(field.name)
        if field.role == "size":
            writes.append(f"        {raw} = @sizeOf(c.{value.native});")
            continue
        if field.role == "presence_mask":
            writes.insert(
                0,
                f"        {raw} = {'false' if field.value.ctype.canonical in {'bool', '_Bool'} else '0'};",
            )
            continue
        if field.role == "count":
            array = next(f for f in value.fields if f.value.length == field.name)
            expression = f"self.{identifier(array.name)}"
            length = (
                f"if ({expression}) |items| items.len else 0"
                if array.presence or array.value.nullable
                else f"{expression}.len"
            )
            writes.append(
                f"        {raw} = std.math.cast(@TypeOf({raw}), {length}) orelse return error.InvalidArgument;"
            )
            continue
        if not field.public:
            continue
        if field.value.kind == "union":
            union = field.value
            default = field_default(values, field)
            fields.append(
                doc(values.bound, f"{value.native}.{field.name}", "    ")
                + f"    {local}: {values.public(union)}"
                + (f" = {default}" if default is not None else "")
                + ","
            )
            captures.append(
                f"            .{local} = {decode(values, union, raw, borrowed=borrowed)},"
            )
            writes.append(submit(values, union, union, f"self.{local}", raw))
            continue
        optional = field.presence and field.presence.mask
        public_type = values.public(field.value)
        default = "null" if optional else field_default(values, field)
        fields.append(
            doc(values.bound, f"{value.native}.{field.name}", "    ")
            + f"    {local}: {'?' if optional else ''}{public_type}"
            + (f" = {default}" if default is not None else "")
            + ","
        )
        copied = decode(values, field.value, raw, borrowed=borrowed)
        if optional:
            path = identifier(field.presence.mask)
            bit = field.presence.bit
            present = f"raw.{path} & c.{bit} != 0"
            mark = f"raw.{path} |= c.{bit}"
            writes.append(
                f"        marshal.present(&raw.{path}, c.{bit}, &{raw}, self.{local});"
                if not dynamic(field.value)
                else f"        if (self.{local}) |item| {{ {mark}; {raw} = {encode(values, field.value, 'item')}; }}"
            )
            captures.append(
                f"            .{local} = if ({present}) {copied} else null,"
            )
        else:
            writes.append(
                f"        {raw} = {encode(values, field.value, 'self.' + local)};"
            )
            captures.append(f"            .{local} = {copied},")
    if value.registration:
        writes.extend(callback_writes)
    encoder_parameters = to_native_parameters(value)
    for name in ("allocator", "roots"):
        if (name in encoder_parameters) != any(name in line for line in writes):
            raise ModelError(
                [
                    f"Zig: {value.native}: toNative {name} use disagrees with its encoding"
                ]
            )
    signature = "".join(
        {
            "allocator": ", allocator: std.mem.Allocator",
            "roots": ", roots: *callbacks.Roots",
        }[name]
        for name in encoder_parameters
    )
    allocates = copies_with_allocator(values, value)
    capture_use = (
        ""
        if not allocates or any("allocator" in line for line in captures)
        else "        _ = allocator;\n"
    )
    if not any("raw." in line for line in captures):
        capture_use += "        _ = raw;\n"
    # A lent copy's text is native's, valid until the callback returns.
    lent_doc = (
        "    /// Borrows the record's text, which native lends until the callback returns.\n"
        if borrowed
        else ""
    )
    # A registration that native returns copies with its callbacks unset.
    copy = (
        f"""{lent_doc}    pub fn fromNative({"allocator: std.mem.Allocator, " if allocates else ""}raw: c.{value.native}) status.Error!{typ} {{
{capture_use}        return .{{
{chr(10).join(captures)}
        }};
    }}
"""
        if not value.registration or value.native in values.bound.returned
        else ""
    )
    if copy and required_callbacks(value):
        # A copy holds no callbacks, so it can't hold the required ones.
        raise ModelError(
            [f"Zig: {value.native}: a returned registration requires callbacks"]
        )
    encoder = (
        f"""    pub fn toNative(self: {typ}{signature}) status.Error!c.{value.native} {{
        var raw = std.mem.zeroes(c.{value.native});
{chr(10).join(writes)}
        return raw;
    }}
"""
        if value.native in values.bound.inputs
        else ""
    )
    rendered = f"""pub const {typ} = struct {{
{chr(10).join(fields)}
{encoder}{chr(10).join(trampolines)}
{copy}}};
"""
    return rendered


def union_declaration(values, value):
    """The union(enum) of a record that holds only a union and its tag. Its
    conversion writes the size, and the tag from the variant."""
    typ = values.public(value)
    member = value.union_member
    raw = f"raw.{identifier(member.name)}"
    writes = [
        f"        raw.{identifier(field.name)} = @sizeOf(c.{value.native});"
        for field in value.fields
        if field.role == "size"
    ]
    writes.append(submit(values, member.value, value, "self", raw))
    signature = "".join(
        {
            "allocator": ", allocator: std.mem.Allocator",
            "roots": ", roots: *callbacks.Roots",
        }[name]
        for name in to_native_parameters(value)
    )
    copied = variant(values, member.value, value, raw, "raw")
    capture_use = "" if "allocator" in copied else "        _ = allocator;\n"
    encoder = (
        f"""    pub fn toNative(self: {typ}{signature}) status.Error!c.{value.native} {{
        var raw = std.mem.zeroes(c.{value.native});
{chr(10).join(writes)}
        return raw;
    }}

"""
        if value.native in values.bound.inputs
        else ""
    )
    return f"""pub const {typ} = union(enum) {{
{values.variants(member.value, value)}
{encoder}    pub fn fromNative(allocator: std.mem.Allocator, raw: c.{value.native}) status.Error!{typ} {{
{capture_use}        return {copied};
    }}
}};
"""
