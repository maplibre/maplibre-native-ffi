"""Swift owned snapshots and arena-backed C materialization."""

from dataclasses import replace

from .swift import camel, identifier, name


def dynamic(value):
    return (
        bool(value.registration)
        or any(
            len(group.fields) > 1 and not group.bit for group in value.presence_groups
        )
        or value.kind in {"buffer", "array", "reference", "union"}
        or any(dynamic(f.value) for f in value.fields if f.role == "value")
    )


def public(values, value):
    if value.kind == "buffer":
        typ = "String" if value.encoding == "utf8" else "Data"
    elif value.kind == "array":
        typ = (
            "Data"
            if value.encoding == "bytes" and values.public(value.element) == "UInt8"
            else f"[{values.public(value.element)}]"
        )
    elif value.kind == "reference":
        typ = values.public(value.element)
    else:
        return None
    return typ + "?" if value.nullable or value.optional == "empty" else typ


def encode_throws(value):
    if value.kind == "buffer":
        return value.length == "nul"
    if value.kind == "reference":
        return encode_throws(value.element)
    if value.kind == "array":
        return (
            value.ctype.kind != "array" and value.length and value.length.isdigit()
        ) or encode_throws(value.element)
    return value.kind not in {"scalar", "enum", "native_pointer"} and dynamic(value)


def encode(values, value, source):
    if value.kind == "enum":
        return (
            f"{value.ctype.declaration or value.native}(rawValue: {source}.rawValue)"
            if value.ctype.canonical.startswith("enum ")
            else f"{source}.rawValue"
        )
    if value.kind == "native_pointer":
        if value.ctype.pointee and value.ctype.pointee.kind == "function":
            return f"unsafeBitCast({source}.addressBitPattern, to: {value.native}.self)"
        return f"{source}.{'unsafeRawPointer' if value.ctype.pointee and value.ctype.pointee.const else 'unsafeMutableRawPointer'}"
    if value.kind in {"buffer", "array", "reference"} and (
        value.nullable or value.optional == "empty"
    ):
        converted = encode(values, replace(value, nullable=False, optional=None), "$0")
        fallback = " ?? mln_buffer_view()" if value.buffer_form == "view" else ""
        # Arena arrays and counted pointer buffers are already optional.
        method = (
            "flatMap"
            if value.kind == "array"
            or value.kind == "buffer"
            and value.buffer_form != "view"
            and value.length != "nul"
            else "map"
        )
        return f"{'try ' if encode_throws(value) else ''}{source}.{method} {{ {converted} }}{fallback}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"try arena.cString({source})"
        if value.buffer_form != "view":
            pointee = (
                "CChar"
                if value.ctype.pointee.spelling.removeprefix("const ") == "char"
                else "UInt8"
            )
            return f"arena.view({source}).data?.assumingMemoryBound(to: {pointee}.self)"
        return f"arena.view({source})"
    if value.kind == "reference":
        return f"arena.store({encode(values, value.element, source)})"
    if value.kind == "array":
        if value.ctype.kind == "array":
            # Swift's imported fixed C arrays use tuples rather than Array.
            count = int(value.length)
            return (
                "("
                + ", ".join(
                    encode(values, value.element, f"{source}[{i}]")
                    for i in range(count)
                )
                + ")"
            )
        materialized = f"{'try ' if encode_throws(value.element) else ''}{source}.map {{ {encode(values, value.element, '$0')} }}"
        return (
            f"try arena.array({materialized}, count: {value.length})"
            if value.length and value.length.isdigit()
            else f"arena.array({materialized})"
        )
    return (
        source
        if value.kind == "scalar"
        else f"try {source}.nativeValue(arena: arena)"
        if dynamic(value)
        else f"{source}.nativeValue()"
    )


def zero(value, typ):
    """The initial value of a field of a record without a native default."""
    if typ.endswith("?"):
        return "nil"
    if typ == "String":
        return '""'
    if typ == "Data":
        return "Data()"
    if typ.startswith("["):
        return "[]"
    if typ == "Bool":
        return "false"
    if value.kind == "scalar":
        return "0"
    if value.kind == "enum":
        return ".init(rawValue: 0)"
    if value.kind == "native_pointer":
        return ".null"
    return ".default"


