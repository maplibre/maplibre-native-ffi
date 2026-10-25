"""Rust records and the conversions of their fields to and from C.

Each record implements the runtime's `ToNative` and `FromNative` traits. A
field converts through the traits when its Rust type selects the C form, and
through a `convert` helper when the C form also depends on another field, such
as an array's count or a presence mask.
"""

from dataclasses import replace

from .rust import Unsupported, doc, identifier, native_identifier, pascal

TRAIT_COPY = "unsafe {{ from_native({}) }}?"


def dynamic(value):
    return (
        bool(value.registration)
        or value.kind in {"buffer", "array", "reference", "union"}
        or any(dynamic(f.value) for f in value.fields if f.role == "value")
    )


def slice_of(values, value):
    """The array field of a `kind=slice` record, which Rust presents as a
    `Vec` in place of the record."""
    return values.bound.slices.get(value.native) if value.kind == "record" else None


def public(values, value):
    if array := slice_of(values, value):
        return values.public(array.value)
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
            # An optional record member takes a presence bit, so only a
            # pointer-shaped value is nullable.
            raise Unsupported(f"{source}: a copied record holds a nullable reference")
        if value.kind == "array" and value.ctype.kind != "array" and not value.stride:
            return f"unsafe {{ convert::copy_optional_array({source}, {count(value, context)}) }}?"
        copied = decode(
            values, replace(value, nullable=False, optional=None), source, context
        )
        return f"if {source}.is_null() {{ None }} else {{ Some({copied}) }}"
    if value.kind == "buffer":
        if value.buffer_form != "view" and value.length != "nul":
            return f"unsafe {{ convert::counted({source}, {count(value, context)}) }}?"
        return TRAIT_COPY.format(source)
    if array := slice_of(values, value):
        place = f"{source}.{native_identifier(array.name)}"
        return decode(values, array.value, place, source)
    if value.kind == "reference" and slice_of(values, value.element):
        view = decode(values, value.element, "view")
        return f'{{ let view = unsafe {{ {source}.as_ref() }}.ok_or_else(|| Error::invalid_argument("null record value"))?; {view} }}'
    if value.kind == "reference":
        return f"unsafe {{ convert::copy_reference({source}) }}?"
    if value.kind == "array":
        if value.ctype.kind == "array":
            return f"unsafe {{ convert::copy_fixed({source}) }}?"
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
    if value.kind == "enum":
        # Open enums and flag sets keep values this binding predates.
        convert = "from_bits_retain" if value.enum_kind == "bitmask" else "from_raw"
        return f"{values.public(value)}::{convert}({source})"
    return TRAIT_COPY.format(source)


def defaultable(value):
    """Whether the value's Rust type implements `Default`. An enum does only
    when 0 is one of its members."""
    if value.kind == "enum":
        return value.enum_kind == "bitmask" or any(
            number == 0 for _, number in value.enum_values
        )
    if value.kind == "handle":
        return False
    if value.kind == "reference":
        return defaultable(value.element)
    if value.kind != "record" or value.default:
        return True
    return all(
        defaultable(field.value)
        for field in value.fields
        if field.public and not (field.presence and field.presence.mask)
    )


def numeric(value):
    """Whether a record holds only numbers, directly or in such records."""
    return (
        value.kind == "record"
        and all(
            field.value.kind == "scalar"
            and not field.value.ctype.pointee
            or field.value.kind == "record"
            and numeric(field.value)
            for field in value.fields
            if field.public
        )
        and not any(field.presence and field.presence.mask for field in value.fields)
    )


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
    return mask, f"sys::{bit}"


