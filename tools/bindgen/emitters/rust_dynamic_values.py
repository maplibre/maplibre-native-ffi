"""Rust records and the conversions of their fields to and from C.

Each record implements the runtime's `ToNative` and `FromNative` traits. A
field converts through the traits when its Rust type selects the C form, and
through a `convert` helper when the C form also depends on another field, such
as an array's count or a presence mask.
"""

from dataclasses import replace
from os.path import commonprefix

from .rust import Unsupported, identifier, native_identifier, pascal

TRAIT_COPY = "unsafe {{ from_native({}) }}?"


def dynamic(value):
    return (
        bool(value.registration)
        or any(
            len(group.fields) > 1 and not group.type for group in value.presence_groups
        )
        or value.kind in {"buffer", "array", "reference", "union"}
        or any(dynamic(f.value) for f in value.fields if f.role == "value")
    )


def public(values, value):
    if value.kind == "buffer":
        result = "String" if value.encoding == "utf8" else "Vec<u8>"
    elif value.kind == "array":
        result = f"Vec<{values.public(value.element)}>"
    elif value.kind == "reference":
        result = values.public(value.element)
    else:
        return None
    return (
        f"Option<{result}>" if value.nullable or value.optional == "empty" else result
    )


def optional(value):
    return value.nullable or value.optional == "empty"


def encode(values, value, source):
    """Converts the place `source` for one native call, with `arena` in scope."""
    if value.kind in {"buffer", "array", "reference"} and optional(value):
        if value.kind == "buffer" and (
            value.buffer_form == "view" or value.length == "nul"
        ):
            return f"to_native(&{source}, arena)?"
        if value.kind == "buffer":
            return f"{source}.as_ref().map_or(std::ptr::null(), |item| item.as_ptr().cast())"
        if value.kind == "array" and value.ctype.kind != "array":
            return f"convert::optional_array({source}.as_deref(), arena)?"
        if value.kind == "reference":
            return f"convert::optional_reference({source}.as_ref(), arena)?"
        raise Unsupported(f"{value.native}: optional fixed arrays need a presence rule")
    if value.kind == "buffer":
        if value.buffer_form == "view" or value.length == "nul":
            return f"to_native(&{source}, arena)?"
        return f"{source}.as_ptr().cast()"
    if value.kind == "reference":
        return f"convert::reference(&{source}, arena)?"
    if value.kind == "array":
        if value.ctype.kind == "array":
            return f"convert::fixed(&{source}, arena)?"
        if value.element.kind == "scalar":
            return f"{source}.as_ptr()"
        return f"convert::array(&{source}, arena)?"
    if value.kind in {"scalar", "native_pointer"}:
        return source
    return f"to_native(&{source}, arena)?"


def count(value, context):
    return (
        value.length
        if value.length.isdigit()
        else f"{context}.{native_identifier(value.length)}"
    )


