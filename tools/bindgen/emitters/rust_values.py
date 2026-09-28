"""Static Rust record materialization and capture from resolved value plans."""

from os.path import commonprefix

from ..semantic import BoundApi, ValuePlan
from .rust import SCALARS, Unsupported, identifier, native_identifier, pascal


class Values:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.used: dict[str, ValuePlan] = {}
        self.item_buffers = {}

    def scalar(self, value: ValuePlan) -> str:
        canonical = value.ctype.canonical
        if value.kind == "enum" and value.enum_underlying:
            canonical = value.enum_underlying.canonical
        canonical = value.scalar_carrier or canonical
        return SCALARS.get(
            canonical,
            SCALARS.get(
                canonical,
                {
                    "unsigned int": "u32",
                    "int": "i32",
                    "unsigned long": "u64",
                    "unsigned long long": "u64",
                    "long": "i64",
                    "long long": "i64",
                    "unsigned char": "u8",
                    "signed char": "i8",
                    "short": "i16",
                    "unsigned short": "u16",
                }.get(canonical, ""),
            ),
        )

    def check(self, value: ValuePlan) -> None:
        if value.response:
            return
        if value.kind == "handle" and any(
            callback.decision and callback.decision.handle.native == value.native
            for callback in self.bound.callbacks.values()
        ):
            return
        if value.registration:
            from .rust_callbacks import validate

            return validate(self, value)
        if value.kind == "native_pointer":
            return
        if value.kind == "scalar" and self.scalar(value):
            return
        if value.kind == "enum":
            return
        if value.kind == "buffer" and value.encoding in {"utf8", "json", "bytes"}:
            return
        if value.kind in {"reference", "array"} and value.element:
            self.check(value.element)
            return
        if value.kind == "union":
            if not value.tag or any(
                not f.presence or not f.presence.variant for f in value.fields
            ):
                raise Unsupported(f"{value.native}: union requires tagged variants")
            for field in value.fields:
                self.check(field.value)
            return
        if value.kind != "record":
            raise Unsupported(f"{value.native}: {value.kind} needs a Rust conversion")
        for field in value.fields:
            if field.role in {
                "size",
                "reserved",
                "presence_mask",
                "count",
                "stride",
                "arena",
            }:
                continue
            if field.presence and field.presence.variant:
                raise Unsupported(
                    f"{value.native}: variant capture requires its discriminant"
                )
            self.check(field.value)
        for group in value.presence_groups:
            if len(group.fields) > 1 and group.type:
                self.check(self.bound.values[group.type])

    def add(self, value: ValuePlan) -> str:
        self.check(value)
        if value.kind == "handle":
            if value.native not in self.used:
                self.used[value.native] = value
                decision = next(
                    callback.decision
                    for callback in self.bound.callbacks.values()
                    if callback.decision
                    and callback.decision.handle.native == value.native
                )
                complete = self.bound.operations_by_name[decision.complete]
                for parameter in complete.inputs:
                    if parameter.name != complete.receiver:
                        self.add(parameter.value)
            return self.public(value)
        if value.response:
            self.used[value.native] = value
            return self.public(value)
        if value.registration:
            from .rust_callbacks import add

            add(self, value)
            return self.public(value)
        if value.kind in {"reference", "array"}:
            if value.item_buffer:
                self.item_buffers[value.element.native] = value.item_buffer
            self.add(value.element)
        if value.kind == "enum":
            self.used[value.native] = value
        if value.kind in {"record", "union"} and value.native not in self.used:
            self.used[value.native] = value
            for field in value.fields:
                if field.role == "value":
                    self.add(field.value)
            for group in value.presence_groups:
                if group.type:
                    self.add(self.bound.values[group.type])
        return self.public(value)

    def name(self, value: ValuePlan) -> str:
        if value.native.startswith("@"):
            parent, field = next(
                (parent, field)
                for parent in self.bound.values.values()
                for field in parent.fields
                if field.value.native == value.native
            )
            return pascal(parent.native) + pascal(field.name)
        return pascal(value.native)

    def public(self, value: ValuePlan) -> str:
        from .rust_dynamic_values import public

        if value.kind == "native_pointer":
            if value.ctype.kind == "typedef":
                return "maplibre_native_ffi_sys::" + value.native
            return (
                "*const std::ffi::c_void"
                if value.ctype.pointee and value.ctype.pointee.const
                else "*mut std::ffi::c_void"
            )
        if value.response:
            return f"crate::generated::{self.name(value)}<'_>"
        specialized = public(self, value)
        if specialized:
            return specialized
        return (
            self.scalar(value)
            if value.kind == "scalar"
            else f"crate::generated::{self.name(value)}"
        )

    def copy(self, value: ValuePlan, expression: str) -> str:
        if value.kind in {"scalar", "native_pointer"}:
            return expression
        return f"{self.public(value)}::from_native({expression})"

    def native(self, value: ValuePlan, expression: str) -> str:
        return (
            expression
            if value.kind in {"scalar", "native_pointer"}
            else f"{expression}.to_native()"
        )

    def declaration(self, value: ValuePlan) -> str:
        from .rust_dynamic_values import declaration, dynamic

        if value.kind == "handle":
            from .rust_callbacks import decision_declaration

            return decision_declaration(self, value)
        if value.response:
            from .rust_callbacks import response_declaration

            return response_declaration(self, value)
        if value.registration:
            from .rust_callbacks import declaration

            return declaration(self, value)
        if value.kind == "union":
            members = ", ".join(
                f"{pascal(field.name)}({self.public(field.value)})"
                for field in value.fields
            )
            if value.empty_variant:
                members += ", Empty"
            return f"#[derive(Debug, Clone, PartialEq)]\npub enum {self.name(value)} {{ {members}, Unknown(u32) }}\nimpl Default for {self.name(value)} {{ fn default() -> Self {{ Self::Unknown(0) }} }}\n"
        if dynamic(value):
            return declaration(self, value)
        if value.kind == "enum":
            public = pascal(value.native)
            raw = self.scalar(value) or "u32"
            prefix = (
                commonprefix([key for key, _ in value.enum_values]).rsplit("_", 1)[0]
                + "_"
            )
            if value.enum_kind == "bitmask":
                members = "\n".join(
                    f"        const {key.removeprefix(prefix)} = {number};"
                    for key, number in value.enum_values
                )
                return f"bitflags::bitflags! {{ #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)] pub struct {public}: {raw} {{\n{members}\n        const _ = !0;\n}} }}\nimpl {public} {{ pub fn from_native(raw: {raw}) -> Self {{ Self::from_bits_retain(raw) }} pub fn to_native(self) -> {raw} {{ self.bits() }} }}\n"
            variants = [
                (pascal(key.removeprefix(prefix).lower()), number)
                for key, number in value.enum_values
            ]
            # Aliased C enum constants share one Rust variant.
            variants = list(
                {number: (name, number) for name, number in reversed(variants)}.values()
            )
            unknown = (
                "Unrecognized"
                if any(name == "Unknown" for name, _ in variants)
                else "Unknown"
            )
            fields = ", ".join(name for name, _ in variants)
            capture = ", ".join(
                f"{number} => Self::{name}" for name, number in variants
            )
            native = ", ".join(f"Self::{name} => {number}" for name, number in variants)
            return f"#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]\npub enum {public} {{ {fields}, {unknown}({raw}) }}\nimpl Default for {public} {{ fn default() -> Self {{ Self::from_raw(0) }} }}\nimpl {public} {{ pub fn from_raw(raw: {raw}) -> Self {{ match raw {{ {capture}, value => Self::{unknown}(value) }} }} pub fn as_raw(self) -> {raw} {{ match self {{ {native}, Self::{unknown}(value) => value }} }} pub fn from_native(raw: {raw}) -> Self {{ Self::from_raw(raw) }} pub fn to_native(self) -> {raw} {{ self.as_raw() }} }}\n"
        fields, to_native, from_native, args, names = [], [], [], [], []
        for field in value.fields:
            if field.role == "presence_mask":
                to_native.append(
                    f"        raw.{field.name} = {'false' if field.value.ctype.canonical in {'_Bool', 'bool'} else '0'};"
                )
        for flag in value.mask_flags:
            mask_plan = next(
                field.value for field in value.fields if field.name == flag.mask
            )
            prefix = (
                commonprefix([key for key, _ in mask_plan.enum_values]).rsplit("_", 1)[
                    0
                ]
                + "_"
            )
            local = identifier(flag.name.removeprefix(prefix).lower())
            fields.append(f"    pub {local}: bool,")
            to_native.append(
                f"        if self.{local} {{ raw.{native_identifier(flag.mask)} |= maplibre_native_ffi_sys::{flag.name}; }}"
            )
            from_native.append(
                f"            {local}: raw.{native_identifier(flag.mask)} & maplibre_native_ffi_sys::{flag.name} != 0,"
            )
        grouped = {
            field
            for group in value.presence_groups
            if len(group.fields) > 1
            for field in group.fields
        }
        for group in value.presence_groups:
            if len(group.fields) == 1:
                continue
            child = self.bound.values[group.type]
            # Header mask names supply the semantic group label; the group type
            # supplies the member names and their conversion rules.
            prefix = value.native.removeprefix("mln_").removesuffix("s").upper() + "_"
            label = group.bit.removeprefix("MLN_").removeprefix(prefix).lower()
            local = identifier(label)
            fields.append(f"    pub {local}: Option<{self.public(child)}>,")
            members = {f.name: f for f in value.fields}
            assignments = " ".join(
                f"raw.{native_identifier(field)} = {self.native(members[field].value, 'value.' + identifier(field))};"
                for field in group.fields
            )
            to_native.append(
                f"        if let Some(value) = self.{local} {{ raw.{native_identifier(group.mask)} |= maplibre_native_ffi_sys::{group.bit}; {assignments} }}"
            )
            captured = ", ".join(
                f"{identifier(field)}: {self.copy(members[field].value, 'raw.' + native_identifier(field))}"
                for field in group.fields
            )
            from_native.append(
                f"            {local}: (raw.{native_identifier(group.mask)} & maplibre_native_ffi_sys::{group.bit} != 0).then(|| {self.public(child)} {{ {captured} }}),"
            )
        for field in value.fields:
            local, raw = identifier(field.name), "raw." + native_identifier(field.name)
            if field.role == "size":
                if not value.default:
                    to_native.append(
                        f"        {raw} = std::mem::size_of::<maplibre_native_ffi_sys::{value.native}>() as _;"
                    )
                continue
            if (
                field.role in {"reserved", "presence_mask", "count", "stride", "arena"}
                or field.name in grouped
            ):
                continue
            public = self.public(field.value)
            optional = field.presence and field.presence.mask
            fields.append(
                f"    pub {local}: {'Option<' + public + '>' if optional else public},"
            )
            converted = self.copy(field.value, raw)
            if optional:
                mask, bit = native_identifier(field.presence.mask), field.presence.bit
                set_presence = (
                    f"raw.{mask} |= maplibre_native_ffi_sys::{bit}"
                    if bit
                    else f"raw.{mask} = true"
                )
                present = (
                    f"raw.{mask} & maplibre_native_ffi_sys::{bit} != 0"
                    if bit
                    else f"raw.{mask}"
                )
                to_native.append(
                    f"        if let Some(value) = self.{local} {{ {set_presence}; {raw} = {self.native(field.value, 'value')}; }}"
                )
                from_native.append(
                    f"            {local}: ({present}).then(|| {converted}),"
                )
            else:
                to_native.append(
                    f"        {raw} = {self.native(field.value, 'self.' + local)};"
                )
                from_native.append(f"            {local}: {converted},")
                args.append(f"{local}: {public}")
                names.append(local)
        public = pascal(value.native)
        initial = (
            f"unsafe {{ maplibre_native_ffi_sys::{value.default}() }}"
            if value.default
            else f"unsafe {{ std::mem::zeroed::<maplibre_native_ffi_sys::{value.native}>() }}"
        )
        constructor = (
            ""
            if value.presence_groups
            else f"    pub const fn new({', '.join(args)}) -> Self {{ Self {{ {', '.join(names)} }} }}\n"
        )
        # A record with a field mask remains copyable: every supported member is
        # a scalar or another recursively copyable record.
        return f"""#[derive(Debug, Clone, Copy, PartialEq{", Default" if not value.default else ""})]
pub struct {public} {{
{chr(10).join(fields)}
}}
{f"impl Default for {public} {{ fn default() -> Self {{ Self::from_native(unsafe {{ maplibre_native_ffi_sys::{value.default}() }}) }} }}" if value.default else ""}
impl {public} {{
{constructor}    #[doc(hidden)]
    pub fn to_native({"&self" if value.presence_groups else "self"}) -> maplibre_native_ffi_sys::{value.native} {{
        let mut raw = {initial};
{chr(10).join(to_native)}
        raw
    }}
    #[doc(hidden)]
    pub fn from_native(raw: maplibre_native_ffi_sys::{value.native}) -> Self {{
        Self {{
{chr(10).join(from_native)}
        }}
    }}
}}
impl crate::values::NativeValue for {public} {{
    type Raw = maplibre_native_ffi_sys::{value.native};
    fn to_native(self) -> Self::Raw {{ {public}::to_native({"&self" if value.presence_groups else "self"}) }}
    fn from_native(value: Self::Raw) -> Self {{ Self::from_native(value) }}
}}
"""

    def render(self) -> str:
        return "\n".join(
            self.declaration(value) for _, value in sorted(self.used.items())
        )