def decode(values, value, source, context="raw"):
    if value.registration and value.native not in values.bound.returned:
        # Native never returns callbacks, so a copy leaves a registration
        # unset.
        return "nil" if value.nullable else f"{values.public(value)}()"
    if value.kind == "enum":
        return f"{values.public(value)}(rawValue: {source}{'.rawValue' if value.ctype.canonical.startswith('enum ') else ''})"
    if value.kind == "native_pointer":
        return f"NativePointer(bitPattern: unsafeBitCast({source}, to: UInt.self))"
    if value.kind in {"buffer", "array", "reference"} and (
        value.nullable or value.optional == "empty"
    ):
        converted = decode(
            values, replace(value, nullable=False, optional=None), source, context
        )
        absent = (
            (
                f"{source}.size == 0"
                if value.optional == "empty"
                else f"{source}.data == nil"
            )
            if value.buffer_form == "view"
            else f"{source} == nil"
        )
        return f"{absent} ? nil : {converted}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"try NativeString.copyCString({source})"
        copier = "copyUTF8" if value.encoding == "utf8" else "copyData"
        if value.buffer_form != "view":
            count = (
                value.length
                if value.length.isdigit()
                else f"{context}.{identifier(value.length)}"
            )
            return f"try NativeString.{copier}(data: {source}, size: Int({count}))"
        return f"try NativeString.{copier}(data: {source}.data, size: {source}.size)"
    if value.kind == "reference":
        return f"try NativeInputArena.copyArray({source}, count: 1).map {{ {decode(values, value.element, '$0', context)} }}[0]"
    if value.kind == "array" and value.stride:
        info = value.item_buffer
        extra = ""
        if info:
            copier = "copyUTF8" if info.encoding == "utf8" else "copyData"
            extra = f", {identifier(camel(info.field))}: NativeInputArena.{copier}Slice(data: {context}.{identifier(info.data)}, size: {context}.{identifier(info.size)}, offset: item.{identifier(info.offset)}, length: item.{identifier(info.length)})"
        return f"try NativeInputArena.copyStrided({source}, count: {context}.{identifier(value.length)}, stride: {context}.{identifier(value.stride)}) {{ item, bytes in try {values.public(value.element)}(raw: item, recordBytes: bytes{extra}) }}"
    if value.kind == "array":
        if value.ctype.kind == "array":
            return (
                "["
                + ", ".join(
                    decode(values, value.element, f"{source}.{i}", context)
                    for i in range(int(value.length))
                )
                + "]"
            )
        count = (
            value.length
            if value.length.isdigit()
            else f"{context}.{identifier(value.length)}"
        )
        copied = f"try NativeInputArena.copyArray({source}, count: Int({count})).map {{ {decode(values, value.element, '$0', context)} }}"
        return (
            f"Data({copied})"
            if value.encoding == "bytes" and values.public(value.element) == "UInt8"
            else copied
        )
    if value.kind == "union":
        parent = next(
            parent
            for parent in values.bound.values.values()
            if any(field.value.native == value.native for field in parent.fields)
        )
        tag_value = next(
            field.value for field in parent.fields if field.name == value.tag
        )
        enum_tag = tag_value.ctype.canonical.startswith("enum ")
        tag = f"{context}.{identifier(value.tag)}" + (".rawValue" if enum_tag else "")
        cases = " ".join(
            f"case {field.presence.variant}.rawValue: return .{identifier(camel(field.name))}({decode(values, field.value, source + '.' + identifier(field.name), context)})"
            for field in value.fields
        )
        if value.empty_variant:
            cases += f" case {value.empty_variant[0]}.rawValue: return .none"
        field = next(
            field for field in parent.fields if field.value.native == value.native
        )
        offset = f"MemoryLayout<{parent.native}>.offset(of: \\.{field.name})!"
        data = f"recordBytes.map {{ Data($0.dropFirst({offset})) }} ?? withUnsafeBytes(of: {source}) {{ Data($0) }}"
        return f"try {{ () throws -> {values.public(value)} in switch {tag} {{ {cases} default: return .unknown({tag}, {data}) }} }}()"
    return (
        source
        if value.kind == "scalar"
        else f"{'try ' if dynamic(value) else ''}{values.public(value)}(raw: {source})"
    )


