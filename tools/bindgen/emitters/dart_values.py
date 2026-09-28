"""Render Dart value ownership and conversions from resolved semantic plans."""

from __future__ import annotations

from dataclasses import replace
from os.path import commonprefix

from .. import native_ports
from ..names import camel, pascal
from ..semantic import BoundApi, ValuePlan

POSITIONAL = {
    "mln_lat_lng",
    "mln_screen_point",
    "mln_vec3",
    "mln_quaternion",
    "mln_image_stretch",
    "mln_unit_bezier",
}

SCALARS = {
    "bool": ("bool", "Bool"),
    "_Bool": ("bool", "Bool"),
    "double": ("double", "Double"),
    "float": ("double", "Float"),
    "uint8_t": ("int", "Uint8"),
    "int8_t": ("int", "Int8"),
    "uint16_t": ("int", "Uint16"),
    "int16_t": ("int", "Int16"),
    "uint32_t": ("int", "Uint32"),
    "int32_t": ("int", "Int32"),
    "uint64_t": ("BigInt", "Uint64"),
    "int64_t": ("int", "Int64"),
    "size_t": ("int", "Size"),
    "ptrdiff_t": ("int", "IntPtr"),
    "intptr_t": ("int", "IntPtr"),
    "uintptr_t": ("int", "UintPtr"),
    "int": ("int", "Int"),
    "unsigned int": ("int", "UnsignedInt"),
    "unsigned long": ("BigInt", "UnsignedLong"),
    "unsigned long long": ("BigInt", "Uint64"),
    "long": ("int", "Long"),
    "long long": ("int", "Int64"),
    "unsigned char": ("int", "Uint8"),
    "signed char": ("int", "Int8"),
    "char": ("int", "Int8"),
    "unsigned short": ("int", "Uint16"),
    "short": ("int", "Int16"),
}


# Hand-written Dart owners: the generated operations mixin each uses, its public
# class, and its native handle type. Every other public handle gets a generated
# owner named after its C type.
HANDWRITTEN_OWNERS = {
    "mln_acquired_frame": ("AcquiredFrame", "AcquiredFrame", "NativeAcquiredFrame"),
    "mln_geojson_source_data": (
        "GeoJsonSourceData",
        "GeoJsonSourceDataHandle",
        "NativeGeoJsonSourceData",
    ),
    "mln_map": ("Map", "MapHandle", "NativeMap"),
    "mln_map_projection": ("Projection", "MapProjectionHandle", "NativeMapProjection"),
    "mln_render_session": (
        "RenderSession",
        "RenderSessionHandle",
        "NativeRenderSession",
    ),
    "mln_resource_request_handle": (
        "ResourceRequest",
        "ResourceRequestHandle",
        "NativeResourceRequest",
    ),
    "mln_runtime": ("Runtime", "RuntimeHandle", "NativeRuntime"),
}


def owner_names(native: str) -> tuple[str, str, str]:
    """The operations mixin, public class, and native type for a handle."""
    if native in HANDWRITTEN_OWNERS:
        return HANDWRITTEN_OWNERS[native]
    name = public_name(native)
    return name, name + "Handle", "Native" + name


def generated_owners(bound) -> list[str]:
    return sorted(bound.public_handles.keys() - HANDWRITTEN_OWNERS.keys())


class Unsupported(ValueError):
    pass


def public_name(native: str) -> str:
    return pascal(native.removeprefix("mln_"))


def identifier(native: str) -> str:
    name = camel(native)
    return (
        name + "Value"
        if name
        in {
            "class",
            "default",
            "switch",
            "operator",
            "in",
            "is",
            "with",
            "return",
            "null",
            "true",
            "false",
        }
        else name
    )