def decode(values, value, source, context="raw"):
    """Copies the C place `source`, whose sibling fields `context` holds."""
    if value.kind in {"buffer", "array", "reference"} and optional(value):
        if value.kind == "buffer" and value.buffer_form == "view":
            if value.nullable:
                return TRAIT_COPY.format(source)
            return f"unsafe {{ convert::nonempty({source}) }}?"
        if value.kind == "buffer" and value.length == "nul":
            return TRAIT_COPY.format(source)
        if value.kind == "reference":
            return f"unsafe {{ convert::copy_optional_reference({source}) }}?"
        if (
            value.kind == "array"
            and value.ctype.kind != "array"
            and not value.stride
            and not value.item_buffer
        ):
            return f"unsafe {{ convert::copy_optional_array({source}, {count(value, context)}) }}?"
        copied = decode(
            values, replace(value, nullable=False, optional=None), source, context
        )
        return f"if {source}.is_null() {{ None }} else {{ Some({copied}) }}"
    if value.kind == "buffer":
        if value.buffer_form != "view" and value.length != "nul":
            return f"unsafe {{ convert::counted({source}, {count(value, context)}) }}?"
        return TRAIT_COPY.format(source)
    if value.kind == "reference":
        return f"unsafe {{ convert::copy_reference({source}) }}?"
    if value.kind == "array":
        if value.ctype.kind == "array":
            return f"unsafe {{ convert::copy_fixed({source}) }}?"
        if value.item_buffer:
            buffer = value.item_buffer
            items = (
                f"unsafe {{ convert::strided_items({source}, {count(value, context)}, {context}.{native_identifier(value.stride)}) }}?"
                if value.stride
                else f"unsafe {{ convert::items({source}, {count(value, context)}) }}?"
            )
            element = values.public(value.element)
            content = f"unsafe {{ convert::arena_string({context}.{buffer.data}, {context}.{buffer.size}, item.{buffer.offset}, item.{buffer.length}) }}?"
            return (
                f"{items}.into_iter().map(|item| -> Result<{element}> {{ "
                f"let mut value: {element} = unsafe {{ from_native(item) }}?; "
                f"value.{identifier(buffer.field)} = {content}; Ok(value) }}).collect::<Result<Vec<_>>>()?"
            )
        if value.stride:
            return f"unsafe {{ convert::copy_strided({source}, {count(value, context)}, {context}.{native_identifier(value.stride)}) }}?"
        return f"unsafe {{ convert::copy_array({source}, {count(value, context)}) }}?"
    if value.kind == "union":
        name = values.public(value)
        cases = []
        for field in value.fields:
            place = f"{source}.{native_identifier(field.name)}"
            copied = decode(values, field.value, place, context)
            if copied == place:
                copied = f"unsafe {{ {place} }}"
            cases.append(
                f"sys::{field.presence.variant} => {name}::{pascal(field.name)}({copied})"
            )
        if value.empty_variant:
            cases.append(f"sys::{value.empty_variant[0]} => {name}::Empty")
        return f"match {context}.{native_identifier(value.tag)} {{ {', '.join(cases)}, tag => {name}::Unknown(tag as u32) }}"
    if value.kind in {"scalar", "native_pointer"}:
        return source
    return TRAIT_COPY.format(source)


def comparable(value):
    return (
        not value.registration
        and all(
            comparable(field.value) for field in value.fields if field.role == "value"
        )
        and (value.element is None or comparable(value.element))
    )


def presence(mask, bit):
    """The mask place and the bit that marks one optional field."""
    return mask, (f"sys::{bit}" if bit else "true")


