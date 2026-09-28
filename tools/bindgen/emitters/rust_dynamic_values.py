"""Rust owned records and temporary C storage for pointer-bearing values."""

from .rust import Unsupported, identifier, native_identifier, pascal


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


def encode(values, value, source):
    source = f"({source})"
    optional = value.nullable or value.optional == "empty"
    if value.kind in {"buffer", "array", "reference"} and optional:
        from dataclasses import replace

        raw = encode(values, replace(value, nullable=False, optional=None), "item")
        null = (
            "maplibre_native_ffi_sys::mln_buffer_view { data: std::ptr::null(), size: 0 }"
            if value.buffer_form == "view"
            else "std::ptr::null()"
        )
        return f"match {source}.as_ref() {{ Some(item) => {raw}, None => {null} }}"
    if value.kind == "buffer":
        if value.length == "nul":
            return f"arena.c_string({source})?"
        data = f"{source}.as_bytes()" if value.encoding == "utf8" else source
        return (
            f"maplibre_native_ffi_sys::mln_buffer_view {{ data: {data}.as_ptr().cast(), size: {data}.len() }}"
            if value.buffer_form == "view"
            else f"{data}.as_ptr().cast()"
        )
    if value.kind == "reference":
        converted = encode(values, value.element, source)
        return f"{{ let item = {converted}; arena.store(item) }}"
    if value.kind == "array":
        if value.element.kind == "scalar" and value.ctype.kind != "array":
            return f"{source}.as_ptr()"
        converted = encode(values, value.element, "item")
        array = f"{source}.iter().map(|item| -> crate::Result<_> {{ Ok({converted}) }}).collect::<crate::Result<Vec<_>>>()?"
        if value.ctype.kind == "array":
            return f'{array}.try_into().map_err(|_| crate::Error::invalid_argument("incorrect fixed array length"))?'
        return f"{{ let items = {array}; arena.array(items) }}"
    if value.kind in {"scalar", "native_pointer"}:
        return f"*({source})"
    return f"{source}.to_native(arena)?" if dynamic(value) else f"{source}.to_native()"


def decode(values, value, source, context="raw"):
    optional = value.nullable or value.optional == "empty"
    if value.kind in {"buffer", "array", "reference"} and optional:
        from dataclasses import replace

        copied = decode(
            values, replace(value, nullable=False, optional=None), source, context
        )
        absent = (
            (f"{source}.data.is_null()" if value.nullable else f"{source}.size == 0")
            if value.buffer_form == "view"
            else f"{source}.is_null()"
        )
        return f"if {absent} {{ None }} else {{ Some({copied}) }}"
    if value.kind == "buffer":
        if value.buffer_form != "view" and value.length != "nul":
            count = (
                value.length
                if value.length.isdigit()
                else f"{context}.{native_identifier(value.length)} as usize"
            )
            source = f"maplibre_native_ffi_sys::mln_buffer_view {{ data: {source}.cast(), size: {count} }}"
        copier = (
            "copy_c_string"
            if value.length == "nul"
            else "copy_string_view"
            if value.encoding == "utf8"
            else "copy_string_view_bytes"
        )
        return f"unsafe {{ crate::string::{copier}({source}) }}?"
    if value.kind == "reference":
        return f'{{ let item = unsafe {{ {source}.as_ref() }}.ok_or_else(|| crate::Error::invalid_argument("null record value"))?; {decode(values, value.element, "*item", context)} }}'
    if value.kind == "array":
        if value.ctype.kind == "array":
            items = f"{source}.iter()"
        else:
            count = (
                value.length
                if value.length.isdigit()
                else f"{context}.{native_identifier(value.length)} as usize"
            )
            items = f"unsafe {{ crate::input::slice({source}, {count}) }}?.iter()"
        if value.stride:
            items = f"unsafe {{ crate::input::strided_values({source}, {count}, {context}.{native_identifier(value.stride)} as usize) }}?.iter()"
        copied = decode(values, value.element, "*item", context)
        if value.item_buffer:
            buffer = value.item_buffer
            content = f"unsafe {{ crate::input::arena_string({context}.{buffer.data}.cast(), {context}.{buffer.size} as usize, item.{buffer.offset} as usize, item.{buffer.length} as usize) }}?"
            copied = f"{{ let mut value = {copied}; value.{identifier(buffer.field)} = {content}; value }}"
        return f"{items}.map(|item| -> crate::Result<_> {{ Ok({copied}) }}).collect::<crate::Result<Vec<_>>>()?"
    if value.kind == "union":
        cases = ", ".join(
            f"maplibre_native_ffi_sys::{field.presence.variant} => {values.public(value)}::{pascal(field.name)}({decode(values, field.value, 'unsafe { ' + source + '.' + native_identifier(field.name) + ' }', context)})"
            for field in value.fields
        )
        if value.empty_variant:
            cases += f", maplibre_native_ffi_sys::{value.empty_variant[0]} => {values.public(value)}::Empty"
        return f"match {context}.{native_identifier(value.tag)} {{ {cases}, tag => {values.public(value)}::Unknown(tag as u32) }}"
    if value.kind in {"scalar", "native_pointer"}:
        return source
    copied = f"{values.public(value)}::from_native({source})"
    return f"unsafe {{ {copied} }}?" if dynamic(value) else copied


