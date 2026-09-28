"""Resolve Kotlin values and emit conversions for each Kotlin native boundary."""

from __future__ import annotations

from dataclasses import replace
from os.path import commonprefix

from ..names import camel, pascal


class Unsupported(ValueError):
    pass


SCALARS = {
    "bool": ("Boolean", "BOOLEAN"),
    "_Bool": ("Boolean", "BOOLEAN"),
    "double": ("Double", "DOUBLE"),
    "float": ("Float", "FLOAT"),
    "uint8_t": ("UByte", "BYTE"),
    "unsigned char": ("UByte", "BYTE"),
    "int8_t": ("Byte", "BYTE"),
    "signed char": ("Byte", "BYTE"),
    "char": ("Byte", "BYTE"),
    "uint16_t": ("UShort", "SHORT"),
    "unsigned short": ("UShort", "SHORT"),
    "int16_t": ("Short", "SHORT"),
    "short": ("Short", "SHORT"),
    "uint32_t": ("UInt", "INT"),
    "unsigned int": ("UInt", "INT"),
    "int32_t": ("Int", "INT"),
    "int": ("Int", "INT"),
    "uint64_t": ("ULong", "LONG"),
    "unsigned long long": ("ULong", "LONG"),
    "unsigned long": ("ULong", "LONG"),
    "size_t": ("ULong", "LONG"),
    "int64_t": ("Long", "LONG"),
    "long long": ("Long", "LONG"),
    "long": ("Long", "LONG"),
}


def name(native):
    return pascal(native.removeprefix("mln_"))


def identifier(native):
    value = camel(native)
    return (
        "`" + value + "`"
        if value
        in {
            "class",
            "object",
            "when",
            "in",
            "is",
            "as",
            "fun",
            "val",
            "var",
            "return",
            "interface",
            "null",
            "true",
            "false",
        }
        else value
    )


def native_identifier(native):
    return "`" + native + "`" if identifier(native).startswith("`") else native


from . import kotlin_callbacks

OWNERS = {
    "mln_event_batch": "org.maplibre.nativeffi.generated.EventBatchHandle",
    "mln_render_frame_batch": "org.maplibre.nativeffi.generated.RenderFrameBatchHandle",
    "mln_buffer": "org.maplibre.nativeffi.generated.BufferHandle",
    "mln_render_session": "org.maplibre.nativeffi.render.RenderSessionHandle",
    "mln_resource_request_handle": "org.maplibre.nativeffi.resource.ResourceRequestHandle",
    "mln_acquired_frame": "org.maplibre.nativeffi.render.AcquiredFrameHandle",
    "mln_map": "org.maplibre.nativeffi.map.MapHandle",
    "mln_runtime": "org.maplibre.nativeffi.runtime.RuntimeHandle",
    "mln_map_projection": "org.maplibre.nativeffi.map.MapProjectionHandle",
    "mln_geojson_source_data": "org.maplibre.nativeffi.style.GeoJsonSourceDataHandle",
}