class Values:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        self.used: dict[str, ValuePlan] = {
            name: value
            for name, value in bound.public_values.items()
            if value.kind == "enum"
        }

        self.projections = tuple(
            value for value in bound.values.values() if value.projection
        )
        for value in self.projections:
            self.check(value.projection)

    def scalar(self, value):
        return SCALARS.get(
            value.scalar_carrier or value.native, SCALARS.get(value.ctype.canonical)
        )

    def check(self, value):
        if value.ownership == "owned":
            raise Unsupported(f"{value.native}: owned result needs adoption")
        if value.kind == "handle" and value.native in self.bound.public_handles:
            return
        if value.kind == "native_pointer":
            return
        if value.kind == "scalar" and self.scalar(value):
            return
        if value.kind == "enum":
            self.used[value.native] = value
            return
        if value.kind == "buffer" and value.encoding in {"utf8", "bytes", "json"}:
            return
        if value.kind in {"array", "reference"} and value.element:
            self.check(value.element)
            return
        if value.kind == "callback":
            callback = self.bound.callbacks[value.native]
            if callback.result.native != "void" or not callback.context:
                raise Unsupported("callback requires a synchronous native adapter")
            for parameter in callback.parameters:
                if parameter.name != callback.context:
                    if native_ports.flattened(parameter.value, parameter.name) is None:
                        raise Unsupported(
                            "callback payload needs another native port copy rule"
                        )
                    self.check(parameter.value)
            self.used[value.native] = value
            return
        if value.registration and self.port_callbacks(value):
            for field in self.fields(value):
                self.check(field.value)
            self.used[value.native] = value
            return
        if value.registration:
            adapters = self.registration_adapters(value)
            if not adapters:
                raise Unsupported(
                    f"{value.native}: registration requires a supported native adapter"
                )
            for adapter in adapters:
                self.check(self.bound.values[adapter.context])
            self.used[value.native] = value
            return
        if value.kind != "record":
            raise Unsupported(
                f"{value.native}: {value.kind} requires another Dart value rule"
            )
        unions = [f for f in self.fields(value) if f.value.kind == "union"]
        if unions:
            if len(unions) != 1 or not unions[0].presence or not unions[0].presence.tag:
                raise Unsupported(
                    f"{value.native}: union requires one explicit discriminator"
                )
            for member in unions[0].value.fields:
                if not member.presence or not member.presence.variant:
                    raise Unsupported(
                        f"{value.native}: union arm requires explicit variant"
                    )
                self.check(member.value)
            for field in self.fields(value):
                if field.value.kind != "union":
                    self.check(field.value)
        else:
            for field in self.fields(value):
                self.check(field.value)
        for group in value.presence_groups:
            if len(group.fields) > 1 and group.type:
                self.check(self.bound.values[group.type])
        self.used[value.native] = value

    def port_callbacks(self, value):
        if not value.registration:
            return []
        entries = [
            entry
            for entry in native_ports.callbacks(self.bound)
            if entry[0].native == value.native
        ]
        return entries if len(entries) == len(value.registration.callbacks) else []

    def registration_adapters(self, value):
        callbacks = {
            f.value.native
            for f in value.fields
            if f.name in value.registration.callbacks
        }
        result = []
        for adapter in self.bound.callback_adapters:
            if adapter.callback in callbacks:
                try:
                    self.check(self.bound.values[adapter.context])
                    result.append(adapter)
                except Unsupported:
                    pass
        return result

    @staticmethod
    def fields(value):
        controls = {
            f.name
            for f in value.fields
            if f.role
            in {"size", "reserved", "presence_mask", "count", "tag", "stride", "arena"}
        }
        controls |= {
            f.presence.mask for f in value.fields if f.presence and f.presence.mask
        }
        if value.registration:
            controls |= {value.registration.user_data, value.registration.release}
        controls |= {f.value.length for f in value.fields if f.value.kind == "array"}
        return tuple(f for f in value.fields if f.name not in controls)

    def public(self, value):
        self.check(value)
        if value.kind == "handle":
            name = owner_names(value.native)[1]
        elif value.kind == "native_pointer":
            name = "NativePointer"
        elif value.kind in {"scalar", "enum"}:
            name = (
                public_name(value.native)
                if value.kind == "enum"
                else self.scalar(value)[0]
            )
        elif value.kind in {"record", "callback"}:
            name = public_name(value.native)
        elif value.kind == "reference":
            name = self.public(replace(value.element, nullable=False))
        elif value.kind == "array":
            name = f"List<{self.public(value.element)}>"
        else:
            name = "String" if value.encoding == "utf8" else "Uint8List"
        return name + "?" if value.nullable or value.optional else name

    def ffi(self, value):
        if value.kind == "native_pointer":
            return "Pointer<Void>"
        if value.kind == "enum":
            scalar = self.scalar(value)
            if not scalar:
                raise Unsupported(f"{value.native}: unsupported enum carrier")
            return scalar[1]
        if value.kind == "scalar":
            return self.scalar(value)[1]
        if value.kind == "record" or value.buffer_form == "view":
            return f"raw.{value.native}"
        if value.kind in {"array", "reference"}:
            return self.ffi(value.element)
        raise Unsupported(f"{value.native}: no native storage type")

    def has_registrations(self, value):
        return bool(
            value.registration
            or (value.element and self.has_registrations(value.element))
            or any(
                self.has_registrations(f.value)
                for f in value.fields
                if f.value.kind != "callback"
            )
        )

    def native(self, value, expression):
        if value.registration:
            return f"registrations.prepare{public_name(value.native)}({expression}).ref"
        if value.kind == "handle":
            return f"{expression}._handle.raw"
        if value.kind == "native_pointer":
            return f"Pointer<Void>.fromAddress({expression}.address).cast()"
        if value.kind == "reference":
            child = value.element
            suffix = (
                "ref"
                if child.kind == "record" or child.buffer_form == "view"
                else "value"
            )
            local = expression + ("!" if value.nullable and "." in expression else "")
            if child.kind == "record":
                code = (
                    f"_write{public_name(child.native)}({local}, arena"
                    + (", registrations" if self.has_registrations(child) else "")
                    + ")"
                )
                if child.registration:
                    code = f"registrations.prepare{public_name(child.native)}({local})"
                return (
                    f"{expression} == null ? nullptr : {code}"
                    if value.nullable
                    else code
                )
            assignment = self.native(child, local)
            code = f"(() {{ final storage = arena<{self.ffi(child)}>(); storage.{suffix} = {assignment}; return storage; }})()"
            return (
                f"{expression} == null ? nullptr : {code}" if value.nullable else code
            )
        if value.nullable or value.optional:
            bare = replace(value, nullable=False, optional=None)
            if (
                value.kind == "buffer"
                and value.buffer_form != "view"
                and value.nullable
            ):
                local = expression + ("!" if "." in expression else "")
                return f"{expression} == null ? nullptr : {self.native(bare, local)}"
            if value.kind == "buffer" and value.nullable:
                local = expression + ("!" if "." in expression else "")
                return f"{expression} == null ? arena<raw.mln_buffer_view>().ref : {self.native(bare, local)}"
            if value.kind == "buffer":
                empty = "''" if value.encoding == "utf8" else "Uint8List(0)"
                return self.native(bare, f"({expression} ?? {empty})")
            raise Unsupported(
                f"{value.native}: optional value requires enclosing storage"
            )
        if value.kind == "scalar":
            public, ffi = self.scalar(value)
            if public == "BigInt":
                bits = f"uint64ToNative({expression}, '{value.native}')"
                return (
                    f"sizeOf<UnsignedLong>() == 4 ? _generatedInteger({bits}, 0, 4294967295) : {bits}"
                    if ffi == "UnsignedLong"
                    else bits
                )
            widths = {
                "Uint8": (0, 255),
                "Int8": (-128, 127),
                "Uint16": (0, 65535),
                "Int16": (-32768, 32767),
                "Uint32": (0, 4294967295),
                "UnsignedInt": (0, 4294967295),
                "Int32": (-2147483648, 2147483647),
                "Int": (-2147483648, 2147483647),
            }
            if ffi in widths:
                lo, hi = widths[ffi]
                return f"_generatedInteger({expression}, {lo}, {hi})"
            if ffi in {"Size", "UintPtr"}:
                return f"_generatedInteger({expression}, 0, sizeOf<{ffi}>() == 4 ? 4294967295 : 0x7fffffffffffffff)"
            if ffi in {"Long", "IntPtr"}:
                return f"sizeOf<{ffi}>() == 4 ? _generatedInteger({expression}, -2147483648, 2147483647) : {expression}"
            return expression
        if value.kind == "enum":
            return f"{expression}.rawValue"
        if value.kind == "record":
            return (
                f"_write{public_name(value.native)}({expression}, arena"
                + (", registrations" if self.has_registrations(value) else "")
                + ").ref"
            )
        if value.kind == "buffer":
            if value.buffer_form == "view":
                return (
                    f"nativeStringView({expression}, arena).value"
                    if value.encoding == "utf8"
                    else f"nativeBufferView({expression}, arena)"
                )
            if value.length == "nul":
                return f"nativeUtf8CString({expression}, arena).pointer.cast<Char>()"
            view = (
                f"nativeStringView({expression}, arena).value"
                if value.encoding == "utf8"
                else f"nativeBufferView({expression}, arena)"
            )
            return f"{view}.data.cast()"
        raise Unsupported(
            f"{value.native}: native materialization needs a storage rule"
        )

    def copy(self, value, expression, count=None):
        if value.registration:
            return f"_read{public_name(value.native)}({expression})"
        if value.kind == "native_pointer":
            result = f"NativePointer({expression}.address)"
        elif value.kind == "scalar":
            result = (
                f"uint64FromNative({expression})"
                if self.scalar(value)[0] == "BigInt"
                else expression
            )
        elif value.kind == "enum":
            result = f"{public_name(value.native)}.fromRawValue({expression})"
        elif value.kind == "record":
            result = f"_read{public_name(value.native)}({expression})"
        elif value.kind == "buffer":
            if value.buffer_form == "view":
                result = f"_copyBufferView({expression})"
                if value.encoding == "utf8":
                    result = f"utf8.decode({result})"
                if value.optional:
                    result = f"{expression}.size == 0 ? null : {result}"
                elif value.nullable:
                    result = f"{expression}.data == nullptr ? null : {result}"
            else:
                if value.length == "nul":
                    result = f"{expression}.cast<Utf8>().toDartString()"
                else:
                    result = f"Uint8List.fromList({expression}.cast<Uint8>().asTypedList({count}))"
                    if value.encoding == "utf8":
                        result = f"utf8.decode({result})"
                if value.optional or value.nullable:
                    absent = (
                        f"{count} == 0"
                        if value.optional == "empty" and value.length != "nul"
                        else f"{expression} == nullptr"
                    )
                    result = f"{absent} ? null : {result}"
        elif value.kind == "array":
            item_expression = f"{expression}[index]"
            if value.stride:
                parent = expression.rsplit(".", 1)[0]
                pointer = f"({expression}.cast<Uint8>() + index * {parent}.{value.stride}).cast<{self.ffi(value.element)}>()"
                item_expression = pointer + ".ref"
                extra = ""
                if value.item_buffer:
                    item_buffer = value.item_buffer
                    extra = f", {item_buffer.field}: _generatedArenaUtf8({parent}.{item_buffer.data}.cast(), {parent}.{item_buffer.size}, {item_expression}.{item_buffer.offset}, {item_expression}.{item_buffer.length})"
                item = f"_read{public_name(value.element.native)}({item_expression}, rawRecord: () => {pointer}.cast<Uint8>().asTypedList({parent}.{value.stride}){extra})"
            else:
                item = self.copy(value.element, item_expression)
            result = f"List<{self.public(value.element)}>.unmodifiable(List.generate({count}, (index) => {item}))"
            if value.stride:
                result = f"(() {{ if ({parent}.{value.stride} < sizeOf<{self.ffi(value.element)}>()) {{ throwInvalidState('native record stride is too small'); }} return {result}; }})()"
        elif value.kind == "reference":
            suffix = (
                "ref"
                if value.element.kind == "record" or value.element.buffer_form == "view"
                else "value"
            )
            result = self.copy(value.element, f"{expression}.{suffix}")
            if value.nullable:
                result = f"{expression} == nullptr ? null : {result}"
        else:
            raise Unsupported(f"{value.native}: result capture needs a conversion")
        return result

    def members(self, value):
        fields = self.fields(value)
        grouped = {
            name: group
            for group in value.presence_groups
            if len(group.fields) > 1
            for name in group.fields
        }
        emitted = set()
        result = []
        for field in fields:
            group = grouped.get(field.name)
            if group:
                if group.mask in emitted:
                    continue
                emitted.add(group.mask)
                if group.bit:
                    enum = next(
                        e
                        for e in self.bound.source.enums
                        if any(v.name == group.bit for v in e.values)
                    )
                    prefix = (
                        commonprefix([v.name for v in enum.values]).rsplit("_", 1)[0]
                        + "_"
                    )
                    name = identifier(group.bit.removeprefix(prefix).lower())
                else:
                    name = identifier(group.mask.removeprefix("has_"))
                children = [f for f in fields if f.name in group.fields]
                typ = (
                    public_name(group.type)
                    if group.type
                    else "({"
                    + ", ".join(
                        f"{self.public(f.value)} {identifier(f.name)}" for f in children
                    )
                    + "})"
                )
                result.append((name, typ + "?", children, group))
            else:
                typ = self.public(field.value)
                if field.presence and field.presence.mask and not typ.endswith("?"):
                    typ += "?"
                result.append((identifier(field.name), typ, [field], None))
        return result

    def enum_constant(self, bit):
        enum = next(
            e for e in self.bound.source.enums if any(v.name == bit for v in e.values)
        )
        return f"raw.{enum.name}.{bit}"

    def present(self, mask, bit):
        return (
            f"(source.{mask} & {self.enum_constant(bit)}) != 0"
            if bit
            else f"source.{mask}"
        )

    def set_present(self, mask, bit):
        return (
            f"result.ref.{mask} |= {self.enum_constant(bit)};"
            if bit
            else f"result.ref.{mask} = true;"
        )

    def default_expression(self, value):
        if value.nullable or value.optional:
            return "null"
        if value.kind == "native_pointer":
            return "NativePointer.nullPointer"
        if value.kind == "scalar":
            return {"double": "0", "int": "0", "bool": "false"}.get(self.public(value))
        if value.kind == "enum":
            return (
                f"const {public_name(value.native)}.fromRawValue(0)"
                if value.enum_kind == "bitmask"
                or any(number == 0 for _, number in value.enum_values)
                else None
            )
        if value.kind == "record":
            if any(f.value.kind == "union" for f in value.fields):
                return None
            args = []
            for name, typ, children, group in self.members(value):
                if typ.endswith("?"):
                    continue
                if group:
                    return None
                default = self.default_expression(children[0].value)
                if default is None:
                    return None
                if value.native in POSITIONAL:
                    args.append(default)
            return f"const {public_name(value.native)}({', '.join(args)})"
        return None

    def port_copy(self, value, offset):
        if value.kind == "record":
            args = []
            for field in self.fields(value):
                expression, offset = self.port_copy(field.value, offset)
                args.append(
                    (
                        ""
                        if value.native in POSITIONAL
                        else identifier(field.name) + ": "
                    )
                    + expression
                )
            return public_name(value.native) + "(" + ", ".join(args) + ")", offset
        expression = f"message[{offset}] as int"
        if value.kind == "enum":
            expression = f"{public_name(value.native)}.fromRawValue({expression})"
        elif self.scalar(value)[0] == "BigInt":
            expression = f"uint64FromNative({expression})"
        elif self.scalar(value)[0] == "bool":
            expression = f"message[{offset}] != 0"
        return expression, offset + 1

    def render_port_registration(self, value):
        public = public_name(value.native)
        fields, args, writes, handlers = [], [], [], []
        ports = {
            field.name: (callback, payload)
            for _, field, callback, payload in self.port_callbacks(value)
        }
        for name, typ, children, group in self.members(value):
            if group or len(children) != 1:
                raise Unsupported(
                    "port descriptor field grouping needs recursive preparation"
                )
            field = children[0]
            fields.append(f"  final {typ} {name};")
            default = self.default_expression(field.value)
            args.append(
                f"this.{name}"
                if typ.endswith("?")
                else f"this.{name} = {default}"
                if default
                else f"required this.{name}"
            )
            if field.name in ports:
                callback, _ = ports[field.name]
                key = f"(raw.mln_adapter_dart_port_callback.{native_ports.constant(value.native, field.name)} & 0xffffffff)"
                decoded, offset = [], 1
                for parameter in callback.parameters:
                    if parameter.name == callback.context:
                        continue
                    expression, offset = self.port_copy(parameter.value, offset)
                    decoded.append(expression)
                invoke = f"value.{name}{'!' if field.value.nullable else ''}({', '.join(decoded)})"
                handlers.append(
                    f"      {'if (value.' + name + ' != null) ' if field.value.nullable else ''}{key}: (message) => {invoke},"
                )
                pointer = f"raw.mln_adapter_dart_port_function({key}).cast()"
                writes.append(
                    f"    result.ref.{field.name} = "
                    + (
                        f"value.{name} == null ? nullptr : "
                        if field.value.nullable
                        else ""
                    )
                    + pointer
                    + ";"
                )
            else:
                expression = "value." + name
                if field.presence and field.presence.mask:
                    writes.append(
                        f"    if ({expression} != null) {{ {self.set_present(field.presence.mask, field.presence.bit)} result.ref.{field.name} = {self.native(field.value, expression + '!')}; }}"
                    )
                else:
                    writes.append(
                        f"    result.ref.{field.name} = {self.native(field.value, expression)};"
                    )
        disabled = (
            " && ".join(f"value.{identifier(name)} == null" for name in ports)
            if all(
                next(f for f in value.fields if f.name == name).value.nullable
                for name in ports
            )
            else None
        )
        disabled_code = (
            f"if ({disabled}) {{ return _NativeRegistration(result, () {{}}, arena.releaseAll); }}"
            if disabled
            else ""
        )
        read_args = ", ".join(
            f"{name}: null"
            if children[0].name in ports
            else f"{name}: {self.copy(children[0].value, 'source.' + children[0].name)}"
            for name, _, children, _ in self.members(value)
        )
        reader = (
            f"{public} _read{public}(raw.{value.native} source) {{\n"
            + "\n".join(
                f"  if (source.{name} != nullptr) {{ throwInvalidState('cannot copy a registered native callback'); }}"
                for name in ports
            )
            + f"\n  return {public}({read_args});\n}}\n"
            if disabled
            else ""
        )
        declaration = (
            f"final class {public} {{\n  const {public}({{{', '.join(args)}}});\n"
            + "\n".join(fields)
            + "\n}"
        )
        initialize = (
            f"result.ref = raw.{value.default}();"
            if value.default
            else f"result.ref.size = sizeOf<raw.{value.native}>();"
        )
        conversion = (
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value, _NativeCallbackPorts roots) {{\n  final arena = Arena();\n  _NativeCallbackPort? port;\n  try {{\n    final result = arena<raw.{value.native}>();\n    {initialize}\n    {disabled_code}\n    port = roots.register({{\n"
            + "\n".join(handlers)
            + "\n    });\n"
            + "\n".join(writes)
            + f"\n    result.ref.{value.registration.user_data} = port.context;\n    result.ref.{value.registration.release} = Native.addressOf<NativeFunction<raw.mln_runtime_callback_releaseFunction>>(raw.mln_adapter_dart_port_release).cast();\n    return _NativeRegistration(result, port.reject, arena.releaseAll);\n  }} catch (_) {{ port?.reject(); arena.releaseAll(); rethrow; }}\n}}\n"
        )
        return declaration, conversion + reader

    def render_registration(self, value):
        public = public_name(value.native)
        declarations = [
            f"sealed class {public} {{",
            f"  const {public}._();",
            f"  const factory {public}.empty() = {public}Empty;",
        ]
        children, cases = (
            [
                f"final class {public}Empty extends {public} {{ const {public}Empty() : super._(); }}"
            ],
            [
                f"    case {public}Empty():\n      final descriptor = arena<raw.{value.native}>();\n      descriptor.ref.size = sizeOf<raw.{value.native}>();\n      return _NativeRegistration(descriptor, arena.releaseAll, arena.releaseAll);"
            ],
        )
        for adapter in self.registration_adapters(value):
            name = identifier(adapter.context.removeprefix("mln_adapter_"))
            variant = public + pascal(name)
            context = public_name(adapter.context)
            callback_field = next(
                f.name for f in value.fields if f.value.native == adapter.callback
            )
            declarations.append(
                f"  const factory {public}.{name}({context} value) = {variant};"
            )
            children.append(
                f"final class {variant} extends {public} {{\n  const {variant}(this.value) : super._();\n  final {context} value;\n}}"
            )
            cases.append(
                f"    case {variant}():\n      final context = _write{context}(value.value, arena);\n      final descriptor = arena<raw.{value.native}>();\n      descriptor.ref.size = sizeOf<raw.{value.native}>();\n      descriptor.ref.{callback_field} = Native.addressOf<NativeFunction<raw.{adapter.callback}Function>>(raw.{adapter.function});\n      descriptor.ref.{value.registration.user_data} = context.cast();\n      descriptor.ref.{value.registration.release} = Native.addressOf<NativeFunction<raw.mln_runtime_callback_releaseFunction>>(raw.mln_adapter_dart_release);\n      transferred = true;\n      roots.register(context.cast(), arena.releaseAll, arena: arena);\n      return _NativeRegistration(descriptor, () => roots.reject(context.cast()));"
            )
        declarations.append("}")
        declarations.extend(children)
        conversion = (
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value, NativeCallbackReleases roots) {{\n  final arena = NativeOwnedArena();\n  var transferred = false;\n  try {{\n    switch(value) {{\n"
            + "\n".join(cases)
            + "\n    }\n  } catch (_) { if (!transferred) { arena.releaseAll(); } rethrow; }\n}\n"
        )
        reader = (
            f"{public} _read{public}(raw.{value.native} source) {{\n"
            + "\n".join(
                f"  if (source.{name} != nullptr) {{ throwInvalidState('cannot copy a registered native callback'); }}"
                for name in value.registration.callbacks
            )
            + f"\n  return const {public}.empty();\n}}\n"
        )
        return "\n".join(declarations), conversion + reader

    def render_union(self, value, union):
        public = public_name(value.native)
        declarations = [f"sealed class {public} {{", f"  const {public}._();"]
        children, writes, reads = [], [], []
        for arm in union.value.fields:
            name = identifier(arm.name)
            variant = public + pascal(arm.name)
            typ = self.public(arm.value)
            declarations.append(
                f"  const factory {public}.{name}({typ} value) = {variant};"
            )
            children.append(
                f"final class {variant} extends {public} {{\n  const {variant}(this.value) : super._();\n  final {typ} value;\n  @override bool operator ==(Object other) => other is {variant} && other.value == value;\n  @override int get hashCode => value.hashCode;\n}}"
            )
            tag = str(
                next(
                    item.value
                    for enum in self.bound.source.enums
                    for item in enum.values
                    if item.name == arm.presence.variant
                )
            )
            writes.append(
                f"    case {variant}():\n      result.ref.{union.presence.tag} = {tag};\n      result.ref.{union.name}.{arm.name} = {self.native(arm.value, 'value.value')};"
            )
            reads.append(
                f"    {tag} => {public}.{name}({self.copy(arm.value, 'source.' + union.name + '.' + arm.name)}),"
            )
        declarations.append("}")
        declarations.extend(children)
        initialize = (
            f"  result.ref = raw.{value.default}();"
            if value.default
            else f"  result.ref.size = sizeOf<raw.{value.native}>();"
            if any(f.role == "size" for f in value.fields)
            else ""
        )
        conversion = (
            f"Pointer<raw.{value.native}> _write{public}({public} value, Arena arena) {{\n  final result = arena<raw.{value.native}>();\n{initialize}\n  switch(value) {{\n"
            + "\n".join(writes)
            + f"\n  }}\n  return result;\n}}\n{public} _read{public}(raw.{value.native} source) => switch(source.{union.presence.tag}) {{\n"
            + "\n".join(reads)
            + "\n    _ => throwInvalidState('native union contains an unsupported variant'),\n};\n"
        )
        return "\n".join(declarations), conversion

    def render_mixed_union(self, value, union):
        public = public_name(value.native)
        payload = public + pascal(union.name)
        members = [f for f in self.fields(value) if f.name != union.name]
        arguments, fields, writes, reads = [], [], [], []
        for field in members:
            name, typ = identifier(field.name), self.public(field.value)
            default = self.default_expression(field.value)
            arguments.append(
                f"this.{name}"
                if typ.endswith("?")
                else f"this.{name} = {default}"
                if default
                else f"required this.{name}"
            )
            fields.append(f"  final {typ} {name};")
            writes.append(
                f"  result.ref.{field.name} = {self.native(field.value, 'value.' + name)};"
            )
            reads.append(
                f"    {name}: {self.copy(field.value, 'source.' + field.name)},"
            )
        item_buffer = next(
            (
                field.value.item_buffer
                for record in self.bound.values.values()
                for field in record.fields
                if field.value.kind == "array"
                and field.value.element
                and field.value.element.native == value.native
                and field.value.item_buffer
            ),
            None,
        )
        if item_buffer:
            arguments.append(f"this.{item_buffer.field} = ''")
            fields.append(f"  final String {item_buffer.field};")
            reads.append(f"    {item_buffer.field}: {item_buffer.field},")
        arguments.append(f"required this.{identifier(union.name)}")
        fields.append(f"  final {payload} {identifier(union.name)};")
        declarations = [
            f"final class {public} {{\n  const {public}({{{', '.join(arguments)}}});\n"
            + "\n".join(fields)
            + "\n}",
            f"sealed class {payload} {{ const {payload}._(); }}",
        ]
        cases, read_cases = [], []
        for arm in union.value.fields:
            variant = payload + pascal(arm.name)
            typ = self.public(arm.value)
            declarations.append(
                f"final class {variant} extends {payload} {{\n  const {variant}(this.value) : super._();\n  final {typ} value;\n}}"
            )
            tag = next(
                item.value
                for enum in self.bound.source.enums
                for item in enum.values
                if item.name == arm.presence.variant
            )
            cases.append(
                f"    case {variant}(:final value):\n      result.ref.{union.presence.tag} = {tag};\n      result.ref.{union.name}.{arm.name} = {self.native(arm.value, 'value')};"
            )
            read_cases.append(
                f"      {tag} => {variant}({self.copy(arm.value, 'source.' + union.name + '.' + arm.name)}),"
            )
        if union.value.empty_variant:
            tag = union.value.empty_variant[1]
            variant = payload + "None"
            declarations.append(
                f"final class {variant} extends {payload} {{ const {variant}() : super._(); }}"
            )
            cases.append(
                f"    case {variant}(): result.ref.{union.presence.tag} = {tag};"
            )
            read_cases.append(f"      {tag} => const {variant}(),")
        unknown = payload + "Unknown"
        declarations.append(
            f"final class {unknown} extends {payload} {{\n  {unknown}(this.tag, Uint8List rawRecord) : rawRecord = Uint8List.fromList(rawRecord).asUnmodifiableView(), super._();\n  final int tag;\n  final Uint8List rawRecord;\n}}"
        )
        cases.append(
            f"    case {unknown}(): throwInvalidArgument('unknown native union variant cannot be submitted');"
        )
        capture = f"withNativeArena((arena) {{ final copy = arena<raw.{value.native}>()..ref = source; return Uint8List.fromList(copy.cast<Uint8>().asTypedList(sizeOf<raw.{value.native}>())); }})"
        read_cases.append(
            f"      final tag => {unknown}(tag, rawRecord?.call() ?? {capture}),"
        )
        reads.append(
            f"    {identifier(union.name)}: switch(source.{union.presence.tag}) {{\n"
            + "\n".join(read_cases)
            + "\n    },"
        )
        initialize = (
            f"  result.ref = raw.{value.default}();"
            if value.default
            else f"  result.ref.size = sizeOf<raw.{value.native}>();"
            if any(f.role == "size" for f in value.fields)
            else ""
        )
        conversion = (
            f"Pointer<raw.{value.native}> _write{public}({public} value, Arena arena) {{\n  final result = arena<raw.{value.native}>();\n{initialize}\n"
            + "\n".join(writes)
            + f"\n  switch(value.{identifier(union.name)}) {{\n"
            + "\n".join(cases)
            + f"\n  }}\n  return result;\n}}\n{public} _read{public}(raw.{value.native} source, {{Uint8List Function()? rawRecord{', String ' + item_buffer.field + " = ''" if item_buffer else ''}}}) => {public}(\n"
            + "\n".join(reads)
            + "\n);\n"
        )
        return "\n".join(declarations), conversion

    def render(self):
        declarations, conversions = [], []
        for value in list(self.used.values()):
            if value.kind == "enum":
                public = public_name(value.native)
                prefix = (
                    commonprefix([name for name, _ in value.enum_values]).rsplit(
                        "_", 1
                    )[0]
                    + "_"
                )
                members = "\n".join(
                    f"  static const {identifier(name.removeprefix(prefix).lower())} = {public}.fromRawValue({number});"
                    for name, number in value.enum_values
                )
                operators = (
                    f"  {public} operator |({public} other) => {public}.fromRawValue(rawValue | other.rawValue);\n  {public} operator &({public} other) => {public}.fromRawValue(rawValue & other.rawValue);\n  bool contains({public} other) => (rawValue & other.rawValue) == other.rawValue;\n"
                    if value.enum_kind == "bitmask"
                    else ""
                )
                declarations.append(
                    f"final class {public} {{\n  const {public}.fromRawValue(this.rawValue);\n  final int rawValue;\n{members}\n{operators}  @override bool operator ==(Object other) => other is {public} && other.rawValue == rawValue;\n  @override int get hashCode => rawValue.hashCode;\n}}\n"
                )
                continue
            public = public_name(value.native)
            if value.kind == "callback":
                callback = self.bound.callbacks[value.native]
                parameters = ", ".join(
                    self.public(p.value)
                    for p in callback.parameters
                    if p.name != callback.context
                )
                declarations.append(f"typedef {public} = void Function({parameters});")
                continue
            if value.registration and self.port_callbacks(value):
                declaration, conversion = self.render_port_registration(value)
                declarations.append(declaration)
                conversions.append(conversion)
                continue
            if value.registration:
                declaration, conversion = self.render_registration(value)
                declarations.append(declaration)
                conversions.append(conversion)
                continue
            union = next((f for f in value.fields if f.value.kind == "union"), None)
            if union:
                declaration, conversion = (
                    self.render_mixed_union(value, union)
                    if len(self.fields(value)) > 1
                    else self.render_union(value, union)
                )
                declarations.append(declaration)
                conversions.append(conversion)
                continue
            members = self.members(value)
            flags = []
            for flag in value.mask_flags:
                mask = next(f.value for f in value.fields if f.name == flag.mask)
                prefix = (
                    commonprefix([name for name, _ in mask.enum_values]).rsplit("_", 1)[
                        0
                    ]
                    + "_"
                )
                flags.append((identifier(flag.name.removeprefix(prefix).lower()), flag))
            fields = "\n".join(f"  final {typ} {name};" for name, typ, _, _ in members)
            fields += "\n" + "\n".join(f"  final bool {name};" for name, _ in flags)
            positional = value.native in POSITIONAL
            args, initializers = [], []
            for name, typ, children, group in members:
                default = (
                    self.default_expression(children[0].value) if not group else None
                )
                copied = typ.startswith("List<") or typ.rstrip("?") == "Uint8List"
                if copied:
                    args.append(
                        ("" if typ.endswith("?") or typ == "Uint8List" else "required ")
                        + f"{typ + '?' if typ == 'Uint8List' else typ} {name}"
                    )
                    expression = (
                        f"Uint8List.fromList({name if typ.endswith('?') else name + ' ?? const <int>[]'}).asUnmodifiableView()"
                        if typ.rstrip("?") == "Uint8List"
                        else f"List.unmodifiable({name})"
                    )
                    if typ.endswith("?"):
                        expression = f"{name} == null ? null : {expression}"
                    initializers.append(f"{name} = {expression}")
                else:
                    args.append(
                        f"this.{name}"
                        if positional or typ.endswith("?")
                        else f"this.{name} = {default}"
                        if default
                        else f"required this.{name}"
                    )
            args += [f"this.{name} = false" for name, _ in flags]
            signature = ", ".join(args)
            if not positional:
                signature = "{" + signature + "}"
            equality = (
                " && ".join(
                    f"_generatedValueEquals(other.{name}, {name})"
                    for name, _, _, _ in members
                )
                or "true"
            )
            equality += "".join(f" && other.{name} == {name}" for name, _ in flags)
            hashes = ", ".join(
                f"_generatedValueHash({name})" for name, _, _, _ in members
            ) + "".join(f", {name}" for name, _ in flags)
            declarations.append(
                f"final class {public} {{\n  {'const ' if not initializers else ''}{public}({signature}){' : ' + ', '.join(initializers) if initializers else ''};\n{fields}\n  @override bool operator ==(Object other) => other is {public} && {equality};\n  @override int get hashCode => Object.hashAll([{hashes}]);\n}}\n"
            )
            write = [
                f"Pointer<raw.{value.native}> _write{public}({public} value, Arena arena"
                + (
                    ", _NativeRegistrations registrations"
                    if self.has_registrations(value)
                    else ""
                )
                + ") {",
                f"  final result = arena<raw.{value.native}>();",
            ]
            if value.default:
                write.append(f"  result.ref = raw.{value.default}();")
            elif any(f.role == "size" for f in value.fields):
                write.append(f"  result.ref.size = sizeOf<raw.{value.native}>();")
            read = []
            for name, flag in flags:
                write.append(
                    f"  if (value.{name}) {{ {self.set_present(flag.mask, flag.name)} }}"
                )
                read.append(f"    {name}: {self.present(flag.mask, flag.name)},")
            for name, typ, children, group in members:
                presence = group or children[0].presence
                optional = presence and presence.mask
                source = f"value.{name}" + ("!" if optional else "")
                if optional:
                    write += [
                        f"  if (value.{name} != null) {{",
                        "    " + self.set_present(presence.mask, presence.bit),
                    ]
                decoded = []
                for field in children:
                    expression = source + (
                        f".{identifier(field.name)}" if group else ""
                    )
                    native_field = f"result.ref.{field.name}"
                    if (
                        field.value.kind == "buffer"
                        and field.value.buffer_form != "view"
                        and field.value.length not in {None, "nul", "1"}
                    ):
                        view = (
                            f"nativeStringView({expression}, arena).value"
                            if field.value.encoding == "utf8"
                            else f"nativeBufferView({expression}, arena)"
                        )
                        write += [
                            f"  final bytes{name} = {view};",
                            f"  {native_field} = bytes{name}.data.cast();",
                            f"  result.ref.{field.value.length} = bytes{name}.size;",
                        ]
                    elif field.value.kind == "array":
                        child = field.value.element
                        write += [
                            f"  {native_field} = arena<{self.ffi(child)}>({expression}.isEmpty ? 1 : {expression}.length);",
                            f"  result.ref.{field.value.length} = {expression}.length;",
                            f"  for (var index = 0; index < {expression}.length; index++) {{ {native_field}[index] = {self.native(child, expression + '[index]')}; }}",
                        ]
                    else:
                        write.append(
                            f"  {native_field} = {self.native(field.value, expression)};"
                        )
                    decoded.append(
                        f"{identifier(field.name)}: {self.copy(field.value, 'source.' + field.name, 'source.' + str(field.value.length))}"
                    )
                if optional:
                    write.append("  }")
                if group and group.type in POSITIONAL:
                    decoded = [item.split(": ", 1)[1] for item in decoded]
                capture = (
                    (public_name(group.type) if group and group.type else "")
                    + "("
                    + ", ".join(decoded)
                    + ",)"
                    if group
                    else decoded[0].split(": ", 1)[1]
                )
                if optional:
                    capture = f"{self.present(presence.mask, presence.bit)} ? {capture} : null"
                read.append(f"    {name}: {capture},")
            write += ["  return result;", "}"]
            if positional:
                read = [line.split(": ", 1)[1] for line in read]
            conversions.append(
                "\n".join(write)
                + f"\n{public} _read{public}(raw.{value.native} source) => {public}(\n"
                + "\n".join(read)
                + "\n);\n"
            )
            for projection in self.projections:
                if projection.projection.native == value.native:
                    conversions.append(
                        f"{public} _read{public_name(projection.native)}(raw.{projection.native} source) => {public}(\n"
                        + "\n".join(read)
                        + "\n);\n"
                    )
        return "\n".join(declarations), "\n".join(conversions)
