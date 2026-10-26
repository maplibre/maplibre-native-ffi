"""Generate Swift record values and C conversion from typed semantic plans."""

from ..model import ModelError
from ..semantic import enum_member_prefix
from .swift import SCALARS, camel, doc, identifier, name


class Values:
    def __init__(self, bound):
        self.bound = bound
        self.used = {}
        self.attachments = {}
        self.views = {}

    def public(self, value):
        from .swift_dynamic_values import public

        specialized = public(self, value)
        if specialized:
            return specialized
        if value.kind == "callback":
            from .swift_callbacks import alias

            return alias(self.bound.callbacks[value.native]) + "?"
        if value.kind == "handle":
            from .swift_ownership import owner_name

            return owner_name(value.handle)
        if value.kind == "native_pointer":
            return "NativePointer"
        if value.kind == "scalar":
            public = SCALARS.get(
                value.scalar_carrier or value.native,
                SCALARS.get(
                    value.ctype.canonical,
                    {
                        "long": "Int64",
                        "long long": "Int64",
                        "unsigned long": "UInt64",
                        "unsigned long long": "UInt64",
                        "unsigned char": "UInt8",
                        "char": "CChar",
                        "int": "Int32",
                        "unsigned int": "UInt32",
                    }.get(value.ctype.canonical),
                ),
            )
            if public:
                return public
        if value.native in self.bound.slices:
            # A slice record is presented as its array alone.
            return self.public(self.bound.slices[value.native].value)
        if value.kind == "union":
            if value.native.startswith("@"):
                parent, field = next(
                    (parent, field)
                    for parent in self.bound.values.values()
                    for field in parent.fields
                    if field.value.native == value.native
                )
                return name(parent.native.removeprefix("mln_")) + name(field.name)
            return name(value.native.removeprefix("mln_"))
        if value.kind in {"record", "enum"}:
            return name(value.native.removeprefix("mln_"))
        raise ModelError(
            [f"Swift: {value.native}: {value.kind} needs a value conversion"]
        )

    def add(self, value):
        if value.response:
            self.used[value.native] = value
            return
        self.public(value)
        if value.kind == "callback":
            callback = self.bound.callbacks[value.native]
            if any(
                name(native.removeprefix("mln_"))
                == name(value.native.removeprefix("mln_"))
                for native in self.bound.public_values
            ):
                raise ModelError(
                    [f"Swift: {value.native}: the type alias collides with a value"]
                )
            for parameter in callback.arguments:
                self.add(parameter.value)
            if callback.result.native != "void":
                self.add(callback.result)
            self.used[value.native] = value
            return
        if value.kind == "enum":
            self.used[value.native] = value
            return
        if value.kind in {"array", "reference"}:
            self.add(value.element)
        if value.kind == "union":
            self.add_variants(value)
            self.used[value.native] = value
            return
        if value.kind != "record":
            return
        if value.union_member:
            # The record is presented as its union, so the union type is not.
            self.add_variants(value.union_member.value)
            self.used[value.native] = value
            return
        if value.native in self.bound.slices:
            self.add(self.bound.slices[value.native].value)
            return
        for field in value.fields:
            if not field.public:
                continue
            if field.presence and field.presence.variant:
                raise ModelError(
                    [f"Swift: {value.native}: variant requires tag decoding"]
                )
            self.add(field.value)
        self.used[value.native] = value

    def add_variants(self, union):
        if not union.tag or any(
            not f.presence or not f.presence.variant for f in union.fields
        ):
            raise ModelError([f"Swift: {union.native}: union requires tagged variants"])
        for field in union.fields:
            self.add(field.value)

    def copy(self, value, expression):
        from .swift_dynamic_values import decode

        return decode(self, value, expression)

    def native(self, value, expression):
        from .swift_dynamic_values import encode

        return encode(self, value, expression)

    def render(self):
        return "\n".join(self.record(value) for _, value in sorted(self.used.items()))

    def record(self, value):
        from .swift_dynamic_values import (
            declaration,
            dynamic,
            field_zero,
            union_cases,
            union_record,
        )

        if value.response:
            public = self.public(value)
            return f"""public final class {public}: @unchecked Sendable {{
  private let pointer: UnsafeMutablePointer<{value.native}>
  private let scope = NativeViewScope()
  init(pointer: UnsafeMutablePointer<{value.native}>) {{ self.pointer = pointer }}
  var nativePointer: UnsafeMutablePointer<{value.native}> {{ get throws {{ try scope.check(); return pointer }} }}
  func expire() {{ scope.expire() }}
}}
"""
        if value.registration:
            from .swift_callbacks import descriptor

            return descriptor(self, value)
        if value.kind == "callback":
            from .swift_callbacks import declaration

            return declaration(self, value)
        if value.kind == "union":
            return f"{doc(self.bound, value.native)}public enum {self.public(value)}: Equatable, Hashable, Sendable {{\n{union_cases(self, value)}}}\n"
        if value.union_member:
            return union_record(self, value)
        if dynamic(value):
            return declaration(self, value)
        public = self.public(value)
        if value.kind == "enum":
            return self.enumeration(value)
        fields, init, assignments, captures, materialize = [], [], [], [], []
        for field in value.fields:
            if field.role == "presence_mask":
                materialize.append(f"    raw.{field.name} = 0")
        for flag in value.mask_flags:
            local = identifier(camel(flag.member))
            fields.append(
                f"{doc(self.bound, flag.name, '  ')}  public var {local}: Bool"
            )
            init.append(f"{local}: Bool = false")
            assignments.append(f"    self.{local} = {local}")
            captures.append(
                f"    self.{local} = raw.{flag.mask} & {flag.name}.rawValue != 0"
            )
            materialize.append(
                f"    if {local} {{ raw.{flag.mask} |= {flag.name}.rawValue }}"
            )
        for field in value.fields:
            local, raw = identifier(camel(field.name)), "raw." + identifier(field.name)
            if field.role == "size":
                materialize.append(
                    f"    {raw} = UInt32(MemoryLayout<{value.native}>.size)"
                )
                continue
            if not field.public:
                continue
            optional = field.presence and field.presence.mask
            field_type = self.public(field.value) + ("?" if optional else "")
            fields.append(
                f"{doc(self.bound, f'{value.native}.{field.name}', '  ')}  public var {local}: {field_type}"
            )
            default = field_zero(self, field, field_type)
            init.append(f"{local}: {field_type}" + (f" = {default}" if default else ""))
            assignments.append(f"    self.{local} = {local}")
            capture = self.copy(field.value, raw)
            if optional:
                mask, bit = identifier(field.presence.mask), field.presence.bit
                present = f"raw.{mask} & {bit}.rawValue != 0"
                set_presence = f"raw.{mask} |= {bit}.rawValue"
                captures.append(f"    self.{local} = {present} ? {capture} : nil")
                materialize.append(
                    f"    if let item = self.{local} {{ {set_presence}; {raw} = {self.native(field.value, 'item')} }}"
                )
            else:
                captures.append(f"    self.{local} = {capture}")
                materialize.append(
                    f"    {raw} = {self.native(field.value, 'self.' + local)}"
                )
        encoder = (
            f"""  func nativeValue() -> {value.native} {{
    var raw = {value.native}()
{chr(10).join(materialize)}
    return raw
  }}
"""
            if value.native in self.bound.inputs
            else ""
        )
        rendered = f"""{doc(self.bound, value.native)}public struct {public}: Equatable, Hashable, Sendable {{
{chr(10).join(fields)}
  public init({", ".join(init)}) {{
{chr(10).join(assignments)}
  }}
  init(raw: {value.native}) {{
{chr(10).join(captures)}
  }}
{encoder}}}
"""

        return rendered

    def enumeration(self, value):
        public = self.public(value)
        canonical = (
            value.scalar_carrier or value.enum_underlying.canonical
            if value.enum_underlying
            else value.ctype.canonical
        )
        raw = SCALARS.get(
            canonical,
            {
                "int": "Int32",
                "unsigned int": "UInt32",
                "long": "Int64",
                "long long": "Int64",
                "unsigned long": "UInt64",
                "unsigned long long": "UInt64",
            }.get(canonical, "UInt32"),
        )
        members = "\n".join(
            f"{doc(self.bound, key, '  ')}  public static let {identifier(enum_member(value, key))}: {public} = "
            + (
                "[]"
                if number == 0 and value.enum_kind == "bitmask"
                else f"{public}(rawValue: {number})"
            )
            for key, number in value.enum_values
        )
        conformance = (
            "OptionSet" if value.enum_kind == "bitmask" else "RawRepresentable"
        )
        description = ""
        if value.enum_kind != "bitmask":
            conformance += ", CustomStringConvertible"
            cases = "\n".join(
                f'case {number}: return "{enum_member(value, key)}"'
                for number, key in {
                    number: key for key, number in value.enum_values
                }.items()
            )
            description = f'''  public var description: String {{
    switch rawValue {{
{cases}
    default: return "{public}(\\(rawValue))"
    }}
  }}
'''
        return f"{doc(self.bound, value.native)}public struct {public}: {conformance}, NativeOpenValue, Equatable, Hashable, Sendable {{\n  public let rawValue: {raw}\n  public init(rawValue: {raw}) {{ self.rawValue = rawValue }}\n{members}\n{description}}}\n"


def enum_member(value, key):
    """The Swift name of an enum constant's static member."""
    prefix = enum_member_prefix((known for known, _ in value.enum_values), value.native)
    return camel(key.removeprefix(prefix).lower())
