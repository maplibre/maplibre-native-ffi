"""Rust value types from resolved value plans."""

from os.path import commonprefix

from ..semantic import BoundApi, ValuePlan
from .rust import SCALARS, Unsupported, doc, pascal


class Values:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.used: dict[str, ValuePlan] = {}
        # The conversions each record needs: "in" to native, "out" from it.
        self.directions: dict[str, set[str]] = {}
        self.item_buffers = {}

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
        if value.kind == "handle" and value.native in self.bound.decisions:
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
        for group in value.presence_groups:
            if len(group.fields) > 1 and group.type:
                self.check(self.bound.values[group.type])

    def add(self, value: ValuePlan, direction: str = "both") -> str:
        """Uses `value`, which converts to native for "in", from native for
        "out", or both ways."""
        self.check(value)
        if value.kind == "handle":
            if value.native not in self.used:
                self.used[value.native] = value
                decision = self.bound.decisions[value.native]
                complete = self.bound.operations_by_name[decision.complete]
                for parameter in complete.inputs:
                    if parameter.name != complete.receiver:
                        self.add(parameter.value, "in")
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
            self.add(value.element, direction)
        if value.kind == "enum":
            self.used[value.native] = value
        if value.kind in {"record", "union"}:
            wanted = {"in", "out"} if direction == "both" else {direction}
            # A native default converts from native for the Default impl.
            if value.default:
                wanted.add("out")
            known = self.directions.setdefault(value.native, set())
            if value.native in self.used and wanted <= known:
                return self.public(value)
            self.used[value.native] = value
            for added in sorted(wanted - known):
                known.add(added)
                for field in value.fields:
                    if field.role == "value":
                        self.add(field.value, added)
                for group in value.presence_groups:
                    if group.type:
                        self.add(self.bound.values[group.type], added)
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
        specialized = public(self, value)
        if specialized:
            return specialized
        return self.scalar(value) if value.kind == "scalar" else self.name(value)

    def declaration(self, value: ValuePlan) -> str:
        from .rust_dynamic_values import declaration

        if value.kind == "handle":
            from .rust_callbacks import decision_declaration

            return decision_declaration(self, value)
        if value.response:
            from .rust_callbacks import response_declaration

            return response_declaration(self, value)
        if value.registration:
            from .rust_callbacks import declaration as registration

            return registration(self, value)
        if value.kind == "union":
            members = ", ".join(
                doc(self.bound, f"{value.native}.{field.name}")
                + f"{pascal(field.name)}({self.public(field.value)})"
                for field in value.fields
            )
            if value.empty_variant:
                members += ", Empty"
            return f"{doc(self.bound, value.native)}#[derive(Debug, Clone, PartialEq)]\npub enum {self.name(value)} {{ {members}, Unknown(u32) }}\nimpl Default for {self.name(value)} {{ fn default() -> Self {{ Self::Unknown(0) }} }}\n"
        if value.kind == "enum":
            return self.enum(value)
        return declaration(self, value)

    def enum(self, value: ValuePlan) -> str:
        public = pascal(value.native)
        raw = self.scalar(value) or "u32"
        prefix = (
            commonprefix([key for key, _ in value.enum_values]).rsplit("_", 1)[0] + "_"
        )
        if value.enum_kind == "bitmask":
            members = "\n".join(
                f"{doc(self.bound, key, '    ')}    const {key.removeprefix(prefix)} = {number};"
                for key, number in value.enum_values
            )
            return f"native_flags! {{\n{doc(self.bound, value.native)}pub struct {public}: {raw} {{\n{members}\n}}\n}}\n"
        variants = [
            (pascal(key.removeprefix(prefix).lower()), number, key)
            for key, number in value.enum_values
        ]
        # Aliased C enum constants share one Rust variant.
        unique = {}
        for variant in variants:
            unique.setdefault(variant[1], variant)
        variants = list(unique.values())
        unknown = (
            "Unrecognized"
            if any(name == "Unknown" for name, _, _ in variants)
            else "Unknown"
        )
        members = "\n".join(
            f"{doc(self.bound, key, '    ')}    {name} = {number},"
            for name, number, key in variants
        )
        return f"native_enum! {{\n{doc(self.bound, value.native)}pub enum {public}: {raw} {{\n{members}\n}} {unknown}\n}}\n"

    def render(self) -> str:
        return "\n".join(
            self.declaration(value) for _, value in sorted(self.used.items())
        )
