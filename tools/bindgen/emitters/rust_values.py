"""Rust value types from resolved value plans."""

from ..protocol import STATUS
from ..semantic import BoundApi, ValuePlan, enum_member_prefix
from .rust import SCALARS, Unsupported, doc, pascal


class Values:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.used: dict[str, ValuePlan] = {}
        self.directions: dict[str, set[str]] = {}
        # The module and the public type of each owned handle, by C name.
        self.owners: dict[str, str] = {}
        self.handle_types: dict[str, str] = {}
        self.borrowed = borrowed_records(bound)
        self.queue_generic = queue_generic_records(bound)
        # The lifetime that a borrowed record's type names: elided where a
        # signature uses it, and the record's own inside its declaration.
        self.lifetime = "'_"
        # The queue kind that a queued registration's type names: the default,
        # `Shared`, where unnamed, and `K` inside a queued view or a generic
        # record's declaration.
        self.queue_kind = ""

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
        if value.kind in {"reference", "array"} and value.element:
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
        if value.native in self.borrowed:
            return f"{self.name(value)}<{self.lifetime}>"
        if value.native in self.queue_generic and self.queue_kind:
            return f"{self.name(value)}<{self.queue_kind}>"
        return self.scalar(value) if value.kind == "scalar" else self.name(value)

    def declaration(self, value: ValuePlan) -> str:
        from .rust_dynamic_values import declaration, union_declaration

        if value.response:
            from .rust_callbacks import response_declaration

            return response_declaration(self, value)
        if value.registration:
            from .rust_callbacks import declaration as registration

            return registration(self, value)
        if value.kind == "enum":
            return self.enum(value)
        if value.native in self.borrowed:
            self.lifetime = "'a"
        if value.native in self.queue_generic:
            if value.native in self.borrowed or value.kind == "union":
                raise Unsupported(f"{value.native}: a borrowed or union queued record")
            self.queue_kind = "K"
        try:
            if value.kind == "union":
                return union_enum(self, value)
            if value.union_member:
                return union_declaration(self, value)
            return declaration(self, value)
        finally:
            self.lifetime = "'_"
            self.queue_kind = ""

    def generic(self, value: ValuePlan) -> str:
        """The parameter of a borrowed record's or queued record's type."""
        if value.native in self.queue_generic:
            return "<K>"
        return "<'a>" if value.native in self.borrowed else ""

    def generics(self, value: ValuePlan) -> tuple[str, str]:
        """The parameters that a record's declaration and its impls name."""
        if value.native in self.queue_generic:
            return "<K: QueueKind = Shared>", "<K: QueueKind>"
        generic = self.generic(value)
        return generic, generic

    def borrowing(self) -> bool:
        """Whether the declaration being rendered borrows its text and bytes."""
        return self.lifetime == "'a"

    def enum(self, value: ValuePlan) -> str:
        if value.native == STATUS:
            return "pub use crate::status::Status;\nimpl ToNative<i32> for Status { fn to_native(&self, _: &mut InputArena) -> Result<i32> { Ok(self.as_raw()) } }\nimpl FromNative<i32> for Status { unsafe fn from_native(raw: i32) -> Result<Self> { Ok(Self::from_raw(raw)) } }\n"
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


def borrowed_records(bound: BoundApi) -> frozenset[str]:
    """The records and unions that borrow their text and bytes from the
    caller: those that only go to native and that no registration keeps, which
    hold text or bytes, directly or in another such record."""

    def values_in(value: ValuePlan | None):
        while value is not None:
            yield value
            value = value.element

    def fields(value: ValuePlan):
        return (
            item
            for field in value.fields
            if field.role == "value"
            for item in values_in(field.value)
        )

    # A registration's state outlives the call that submits it.
    kept: set[str] = set()
    pending = [
        item
        for value in bound.values.values()
        if value.registration
        for field in value.fields
        if field.name not in value.registration.callbacks
        for item in values_in(field.value)
    ]
    while pending:
        value = pending.pop()
        if value.kind in {"record", "union"} and value.native not in kept:
            kept.add(value.native)
            pending.extend(fields(value))
    candidates = [
        value
        for native, value in bound.values.items()
        if value.kind in {"record", "union"}
        and not value.registration
        and not value.response
        and native not in bound.returned
        and native not in kept
        and native not in bound.slices
    ]
    borrowed: set[str] = set()
    while added := [
        value.native
        for value in candidates
        if value.native not in borrowed
        and any(
            item.kind == "buffer"
            and item.encoding in {"utf8", "json", "bytes"}
            or item.native in borrowed
            for item in fields(value)
        )
    ]:
        borrowed.update(added)
    return frozenset(borrowed)


def queue_generic_records(bound: BoundApi) -> frozenset[str]:
    """The registrations that a delivery queue can carry, and the records that
    hold one, which are generic over the queue's kind."""
    from .rust_callbacks import queued_registrations

    found = set(queued_registrations(bound))

    def holds(value: ValuePlan | None) -> bool:
        while value is not None:
            if value.native in found:
                return True
            value = value.element
        return False

    while added := [
        native
        for native, value in bound.values.items()
        if native not in found
        and value.kind == "record"
        and not value.registration
        and any(holds(field.value) for field in value.fields if field.role == "value")
    ]:
        found.update(added)
    return frozenset(found)


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
    generic = values.generic(owner)
    rendered = f"{doc(values.bound, owner.native)}#[derive(Debug, Clone, PartialEq)]\npub enum {name}{generic} {{ {members} }}\n"
    if unknown:
        rendered += f"impl{generic} Default for {name}{generic} {{ fn default() -> Self {{ Self::Unknown(0) }} }}\n"
    return rendered
