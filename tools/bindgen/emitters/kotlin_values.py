"""Resolve Kotlin value types and emit their common native codecs.

A codec reads or writes one C record at an address through the typed accessors
in the binding's `internal/memory` package. Only the directions an operation
or callback uses are emitted: `putX` writes a record in place, `writeX`
allocates and writes one, and `readX` copies one into its Kotlin value.
"""

from __future__ import annotations

from dataclasses import replace

from .. import docs
from ..names import camel, pascal
from ..semantic import enum_member_prefix
from .kotlin_abi import Abi, LayoutError, width_expression


class Unsupported(ValueError):
    pass


SCALARS = {
    "bool": ("Boolean", "Bool"),
    "_Bool": ("Boolean", "Bool"),
    "double": ("Double", "F64"),
    "float": ("Float", "F32"),
    "uint8_t": ("UByte", "U8"),
    "unsigned char": ("UByte", "U8"),
    "int8_t": ("Byte", "I8"),
    "signed char": ("Byte", "I8"),
    "char": ("Byte", "I8"),
    "uint16_t": ("UShort", "U16"),
    "unsigned short": ("UShort", "U16"),
    "int16_t": ("Short", "I16"),
    "short": ("Short", "I16"),
    "uint32_t": ("UInt", "U32"),
    "unsigned int": ("UInt", "U32"),
    "int32_t": ("Int", "I32"),
    "int": ("Int", "I32"),
    "uint64_t": ("ULong", "U64"),
    "unsigned long long": ("ULong", "U64"),
    "unsigned long": ("ULong", "Size"),
    "size_t": ("ULong", "Size"),
    "int64_t": ("Long", "I64"),
    "long long": ("Long", "I64"),
    "long": ("Long", "I64"),
}

CARRIER = {
    "Boolean": "",
    "Double": "",
    "Float": "",
    "Byte": "",
    "Short": "",
    "Int": "",
    "Long": "",
    "UByte": ".toByte()",
    "UShort": ".toShort()",
    "UInt": ".toInt()",
    "ULong": ".toLong()",
}

# Kotlin's hard keywords, which an identifier can use only in backticks.
KEYWORDS = {
    "as",
    "break",
    "class",
    "continue",
    "do",
    "else",
    "false",
    "for",
    "fun",
    "if",
    "in",
    "interface",
    "is",
    "null",
    "object",
    "package",
    "return",
    "super",
    "this",
    "throw",
    "true",
    "try",
    "typealias",
    "typeof",
    "val",
    "var",
    "when",
    "while",
}


def doc(bound, native: str, indent: str = "") -> str:
    """The KDoc comment of a declaration, or empty when it has none."""
    return docs.block_comment(bound.doc(native), indent)


def name(native):
    return pascal(native.removeprefix("mln_"))


def identifier(native):
    value = camel(native)
    return "`" + value + "`" if value in KEYWORDS else value


def owner_name(handle):
    """Name a handle plan's generated owner class after its stem."""
    return pascal(handle.stem) + "Handle"


def owner_class(handle):
    return owner_name(handle)


def literal(number, typ):
    """A Kotlin literal of [typ] for the integer [number]."""
    if typ in {"ULong", "UInt", "UShort", "UByte"}:
        bits = {"ULong": 64, "UInt": 32, "UShort": 16, "UByte": 8}[typ]
        text = f"{number % (1 << bits)}u" + ("L" if typ == "ULong" else "")
        return text if typ in {"ULong", "UInt"} else f"{text}.to{typ}()"
    if typ == "Long":
        return f"{number}L"
    if typ == "Boolean":
        return "true" if number else "false"
    if typ == "Double":
        return f"{number}.0"
    if typ == "Float":
        return f"{number}f"
    return str(number)


def content_equality(public, members):
    """The class body that makes a data class compare its ByteArray members by content.

    A data class compares an array member by identity, so a class with one replaces the equals and
    hashCode that the compiler derives. A deep array comparison keeps the derived semantics for
    every other member, including the total order of floating-point values. Other classes keep the
    derived members and get no body.
    """
    if not any(typ.rstrip("?") == "ByteArray" for _, typ in members):
        return ""
    names = ", ".join(member for member, _ in members)
    return (
        " {\n"
        f"  private fun contentFields(): Array<Any?> = arrayOf({names})\n"
        f"  override fun equals(other: Any?): Boolean = this === other || other is {public} && contentFields().contentDeepEquals(other.contentFields())\n"
        "  override fun hashCode(): Int = contentFields().contentDeepHashCode()\n"
        "}"
    )