def comparable(value):
    return (
        not value.registration
        and all(
            comparable(field.value) for field in value.fields if field.role == "value"
        )
        and (value.element is None or comparable(value.element))
    )


def declaration(values, value):
    public_name = pascal(value.native)
    fields, captures, writes = [], [], []
    group_declarations = []
    grouped = set()
    for group in value.presence_groups:
        if len(group.fields) < 2 or group.type:
            continue
        local = identifier(group.mask.removeprefix("has_"))
        group_name = public_name + pascal(local)
        members = [
            next(field for field in value.fields if field.name == name)
            for name in group.fields
        ]
        group_declarations.append(
            f"#[derive(Debug, Clone, PartialEq, Default)] pub struct {group_name} {{ "
            + ", ".join(
                f"pub {identifier(field.name)}: {values.public(field.value)}"
                for field in members
            )
            + " }"
        )
        fields.append(f"    pub {local}: Option<{group_name}>,")
        mask = native_identifier(group.mask)
        present = (
            f"raw.{mask} & maplibre_native_ffi_sys::{group.bit} != 0"
            if group.bit
            else f"raw.{mask}"
        )
        mark = (
            f"raw.{mask} |= maplibre_native_ffi_sys::{group.bit}"
            if group.bit
            else f"raw.{mask} = true"
        )
        copied = ", ".join(
            f"{identifier(field.name)}: {decode(values, field.value, 'raw.' + native_identifier(field.name))}"
            for field in members
        )
        assigned = " ".join(
            f"raw.{native_identifier(field.name)} = {encode(values, field.value, '&item.' + identifier(field.name))};"
            for field in members
        )
        captures.append(
            f"            {local}: if {present} {{ Some({group_name} {{ {copied} }}) }} else {{ None }},"
        )
        writes.append(
            f"        if let Some(item) = &self.{local} {{ {mark}; {assigned} }}"
        )
        grouped.update(group.fields)
    item_buffer = values.item_buffers.get(value.native)
    if item_buffer:
        fields.append(f"    pub {identifier(item_buffer.field)}: String,")
        captures.append(f"            {identifier(item_buffer.field)}: String::new(),")
    for field in value.fields:
        if field.name in grouped:
            continue
        if item_buffer and field.name in {item_buffer.offset, item_buffer.length}:
            continue
        raw = f"raw.{native_identifier(field.name)}"
        local = identifier(field.name)
        if field.role == "size":
            writes.append(
                f"        {raw} = std::mem::size_of::<maplibre_native_ffi_sys::{value.native}>() as _;"
            )
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
            arrays = [f for f in value.fields if f.value.length == field.name]
            if not arrays:
                raise Unsupported(f"{value.native}.{field.name}: count has no array")
            array = arrays[0]
            if array.role == "arena":
                continue
            expression = f"self.{identifier(array.name)}"
            length = (
                f"{expression}.as_ref().map_or(0, |items| items.len())"
                if array.value.nullable
                or array.value.optional == "empty"
                or array.presence
                else f"{expression}.len()"
            )
            writes.append(
                f'        {raw} = {length}.try_into().map_err(|_| crate::Error::invalid_argument("array exceeds native count range"))?;'
            )
            continue
        if field.value.kind == "union":
            union = field.value
            typ = values.public(union)
            fields.append(f"    pub {local}: {typ},")
            captures.append(f"            {local}: {decode(values, union, raw)},")
            cases = " ".join(
                f"{typ}::{pascal(member.name)}(item) => {{ raw.{native_identifier(union.tag)} = maplibre_native_ffi_sys::{member.presence.variant}; {raw}.{native_identifier(member.name)} = {encode(values, member.value, 'item')}; }},"
                for member in union.fields
            )
            if union.empty_variant:
                cases += f" {typ}::Empty => {{ raw.{native_identifier(union.tag)} = maplibre_native_ffi_sys::{union.empty_variant[0]}; }},"
            writes.append(
                f'        match &self.{local} {{ {cases} {typ}::Unknown(_) => return Err(crate::Error::invalid_argument("unknown union variant cannot be submitted")), }}'
            )
            continue
        optional = field.presence and field.presence.mask
        typ = values.public(field.value)
        fields.append(f"    pub {local}: {'Option<' + typ + '>' if optional else typ},")
        copied = decode(values, field.value, raw)
        if optional:
            mask, bit = native_identifier(field.presence.mask), field.presence.bit
            present = (
                f"raw.{mask} & maplibre_native_ffi_sys::{bit} != 0"
                if bit
                else f"raw.{mask}"
            )
            mark = (
                f"raw.{mask} |= maplibre_native_ffi_sys::{bit}"
                if bit
                else f"raw.{mask} = true"
            )
            writes.append(
                f"        if let Some(item) = &self.{local} {{ {mark}; {raw} = {encode(values, field.value, 'item')}; }}"
            )
            captures.append(
                f"            {local}: if {present} {{ Some({copied}) }} else {{ None }},"
            )
        else:
            writes.append(
                f"        {raw} = {encode(values, field.value, '&self.' + local)};"
            )
            captures.append(f"            {local}: {copied},")
    constructors = []
    public_fields = [field for field in value.fields if field.role == "value"]
    if len(public_fields) == 1 and public_fields[0].value.kind == "union":
        field = public_fields[0]
        for member in field.value.fields:
            name = (
                member.name + "_"
                if member.name in {"box", "type"}
                else identifier(member.name)
            )
            constructors.append(
                f"    pub fn {name}(value: {values.public(member.value)}) -> Self {{ Self {{ {identifier(field.name)}: {values.public(field.value)}::{pascal(member.name)}(value) }} }}"
            )
    initial = (
        f"unsafe {{ maplibre_native_ffi_sys::{value.default}() }}"
        if value.default
        else "unsafe { std::mem::zeroed() }"
    )
    return f"""{chr(10).join(group_declarations)}
#[derive(Debug, Clone{", PartialEq" if comparable(value) else ""}{", Default" if not value.default else ""})]
pub struct {public_name} {{
{chr(10).join(fields)}
}}
{f'impl Default for {public_name} {{ fn default() -> Self {{ unsafe {{ Self::from_native(maplibre_native_ffi_sys::{value.default}()).expect("native default must be valid") }} }} }}' if value.default else ""}
impl {public_name} {{
{chr(10).join(constructors)}
    pub fn to_native(&self, arena: &mut crate::input::InputArena) -> crate::Result<maplibre_native_ffi_sys::{value.native}> {{
        let mut raw: maplibre_native_ffi_sys::{value.native} = {initial};
{chr(10).join(writes)}
        Ok(raw)
    }}
    /// Copies all borrowed storage before the native callback returns.
    ///
    /// # Safety
    /// Pointer fields must remain readable for the lengths declared by this record.
    pub unsafe fn from_native(raw: maplibre_native_ffi_sys::{value.native}) -> crate::Result<Self> {{
        Ok(Self {{
{chr(10).join(captures)}
        }})
    }}
}}
"""
