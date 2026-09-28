"""Zig borrowed input records and arena-owned result snapshots."""

from dataclasses import replace

from .rust_dynamic_values import dynamic
from .zig import identifier, pascal


def public(values, value):
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
    return "?" + typ if value.nullable or value.optional == "empty" else typ


def encode(values, value, source, depth=0):
    item = f"array_item_{depth}"
    optional = value.nullable or value.optional == "empty"
    if value.kind in {"buffer", "array", "reference"} and optional:
        fallback = (
            "std.mem.zeroes(c.mln_buffer_view)"
            if value.native == "mln_buffer_view"
            else "null"
        )
        return f"if ({source}) |{item}| {encode(values, replace(value, nullable=False, optional=None), item, depth + 1)} else {fallback}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"try cString(allocator, {source})"
        return (
            f"view({source})"
            if value.native == "mln_buffer_view"
            else f"@ptrCast({source}.ptr)"
        )
    if value.kind == "reference":
        return f"try store(allocator, {encode(values, value.element, source)})"
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
    return (
        f"try {source}.toNative(allocator, roots)"
        if dynamic(value)
        else f"{source}.toNative()"
    )


def decode(values, value, source, context="raw"):
    optional = value.nullable or value.optional == "empty"
    if value.kind in {"buffer", "array", "reference"} and optional:
        absent = (
            (f"{source}.data == null" if value.nullable else f"{source}.size == 0")
            if value.native == "mln_buffer_view"
            else f"{source} == null"
        )
        return f"if ({absent}) null else {decode(values, replace(value, nullable=False, optional=None), source, context)}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"try allocator.dupe(u8, std.mem.span({source} orelse return error.NativeError))"
        if value.native != "mln_buffer_view":
            count = (
                value.length
                if value.length.isdigit()
                else f"{context}.{identifier(value.length)}"
            )
            source = f".{{ .data = {source}, .size = {count} }}"
        return f"try copyView(allocator, {source})"
    if value.kind == "reference":
        return decode(
            values,
            value.element,
            f"({source} orelse return error.NativeError).*",
            context,
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
            else f"try nativeSlice({values.native_type(value.element)}, {source}, {count})"
        )
        storage = (
            f"var copied: [{value.length}]{values.public(value.element)} = undefined;"
            if fixed
            else f"const copied = try allocator.alloc({values.public(value.element)}, {count});"
        )
        copied = decode(values, value.element, "item", context)
        if value.item_buffer:
            buf = value.item_buffer
            copied = f"blk_item: {{ var converted = {copied}; converted.{identifier(buf.field)} = try copyArenaString(allocator, {context}.{identifier(buf.data)}, {context}.{identifier(buf.size)}, item.{identifier(buf.offset)}, item.{identifier(buf.length)}); break :blk_item converted; }}"
        if value.stride:
            return f"blk: {{ {storage} for (0..{count}) |index| {{ const item = try stridedAt({values.native_type(value.element)}, {source}, {count}, {context}.{identifier(value.stride)}, index); copied[index] = {copied}; }} break :blk copied; }}"
        return f"blk: {{ {storage} for ({items}, 0..) |item, index| copied[index] = {copied}; break :blk copied; }}"
    if value.kind == "union":
        cases = ", ".join(
            f"c.{field.presence.variant} => .{{ .{identifier(field.name)} = {decode(values, field.value, source + '.' + identifier(field.name), context)} }}"
            for field in value.fields
        )
        if value.empty_variant:
            cases += f", c.{value.empty_variant[0]} => .empty"
        return f"switch ({context}.{identifier(value.tag)}) {{ {cases}, else => |tag| .{{ .unknown = @intCast(tag) }} }}"
    if value.kind in {"scalar", "native_pointer"}:
        return source
    return (
        f"try {values.public(value)}.fromNative(allocator, {source})"
        if dynamic(value)
        else f"{values.public(value)}.fromNative({source})"
    )


