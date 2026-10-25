"""Swift owned snapshots and arena-backed C materialization."""

from dataclasses import replace

from .swift import TEXT_ENCODINGS, camel, doc, identifier


def dynamic(value):
    return (
        bool(value.registration)
        or value.kind in {"buffer", "array", "reference", "union"}
        or any(dynamic(f.value) for f in value.fields if f.role == "value")
    )


def public(values, value):
    if value.kind == "buffer":
        typ = "String" if value.encoding in TEXT_ENCODINGS else "Data"
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
    return typ + "?" if value.nullable else typ


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
    if value.kind in {"buffer", "array", "reference"} and value.nullable:
        converted = encode(values, replace(value, nullable=False), "$0")
        fallback = " ?? mln_buffer_view()" if value.buffer_form == "view" else ""
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
    if value.native in values.bound.slices:
        array = values.bound.slices[value.native]
        return decode(values, array.value, f"{source}.{identifier(array.name)}", source)
    if value.kind == "enum":
        return f"{values.public(value)}(rawValue: {source}{'.rawValue' if value.ctype.canonical.startswith('enum ') else ''})"
    if value.kind == "native_pointer":
        return f"NativePointer(bitPattern: unsafeBitCast({source}, to: UInt.self))"
    if value.kind in {"buffer", "array", "reference"} and value.nullable:
        converted = decode(values, replace(value, nullable=False), source, context)
        absent = (
            f"{source}.data == nil"
            if value.buffer_form == "view"
            else f"{source} == nil"
        )
        return f"{absent} ? nil : {converted}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"try NativeString.copyCString({source})"
        copier = "copyUTF8" if value.encoding in TEXT_ENCODINGS else "copyData"
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
        item = (
            f"item, bytes in try {values.public(value.element)}(raw: item, recordBytes: bytes)"
            if dynamic(value.element)
            else f"item, _ in {values.public(value.element)}(raw: item)"
        )
        return f"try NativeInputArena.copyStrided({source}, count: {context}.{identifier(value.length)}, stride: {context}.{identifier(value.stride)}) {{ {item} }}"
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
        fallback = (
            f"return .unknown({tag}, {data})"
            if value.native in values.bound.returned
            else 'throw MaplibreError.invalidState("native union contains an unsupported variant")'
        )
        typ = values.public(parent if parent.union_member else value)
        return f"try {{ () throws -> {typ} in switch {tag} {{ {cases} default: {fallback} }} }}()"
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
    for field in value.fields:
        if field.role == "presence_mask":
            encode_lines.append(f"    raw.{identifier(field.name)} = 0")
    for f in value.fields:
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
            optional = array.presence or array.value.nullable
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
            default = (
                f"{typ}.default.{local}"
                if value.default
                else ".default"
                if union_default(values, union)
                else None
            )
            fields.append(
                f"{doc(values.bound, f'{value.native}.{f.name}', '  ')}  public var {local}: {union_type}"
            )
            args.append(f"{local}: {union_type}" + (f" = {default}" if default else ""))
            init.append(f"    self.{local} = {local}")
            decode_lines.append(f"    self.{local} = {decode(values, union, raw)}")
            encode_lines.append(union_encode(values, value, f, f"self.{local}"))
            continue
        if f.value.kind == "array" and f.value.ctype.kind == "array":
            encode_lines.append(
                f'    guard self.{local}.count == {f.value.length} else {{ throw MaplibreError.invalidArgument("incorrect fixed array length") }}'
            )
        optional = f.presence and f.presence.mask
        field_type = values.public(f.value) + ("?" if optional else "")
        default = (
            None
            if f.value.union_member
            and not field_type.endswith("?")
            and not union_default(values, f.value.union_member.value)
            else zero(f.value, field_type)
        )
        fields.append(
            f"{doc(values.bound, f'{value.native}.{f.name}', '  ')}  public var {local}: {field_type}"
        )
        args.append(
            f"{local}: {field_type} = {typ}.default.{local}"
            if value.default
            else f"{local}: {field_type}" + (f" = {default}" if default else "")
        )
        init.append(f"    self.{local} = {local}")
        capture = decode(values, f.value, raw)
        if optional:
            path = identifier(f.presence.mask)
            bit = f.presence.bit
            present = f"raw.{path} & {bit}.rawValue != 0"
            mark = f"raw.{path} |= {bit}.rawValue"
            encode_lines.append(
                f"    if let item = self.{local} {{ {mark}; {raw} = {encode(values, f.value, 'item')} }}"
            )
            decode_lines.append(f"    self.{local} = {present} ? {capture} : nil")
        else:
            encode_lines.append(
                f"    {raw} = {encode(values, f.value, 'self.' + local)}"
            )
            decode_lines.append(f"    self.{local} = {capture}")
    initial = f"{value.default}()" if value.default else f"{value.native}()"
    # A record with a required member has no zero value to default to.
    default_member = (
        f"  public static var `default`: Self {{ try! Self(raw: nativeDefault({value.default})) }}"
        if value.default
        else "  public static var `default`: Self { Self() }"
        if all("=" in arg for arg in args)
        else ""
    )
    encoder = (
        f"""  func nativeValue(arena: NativeInputArena) throws -> {value.native} {{
    var raw = {initial}
{chr(10).join(encode_lines)}
    return raw
  }}
"""
        if value.native in values.bound.inputs
        else ""
    )
    rendered = f"""{doc(values.bound, value.native)}public struct {typ}: {conformances} {{
{chr(10).join(fields)}
{default_member}
  public init({", ".join(args)}) {{
{chr(10).join(init)}
  }}
  init(raw: {value.native}, recordBytes: UnsafeRawBufferPointer? = nil) throws {{
{chr(10).join(decode_lines)}
  }}
{encoder}}}
"""
    return rendered


