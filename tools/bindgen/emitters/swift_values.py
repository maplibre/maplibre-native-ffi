"""Generate Swift record values and C conversion from typed semantic plans."""

from os.path import commonprefix

from ..model import ModelError
from .swift import SCALARS, camel, doc, identifier, name


class Values:
    def __init__(self, bound):
        self.bound = bound
        self.used = {}
        self.attachments = {}
        self.views = {}
        self.item_buffers = {
            field.value.element.native: field.value.item_buffer
            for value in bound.values.values()
            for field in value.fields
            if field.value.item_buffer and field.value.element
        }

    def public(self, value):
        from .swift_dynamic_values import public

        specialized = public(self, value)
        if specialized:
            return specialized
        if value.kind == "callback":
            from .swift_callbacks import closure_type

            return "(" + closure_type(self, self.bound.callbacks[value.native]) + ")?"
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
            for parameter in callback.parameters:
                if parameter.name != callback.context:
                    self.add(parameter.value)
            if callback.result.native != "void":
                self.add(callback.result)
            return
        if value.kind == "enum":
            self.used[value.native] = value
            return
        if value.kind in {"array", "reference"}:
            self.add(value.element)
        if value.kind == "union":
            if not value.tag or any(
                not f.presence or not f.presence.variant for f in value.fields
            ):
                raise ModelError(
                    [f"Swift: {value.native}: union requires tagged variants"]
                )
            for field in value.fields:
                self.add(field.value)
            self.used[value.native] = value
            return
        if value.kind != "record":
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

    def copy(self, value, expression):
        from .swift_dynamic_values import decode

        return decode(self, value, expression)

    def native(self, value, expression):
        from .swift_dynamic_values import encode

        return encode(self, value, expression)

    def render(self):
        return "\n".join(self.record(value) for _, value in sorted(self.used.items()))

    def record(self, value):
        from .swift_dynamic_values import declaration, dynamic

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
        if value.kind == "union":
            variants = "\n".join(
                f"{doc(self.bound, f'{value.native}.{field.name}', '  ')}  case {identifier(camel(field.name))}({self.public(field.value)})"
                for field in value.fields
            )
            return f"{doc(self.bound, value.native)}public enum {self.public(value)}: Equatable, Hashable, Sendable {{\n{variants}\n{'  case none' if value.empty_variant else ''}\n  case unknown(UInt32, Data)\n  public static var `default`: Self {{ {'.none' if value.empty_variant else '.unknown(0, Data())'} }}\n}}\n"
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
            init.append(f"{local}: Bool = {public}.default.{local}")
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
                if not value.default:
                    materialize.append(
                        f"    {raw} = UInt32(MemoryLayout<{value.native}>.size)"
                    )
                continue
            if not field.public:
                continue
            optional = field.presence and field.presence.mask
            field_type = self.public(field.value)
            fields.append(
                f"{doc(self.bound, f'{value.native}.{field.name}', '  ')}  public var {local}: {field_type}{'?' if optional else ''}"
            )
            init.append(
                f"{local}: {field_type}{'?' if optional else ''} = {public}.default.{local}"
            )
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
        initial = f"{value.default}()" if value.default else f"{value.native}()"
        return f"""{doc(self.bound, value.native)}public struct {public}: Equatable, Hashable, Sendable {{
{chr(10).join(fields)}
  public static var `default`: Self {{ Self(raw: {initial}) }}
  public init({", ".join(init)}) {{
{chr(10).join(assignments)}
  }}
  init(raw: {value.native}) {{
{chr(10).join(captures)}
  }}
  func nativeValue() -> {value.native} {{
    var raw = {initial}
{chr(10).join(materialize)}
    return raw
  }}
}}
"""

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
        prefix = (
            commonprefix([key for key, _ in value.enum_values]).rsplit("_", 1)[0] + "_"
        )
        members = "\n".join(
            f"{doc(self.bound, key, '  ')}  public static let {identifier(camel(key.removeprefix(prefix).lower()))}: {public} = "
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
        return f"{doc(self.bound, value.native)}public struct {public}: {conformance}, NativeOpenValue, Equatable, Hashable, Sendable {{\n  public let rawValue: {raw}\n  public init(rawValue: {raw}) {{ self.rawValue = rawValue }}\n{members}\n}}\n"