class Values:
    def __init__(self, bound):
        self.bound = bound
        self.abi = Abi(bound.source)
        self.used = {}
        self.views = set()
        self.multiple = {}
        self.attachments = {}
        self.writers = {}
        self.readers = {}
        self.sites = None
        # The split lowering in progress, if any: THREADED for the forms that
        # threadedMain declares, or ASYNC for those that wasmJsMain declares.
        # Each split lowering keeps the callback values that it used and the
        # writers of its split registrations.
        self.split = None
        self.split_used = {"threaded": {}, "async": {}}
        self.split_writers = {"threaded": {}, "async": {}}

    def needs_registration(self, value):
        return bool(
            value.registration
            or (value.element and self.needs_registration(value.element))
            or any(self.needs_registration(f.value) for f in value.fields)
        )

    def scalar(self, value):
        typ = value.enum_underlying or value.ctype
        return SCALARS.get(
            value.scalar_carrier or typ.spelling,
            SCALARS.get(typ.spelling, SCALARS.get(typ.canonical)),
        )

    def fields(self, value):
        registration_controls = (
            {value.registration.user_data, value.registration.release}
            if value.registration
            else set()
        )
        controls = registration_controls | {
            f.name
            for f in value.fields
            if f.role
            in {
                "size",
                "count",
                "reserved",
                "presence_mask",
                "tag",
                "context",
                "stride",
                "delivery_queue",
            }
        }
        controls |= {
            f.presence.mask for f in value.fields if f.presence and f.presence.mask
        }
        if value.registration:
            from . import kotlin_callbacks

            controls |= kotlin_callbacks.omitted_fields(value, self)
        return tuple(f for f in value.fields if f.name not in controls)

    def check(self, value):
        from . import kotlin_callbacks

        if kotlin_callbacks.check(value, self):
            return
        if value.kind == "native_pointer":
            return
        if value.kind == "handle" and value.native in self.bound.public_handles:
            return
        if value.ownership == "owned" or value.registration or value.response:
            raise Unsupported("value needs an ownership or callback protocol")
        if value.kind == "scalar" and self.scalar(value):
            return
        if value.kind == "enum":
            if not self.scalar(value):
                raise Unsupported("enum underlying scalar is unsupported")
            self.used[value.native] = value
            return
        if value.kind == "buffer" and (value.buffer_form == "view" or value.length):
            return
        if value.kind == "array" and value.element:
            if value.element.kind not in {"record", "buffer", "scalar", "enum"}:
                raise Unsupported("array element needs a Kotlin storage rule")
            self.check(value.element)
            return
        if value.kind == "reference" and value.element:
            self.check(value.element)
            return
        if value.kind != "record":
            raise Unsupported(
                f"{value.native}: {value.kind} needs another Kotlin value rule"
            )
        if value.native in self.bound.slices:
            # A slice record reaches host code as its list alone.
            self.check(self.bound.slices[value.native].value)
            return
        try:
            self.abi.record_named(value.native, 64)
        except LayoutError as error:
            raise Unsupported(str(error)) from error
        for field in self.fields(value):
            if field.value.kind == "union":
                if not field.presence or not field.presence.tag:
                    raise Unsupported("union requires an explicit tag")
                for variant in field.value.fields:
                    if not variant.presence or not variant.presence.variant:
                        raise Unsupported("union arm requires a variant")
                    self.check(variant.value)
            else:
                self.check(field.value)
        self.used[value.native] = value

    def public(self, value):
        from . import kotlin_callbacks

        self.check(value)
        callback_type = kotlin_callbacks.public(value, self)
        if callback_type is not None:
            return callback_type
        if value.kind == "native_pointer":
            result = "NativePointer"
        elif value.kind == "handle":
            result = owner_class(value.handle)
        elif value.native in self.bound.slices:
            result = self.public(self.bound.slices[value.native].value)
        elif value.kind in {"record", "enum"}:
            result = name(value.native)
        elif value.kind == "array":
            result = "List<" + self.public(value.element) + ">"
        elif value.kind == "reference":
            result = self.public(replace(value.element, nullable=False))
        elif value.kind == "buffer":
            result = "String" if value.encoding == "utf8" else "ByteArray"
        else:
            result = self.scalar(value)[0]
        return result + ("?" if value.nullable or value.optional else "")

    def members(self, value):
        """The public members of a record: (name, type, field)."""
        result = []
        for field in self.fields(value):
            typ = (
                name(value.native) + pascal(field.name)
                if field.value.kind == "union"
                else self.public(field.value)
            )
            if field.presence and field.presence.mask and not typ.endswith("?"):
                typ += "?"
            result.append((identifier(field.name), typ, field))
        return result

    def flag_name(self, value, flag):
        return identifier(flag.member)

    def output_only(self, value):
        """Whether only the library creates [value], so no call or callback answer writes it."""
        return value.native not in self.writers and not any(
            value.native in writers for writers in self.split_writers.values()
        )

    def field_default(self, field, record=None):
        if record and record.default:
            return f"{name(record.native)}.default.{identifier(field.name)}"
        return self.default(field.value)

    def default(self, value):
        """A field's default, or None when a caller must give it."""
        if value.nullable or value.optional:
            return "null"
        if value.kind == "scalar":
            return literal(0, self.public(value))
        if value.kind == "array":
            return "emptyList()"
        if value.kind == "buffer":
            return '""' if value.encoding == "utf8" else "byteArrayOf()"
        if value.kind == "enum":
            constant = zero_constant(value)
            return f"{name(value.native)}.{constant}" if constant else None
        if value.kind == "record":
            return (
                name(value.native) + "()"
                if not self.output_only(value)
                and all(
                    typ.endswith("?") or self.field_default(field)
                    for _, typ, field in self.members(value)
                )
                else None
            )
        return None

    def common(self):
        """The public value types."""
        from . import kotlin_callbacks

        result = [
            "// Generated by tools/bindgen. Do not edit.",
            "package org.maplibre.nativeffi.generated",
            "",
            *(
                ["import org.maplibre.nativeffi.internal.lifecycle.ViewScope"]
                if self.views
                else []
            ),
            "import kotlin.jvm.JvmInline",
            "import org.maplibre.nativeffi.internal.callback.NativeRegistration",
            "import org.maplibre.nativeffi.internal.values.flagLabel",
            "import org.maplibre.nativeffi.render.NativePointer",
            "",
        ]
        for value in self.used.values():
            if value.kind == "callback" or value.registration or value.response:
                continue
            public = name(value.native)
            if value.kind == "enum":
                scalar = self.scalar(value)[0]
                prefix = enum_member_prefix(
                    (n for n, _ in value.enum_values), value.native
                )
                bitmask = value.enum_kind == "bitmask"
                constants = [
                    f"{doc(self.bound, native, '    ')}    public val {native.removeprefix(prefix)}: {public} = {public}({literal(number, scalar)})"
                    for native, number in value.enum_values
                ]
                if bitmask and not any(n == 0 for _, n in value.enum_values):
                    constants.insert(
                        0,
                        f"    /** No flags. */\n    public val NONE: {public} = {public}({literal(0, scalar)})",
                    )
                result.extend(
                    [
                        f"{enum_doc(self.bound, value)}@JvmInline\npublic value class {public}(public val rawValue: {scalar}) {{",
                        *(
                            [
                                f"  public infix fun or(other: {public}): {public} = {public}(rawValue or other.rawValue)",
                                f"  public infix fun and(other: {public}): {public} = {public}(rawValue and other.rawValue)",
                                f"  public operator fun contains(other: {public}): Boolean = (rawValue and other.rawValue) == other.rawValue",
                                flag_label(value, prefix),
                            ]
                            if bitmask
                            else [enum_label(public, value, prefix, scalar)]
                        ),
                        "  public companion object {",
                        *constants,
                        "  }",
                        "}",
                    ]
                )
                continue
            for field in self.fields(value):
                if field.value.kind != "union":
                    continue
                union_name = public + pascal(field.name)
                variants = [
                    f"{doc(self.bound, f'{field.value.native}.{v.name}', '  ')}  public data class {pascal(v.name)}(public val value: {self.public(v.value)}): {union_name}"
                    + content_equality(
                        pascal(v.name), [("value", self.public(v.value))]
                    )
                    for v in field.value.fields
                ]
                if field.value.empty_variant:
                    variants.append(f"  public data object None: {union_name}")
                variants.append(
                    f"  public data class Unknown(public val tag: UInt, public val bytes: ByteArray): {union_name}"
                    + content_equality(
                        "Unknown", [("tag", "UInt"), ("bytes", "ByteArray")]
                    )
                )
                result.append(
                    f"{doc(self.bound, field.value.native)}public sealed interface {union_name} {{\n"
                    + "\n".join(variants)
                    + "\n}"
                )
            args = []
            typed = []
            # A record that only the library creates takes every field, so a
            # field that C appends reaches each caller that fabricates one.
            required = self.output_only(value)
            for member, typ, field in self.members(value):
                typed.append((member, typ))
                default = (
                    None
                    if required
                    else "null"
                    if typ.endswith("?")
                    else self.field_default(field, value)
                )
                args.append(
                    doc(self.bound, f"{value.native}.{field.name}", "  ")
                    + f"  public val {member}: {typ}"
                    + (" = " + default if default is not None else "")
                )
            args.extend(
                f"{doc(self.bound, flag.name, '  ')}  public val {self.flag_name(value, flag)}: Boolean"
                + ("" if required else " = false")
                for flag in value.mask_flags
            )
            typed.extend(
                (self.flag_name(value, flag), "Boolean") for flag in value.mask_flags
            )
            if value.default:
                companion = f"  public companion object {{ internal val default: {public} by lazy {{ GeneratedApi.{camel(value.default.removeprefix('mln_'))}() }} }}"
            else:
                companion = ""
            if value.native in self.views:
                args = [arg.replace("public val ", "") for arg in args]
                getters = [
                    f"  private val stored{pascal(member.strip('`'))}: {typ} = {member}\n  public val {member}: {typ} get() {{ bindingScope?.ensureActive(); return stored{pascal(member.strip('`'))} }}"
                    for member, typ, _ in self.members(value)
                ]
                result.append(
                    f"{doc(self.bound, value.native)}public class {public}(\n"
                    + ",\n".join(args)
                    + "\n) {\n  internal var bindingScope: ViewScope? = null\n"
                    + "\n".join(getters)
                    + "\n}"
                )
            else:
                result.append(
                    f"{doc(self.bound, value.native)}public data class {public}(\n"
                    + ",\n".join(args)
                    + "\n)"
                    + content_equality(public, typed)
                )
            if companion:
                declaration = result[-1]
                if declaration.rstrip().endswith("}"):
                    result[-1] = declaration.rstrip()[:-1] + "\n" + companion + "\n}"
                else:
                    result[-1] += " {\n" + companion + "\n}"
        for public, (field, value) in self.attachments.items():
            result.append(
                f"/** A new {field} and the completion that reports it ready. */\n"
                f"public data class {public}(public val {field}: {self.public(value)}, public val ready: kotlinx.coroutines.Deferred<Unit>)"
                + content_equality(
                    public,
                    [(field, self.public(value)), ("ready", "Deferred")],
                )
            )
        for public, fields in self.multiple.items():
            result.append(
                f"public data class {public}(\n"
                + ",\n".join(
                    f"  public val {field}: {self.public(value)}"
                    for field, value in fields
                )
                + "\n)"
                + content_equality(
                    public, [(field, self.public(value)) for field, value in fields]
                )
            )
        return "\n".join(result) + "\n" + kotlin_callbacks.common(self)

    def storage(self, ctype):
        """Allocate one value of a C type with that type's size and alignment."""
        return (
            f"allocate({width_expression(self.abi.size(ctype))}, "
            f"{width_expression(self.abi.align(ctype))})"
        )

    def size(self, native):
        return width_expression(
            (
                self.abi.record_named(native, 32).size,
                self.abi.record_named(native, 64).size,
            )
        )

    def align(self, native):
        return width_expression(
            (
                self.abi.record_named(native, 32).align,
                self.abi.record_named(native, 64).align,
            )
        )

    def element_size(self, element):
        if element.kind == "buffer":
            return "2 * NativeMemory.addressSize"
        if element.kind in {"scalar", "enum"}:
            return width_expression(self.abi.size(element.ctype))
        return self.size(element.native)

    def element_align(self, element):
        if element.kind == "buffer":
            return "NativeMemory.addressSize"
        if element.kind in {"scalar", "enum"}:
            return width_expression(self.abi.align(element.ctype))
        return self.align(element.native)

    def at(self, base, record, field):
        offset = width_expression(self.abi.offset(record.native, field))
        return base if offset == "0" else f"{base} + {offset}"

    def field_plan(self, record, field):
        return next(f for f in record.fields if f.name == field)

    def accessor(self, value):
        """The accessor suffix and public type of a scalar or enum's storage."""
        typ, suffix = self.scalar(value)
        return suffix, typ

    def write_scalar(self, value, address, expression):
        suffix, _typ = self.accessor(value)
        return f"write{suffix}({address}, {expression})"

    def put(self, record):
        """Name the in-place writer of [record], emitting it once."""
        from . import kotlin_callbacks

        if kotlin_callbacks.split(record, self):
            self.split_writers[self.split].setdefault(record.native, record)
            return "put" + kotlin_callbacks.public_name(record, self)
        self.writers.setdefault(record.native, record)
        return "put" + name(record.native)

    def write(self, record):
        """Name the allocating writer of [record], emitting it once."""
        return "write" + self.put(record).removeprefix("put")

    def argument(self, value, expression):
        """An expression for [value] as a native-call carrier, in a NativeArena receiver."""
        from . import kotlin_callbacks

        callback = kotlin_callbacks.argument(value, expression)
        if callback is not None:
            return callback
        if value.kind == "handle":
            return f"argument({expression}, {expression}.binding)"
        if value.kind == "native_pointer":
            return f"{expression}.address"
        if value.kind == "reference":
            if value.element.kind not in {"record", "buffer"}:
                raise Unsupported("pointer input requires a record")
            if value.nullable:
                inner = self.argument(value.element, "it")
                return f"{expression}?.let {{ {inner} }} ?: 0L"
            return self.argument(value.element, expression)
        if value.kind == "enum":
            _suffix, typ = self.accessor(value)
            return expression + ".rawValue" + CARRIER[typ]
        if value.kind == "scalar":
            typ = self.scalar(value)[0]
            return expression + CARRIER[typ]
        if value.kind == "record":
            return f"{self.write(value)}({expression})"
        if value.kind == "array":
            element = value.element
            nullable = value.nullable or value.optional
            items = "it" if nullable else expression
            written = (
                f"array({items}, {self.element_size(element)}, {self.element_align(element)}) "
                f"{{ at, item -> {self.place(element, 'at', 'item')} }}"
            )
            return f"{expression}?.let {{ {written} }} ?: 0L" if nullable else written
        if value.kind == "buffer" and value.buffer_form == "view":
            return f"view({expression})"
        if value.kind == "buffer" and value.length == "nul":
            if value.nullable or value.optional:
                return f"{expression}?.let {{ cString(it) }} ?: 0L"
            return f"cString({expression})"
        if value.kind == "buffer":
            data = (
                f"{expression}.encodeToByteArray()"
                if value.encoding == "utf8"
                else expression
            )
            if value.nullable or value.optional:
                data = data.replace(expression, "it", 1)
                return f"{expression}?.let {{ bytes({data}) }} ?: 0L"
            return f"bytes({data})"
        raise Unsupported("input storage requires another Kotlin rule")

    def length(self, value, expression):
        """The element or byte count of an input array or buffer, as a carrier."""
        if value.kind == "buffer" and value.encoding == "utf8":
            expression += ".encodeToByteArray()"
        if value.nullable or value.optional:
            return f"({expression}?.size ?: 0).toLong()"
        return f"{expression}.size.toLong()"

    def place(self, value, address, expression):
        """A statement that writes [value] at [address]."""
        if value.kind == "record":
            return f"{self.put(value)}({address}, {expression})"
        if value.kind == "buffer" and value.buffer_form == "view":
            return f"putView({address}, {expression})"
        if value.kind in {"scalar", "enum"}:
            raw = expression + (".rawValue" if value.kind == "enum" else "")
            return self.write_scalar(value, address, raw)
        return f"writeAddress({address}, {self.argument(value, expression)})"

    def receiver(self, value):
        """The receiver a record's writer needs: a call when it registers callbacks."""
        return "NativeCall" if self.needs_registration(value) else "NativeArena"

    def put_function(self, value):
        if value.registration:
            from . import kotlin_callbacks

            return kotlin_callbacks.put_function(value, self)
        public = name(value.native)
        receiver = self.receiver(value)
        lines = [
            f"internal fun {receiver}.{self.put(value)}(target: Long, value: {public}) {{"
        ]
        for field in value.fields:
            if field.role == "size":
                lines.append(
                    "  "
                    + self.write_scalar(
                        field.value,
                        self.at("target", value, field.name),
                        f"{self.size(value.native)}.toUInt()",
                    )
                )
            if field.role == "presence_mask":
                lines.append(
                    f"  var {identifier(field.name)} = {literal(0, self.scalar(field.value)[0])}"
                )
        for flag in value.mask_flags:
            member = self.flag_name(value, flag)
            lines.append(
                f"  if (value.{member}) {self.mark(value, flag.mask, flag.name)}"
            )
        for member, _typ, field in self.members(value):
            if field.value.kind == "union":
                lines += self.union_write(value, field, f"value.{member}")
                continue
            presence = field.presence
            optional = bool(presence and presence.mask)
            body = []
            local = "it" if optional else f"value.{member}"
            if optional:
                body.append(self.mark(value, presence.mask, presence.bit))
            body += self.field_write(value, field, local)
            if optional:
                lines.append(f"  value.{member}?.let {{")
                lines += ["    " + line for line in body]
                lines.append("  }")
            else:
                lines += ["  " + line for line in body]
        for field in value.fields:
            if field.role == "presence_mask":
                lines.append(
                    "  "
                    + self.write_scalar(
                        field.value,
                        self.at("target", value, field.name),
                        identifier(field.name),
                    )
                )
        lines.append("}")
        lines.append(
            f"internal fun {receiver}.{self.write(value)}(value: {public}): Long = "
            f"allocate({self.size(value.native)}, {self.align(value.native)}).also {{ {self.put(value)}(it, value) }}"
        )
        return "\n".join(lines)

    def field_write(self, record, field, expression):
        value = field.value
        lines = [self.place(value, self.at("target", record, field.name), expression)]
        if value.kind == "array" or (
            value.kind == "buffer" and value.length not in {None, "nul", "1"}
        ):
            lines.append(self.write_count(record, value, expression))
        return lines

    def write_count(self, record, value, expression):
        count = self.field_plan(record, value.length)
        typ = self.scalar(count.value)[0]
        size = f"{expression}.size"
        if value.nullable or value.optional:
            size = size.replace(f"{expression}.", f"{expression}?.", 1) + " ?: 0"
            size = f"({size})"
        converted = {
            "ULong": f"{size}.toULong()",
            "UInt": f"{size}.toUInt()",
            "UShort": f"{size}.toUShort()",
            "UByte": f"{size}.toUByte()",
            "Long": f"{size}.toLong()",
            "Int": size,
            "Short": f"{size}.toShort()",
            "Byte": f"{size}.toByte()",
        }[typ]
        return self.write_scalar(
            count.value, self.at("target", record, value.length), converted
        )

    def sized(self, record, size):
        """Whether [record]'s size field is a leading `uint32_t`, which `sized` writes."""
        address = self.at("it", record, size.name)
        return address == "it" and self.accessor(size.value)[0] == "U32"

    def mark(self, record, mask, bit):
        typ = self.scalar(self.field_plan(record, mask).value)[0]
        local = identifier(mask)
        return f"{local} = {local} or {literal(self.bound.enum_number(bit), typ)}"

    def union_write(self, record, field, expression):
        public = name(record.native) + pascal(field.name)
        union = self.at("target", record, field.name)
        tag = self.field_plan(record, field.presence.tag)
        tag_address = self.at("target", record, field.presence.tag)
        tag_type = self.scalar(tag.value)[0]
        lines = [f"  when (val variant = {expression}) {{"]
        for variant in field.value.fields:
            number = self.bound.enum_number(variant.presence.variant)
            lines.append(
                f"    is {public}.{pascal(variant.name)} -> {{ "
                + self.write_scalar(tag.value, tag_address, literal(number, tag_type))
                + "; "
                + self.place(variant.value, union, "variant.value")
                + " }"
            )
        lines.append(
            f'    is {public}.Unknown -> throw IllegalArgumentException("unknown native union variants cannot be submitted")'
        )
        lines.append("  }")
        return lines

    def read(self, record):
        """Name the reader of [record], emitting it once."""
        self.readers.setdefault(record.native, record)
        return "read" + name(record.native)

    def decode(self, value, address, scope=None):
        """An expression that copies the [value] stored at [address]."""
        from . import kotlin_callbacks

        kotlin_callbacks.check_decode(value)
        if value.kind == "native_pointer":
            raw = (
                f"readAddress({address})"
                if value.ctype.kind == "pointer" or "*" in value.ctype.canonical
                else f"readI64({address})"
            )
            pointer = f"NativePointer.ofAddress({raw})"
            if scope:
                return f"{raw}.let {{ if ({scope} != null) NativePointer.scoped(it, {scope}) else NativePointer.ofAddress(it) }}"
            return pointer
        if value.kind == "enum":
            suffix, _typ = self.accessor(value)
            return f"{name(value.native)}(read{suffix}({address}))"
        if value.kind == "scalar":
            suffix, _typ = self.accessor(value)
            return f"read{suffix}({address})"
        if value.registration and value.native not in self.bound.returned:
            # Native never returns callbacks, so a copy leaves a registration
            # unset.
            return "null" if value.nullable else f"{name(value.native)}()"
        if value.kind == "record":
            reader = self.read(value)
            return (
                f"{reader}({address}, {scope})"
                if value.native in self.views and scope
                else f"{reader}({address})"
            )
        if value.kind == "reference":
            inner = self.decode(value.element, "it", scope)
            if value.nullable:
                return (
                    f"readAddress({address}).takeIf {{ it != 0L }}?.let {{ {inner} }}"
                )
            return self.decode(value.element, f"readAddress({address})", scope)
        if value.kind == "buffer" and value.length == "nul":
            return (
                f"readCStringOrNull(readAddress({address}))"
                if value.nullable
                else f"readCString(readAddress({address}))"
            )
        if value.kind == "buffer" and value.buffer_form == "view":
            text = value.encoding == "utf8"
            if value.nullable:
                return f"readView{'String' if text else ''}OrNull({address})"
            result = f"readView{'String' if text else ''}({address})"
            return result + (".takeIf { it.isNotEmpty() }" if value.optional else "")
        raise Unsupported("output storage requires another Kotlin rule")

    def read_function(self, value):
        if value.registration:
            from . import kotlin_callbacks

            return kotlin_callbacks.read_function(value, self)
        if value.native in self.bound.slices:
            field = self.bound.slices[value.native]
            return (
                f"internal fun {self.read(value)}(source: Long): "
                f"{self.public(field.value)} = {self.field_read(value, field, None)}"
            )
        public = name(value.native)
        scoped = value.native in self.views
        scope = "scope" if scoped else None
        parameters = "source: Long"
        if scoped:
            parameters += ", scope: ViewScope? = null"
        locals = []
        for field in value.fields:
            if field.role == "presence_mask":
                suffix, _ = self.accessor(field.value)
                locals.append(
                    f"val {identifier(field.name)} = read{suffix}({self.at('source', value, field.name)})"
                )
        arguments = []
        for flag in value.mask_flags:
            arguments.append(
                f"{self.flag_name(value, flag)} = {self.present('source', value, flag.mask, flag.name)}"
            )
        for member, _typ, field in self.members(value):
            if field.value.kind == "union":
                arguments.append(f"{member} = {self.union_read(value, field)}")
                continue
            decoded = self.field_read(value, field, scope)
            presence = field.presence
            if presence and presence.mask:
                decoded = f"if ({self.present('source', value, presence.mask, presence.bit)}) {decoded} else null"
            arguments.append(f"{member} = {decoded}")
        body = f"{public}(" + ", ".join(arguments) + ")"
        if scoped:
            body += ".also { it.bindingScope = scope }"
        if locals:
            return (
                f"internal fun {self.read(value)}({parameters}): {public} {{\n  "
                + "\n  ".join(locals)
                + f"\n  return {body}\n}}"
            )
        return f"internal fun {self.read(value)}({parameters}): {public} = {body}"

    def field_read(self, record, field, scope):
        value = field.value
        address = self.at("source", record, field.name)
        if value.kind == "array" or (
            value.kind == "buffer" and value.length not in {None, "nul", "1"}
        ):
            count = self.read_count(record, value.length)
            pointer = f"readAddress({address})"
            if value.kind == "buffer":
                result = f"readBytes({pointer}, {count})"
                if value.encoding == "utf8":
                    result += ".decodeToString()"
                if value.optional:
                    result += ".takeIf { it.isNotEmpty() }"
                return result
            element = value.element
            if value.stride:
                stride = self.read_count(record, value.stride)
                item = self.decode_item(element, "item")
                return f"readStrided({pointer}, {count}, {stride}, {self.element_size(element)}) {{ item -> {item} }}"
            item = self.decode_item(element, "item")
            return f"readArray({pointer}, {count}, {self.element_size(element)}.toLong()) {{ item -> {item} }}"
        return self.decode(value, address, scope)

    def decode_item(self, element, address):
        if element.kind == "buffer":
            return self.decode(element, address)
        return f"{self.read(element)}({address})"

    def read_count(self, record, path):
        field = self.field_plan(record, path)
        suffix, typ = self.accessor(field.value)
        raw = f"read{suffix}({self.at('source', record, path)})"
        return raw if typ == "ULong" else f"{raw}.toULong()"

    def present(self, base, record, mask, bit):
        mask_field = self.field_plan(record, mask)
        _suffix, typ = self.accessor(mask_field.value)
        number = self.bound.enum_number(bit)
        zero = literal(0, typ)
        return f"({identifier(mask)} and {literal(number, typ)}) != {zero}"

    def union_read(self, record, field):
        public = name(record.native) + pascal(field.name)
        tag = self.field_plan(record, field.presence.tag)
        tag_suffix, tag_type = self.accessor(tag.value)
        union = self.at("source", record, field.name)
        arms = []
        for variant in field.value.fields:
            number = self.bound.enum_number(variant.presence.variant)
            arms.append(
                f"{literal(number, tag_type)} -> {public}.{pascal(variant.name)}({self.decode(variant.value, union)})"
            )
        if field.value.empty_variant:
            arms.append(
                f"{literal(field.value.empty_variant[1], tag_type)} -> {public}.None"
            )
        sizes = (
            self.abi.size_align(self.abi.classify(field.value.ctype), 32)[0],
            self.abi.size_align(self.abi.classify(field.value.ctype), 64)[0],
        )
        arms.append(
            f"else -> {public}.Unknown({'tag' if tag_type == 'UInt' else 'tag.toUInt()'}, NativeMemory.getBytes({union}, {width_expression(sizes)}))"
        )
        return (
            f"read{tag_suffix}({self.at('source', record, field.presence.tag)}).let {{ tag -> when (tag) {{ "
            + "; ".join(arms)
            + " } }"
        )

    def split_codecs(self, lowering):
        """The writers of a split lowering's registrations, which its source set declares."""
        previous = self.split
        self.split = lowering
        try:
            return [
                self.put_function(value)
                for value in self.split_writers[lowering].values()
            ]
        finally:
            self.split = previous

    def codecs(self):
        """The common read and write functions for every record a binding uses."""
        functions = []
        emitted_writers, emitted_readers = set(), set()
        while True:
            pending = [
                (native, value)
                for native, value in list(self.writers.items())
                if native not in emitted_writers
            ]
            pending_reads = [
                (native, value)
                for native, value in list(self.readers.items())
                if native not in emitted_readers
            ]
            if not pending and not pending_reads:
                break
            for native, value in pending:
                emitted_writers.add(native)
                functions.append(self.put_function(value))
            for native, value in pending_reads:
                emitted_readers.add(native)
                functions.append(self.read_function(value))
        return (
            "// Generated by tools/bindgen. Do not edit.\n"
            "package org.maplibre.nativeffi.generated\n\n"
            "import org.maplibre.nativeffi.internal.c.C\n"
            "import org.maplibre.nativeffi.internal.c.UpcallStubs\n"
            "import org.maplibre.nativeffi.internal.call.NativeCall\n"
            "import org.maplibre.nativeffi.internal.delivery.HostDelivery\n"
            "import org.maplibre.nativeffi.render.NativePointer\n"
            + (
                "import org.maplibre.nativeffi.internal.lifecycle.ViewScope\n"
                if self.views
                else ""
            )
            + "import org.maplibre.nativeffi.internal.memory.*\n\n"
            + "\n\n".join(functions)
            + "\n"
        )