def declaration(values, value):
    typ = values.public(value)
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
    grouped = set()
    groups = []
    for group in value.presence_groups:
        if len(group.fields) < 2 or group.type:
            continue
        local = identifier(group.mask.removeprefix("has_"))
        name = typ + pascal(group.mask.removeprefix("has_"))
        members = [next(f for f in value.fields if f.name == n) for n in group.fields]
        groups.append(
            f"pub const {name} = struct {{ "
            + ", ".join(
                f"{identifier(f.name)}: {values.public(f.value)}" for f in members
            )
            + " };\n"
        )
        fields.append(f"    {local}: ?{name} = null,")
        mask = ".".join(identifier(part) for part in group.mask.split("."))
        present = f"raw.{mask} & c.{group.bit} != 0" if group.bit else f"raw.{mask}"
        mark = f"raw.{mask} |= c.{group.bit}" if group.bit else f"raw.{mask} = true"
        assigned = " ".join(
            f"raw.{identifier(f.name)} = {encode(values, f.value, 'item.' + identifier(f.name))};"
            for f in members
        )
        copied = ", ".join(
            f".{identifier(f.name)} = {decode(values, f.value, 'raw.' + identifier(f.name))}"
            for f in members
        )
        writes.append(f"        if (self.{local}) |item| {{ {mark}; {assigned} }}")
        captures.append(
            f"            .{local} = if ({present}) .{{ {copied} }} else null,"
        )
        grouped.update(group.fields)
    item_buffer = values.item_buffers.get(value.native)
    if item_buffer:
        fields.append(f"    {identifier(item_buffer.field)}: []const u8 = &.{{}},")
        captures.append(f"            .{identifier(item_buffer.field)} = &.{{}},")
    for field in value.fields:
        if item_buffer and field.name in {item_buffer.offset, item_buffer.length}:
            continue
        if field.name in grouped or field.name in hidden:
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
        if field.role in {"reserved", "tag", "stride", "arena"}:
            continue
        if field.role == "count":
            array = next(f for f in value.fields if f.value.length == field.name)
            if array.role == "arena":
                continue
            expression = f"self.{identifier(array.name)}"
            length = (
                f"if ({expression}) |items| items.len else 0"
                if array.presence
                or array.value.nullable
                or array.value.optional == "empty"
                else f"{expression}.len"
            )
            writes.append(
                f"        {raw} = std.math.cast(@TypeOf({raw}), {length}) orelse return error.InvalidArgument;"
            )
            continue
        if field.value.kind == "union":
            union = field.value
            fields.append(f"    {local}: {values.public(union)} = .{{ .unknown = 0 }},")
            captures.append(f"            .{local} = {decode(values, union, raw)},")
            cases = " ".join(
                f".{identifier(member.name)} => |item| {{ raw.{identifier(union.tag)} = c.{member.presence.variant}; {raw}.{identifier(member.name)} = {encode(values, member.value, 'item')}; }},"
                for member in union.fields
            )
            if union.empty_variant:
                cases += f" .empty => {{ raw.{identifier(union.tag)} = c.{union.empty_variant[0]}; }},"
            writes.append(
                f"        switch (self.{local}) {{ {cases} .unknown => return error.InvalidArgument, }}"
            )
            continue
        optional = field.presence and field.presence.mask
        public_type = values.public(field.value)
        default = (
            "null"
            if optional or public_type.startswith("?")
            else "&.{}"
            if public_type.startswith("[]")
            else ".{}"
            if field.value.kind == "record"
            else "std.mem.zeroes(" + public_type + ")"
        )
        fields.append(
            f"    {local}: {'?' if optional else ''}{public_type} = {default},"
        )
        copied = decode(values, field.value, raw)
        if optional:
            path = ".".join(identifier(part) for part in field.presence.mask.split("."))
            bit = field.presence.bit
            present = f"raw.{path} & c.{bit} != 0" if bit else f"raw.{path}"
            mark = f"raw.{path} |= c.{bit}" if bit else f"raw.{path} = true"
            writes.append(
                f"        if (self.{local}) |item| {{ {mark}; {raw} = {encode(values, field.value, 'item')}; }}"
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
    initial = (
        f"c.{value.default}()" if value.default else f"std.mem.zeroes(c.{value.native})"
    )
    write_use = (
        ""
        if any("allocator" in line for line in writes)
        else "        _ = allocator;\n"
    )
    roots_use = (
        "" if any("roots" in line for line in writes) else "        _ = roots;\n"
    )
    capture_use = (
        ""
        if any("allocator" in line for line in captures)
        else "        _ = allocator;\n"
    )
    if not any("raw." in line for line in captures):
        capture_use += "        _ = raw;\n"
    return (
        "".join(groups)
        + f"""pub const {typ} = struct {{
{chr(10).join(fields)}
    pub fn toNative(self: {typ}, allocator: std.mem.Allocator, roots: *callback.Roots) status.Error!c.{value.native} {{
{write_use}{roots_use}        var raw = {initial};
{chr(10).join(writes)}
        return raw;
    }}
{chr(10).join(trampolines)}
    pub fn fromNative(allocator: std.mem.Allocator, raw: c.{value.native}) status.Error!{typ} {{
{capture_use}        return .{{
{chr(10).join(captures)}
        }};
    }}
}};
fn copy{typ}Value(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!OwnedValue({typ}) {{
    var arena = std.heap.ArenaAllocator.init(target.*);
    errdefer arena.deinit();
    const value = try {typ}.fromNative(arena.allocator(), try completion.value(c.{value.native})(result));
    return .{{ .arena = arena, .value = value }};
}}
fn copyOptional{typ}Value(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!?OwnedValue({typ}) {{
    if (result.value_count == 0) return null;
    return try copy{typ}Value(result, target);
}}
"""
    )