def declaration(values, value):
    """The struct, its conversions, and its constructors for one record."""
    name = pascal(value.native)
    raw = f"sys::{value.native}"
    copyable = not dynamic(value)
    fields, writes, copies, args, names = [], [], [], [], []
    extra = []
    for field in value.fields:
        if field.role == "presence_mask":
            reset = "false" if field.value.ctype.canonical in {"_Bool", "bool"} else "0"
            writes.append(f"raw.{native_identifier(field.name)} = {reset};")
        elif field.role == "size":
            writes.insert(
                0,
                f"raw.{native_identifier(field.name)} = std::mem::size_of::<{raw}>() as _;",
            )
    for flag in value.mask_flags:
        mask_plan = next(f.value for f in value.fields if f.name == flag.mask)
        prefix = (
            commonprefix([key for key, _ in mask_plan.enum_values]).rsplit("_", 1)[0]
            + "_"
        )
        local = identifier(flag.name.removeprefix(prefix).lower())
        mask = native_identifier(flag.mask)
        fields.append(f"    pub {local}: bool,")
        writes.append(
            f"convert::set_flag(&mut raw.{mask}, sys::{flag.name}, self.{local});"
        )
        copies.append(f"{local}: raw.{mask} & sys::{flag.name} != 0,")
    members = {field.name: field for field in value.fields}
    grouped = set()
    for group in value.presence_groups:
        if len(group.fields) < 2:
            continue
        grouped.update(group.fields)
        mask, bit = presence(f"raw.{native_identifier(group.mask)}", group.bit)
        if group.type:
            # Header mask names supply the semantic group label; the group type
            # supplies the member names and their conversion rules.
            child = values.bound.values[group.type]
            prefix = value.native.removeprefix("mln_").removesuffix("s").upper() + "_"
            local = identifier(
                group.bit.removeprefix("MLN_").removeprefix(prefix).lower()
            )
            group_name = values.public(child)
        else:
            local = identifier(group.mask.removeprefix("has_"))
            group_name = name + pascal(local)
            extra.append(
                f"#[derive(Debug, Clone, PartialEq, Default)] pub struct {group_name} {{ "
                + ", ".join(
                    f"pub {identifier(member)}: {values.public(members[member].value)}"
                    for member in group.fields
                )
                + " }"
            )
        fields.append(f"    pub {local}: Option<{group_name}>,")
        assigned = " ".join(
            f"raw.{native_identifier(member)} = {encode(values, members[member].value, 'item.' + identifier(member))};"
            for member in group.fields
        )
        writes.append(
            f"if let Some(item) = &self.{local} {{ {mask_mark(mask, bit)}; {assigned} }}"
        )
        copied = ", ".join(
            f"{identifier(member)}: {decode(values, members[member].value, 'raw.' + native_identifier(member))}"
            for member in group.fields
        )
        copies.append(
            f"{local}: if {mask_test(mask, bit)} {{ Some({group_name} {{ {copied} }}) }} else {{ None }},"
        )
    item_buffer = values.item_buffers.get(value.native)
    if item_buffer:
        fields.append(f"    pub {identifier(item_buffer.field)}: String,")
        copies.append(f"{identifier(item_buffer.field)}: String::new(),")
    for field in value.fields:
        if field.name in grouped:
            continue
        if item_buffer and field.name in {item_buffer.offset, item_buffer.length}:
            continue
        place, local = f"raw.{native_identifier(field.name)}", identifier(field.name)
        if field.role in {
            "size",
            "presence_mask",
            "reserved",
            "tag",
            "stride",
            "arena",
        }:
            continue
        if field.role == "count":
            arrays = [f for f in value.fields if f.value.length == field.name]
            if not arrays:
                raise Unsupported(f"{value.native}.{field.name}: count has no array")
            array = arrays[0]
            if array.role == "arena":
                continue
            source = f"self.{identifier(array.name)}"
            length = (
                f"{source}.as_ref().map_or(0, |items| items.len())"
                if optional(array.value) or array.presence
                else f"{source}.len()"
            )
            writes.append(f"{place} = convert::count({length})?;")
            continue
        if field.value.kind == "union":
            union = field.value
            union_name = values.public(union)
            fields.append(f"    pub {local}: {union_name},")
            copies.append(f"{local}: {decode(values, union, place)},")
            tag = native_identifier(union.tag)
            cases = " ".join(
                f"{union_name}::{pascal(member.name)}(item) => {{ raw.{tag} = sys::{member.presence.variant}; {place}.{native_identifier(member.name)} = {encode(values, member.value, '(*item)')}; }}"
                for member in union.fields
            )
            if union.empty_variant:
                cases += f" {union_name}::Empty => raw.{tag} = sys::{union.empty_variant[0]},"
            writes.append(
                f'match &self.{local} {{ {cases} {union_name}::Unknown(_) => return Err(Error::invalid_argument("unknown union variant cannot be submitted")), }}'
            )
            continue
        typ = values.public(field.value)
        masked = field.presence and field.presence.mask
        fields.append(f"    pub {local}: {'Option<' + typ + '>' if masked else typ},")
        copied = decode(values, field.value, place)
        if masked:
            write, copy = masked_field(values, field, place, local)
            writes.append(write)
            copies.append(copy)
        else:
            writes.append(f"{place} = {encode(values, field.value, 'self.' + local)};")
            copies.append(f"{local}: {copied},")
            args.append(f"{local}: {typ}")
            names.append(local)
    constructors = []
    public_fields = [field for field in value.fields if field.role == "value"]
    if len(public_fields) == 1 and public_fields[0].value.kind == "union":
        field = public_fields[0]
        union_name = values.public(field.value)
        for member in field.value.fields:
            method = (
                member.name + "_"
                if member.name in {"box", "type"}
                else identifier(member.name)
            )
            constructors.append(
                f"pub fn {method}(value: {values.public(member.value)}) -> Self {{ Self {{ {identifier(field.name)}: {union_name}::{pascal(member.name)}(value) }} }}"
            )
    elif copyable and not value.presence_groups:
        constructors.append(
            f"pub const fn new({', '.join(args)}) -> Self {{ Self {{ {', '.join(names)} }} }}"
        )
    initial = (
        f"unsafe {{ sys::{value.default}() }}"
        if value.default
        else "unsafe { std::mem::zeroed() }"
    )
    derives = ["Debug", "Clone"]
    if copyable:
        derives += ["Copy", "PartialEq"]
    elif comparable(value):
        derives.append("PartialEq")
    if not value.default:
        derives.append("Default")
    default = (
        f"impl Default for {name} {{ fn default() -> Self {{ convert::native_default(unsafe {{ sys::{value.default}() }}) }} }}\n"
        if value.default
        else ""
    )
    arena = "arena" if any("arena" in line for line in writes) else "_arena"
    impl = f"impl {name} {{ {' '.join(constructors)} }}\n" if constructors else ""
    directions = values.directions.get(value.native, {"in", "out"})
    to_native = (
        f"impl ToNative<{raw}> for {name} {{\n"
        + f"    fn to_native(&self, {arena}: &mut InputArena) -> Result<{raw}> {{\n"
        + f"        let mut raw: {raw} = {initial};\n"
        + "".join(f"        {line}\n" for line in writes)
        + "        Ok(raw)\n    }\n}\n"
        if "in" in directions
        else ""
    )
    from_native = (
        f"impl FromNative<{raw}> for {name} {{\n"
        + f"    unsafe fn from_native(raw: {raw}) -> Result<Self> {{\n"
        + "        Ok(Self {\n"
        + "".join(f"            {line}\n" for line in copies)
        + "        })\n    }\n}\n"
        if "out" in directions
        else ""
    )
    return (
        "\n".join(extra)
        + f"\n#[derive({', '.join(derives)})]\npub struct {name} {{\n"
        + "\n".join(fields)
        + "\n}\n"
        + default
        + impl
        + to_native
        + from_native
    )


def masked_field(values, field, place, local):
    """The write and the copy of an optional field that a mask marks present."""
    mask, bit = presence(
        f"raw.{native_identifier(field.presence.mask)}", field.presence.bit
    )
    written = encode(values, field.value, "(*item)").replace("(*item)", "*item")
    write = f"if let Some(item) = &self.{local} {{ {mask_mark(mask, bit)}; {place} = {written}; }}"
    copied = decode(values, field.value, place)
    if copied == place:
        copy = f"{local}: ({mask_test(mask, bit)}).then_some({place}),"
    elif copied == TRAIT_COPY.format(place):
        copy = f"{local}: unsafe {{ convert::present({mask}, {bit}, {place}) }}?,"
    else:
        copy = (
            f"{local}: if {mask_test(mask, bit)} {{ Some({copied}) }} else {{ None }},"
        )
    return write, copy


def mask_mark(mask, bit):
    return f"{mask} = true" if bit == "true" else f"{mask} |= {bit}"


def mask_test(mask, bit):
    return mask if bit == "true" else f"{mask} & {bit} != 0"