def declaration(values, value):
    from .swift_callbacks import contains_callback

    conformances = (
        "Sendable" if contains_callback(value) else "Equatable, Hashable, Sendable"
    )
    typ = values.public(value)
    fields, args, init, encode_lines, decode_lines = [], [], [], [], []
    controls = {
        name
        for field in value.fields
        if field.value.item_buffer
        for name in (
            field.value.item_buffer.data,
            field.value.item_buffer.size,
            field.value.stride,
        )
    }
    item = values.item_buffers.get(value.native)
    extra_parameters = ""
    if item:
        controls.update((item.offset, item.length))
        local = identifier(camel(item.field))
        typ_ = "String" if item.encoding == "utf8" else "Data"
        empty = '""' if item.encoding == "utf8" else "Data()"
        fields.append(f"  public var {local}: {typ_}")
        args.append(f"{local}: {typ_} = {empty}")
        init.append(f"    self.{local} = {local}")
        decode_lines.append(f"    self.{local} = {local}")
        extra_parameters = f", {local}: {typ_} = {empty}"
    for field in value.fields:
        if field.role == "presence_mask":
            encode_lines.append(
                f"    raw.{identifier(field.name)} = {'false' if field.value.ctype.canonical in {'bool', '_Bool'} else '0'}"
            )
    group_declarations = []
    grouped = set()
    for group in value.presence_groups:
        if len(group.fields) < 2:
            continue
        grouped.update(group.fields)
        local = identifier(camel(group.member))
        members = [field for field in value.fields if field.name in group.fields]
        group_type = (
            values.public(values.bound.values[group.type])
            if group.type
            else typ + name(group.member)
        )
        if not group.type:
            group_fields = "\n".join(
                f"  public var {identifier(camel(field.name))}: {values.public(field.value)}"
                for field in members
            )
            group_args = ", ".join(
                f"{identifier(camel(field.name))}: {values.public(field.value)}"
                for field in members
            )
            group_init = "\n".join(
                f"    self.{identifier(camel(field.name))} = {identifier(camel(field.name))}"
                for field in members
            )
            group_declarations.append(
                f"public struct {group_type}: Equatable, Hashable, Sendable {{\n{group_fields}\n  public init({group_args}) {{\n{group_init}\n  }}\n}}\n"
            )
        fields.append(f"  public var {local}: {group_type}?")
        args.append(f"{local}: {group_type}? = nil")
        init.append(f"    self.{local} = {local}")
        path = ".".join(identifier(part) for part in group.mask.split("."))
        present = (
            f"raw.{path} & {group.bit}.rawValue != 0" if group.bit else f"raw.{path}"
        )
        mark = (
            f"raw.{path} |= {group.bit}.rawValue" if group.bit else f"raw.{path} = true"
        )
        captured = ", ".join(
            f"{identifier(camel(field.name))}: {decode(values, field.value, 'raw.' + identifier(field.name))}"
            for field in members
        )
        decode_lines.append(
            f"    self.{local} = {present} ? {group_type}({captured}) : nil"
        )
        materialized = "; ".join(
            f"raw.{identifier(field.name)} = {encode(values, field.value, 'item.' + identifier(camel(field.name)))}"
            for field in members
        )
        encode_lines.append(
            f"    if let item = self.{local} {{ {mark}; {materialized} }}"
        )
    for f in value.fields:
        if f.name in controls or f.name in grouped:
            continue
        raw, local = f"raw.{identifier(f.name)}", identifier(camel(f.name))
        if f.role == "size":
            encode_lines.append(
                f"    {raw} = UInt32(MemoryLayout<{value.native}>.size)"
            )
            continue
        if f.role == "count":
            array = next(
                member for member in value.fields if member.value.length == f.name
            )
            optional = (
                array.presence
                or array.value.nullable
                or array.value.optional == "empty"
            )
            source = "self." + identifier(camel(array.name))
            encode_lines.append(
                f"    {raw} = try NativeInputArena.count({source}{'?.count ?? 0' if optional else '.count'})"
            )
            continue
        if not f.public:
            continue
        if f.value.kind == "union":
            union = f.value
            union_type = values.public(union)
            fields.append(f"  public var {local}: {union_type}")
            args.append(
                f"{local}: {union_type} = {typ}.default.{local}"
                if value.default
                else f"{local}: {union_type} = .default"
            )
            init.append(f"    self.{local} = {local}")
            decode_lines.append(f"    self.{local} = {decode(values, union, raw)}")
            tag_value = next(
                field.value for field in value.fields if field.name == union.tag
            )
            tag_suffix = (
                "" if tag_value.ctype.canonical.startswith("enum ") else ".rawValue"
            )
            cases = "\n".join(
                f"    case .{identifier(camel(member.name))}(let item): raw.{identifier(union.tag)} = {member.presence.variant}{tag_suffix}; {raw}.{identifier(member.name)} = {encode(values, member.value, 'item')}"
                for member in union.fields
            )
            if union.empty_variant:
                cases += f"\n    case .none: raw.{identifier(union.tag)} = {union.empty_variant[0]}{tag_suffix}"
            encode_lines.append(
                f'    switch self.{local} {{\n{cases}\n    case .unknown: throw NativeStringError("unknown union variant cannot be submitted")\n    }}'
            )
            continue
        if f.value.kind == "array" and f.value.ctype.kind == "array":
            encode_lines.append(
                f'    guard self.{local}.count == {f.value.length} else {{ throw NativeStringError("incorrect fixed array length") }}'
            )
        optional = f.presence and f.presence.mask
        field_type = values.public(f.value) + ("?" if optional else "")
        default = zero(f.value, field_type)
        fields.append(f"  public var {local}: {field_type}")
        args.append(
            f"{local}: {field_type} = {typ}.default.{local}"
            if value.default
            else f"{local}: {field_type} = {default}"
        )
        init.append(f"    self.{local} = {local}")
        capture = decode(values, f.value, raw)
        if optional:
            path = ".".join(identifier(part) for part in f.presence.mask.split("."))
            bit = f.presence.bit
            present = f"raw.{path} & {bit}.rawValue != 0" if bit else f"raw.{path}"
            mark = f"raw.{path} |= {bit}.rawValue" if bit else f"raw.{path} = true"
            encode_lines.append(
                f"    if let item = self.{local} {{ {mark}; {raw} = {encode(values, f.value, 'item')} }}"
            )
            decode_lines.append(f"    self.{local} = {present} ? {capture} : nil")
        else:
            encode_lines.append(
                f"    {raw} = {encode(values, f.value, 'self.' + local)}"
            )
            decode_lines.append(f"    self.{local} = {capture}")
    constructors = []
    members = [f for f in value.fields if f.role == "value"]
    if len(members) == 1 and members[0].value.kind == "union":
        field = members[0]
        for variant in field.value.fields:
            constructors.append(
                f"  public static func {identifier(camel(variant.name))}(_ value: {values.public(variant.value)}) -> Self {{ Self({identifier(camel(field.name))}: .{identifier(camel(variant.name))}(value)) }}"
            )
    initial = f"{value.default}()" if value.default else f"{value.native}()"
    if controls:
        encode_lines = [
            '    throw NativeStringError("borrowed arena snapshots cannot be submitted")'
        ]
    return (
        "\n".join(group_declarations)
        + f"""public struct {typ}: {conformances} {{
{chr(10).join(fields)}
  public static var `default`: Self {{ {f"try! Self(raw: {initial})" if value.default else "Self()"} }}
{chr(10).join(constructors)}
  public init({", ".join(args)}) {{
{chr(10).join(init)}
  }}
  init(raw: {value.native}, recordBytes: UnsafeRawBufferPointer? = nil{extra_parameters}) throws {{
{chr(10).join(decode_lines)}
  }}
  func nativeValue(arena: NativeInputArena) throws -> {value.native} {{
    {"" if controls else "var raw = " + initial}
{chr(10).join(encode_lines)}
    {"" if controls else "return raw"}
  }}
}}
"""
    )
