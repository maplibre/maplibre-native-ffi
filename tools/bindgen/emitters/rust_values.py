"""Rust value types from resolved value plans."""

from ..semantic import BoundApi, ValuePlan, enum_member_prefix
from .rust import SCALARS, Unsupported, doc, pascal


class Values:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.used: dict[str, ValuePlan] = {}
        self.directions: dict[str, set[str]] = {}

    def scalar(self, value: ValuePlan) -> str:
        canonical = value.ctype.canonical
        if value.kind == "enum" and value.enum_underlying:
            canonical = value.enum_underlying.canonical
        canonical = value.scalar_carrier or canonical
        return SCALARS.get(
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
        )

    def check(self, value: ValuePlan) -> None:
        if value.response:
            return
        # A callback adopts an owned handle into its generated owner.
        if value.kind == "handle":
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
            if not field.public:
                continue
            if field.presence and field.presence.variant:
                raise Unsupported(
                    f"{value.native}: variant capture requires its discriminant"
                )
            self.check(field.value)

    def add(self, value: ValuePlan, direction: str = "both") -> str:
        """Uses `value`, which converts to native for "in", from native for
        "out", or both ways."""
        self.check(value)
        if value.kind == "handle":
            return self.public(value)
        if value.response:
            self.used[value.native] = value
            return self.public(value)
        if value.registration:
            from .rust_callbacks import add

            add(self, value)
            return self.public(value)
        if array := self.bound.slices.get(value.native):
            # A slice record presents only its array.
            return self.add(array.value, direction)
        if value.kind in {"reference", "array"}:
            self.add(value.element, direction)
        if value.kind == "enum":
            self.used[value.native] = value
        if value.kind in {"record", "union"}:
            wanted = {"in", "out"} if direction == "both" else {direction}
            if value.native not in self.bound.inputs:
                wanted.discard("in")
            known = self.directions.setdefault(value.native, set())
            if value.native in self.used and wanted <= known:
                return self.public(value)
            self.used[value.native] = value
            # A record that holds only a union presents the union's variants.
            member = value.union_member
            fields = member.value.fields if member else value.fields
            for added in sorted(wanted - known):
                known.add(added)
                for field in fields:
                    if field.role == "value":
                        self.add(field.value, added)
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
                return "sys::" + value.native
            return (
                "*const std::ffi::c_void"
                if value.ctype.pointee and value.ctype.pointee.const
                else "*mut std::ffi::c_void"
            )
        if value.response:
            return f"{self.name(value)}<'_>"
        if value.kind == "handle":
            from .rust_owners import owner_name

            return owner_name(value.native)
        specialized = public(self, value)
        if specialized:
            return specialized
        return self.scalar(value) if value.kind == "scalar" else self.name(value)

    def declaration(self, value: ValuePlan) -> str:
        from .rust_dynamic_values import declaration, union_declaration

        if value.response:
            from .rust_callbacks import response_declaration

            return response_declaration(self, value)
        if value.registration:
            from .rust_callbacks import declaration as registration

            return registration(self, value)
        if value.kind == "union":
            return union_enum(self, value)
        if value.kind == "enum":
            return self.enum(value)
        if value.union_member:
            return union_declaration(self, value)
        return declaration(self, value)

    def enum(self, value: ValuePlan) -> str:
        if value.native == "mln_status":
            return "pub use maplibre_core::Status;\nimpl ToNative<i32> for Status { fn to_native(&self, _: &mut InputArena) -> Result<i32> { Ok(self.as_raw()) } }\nimpl FromNative<i32> for Status { unsafe fn from_native(raw: i32) -> Result<Self> { Ok(Self::from_raw(raw)) } }\n"
        public = pascal(value.native)
        raw = self.scalar(value) or "u32"
        if value.enum_kind == "bitmask":
            members = "\n".join(
                f"{doc(self.bound, key, '    ')}    const {name} = {number};"
                for key, number in value.enum_values
                if (name := enum_member(value, key))
            )
            return f"native_flags! {{\n{doc(self.bound, value.native)}pub struct {public}: {raw} {{\n{members}\n}}\n}}\n"
        # Aliased C enum constants share one Rust variant.
        variants = [
            (enum_member(value, key), number, key)
            for key, number in value.enum_values
            if key == _first_constant(value, number)
        ]
        unknown = (
            "Unrecognized"
            if any(name == "Unknown" for name, _, _ in variants)
            else "Unknown"
        )
        members = "\n".join(
            f"{doc(self.bound, key, '    ')}    {name} = {number},"
            for name, number, key in variants
        )
        # Only a declared zero member is a default; an unknown 0 is not.
        zero = next((name for name, number, _ in variants if number == 0), None)
        default = (
            f"impl Default for {public} {{ fn default() -> Self {{ Self::{zero} }} }}\n"
            if zero
            else ""
        )
        return f"native_enum! {{\n{doc(self.bound, value.native)}pub enum {public}: {raw} {{\n{members}\n}} {unknown}\n}}\n{default}"

    def render(self) -> str:
        return "\n".join(
            self.declaration(value) for _, value in sorted(self.used.items())
        )


def _first_constant(value: ValuePlan, number: int) -> str:
    return next(key for key, known in value.enum_values if known == number)


def enum_member(value: ValuePlan, key: str) -> str | None:
    """The Rust name of an enum constant: a bitflags constant, or a variant,
    which aliased constants share. bitflags treats a zero flag as contained in
    every value, so the empty set stands for it, and it has no name."""
    prefix = enum_member_prefix((known for known, _ in value.enum_values), value.native)
    number = dict(value.enum_values)[key]
    if value.enum_kind == "bitmask":
        return key.removeprefix(prefix) if number != 0 else None
    return pascal(_first_constant(value, number).removeprefix(prefix).lower())


def union_enum(values: Values, owner: ValuePlan) -> str:
    """The enum of a union's variants, named and documented after `owner`: the
    union itself, or the record that holds only it. A union that native returns
    keeps tags this binding predates as `Unknown`."""
    union = owner.union_member.value if owner.union_member else owner
    members = ", ".join(
        doc(values.bound, f"{union.native}.{field.name}")
        + f"{pascal(field.name)}({values.public(field.value)})"
        for field in union.fields
    )
    if union.empty_variant:
        members += ", Empty"
    unknown = owner.native in values.bound.returned
    if unknown:
        members += ", Unknown(u32)"
    name = values.name(owner)
    rendered = f"{doc(values.bound, owner.native)}#[derive(Debug, Clone, PartialEq)]\npub enum {name} {{ {members} }}\n"
    if unknown:
        rendered += f"impl Default for {name} {{ fn default() -> Self {{ Self::Unknown(0) }} }}\n"
    return rendered