def union_default(values, union):
    """The default case of a union, or None when it has no case to default to."""
    if union.empty_variant:
        return ".none"
    if union.native in values.bound.returned:
        return ".unknown(0, Data())"
    return None


def union_cases(values, union):
    """The cases of a union's enum and its default."""
    lines = [
        f"{doc(values.bound, f'{union.native}.{field.name}', '  ')}  case {identifier(camel(field.name))}({values.public(field.value)})"
        for field in union.fields
    ]
    if union.empty_variant:
        lines.append("  case none")
    # Only a union that native returns can hold a tag this binding does not know.
    if union.native in values.bound.returned:
        lines.append("  case unknown(UInt32, Data)")
    default = union_default(values, union)
    if default:
        lines.append(f"  public static var `default`: Self {{ {default} }}")
    return "".join(line + "\n" for line in lines)


def union_encode(values, record, field, selector):
    """Write the tag and the union storage of [record]'s union [field] from [selector]."""
    union = field.value
    raw = f"raw.{identifier(field.name)}"
    tag_value = next(f.value for f in record.fields if f.name == union.tag)
    tag_suffix = "" if tag_value.ctype.canonical.startswith("enum ") else ".rawValue"
    tag = f"raw.{identifier(union.tag)}"
    cases = [
        f"    case .{identifier(camel(member.name))}(let item): {tag} = {member.presence.variant}{tag_suffix}; {raw}.{identifier(member.name)} = {encode(values, member.value, 'item')}"
        for member in union.fields
    ]
    if union.empty_variant:
        cases.append(f"    case .none: {tag} = {union.empty_variant[0]}{tag_suffix}")
    if union.native in values.bound.returned:
        cases.append(
            '    case .unknown: throw MaplibreError.invalidArgument("unknown union variant cannot be submitted")'
        )
    return f"    switch {selector} {{\n" + "\n".join(cases) + "\n    }"


def union_record(values, value):
    """A record of only a tag and its union, presented as the union under the record's name."""
    field = value.union_member
    typ = values.public(value)
    size = "".join(
        f"    raw.{identifier(f.name)} = UInt32(MemoryLayout<{value.native}>.size)\n"
        for f in value.fields
        if f.role == "size"
    )
    initial = f"{value.default}()" if value.default else f"{value.native}()"
    encoder = (
        f"""  func nativeValue(arena: NativeInputArena) throws -> {value.native} {{
    var raw = {initial}
{size}{union_encode(values, value, field, "self")}
    return raw
  }}
"""
        if value.native in values.bound.inputs
        else ""
    )
    decoder = (
        f"""  init(raw: {value.native}, recordBytes: UnsafeRawBufferPointer? = nil) throws {{
    self = {decode(values, field.value, f"raw.{identifier(field.name)}")}
  }}
"""
        if value.native in values.bound.returned
        else ""
    )
    return f"""{doc(values.bound, value.native)}public enum {typ}: Equatable, Hashable, Sendable {{
{union_cases(values, field.value)}{decoder}{encoder}}}
"""