# What every enum and flag set says about values that C adds later.
ENUM_NOTE = "Later versions may add values; an unknown one keeps its `rawValue`."
FLAGS_NOTE = (
    "Later versions may add flags; unknown bits keep their place in `rawValue`."
)


def enum_doc(bound, value):
    """The KDoc of an enum or flag set, which says that C may add values."""
    found = bound.doc(value.native)
    note = FLAGS_NOTE if value.enum_kind == "bitmask" else ENUM_NOTE
    if found is None:
        return docs.block_comment(docs.Doc(value.native, note))
    return docs.block_comment(replace(found, notes=(*found.notes, note)))


def zero_constant(value):
    """The companion constant whose raw value is zero: a C member, or a flag set's `NONE`."""
    prefix = enum_member_prefix((n for n, _ in value.enum_values), value.native)
    member = next((n for n, number in value.enum_values if number == 0), None)
    if member is not None:
        return member.removeprefix(prefix)
    return "NONE" if value.enum_kind == "bitmask" else None


def flag_label(value, prefix):
    """A `toString` that names a flag set's single-bit members and keeps unknown bits in hex."""
    bits = {}
    for native, number in value.enum_values:
        if number > 0 and number & (number - 1) == 0:
            bits.setdefault(number, native.removeprefix(prefix))
    flags = ", ".join(f'{number}uL to "{label}"' for number, label in bits.items())
    return f'  override fun toString(): String = flagLabel(rawValue.toULong(), "{zero_constant(value)}", listOf({flags}))'


def enum_label(public, value, prefix, scalar):
    """A `toString` that names an open enum's constant and keeps an unknown value's number."""
    labels = {}
    for native, number in value.enum_values:
        labels.setdefault(number, native.removeprefix(prefix))
    branches = "; ".join(
        f'{literal(number, scalar)} -> "{label}"' for number, label in labels.items()
    )
    return f'  override fun toString(): String = when (rawValue) {{ {branches}; else -> "{public}($rawValue)" }}'


def generated_owners(bound) -> list[str]:
    return sorted(bound.public_handles)