def declaration(values, value):
    """The struct, its conversions, and its constructors for one record."""
    name = pascal(value.native)
    raw = f"sys::{value.native}"
    copyable = not dynamic(value)
    fields, writes, copies, args, names = [], [], [], [], []
    for field in value.fields:
        if field.role == "presence_mask":
            writes.append(f"raw.{native_identifier(field.name)} = 0;")
        elif field.role == "size":
            writes.insert(
                0,
                f"raw.{native_identifier(field.name)} = std::mem::size_of::<{raw}>() as _;",
            )
    for flag in value.mask_flags:
        local = identifier(flag.member)
        mask = native_identifier(flag.mask)
        fields.append(f"{doc(values.bound, flag.name, '    ')}    pub {local}: bool,")
        writes.append(
            f"convert::set_flag(&mut raw.{mask}, sys::{flag.name}, self.{local});"
        )
        copies.append(f"{local}: raw.{mask} & sys::{flag.name} != 0,")
    for field in value.fields:
        place, local = f"raw.{native_identifier(field.name)}", identifier(field.name)
        if field.role == "count":
            arrays = [f for f in value.fields if f.value.length == field.name]
            if not arrays:
                raise Unsupported(f"{value.native}.{field.name}: count has no array")
            array = arrays[0]
            source = f"self.{identifier(array.name)}"
            length = (
                f"{source}.as_ref().map_or(0, |items| items.len())"
                if optional(array.value) or array.presence
                else f"{source}.len()"
            )
            writes.append(f"{place} = convert::count({length})?;")
            continue
        if not field.public:
            continue
        if field.value.kind == "union":
            union = field.value
            union_name = values.public(union)
            fields.append(
                f"{doc(values.bound, f'{value.native}.{field.name}', '    ')}    pub {local}: {union_name},"
            )
            copies.append(f"{local}: {decode(values, union, place)},")
            tag = native_identifier(union.tag)
            cases = " ".join(
                f"{union_name}::{pascal(member.name)}(item) => {{ raw.{tag} = sys::{member.presence.variant}; {place}.{native_identifier(member.name)} = {encode(values, member.value, '(*item)')}; }}"
                for member in union.fields
            )
            writes.append(
                f'match &self.{local} {{ {cases} {union_name}::Unknown(_) => return Err(Error::invalid_argument("unknown union variant cannot be submitted")), }}'
            )
            continue
        typ = values.public(field.value)
        masked = field.presence and field.presence.mask
        fields.append(
            f"{doc(values.bound, f'{value.native}.{field.name}', '    ')}    pub {local}: {'Option<' + typ + '>' if masked else typ},"
        )
        copied = decode(values, field.value, place)
        if field.value.registration:
            # Native never returns callbacks, so a copy leaves a registration
            # unset.
            copied = "None" if masked else "Default::default()"
        if masked:
            write, copy = masked_field(values, field, place, local)
            writes.append(write)
            copies.append(f"{local}: None," if field.value.registration else copy)
        else:
            writes.append(f"{place} = {encode(values, field.value, 'self.' + local)};")
            copies.append(f"{local}: {copied},")
            args.append(f"{local}: {typ}")
            names.append(local)
    constructors = []
    public_fields = [field for field in value.fields if field.public]
    if len(public_fields) == 1 and public_fields[0].value.kind == "union":
        field = public_fields[0]
        union_name = values.public(field.value)
        for member in field.value.fields:
            method = identifier(member.name)
            constructors.append(
                f"pub fn {method}(value: {values.public(member.value)}) -> Self {{ Self {{ {identifier(field.name)}: {union_name}::{pascal(member.name)}(value) }} }}"
            )
    elif numeric(value) and "in" in values.directions.get(value.native, {"in", "out"}):
        # Only a small value made of numbers, such as a coordinate, takes its
        # fields in order. Any other record is built from its default, so a
        # field C adds doesn't break callers.
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
    if not value.default and defaultable(value):
        derives.append("Default")
    default = (
        f"impl Default for {name} {{ fn default() -> Self {{ maplibre_core::require_abi_version(); convert::native_default(unsafe {{ sys::{value.default}() }}) }} }}\n"
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
        f"\n{doc(values.bound, value.native)}#[derive({', '.join(derives)})]\npub struct {name} {{\n"
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
    return f"{mask} |= {bit}"


def mask_test(mask, bit):
    return f"{mask} & {bit} != 0"