class Values:
    def __init__(self, bound):
        self.bound = bound
        self.used = {}
        self.arrays = {}
        self.groups = {}
        self.views = set()
        self.item_buffers = {}
        self.multiple = {}
        self.attachments = {}

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
                "arena",
            }
        }
        if value.native in self.item_buffers:
            arena = self.item_buffers[value.native]
            controls |= {arena.offset, arena.length}
        controls |= {
            f.presence.mask for f in value.fields if f.presence and f.presence.mask
        }
        return tuple(f for f in value.fields if f.name not in controls)

    def check(self, value):
        if kotlin_callbacks.check(value, self):
            return
        if value.kind == "native_pointer":
            return
        if value.kind == "handle" and value.native in OWNERS:
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
            if value.item_buffer:
                self.item_buffers[value.element.native] = value.item_buffer
            if value.element.kind not in {"record", "buffer"}:
                raise Unsupported("array element needs a Kotlin storage rule")
            self.check(value.element)
            self.arrays[value.element.native] = value.element
            return
        if value.kind == "reference" and value.element:
            self.check(value.element)
            return
        if value.kind != "record":
            raise Unsupported(
                f"{value.native}: {value.kind} needs another Kotlin value rule"
            )
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
        for group in value.presence_groups:
            if len(group.fields) > 1 and group.type:
                self.check(self.bound.values[group.type])
        self.used[value.native] = value

    def public(self, value):
        self.check(value)
        callback_type = kotlin_callbacks.public(value, self)
        if callback_type is not None:
            return callback_type
        if value.kind == "native_pointer":
            result = "org.maplibre.nativeffi.render.NativePointer"
        elif value.kind == "handle":
            result = OWNERS[value.native]
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
        fields = self.fields(value)
        grouped = {
            field: group
            for group in value.presence_groups
            if len(group.fields) > 1
            for field in group.fields
        }
        seen, result = set(), []
        for field in fields:
            group = grouped.get(field.name)
            if group:
                key = (group.mask, group.bit)
                if key in seen:
                    continue
                seen.add(key)
                enum = next(
                    (
                        e
                        for e in self.bound.source.enums
                        if any(v.name == group.bit for v in e.values)
                    ),
                    None,
                )
                prefix = (
                    commonprefix([v.name for v in enum.values]).rsplit("_", 1)[0] + "_"
                    if enum
                    else "has_"
                )
                member = identifier(
                    (group.bit or group.mask).removeprefix(prefix).lower()
                )
                group_name = (
                    name(group.type)
                    if group.type
                    else name(value.native) + pascal(member)
                )
                if not group.type:
                    self.groups[group_name] = tuple(
                        f for f in fields if f.name in group.fields
                    )
                result.append(
                    (
                        member,
                        group_name + "?",
                        [f for f in fields if f.name in group.fields],
                        group,
                    )
                )
            else:
                typ = (
                    name(value.native) + pascal(field.name)
                    if field.value.kind == "union"
                    else self.public(field.value)
                )
                if field.presence and field.presence.mask and not typ.endswith("?"):
                    typ += "?"
                result.append((identifier(field.name), typ, [field], None))
        return result

    def flag_name(self, value, flag):
        enum = next(f.value for f in value.fields if f.name == flag.mask)
        prefix = commonprefix([n for n, _ in enum.enum_values]).rsplit("_", 1)[0] + "_"
        return identifier(flag.name.removeprefix(prefix).lower())

    def default(self, value):
        if value.nullable or value.optional:
            return "null"
        if value.kind == "scalar":
            public = self.public(value)
            return {
                "Double": "0.0",
                "Float": "0f",
                "Boolean": "false",
                "ULong": "0uL",
                "UInt": "0u",
                "UShort": "0u",
                "UByte": "0u",
                "Long": "0L",
                "Int": "0",
                "Short": "0",
                "Byte": "0",
            }[public]
        if value.kind == "array":
            return "emptyList()"
        if value.kind == "buffer":
            return '""' if value.encoding == "utf8" else "byteArrayOf()"
        if value.kind == "enum":
            return (
                name(value.native)
                + "("
                + (
                    "0uL"
                    if self.scalar(value)[0] == "ULong"
                    else "0u"
                    if self.scalar(value)[0] == "UInt"
                    else "0"
                )
                + ")"
                if any(n == 0 for _, n in value.enum_values)
                else None
            )
        if value.kind == "record":
            return (
                name(value.native) + "()"
                if all(
                    typ.endswith("?") or self.default(children[0].value)
                    for _, typ, children, group in self.members(value)
                )
                else None
            )
        return None

    def common(self):
        result = [
            "// Generated by tools/bindgen. Do not edit.",
            "package org.maplibre.nativeffi.generated",
            "",
        ]
        for value in self.used.values():
            if value.kind == "callback" or value.registration or value.response:
                continue
            public = name(value.native)
            if value.kind == "enum":
                scalar = self.scalar(value)[0]
                prefix = (
                    commonprefix([n for n, _ in value.enum_values]).rsplit("_", 1)[0]
                    + "_"
                )
                constants = []
                for native, number in value.enum_values:
                    literal = str(number) + (
                        "uL"
                        if scalar == "ULong"
                        else "u"
                        if scalar == "UInt"
                        else "L"
                        if scalar == "Long"
                        else ""
                    )
                    constants.append(
                        f"    public val {native.removeprefix(prefix)}: {public} = {public}({literal})"
                    )
                result.extend(
                    [
                        f"public data class {public}(public val rawValue: {scalar}) {{",
                        *(
                            [
                                f"  public infix fun or(other: {public}): {public} = {public}(rawValue or other.rawValue)",
                                f"  public infix fun and(other: {public}): {public} = {public}(rawValue and other.rawValue)",
                                f"  public operator fun contains(other: {public}): Boolean = (rawValue and other.rawValue) == other.rawValue",
                            ]
                            if value.enum_kind == "bitmask"
                            else []
                        ),
                        "  public companion object {",
                        *constants,
                        "  }",
                        "}",
                    ]
                )
            else:
                for field in self.fields(value):
                    if field.value.kind != "union":
                        continue
                    union_name = name(value.native) + pascal(field.name)
                    variants = [
                        f"  public data class {pascal(v.name)}(public val value: {self.public(v.value)}): {union_name}"
                        for v in field.value.fields
                    ]
                    if field.value.empty_variant:
                        variants.append(f"  public data object None: {union_name}")
                    variants.append(
                        f"  public data class Unknown(public val tag: UInt, public val bytes: ByteArray): {union_name}"
                    )
                    result.append(
                        f"public sealed interface {union_name} {{\n"
                        + "\n".join(variants)
                        + "\n}"
                    )
                args = []
                for member, typ, children, group in self.members(value):
                    default = (
                        "null" if typ.endswith("?") else self.default(children[0].value)
                    )
                    args.append(
                        f"  public val {member}: {typ}"
                        + (" = " + default if default is not None else "")
                    )
                if value.native in self.item_buffers:
                    arena = self.item_buffers[value.native]
                    args.append(
                        f"  public val {identifier(arena.field)}: "
                        + (
                            'String = ""'
                            if arena.encoding == "utf8"
                            else "ByteArray = byteArrayOf()"
                        )
                    )
                args.extend(
                    f"  public val {self.flag_name(value, flag)}: Boolean = false"
                    for flag in value.mask_flags
                )
                if value.native in self.views:
                    members = self.members(value)
                    args = [arg.replace("public val ", "") for arg in args]
                    getters = [
                        f"  private val stored{pascal(member.strip('`'))}: {typ} = {member}\n  public val {member}: {typ} get() {{ bindingScope?.ensureActive(); return stored{pascal(member.strip('`'))} }}"
                        for member, typ, _, _ in members
                    ]
                    result.append(
                        f"public class {public}(\n"
                        + ",\n".join(args)
                        + "\n) {\n  internal var bindingScope: org.maplibre.nativeffi.render.FrameScope? = null\n"
                        + "\n".join(getters)
                        + "\n}"
                    )
                else:
                    result.append(
                        f"public data class {public}(\n" + ",\n".join(args) + "\n)"
                    )
        for public, fields in self.groups.items():
            args = []
            for field in fields:
                default = self.default(field.value)
                args.append(
                    f"  public val {identifier(field.name)}: {self.public(field.value)}"
                    + (f" = {default}" if default is not None else "")
                )
            result.append(f"public data class {public}(\n" + ",\n".join(args) + "\n)")
        for public, (field, value) in self.attachments.items():
            result.append(
                f"public data class {public}(public val {field}: {self.public(value)}, public val ready: kotlinx.coroutines.Deferred<Unit>)"
            )
        for public, fields in self.multiple.items():
            result.append(
                f"public data class {public}(\n"
                + ",\n".join(
                    f"  public val {field}: {self.public(value)}"
                    for field, value in fields
                )
                + "\n)"
            )
        return "\n".join(result) + "\n" + kotlin_callbacks.common(self)

    def cast_native(self, value, expression, platform):
        callback = kotlin_callbacks.cast_native(value, expression, self, platform)
        if callback is not None:
            return callback
        if value.kind == "reference":
            raw = self.cast_native(
                value.element, expression + ("!!" if value.nullable else ""), platform
            )
            absent = "MemorySegment.NULL" if platform == "jvmMain" else "null"
            return (
                f"if ({expression} == null) {absent} else {raw}"
                if value.nullable
                else raw
            )
        if value.kind == "native_pointer":
            raw = expression + ".address"
            if value.ctype.kind == "pointer" or "*" in value.ctype.canonical:
                return (
                    f"MemorySegment.ofAddress({raw})"
                    if platform == "jvmMain"
                    else f"({raw}).toCPointer<ByteVar>()"
                    if platform == "nativeMain"
                    else f"org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer({raw})"
                )
            return raw + (".toULong()" if platform == "nativeMain" else "")
        if value.kind == "buffer" and value.nullable and value.buffer_form == "view":
            return f"GeneratedValues.optional{'String' if value.encoding == 'utf8' else 'Bytes'}View(arena, {expression})"
        if value.kind == "array":
            return f"GeneratedValues.write{name(value.element.native)}Array(arena, {expression})"
        if value.kind == "buffer" and value.optional:
            expression = (
                f"({expression} ?: "
                + ('""' if value.encoding == "utf8" else "byteArrayOf()")
                + ")"
            )
        if value.kind == "enum":
            return self.cast_native(
                replace(
                    value,
                    kind="scalar",
                    native=(value.enum_underlying or value.ctype).spelling,
                    ctype=value.enum_underlying or value.ctype,
                ),
                expression + ".rawValue",
                platform,
            )
        if (
            value.kind == "scalar"
            and (value.scalar_carrier or value.ctype.spelling) == "size_t"
            and platform == "nativeMain"
        ):
            return expression + ".convert<size_t>()"
        if value.kind == "scalar":
            typ, _layout = self.scalar(value)
            return (
                expression
                if platform == "nativeMain" or not typ.startswith("U")
                else expression + ".to" + typ[1:] + "()"
            )
        if value.kind == "record":
            return (
                f"GeneratedValues.write{name(value.native)}(arena, {expression}"
                + (", registrations" if self.needs_registration(value) else "")
                + ")"
            )
        if value.kind == "buffer" and value.buffer_form != "view" and value.nullable:
            present = self.cast_native(
                replace(value, nullable=False), expression + "!!", platform
            )
            null = "MemorySegment.NULL" if platform == "jvmMain" else "null"
            return f"if ({expression} == null) {null} else {present}"
        if value.kind == "buffer" and value.length == "nul":
            return f"GeneratedValues.cString(arena, {expression})"
        if value.kind == "buffer" and value.buffer_form != "view":
            data = (
                f"{expression}.encodeToByteArray()"
                if value.encoding == "utf8"
                else expression
            )
            pointer = f"GeneratedValues.rawBytes(arena, {data})"
            if (
                platform == "nativeMain"
                and value.ctype.pointee
                and value.ctype.pointee.spelling not in {"void", "char", "const char"}
            ):
                pointer += ".reinterpret()"
            return pointer
        if value.kind == "buffer":
            return f"GeneratedValues.{'stringView' if value.encoding == 'utf8' else 'byteView'}(arena, {expression})"
        raise Unsupported("input storage requires another Kotlin rule")

    def cast_public(self, value, expression, platform, scope=None):
        callback = kotlin_callbacks.cast_public(
            value, expression, self, platform, scope
        )
        if callback is not None:
            return callback
        if value.kind == "reference":
            child = value.element
            pointer = expression + (
                f".reinterpret({child.native}.sizeof())"
                if platform == "jvmMain"
                else "!!.pointed"
                if platform == "nativeMain"
                else ""
            )
            raw = self.cast_public(child, pointer, platform, scope)
            absent = expression + (
                ".address() == 0L"
                if platform == "jvmMain"
                else " == null"
                if platform == "nativeMain"
                else f" == null || {expression}.isNull"
            )
            return f"if ({absent}) null else {raw}" if value.nullable else raw
        if value.kind == "native_pointer":
            raw = expression
            if value.ctype.kind == "pointer" or "*" in value.ctype.canonical:
                raw = (
                    f"({expression}?.rawValue?.toLong() ?: 0L)"
                    if platform == "nativeMain"
                    else expression + ".address()"
                    if platform == "jvmMain"
                    else f"({expression}?.address() ?: 0L)"
                )
            elif platform == "nativeMain":
                raw += ".toLong()"
            return (
                f"if ({scope} != null) org.maplibre.nativeffi.render.NativePointer.scoped({raw}, {scope}) else org.maplibre.nativeffi.render.NativePointer.ofAddress({raw})"
                if scope
                else f"org.maplibre.nativeffi.render.NativePointer.ofAddress({raw})"
            )
        if value.kind == "buffer" and value.length == "nul":
            decoded = expression + (
                ".reinterpret(Long.MAX_VALUE).getString(0)"
                if platform == "jvmMain"
                else "!!.toKString()"
                if platform == "nativeMain"
                else ".string"
            )
            if value.nullable:
                absent = expression + (
                    ".address() == 0L"
                    if platform == "jvmMain"
                    else " == null"
                    if platform == "nativeMain"
                    else f" == null || {expression}.isNull"
                )
                return f"if ({absent}) null else {decoded}"
            return decoded
        if value.kind == "enum":
            raw = self.cast_public(
                replace(
                    value,
                    kind="scalar",
                    native=(value.enum_underlying or value.ctype).spelling,
                    ctype=value.enum_underlying or value.ctype,
                ),
                expression,
                platform,
            )
            return f"{name(value.native)}({raw})"
        if (
            value.kind == "scalar"
            and (value.scalar_carrier or value.ctype.spelling) == "size_t"
            and platform == "nativeMain"
        ):
            return expression + ".toULong()"
        if value.kind == "scalar":
            typ = self.scalar(value)[0]
            return (
                expression
                if platform == "nativeMain" or not typ.startswith("U")
                else expression + ".to" + typ + "()"
            )
        if value.kind == "record":
            return f"GeneratedValues.read{name(value.native)}({expression})"
        if value.kind == "buffer":
            result = f"GeneratedValues.{'readString' if value.encoding == 'utf8' else 'readBytes'}({expression})"
            if value.optional:
                return result + ".takeIf { it.isNotEmpty() }"
            if value.nullable:
                absent = (
                    f"mln_buffer_view.data({expression}).address() == 0L"
                    if platform == "jvmMain"
                    else f"{expression}.data == null"
                    if platform == "nativeMain"
                    else f"{expression}.data() == null || {expression}.data().isNull"
                )
                return f"if ({absent}) null else {result}"
            return result
        raise Unsupported("output storage requires another Kotlin rule")

    def read_buffer(self, value, pointer, count, platform):
        count = count + ".toULong()" if platform == "nativeMain" else count
        result = f"GeneratedValues.readRawBytes({pointer}, {count})"
        if value.encoding == "utf8":
            result += ".decodeToString()"
        if value.optional:
            result += ".takeIf { it.isNotEmpty() }"
        return result

    def read_array(self, value, pointer, count, platform):
        count = count + ".toULong()" if platform == "nativeMain" else count
        return (
            f"GeneratedValues.read{name(value.element.native)}Array({pointer}, {count})"
        )

    def union_arm(self, record, field, variant, base, platform):
        anonymous = field.value.native.startswith("@")
        if platform == "jvmMain":
            union_type = (
                record.native + "." + field.name if anonymous else field.value.native
            )
            return f"{union_type}.{variant.name}({record.native}.{field.name}({base}))"
        if platform == "androidMain":
            return (
                f"{base}.{field.name}_{variant.name}()"
                if anonymous
                else f"{base}.{field.name}().{variant.name}()"
            )
        return f"{base}.{field.name}.{variant.name}"

    def union_write(self, record, field, expression, base, platform):
        public = name(record.native) + pascal(field.name)
        lines = [f"    when (val variant = {expression}) {{"]
        for variant in field.value.fields:
            constant = (
                f"MapLibreNativeC.{variant.presence.variant}()"
                if platform == "jvmMain"
                else f"MaplibreNativeC.{variant.presence.variant}"
                if platform == "androidMain"
                else variant.presence.variant
            )
            target = self.union_arm(record, field, variant, base, platform)
            encoded = self.cast_native(variant.value, "variant.value", platform)
            write = (
                target + f".copyFrom({encoded})"
                if platform == "jvmMain"
                else f"{encoded}.pointed.readValue().place({target}.ptr)"
                if platform == "nativeMain"
                else target[:-2] + f"({encoded})"
            )
            tag = self.assign(record, field.presence.tag, base, constant, platform)
            lines.append(
                f"      is {public}.{pascal(variant.name)} -> {{ {tag}; {write} }}"
            )
        if field.value.empty_variant:
            constant = (
                f"MapLibreNativeC.{field.value.empty_variant[0]}()"
                if platform == "jvmMain"
                else f"MaplibreNativeC.{field.value.empty_variant[0]}"
                if platform == "androidMain"
                else field.value.empty_variant[0]
            )
            lines.append(
                f"      {public}.None -> {{ {self.assign(record, field.presence.tag, base, constant, platform)} }}"
            )
        lines.append(
            f'      is {public}.Unknown -> throw IllegalArgumentException("unknown native union variants cannot be submitted")'
        )
        lines.append("    }")
        return lines

    def union_read(self, record, field, base, platform):
        public = name(record.native) + pascal(field.name)
        tag = self.field(record, field.presence.tag, base, platform)
        arms = []
        for variant in field.value.fields:
            constant = (
                f"MapLibreNativeC.{variant.presence.variant}()"
                if platform == "jvmMain"
                else f"MaplibreNativeC.{variant.presence.variant}"
                if platform == "androidMain"
                else variant.presence.variant
            )
            decoded = self.cast_public(
                variant.value,
                self.union_arm(record, field, variant, base, platform),
                platform,
            )
            arms.append(f"{constant} -> {public}.{pascal(variant.name)}({decoded})")
        if field.value.empty_variant:
            constant = (
                f"MapLibreNativeC.{field.value.empty_variant[0]}()"
                if platform == "jvmMain"
                else f"MaplibreNativeC.{field.value.empty_variant[0]}"
                if platform == "androidMain"
                else field.value.empty_variant[0]
            )
            arms.append(f"{constant} -> {public}.None")
        union = self.field(record, field.name, base, platform)
        raw = (
            f"{union}.toArray(ValueLayout.JAVA_BYTE)"
            if platform == "jvmMain"
            else f"GeneratedValues.readUnionBytes({union})"
            if platform == "nativeMain"
            else f"ByteArray({union}.sizeof()).also {{ BytePointer({union}).get(it) }}"
        )
        if platform == "androidMain" and field.value.native.startswith("@"):
            raise Unsupported(
                "anonymous union requires a named C typedef for safe JavaCPP copying"
            )
        arms.append(f"else -> {public}.Unknown({tag}.toUInt(), {raw})")
        return f"when ({tag}) {{ " + "; ".join(arms) + " }"

    def field(self, record, path, base, platform):
        for part in path.split("."):
            field = next(f for f in record.fields if f.name == part)
            native_part = native_identifier(part)
            if platform == "jvmMain":
                base = f"{record.native}.{native_part}({base})"
            elif platform == "androidMain":
                base += f".{'_' + part if part in {'position', 'limit', 'capacity', 'address'} else native_part}()"
            else:
                base += f".{native_part}"
            record = field.value
        return base

    def assign(self, record, path, base, expression, platform):
        parts = path.split(".")
        for part in parts[:-1]:
            field = next(f for f in record.fields if f.name == part)
            base = self.field(record, part, base, platform)
            record = field.value
        field = next(f for f in record.fields if f.name == parts[-1])
        member = parts[-1]
        android_member = (
            "_" + member
            if member in {"position", "limit", "capacity", "address"}
            else native_identifier(member)
        )
        native_member = native_identifier(member)
        if (
            platform == "nativeMain"
            and (field.value.scalar_carrier or field.value.ctype.spelling) == "size_t"
        ):
            expression = f"({expression}).convert()"
        if field.value.kind == "record" or field.value.buffer_form == "view":
            target = self.field(record, member, base, platform)
            return (
                f"{target}.copyFrom({expression})"
                if platform == "jvmMain"
                else f"{expression}.pointed.readValue().place({target}.ptr)"
                if platform == "nativeMain"
                else f"{base}.{android_member}({expression})"
            )
        return (
            f"{record.native}.{native_member}({base}, {expression})"
            if platform == "jvmMain"
            else f"{base}.{android_member}({expression})"
            if platform == "androidMain"
            else f"{base}.{native_member} = {expression}"
        )

    def condition(self, record, presence, base, platform):
        mask = self.field(record, presence.mask, base, platform)
        if not presence.bit:
            return mask
        bit = (
            f"MapLibreNativeC.{presence.bit}()"
            if platform == "jvmMain"
            else f"MaplibreNativeC.{presence.bit}"
            if platform == "androidMain"
            else presence.bit
        )
        return f"({mask} and {bit}) != " + ("0u" if platform == "nativeMain" else "0")

    def mark(self, record, presence, base, platform):
        mask = self.field(record, presence.mask, base, platform)
        bit = (
            f"MapLibreNativeC.{presence.bit}()"
            if platform == "jvmMain"
            else f"MaplibreNativeC.{presence.bit}"
            if platform == "androidMain"
            else presence.bit
        )
        return self.assign(
            record,
            presence.mask,
            base,
            f"{mask} or {bit}" if presence.bit else "true",
            platform,
        )

    def conversions(self, platform):
        imports = {
            "jvmMain": [
                "java.lang.foreign.*",
                "org.maplibre.nativeffi.internal.c.*",
                "org.maplibre.nativeffi.internal.loader.NativeAccess",
            ],
            "nativeMain": [
                "kotlinx.cinterop.*",
                "platform.posix.size_t",
                "platform.posix.size_tVar",
                "org.maplibre.nativeffi.internal.c.*",
            ],
            "androidMain": [
                "org.bytedeco.javacpp.*",
                "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC",
            ],
        }[platform]
        imports.append("org.maplibre.nativeffi.internal.callback.*")
        imports.append("org.maplibre.nativeffi.internal.status.Status as BindingStatus")
        result = [
            "// Generated by tools/bindgen. Do not edit.",
            *(
                ['@file:kotlin.jvm.JvmName("GeneratedPlatformValues")']
                if platform in {"jvmMain", "androidMain"}
                else []
            ),
            "package org.maplibre.nativeffi.generated",
            *(f"import {i}" for i in imports),
            "",
        ]
        if platform == "nativeMain":
            result.append("@OptIn(ExperimentalForeignApi::class)")
        result.append("internal object GeneratedValues {")
        arena_type = {
            "jvmMain": "Arena",
            "nativeMain": "MemScope",
            "androidMain": "PointerScope",
        }[platform]
        for value in self.used.values():
            if value.kind != "record" or value.registration or value.response:
                continue
            public = name(value.native)
            native = value.native
            input_type = (
                "MemorySegment"
                if platform == "jvmMain"
                else native
                if platform == "nativeMain"
                else "MaplibreNativeC." + native
            )
            output_type = (
                f"CPointer<{native}>" if platform == "nativeMain" else input_type
            )
            if platform == "jvmMain":
                init = (
                    f"MapLibreNativeC.{value.default}(arena)"
                    if value.default
                    else f"{native}.allocate(arena)"
                )
            elif platform == "androidMain":
                init = (
                    f"MaplibreNativeC.{value.default}()"
                    if value.default
                    else f"MaplibreNativeC.{native}()"
                )
            else:
                init = f"arena.alloc<{native}>().ptr"
            write = [
                f"  fun write{public}(arena: {arena_type}, value: {public}"
                + (
                    ", registrations: org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope"
                    if self.needs_registration(value)
                    else ""
                )
                + f"): {output_type} {{",
                f"    val result = {init}",
            ]
            base = "result.pointed" if platform == "nativeMain" else "result"
            if platform == "nativeMain" and value.default:
                write.append(f"    {value.default}().place(result)")
            if not value.default and platform == "nativeMain":
                write.append(
                    f"    result.reinterpret<ByteVar>().let {{ bytes -> repeat(sizeOf<{native}>().toInt()) {{ bytes[it] = 0 }} }}"
                )
            if not value.default:
                for field in value.fields:
                    if field.role == "size":
                        size = (
                            f"{native}.sizeof().toInt()"
                            if platform == "jvmMain"
                            else f"sizeOf<{native}>().toUInt()"
                            if platform == "nativeMain"
                            else "result.sizeof()"
                        )
                        write.append(
                            "    "
                            + self.assign(value, field.name, base, size, platform)
                        )
            read = []
            for field in value.fields:
                if field.role == "presence_mask":
                    scalar = replace(
                        field.value,
                        kind="scalar",
                        ctype=field.value.enum_underlying or field.value.ctype,
                        nullable=False,
                        optional=None,
                    )
                    zero = self.cast_native(scalar, self.default(scalar), platform)
                    write.append(
                        "    " + self.assign(value, field.name, base, zero, platform)
                    )
            for flag in value.mask_flags:
                member = self.flag_name(value, flag)
                from ..semantic import PresenceGroup

                presence = PresenceGroup(flag.mask, flag.name, ())
                write.append(
                    f"    if (value.{member}) {{ {self.mark(value, presence, base, platform)} }}"
                )
                read.append(
                    f"      {member} = {self.condition(value, presence, 'source', platform)}"
                )
            for member, typ, children, group in self.members(value):
                if children[0].value.kind == "union":
                    field = children[0]
                    write += self.union_write(
                        value, field, f"value.{member}", base, platform
                    )
                    read.append(
                        f"      {member} = {self.union_read(value, field, 'source', platform)}"
                    )
                    continue
                presence = group or children[0].presence
                optional = bool(presence and presence.mask)
                expression = f"value.{member}" + ("!!" if optional else "")
                if optional:
                    write.append(f"    if (value.{member} != null) {{")
                    write.append("      " + self.mark(value, presence, base, platform))
                copied = []
                for field in children:
                    child_expression = expression + (
                        "." + identifier(field.name) if group else ""
                    )
                    write.append(
                        "    "
                        + self.assign(
                            value,
                            field.name,
                            base,
                            self.cast_native(field.value, child_expression, platform),
                            platform,
                        )
                    )
                    if field.value.kind == "array" or (
                        field.value.kind == "buffer"
                        and field.value.length not in {None, "nul", "1"}
                    ):
                        count_field = next(
                            f for f in value.fields if f.name == field.value.length
                        )
                        length_expression = child_expression + (
                            ".encodeToByteArray()"
                            if field.value.kind == "buffer"
                            and field.value.encoding == "utf8"
                            else ""
                        )
                        count_expression = (
                            length_expression
                            + ".size."
                            + ("toULong()" if platform == "nativeMain" else "toLong()")
                        )
                        write.append(
                            "    "
                            + self.assign(
                                value,
                                count_field.name,
                                base,
                                count_expression,
                                platform,
                            )
                        )
                        if field.value.stride:
                            from .kotlin_arrays import read_strided

                            decoded_field = read_strided(value, field, self, platform)
                        else:
                            decoded_field = (
                                self.read_array
                                if field.value.kind == "array"
                                else self.read_buffer
                            )(
                                field.value,
                                self.field(value, field.name, "source", platform),
                                self.field(
                                    value, field.value.length, "source", platform
                                ),
                                platform,
                            )
                    else:
                        decoded_field = self.cast_public(
                            field.value,
                            self.field(value, field.name, "source", platform),
                            platform,
                            "scope" if value.native in self.views else None,
                        )
                    copied.append(
                        (identifier(field.name) + " = " if group else "")
                        + decoded_field
                    )
                if optional:
                    write.append("    }")
                decoded = (
                    f"{typ.removesuffix('?')}(" + ", ".join(copied) + ")"
                    if group
                    else copied[0]
                )
                if optional:
                    decoded = f"if ({self.condition(value, presence, 'source', platform)}) {decoded} else null"
                read.append(f"      {member} = {decoded}")
            write += ["    return result", "  }"]
            scope_parameter = (
                ", scope: org.maplibre.nativeffi.render.FrameScope? = null"
                if value.native in self.views
                else ""
            )
            if value.native in self.item_buffers:
                item = self.item_buffers[value.native]
                scope_parameter += ", message: " + (
                    'String = ""'
                    if item.encoding == "utf8"
                    else "ByteArray = byteArrayOf()"
                )
                read.append(f"      {identifier(item.field)} = message")
            result += write + [
                f"  fun read{public}(source: {input_type}{scope_parameter}): {public} = {public}(",
                ",\n".join(read),
                "  )"
                + (
                    ".also { it.bindingScope = scope }"
                    if value.native in self.views
                    else ""
                ),
            ]
        for native, element in self.arrays.items():
            public = self.public(element)
            suffix = name(native) + "Array"
            encoded = self.cast_native(element, "item", platform)
            if platform == "jvmMain":
                decoded = self.cast_public(
                    element,
                    f"source.reinterpret(count * {native}.sizeof()).asSlice(index.toLong() * {native}.sizeof(), {native}.sizeof())",
                    platform,
                )
                result += [
                    f"  fun write{suffix}(arena: Arena, value: List<{public}>): MemorySegment {{ val result = arena.allocate({native}.layout(), maxOf(1, value.size).toLong()); value.forEachIndexed {{ index, item -> result.asSlice(index.toLong() * {native}.sizeof(), {native}.sizeof()).copyFrom({encoded}) }}; return result }}",
                    f"  fun read{suffix}(source: MemorySegment, count: Long): List<{public}> {{ require(count in 0..Int.MAX_VALUE.toLong()); return List(count.toInt()) {{ index -> {decoded} }} }}",
                ]
            elif platform == "nativeMain":
                decoded = self.cast_public(element, "source!![index]", platform)
                result += [
                    f"  fun write{suffix}(arena: MemScope, value: List<{public}>): CPointer<{native}> {{ val result = arena.allocArray<{native}>(maxOf(1, value.size)); value.forEachIndexed {{ index, item -> {encoded}.pointed.readValue().place(result[index].ptr) }}; return result }}",
                    f"  fun read{suffix}(source: CPointer<{native}>?, count: ULong): List<{public}> {{ require(count <= Int.MAX_VALUE.toULong()); return List(count.toInt()) {{ index -> {decoded} }} }}",
                ]
            else:
                decoded = self.cast_public(
                    element, "data.position(index.toLong())", platform
                )
                result += [
                    f"  fun write{suffix}(arena: PointerScope, value: List<{public}>): MaplibreNativeC.{native} {{ val result = MaplibreNativeC.{native}(maxOf(1, value.size).toLong()); value.forEachIndexed {{ index, item -> result.position(index.toLong()).put<MaplibreNativeC.{native}>({encoded}) }}; return result.position(0) }}",
                    f"  fun read{suffix}(source: MaplibreNativeC.{native}?, count: Long): List<{public}> {{ require(count in 0..Int.MAX_VALUE.toLong()); if (count == 0L) return emptyList(); val data = requireNotNull(source); require(!data.isNull); return List(count.toInt()) {{ index -> {decoded} }} }}",
                ]
        if platform == "jvmMain":
            result += [
                "  fun rawBytes(arena: Arena, value: ByteArray): MemorySegment = arena.allocate(maxOf(1, value.size).toLong()).also { if (value.isNotEmpty()) it.copyFrom(MemorySegment.ofArray(value)) }",
                "  fun readRawBytes(source: MemorySegment, count: Long): ByteArray { require(count in 0..Int.MAX_VALUE.toLong()); return if (count == 0L) byteArrayOf() else source.reinterpret(count).toArray(ValueLayout.JAVA_BYTE) }",
                "  fun optionalBytesView(arena: Arena, value: ByteArray?): MemorySegment = if (value == null) mln_buffer_view.allocate(arena) else byteView(arena, value)",
                "  fun optionalStringView(arena: Arena, value: String?): MemorySegment = optionalBytesView(arena, value?.encodeToByteArray())",
                "  fun stringView(arena: Arena, value: String): MemorySegment = byteView(arena, value.encodeToByteArray())",
                "  fun byteView(arena: Arena, value: ByteArray): MemorySegment = mln_buffer_view.allocate(arena).also { mln_buffer_view.data(it, rawBytes(arena, value)); mln_buffer_view.size(it, value.size.toLong()) }",
                "  fun readBytes(source: MemorySegment): ByteArray { val count = mln_buffer_view.size(source); require(count in 0..Int.MAX_VALUE.toLong()); return if (count == 0L) byteArrayOf() else mln_buffer_view.data(source).reinterpret(count).toArray(ValueLayout.JAVA_BYTE) }",
                "  fun readString(source: MemorySegment): String = readBytes(source).decodeToString()",
            ]
        elif platform == "nativeMain":
            result += [
                "  fun rawBytes(arena: MemScope, value: ByteArray): CPointer<ByteVar> = arena.allocArray<ByteVar>(maxOf(1, value.size)).also { result -> value.forEachIndexed { index, byte -> result[index] = byte } }",
                "  inline fun <reified T: CVariable> readUnionBytes(source: T): ByteArray = source.ptr.reinterpret<ByteVar>().readBytes(sizeOf<T>().toInt())",
                "  fun readRawBytes(source: CPointer<*>?, count: ULong): ByteArray { require(count <= Int.MAX_VALUE.toULong()); return if (count == 0uL) byteArrayOf() else source!!.reinterpret<ByteVar>().readBytes(count.toInt()) }",
                "  fun optionalBytesView(arena: MemScope, value: ByteArray?): CPointer<mln_buffer_view> = if (value == null) arena.alloc<mln_buffer_view>().apply { data = null; size = 0u }.ptr else byteView(arena, value)",
                "  fun optionalStringView(arena: MemScope, value: String?): CPointer<mln_buffer_view> = optionalBytesView(arena, value?.encodeToByteArray())",
                "  fun stringView(arena: MemScope, value: String): CPointer<mln_buffer_view> = byteView(arena, value.encodeToByteArray())",
                "  fun byteView(arena: MemScope, value: ByteArray): CPointer<mln_buffer_view> = arena.alloc<mln_buffer_view>().apply { data = rawBytes(arena, value); size = value.size.convert() }.ptr",
                "  fun readBytes(source: mln_buffer_view): ByteArray { require(source.size <= Int.MAX_VALUE.toULong()); return source.data?.reinterpret<ByteVar>()?.readBytes(source.size.toInt()) ?: byteArrayOf() }",
                "  fun readString(source: mln_buffer_view): String = readBytes(source).decodeToString()",
            ]
        else:
            result += [
                "  fun rawBytes(arena: PointerScope, value: ByteArray): BytePointer = BytePointer(maxOf(1, value.size).toLong()).also { if (value.isNotEmpty()) it.put(*value) }",
                "  fun readRawBytes(source: Pointer?, count: Long): ByteArray { require(count in 0..Int.MAX_VALUE.toLong()); return ByteArray(count.toInt()).also { if (it.isNotEmpty()) BytePointer(source).get(it) } }",
                "  fun optionalBytesView(arena: PointerScope, value: ByteArray?): MaplibreNativeC.mln_buffer_view = if (value == null) MaplibreNativeC.mln_buffer_view().data(null as Pointer?).size(0) else byteView(arena, value)",
                "  fun optionalStringView(arena: PointerScope, value: String?): MaplibreNativeC.mln_buffer_view = optionalBytesView(arena, value?.encodeToByteArray())",
                "  fun byteView(arena: PointerScope, value: ByteArray): MaplibreNativeC.mln_buffer_view = MaplibreNativeC.mln_buffer_view().data(rawBytes(arena, value)).size(value.size.toLong())",
                "  fun stringView(arena: PointerScope, value: String): MaplibreNativeC.mln_buffer_view = byteView(arena, value.encodeToByteArray())",
                "  fun readBytes(source: MaplibreNativeC.mln_buffer_view): ByteArray { require(source.size() in 0..Int.MAX_VALUE.toLong()); return ByteArray(source.size().toInt()).also { if (it.isNotEmpty()) BytePointer(source.data()).get(it) } }",
                "  fun readString(source: MaplibreNativeC.mln_buffer_view): String = readBytes(source).decodeToString()",
            ]
        arena_type = (
            "Arena"
            if platform == "jvmMain"
            else "MemScope"
            if platform == "nativeMain"
            else "PointerScope"
        )
        pointer_type = (
            "MemorySegment"
            if platform == "jvmMain"
            else "CPointer<ByteVar>"
            if platform == "nativeMain"
            else "BytePointer"
        )
        result += [
            f"  fun cString(arena: {arena_type}, value: String): {pointer_type} {{ BindingStatus.requireArgument('\\u0000' !in value) {{ \"text contains an embedded NUL\" }}; return rawBytes(arena, value.encodeToByteArray() + byteArrayOf(0)) }}"
        ]
        return "\n".join(result + ["}", ""]) + kotlin_callbacks.conversions(
            self, platform
        )
