"""Render Dart value ownership and conversions from resolved semantic plans."""

from __future__ import annotations

from dataclasses import replace
from os.path import commonprefix

from .. import docs, native_ports
from ..managed_contracts import DART_RESERVED
from ..names import camel, pascal
from ..native_capture import arguments_record, deferred_constant
from ..semantic import BoundApi, ValuePlan

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


def deferred_key(callback) -> str:
    """The Dart expression for a deferred callback's port message key."""
    return f"(raw.{deferred_constant(callback.native)} & 0xffffffff)"


def owner_names(handle) -> tuple[str, str]:
    """The public owner class and native handle type for a handle plan."""
    name = pascal(handle.stem)
    return name + "Handle", "Native" + name


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The dartdoc comment of a declaration, or empty when it has none."""
    return docs.line_comment(bound.doc(native), indent)


class Unsupported(ValueError):
    pass


def public_name(native: str) -> str:
    return pascal(native.removeprefix("mln_"))


def identifier(native: str) -> str:
    name = camel(native)
    return name + "Value" if name in DART_RESERVED else name


class Values:
    def __init__(self, bound: BoundApi):
        self.bound = bound
        # The owners whose generated operations root callback ports.
        self.port_owners: set[str] = set()
        self.used: dict[str, ValuePlan] = {
            name: value
            for name, value in bound.public_values.items()
            if value.kind == "enum"
        }

        # The completion descriptors that queries name, by descriptor name.
        self.results: dict[str, str] = {}

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
            if callback.deferred:
                self.check_deferred(callback)
                self.used[value.native] = value
                return
            if callback.result.native != "void" or not callback.context:
                raise Unsupported("callback requires a synchronous native adapter")
            if callback.synchronous:
                raise Unsupported("callback must finish on its native thread")
            for parameter in callback.parameters:
                if parameter.name != callback.context:
                    if native_ports.flattened(parameter.value, parameter.name) is None:
                        raise Unsupported(
                            "callback payload needs another native port copy rule"
                        )
                    self.check(parameter.value)
            self.used[value.native] = value
            return
        if value.registration and (
            self.port_callbacks(value) or self.deferred_callback(value)
        ):
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

    def deferred_callback(self, value):
        """The one callback of a registration that a deferred adapter answers
        at once and delivers to this isolate later, or None.

        A callback that native callback adapters serve keeps their variants.
        """
        if not value.registration or len(value.registration.callbacks) != 1:
            return None
        field = next(f for f in value.fields if f.name in value.registration.callbacks)
        if any(
            adapter.callback == field.value.native
            for adapter in self.bound.callback_adapters
        ):
            return None
        return field if self.bound.callbacks[field.value.native].deferred else None

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

    def registration_ports(self, value):
        """Whether an adapter variant of a registration binds a deferred port."""
        return any(
            self.deferred_field(self.bound.values[adapter.context])
            for adapter in self.registration_adapters(value)
        )

    def check_deferred(self, callback):
        """Accept a deferred callback whose copied arguments Dart can decode."""
        for parameter in callback.parameters:
            if parameter.name == callback.context:
                continue
            if callback.decision and parameter.name == callback.decision.parameter:
                if parameter.value.native not in self.bound.public_handles:
                    raise Unsupported("deferred decision requires a generated owner")
                continue
            if parameter.value.kind == "handle":
                raise Unsupported("deferred handle argument requires an owner rule")
            if parameter.value.kind == "reference" and (
                not parameter.value.element or parameter.value.element.kind != "record"
            ):
                raise Unsupported("deferred reference argument requires a record")
            self.check(parameter.value)

    def deferred_field(self, value):
        """The deferred callback field of a record and the context it pairs with.

        A record that is not a registration descriptor names its callback's
        context in its single context field.
        """
        if value.registration or value.kind != "record":
            return None
        callbacks = [
            f
            for f in value.fields
            if f.value.kind == "callback"
            and self.bound.callbacks[f.value.native].deferred
        ]
        if not callbacks:
            return None
        contexts = [f.name for f in value.fields if f.role == "context"]
        if len(callbacks) != 1 or len(contexts) != 1:
            raise Unsupported(
                f"{value.native}: deferred callback field requires one paired context"
            )
        return callbacks[0], contexts[0]

    def fields(self, value):
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
        deferred = self.deferred_field(value)
        if deferred:
            controls.add(deferred[1])
        if value.default:
            controls |= {f.name for f in value.fields if self.left_disabled(f.value)}
        return tuple(f for f in value.fields if f.name not in controls)

    def left_disabled(self, value):
        """Whether a field keeps its native default, a disabled registration.

        Dart runs host code only on its isolate, so it cannot run a synchronous
        callback that native code calls on its own thread. A registration with
        one has no Dart form. A record that holds it and has a native default
        value keeps the field disabled, as that default provides it.
        """
        if not value.registration:
            return False
        callbacks = [
            f.value for f in value.fields if f.name in value.registration.callbacks
        ]
        return all(callback.nullable for callback in callbacks) and any(
            self.bound.callbacks[callback.native].synchronous for callback in callbacks
        )

    def public(self, value):
        self.check(value)
        if value.kind == "handle":
            name = owner_names(value.handle)[0]
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

    def prepared(self, value, expression):
        """A registration descriptor added to the call's transaction."""
        roots = (
            "registrations.ports"
            if self.port_callbacks(value) or self.deferred_callback(value)
            else "_callbackReleases, registrations.ports"
            if self.registration_ports(value)
            else "_callbackReleases"
        )
        # A callback that calls back only into its registering receiver
        # arrives after the call, so it is dropped once that receiver closes.
        closed = ", () => isClosed" if value.registration.receiver_owned else ""
        return f"registrations.add(_prepare{public_name(value.native)}({expression}, {roots}{closed}))"

    def native(self, value, expression):
        if value.registration:
            return f"{self.prepared(value, expression)}.ref"
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
                    code = self.prepared(child, local)
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
                    f"sizeOf<UnsignedLong>() == 4 ? _nativeInteger({bits}, 0, 4294967295) : {bits}"
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
                return f"_nativeInteger({expression}, {lo}, {hi})"
            if ffi in {"Size", "UintPtr"}:
                return f"_nativeInteger({expression}, 0, sizeOf<{ffi}>() == 4 ? 4294967295 : 0x7fffffffffffffff)"
            if ffi in {"Long", "IntPtr"}:
                return f"sizeOf<{ffi}>() == 4 ? _nativeInteger({expression}, -2147483648, 2147483647) : {expression}"
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
            public = public_name(value.native)
            # Native never returns callbacks. A registration's own default
            # copies its other fields, and any other copy leaves it unset.
            if value.native in self.bound.returned:
                return f"_read{public}({expression})"
            if value.nullable:
                return "null"
            if self.port_callbacks(value):
                return f"const {public}()"
            return f"const {public}.empty()"
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
                    extra = f", {item_buffer.field}: _arenaUtf8({parent}.{item_buffer.data}.cast(), {parent}.{item_buffer.size}, {item_expression}.{item_buffer.offset}, {item_expression}.{item_buffer.length})"
                # Only a record that ends in a tagged union keeps the raw bytes
                # an unknown variant forwards.
                raw_record = (
                    f", rawRecord: () => {pointer}.cast<Uint8>().asTypedList({parent}.{value.stride})"
                    if any(f.value.kind == "union" for f in value.element.fields)
                    and len(self.fields(value.element)) > 1
                    else ""
                )
                item = f"_read{public_name(value.element.native)}({item_expression}{raw_record}{extra})"
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
        """The public members of a record: (name, type, field)."""
        result = []
        for field in self.fields(value):
            typ = self.public(field.value)
            if field.presence and field.presence.mask and not typ.endswith("?"):
                typ += "?"
            result.append((identifier(field.name), typ, field))
        return result

    def enum_constant(self, bit):
        if not any(v.name == bit for e in self.bound.source.enums for v in e.values):
            raise Unsupported(f"{bit}: no C enum declares this constant")
        return f"raw.{bit}"

    def present(self, mask, bit):
        return f"(source.{mask} & {self.enum_constant(bit)}) != 0"

    def set_present(self, mask, bit):
        return f"result.ref.{mask} |= {self.enum_constant(bit)};"

    def field_default(self, field):
        """A field's default: its annotated initial value, or its type's."""
        value, initial = field.value, field.initial
        if initial is None:
            return self.default_expression(value)
        if value.kind == "enum":
            return f"{public_name(value.native)}.{identifier(initial.member)}"
        return initial.literal

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
            for _name, typ, field in self.members(value):
                if typ.endswith("?"):
                    continue
                default = self.field_default(field)
                if default is None:
                    return None
                if value.ordered:
                    args.append(default)
            return f"const {public_name(value.native)}({', '.join(args)})"
        return None

    def write_deferred_field(self, field, context, expression):
        """Bind a deferred callback field to a port the written arena retires.

        The arena runs the context release when native code releases the
        record that holds it, so the port closes after the final call.
        """
        callback = self.bound.callbacks[field.value.native]
        key = deferred_key(callback)
        local = "port" + pascal(field.name)
        return [
            f"  final {local} = ports.registerDeferred({key}, (message) => _deliver{public_name(callback.native)}({expression}, message));",
            f"  arena.adoptRelease(Native.addressOf<NativeFunction<raw.mln_user_data_releaseFunction>>(raw.mln_adapter_deferred_callback_release), {local}.context);",
            f"  result.ref.{field.name} = raw.mln_adapter_deferred_callback_function({key}).cast();",
            f"  result.ref.{context} = {local}.context;",
        ]

    def render_deferred_delivery(self, callback):
        """Decode one deferred call record and run the host callback with it.

        MapLibre already has the deferred answer, so a callback error reaches
        the registering zone instead of the native caller. The owner adopts a
        decision handle before the callback runs; closing it after a failure
        releases the unanswered request, which native then fails.
        """
        public = public_name(callback.native)
        decision = callback.decision
        lines = [
            f"void _deliver{public}({public} callback, List<dynamic> message) {{",
            "  final record = Pointer<raw.mln_adapter_deferred_call_record>.fromAddress(message[1] as int);",
        ]
        if decision:
            owner, native = owner_names(decision.handle)
            lines += [f"  {owner}? owner;"]
        lines += [
            "  try {",
            f"    final arguments = record.ref.arguments.cast<raw.{arguments_record(callback.native)}>().ref;",
        ]
        decoded = []
        for parameter in callback.parameters:
            if parameter.name == callback.context:
                continue
            if decision and parameter.name == decision.parameter:
                lines += [
                    f"    final adopted = owner = {owner}._({native}(arguments.{parameter.name}));",
                    "    raw.mln_adapter_deferred_call_record_adopt(record);",
                ]
                decoded.append("adopted")
            else:
                decoded.append(
                    self.copy(parameter.value, f"arguments.{parameter.name}")
                )
        lines.append(f"    callback({', '.join(decoded)});")
        if decision:
            lines += [
                "  } catch (_) {",
                "    owner?.close();",
                "    rethrow;",
                "  } finally {",
                "    raw.mln_adapter_deferred_call_record_destroy(record);",
                "  }",
            ]
        else:
            lines += [
                "  } finally {",
                "    raw.mln_adapter_deferred_call_record_destroy(record);",
                "  }",
            ]
        lines.append("}")
        return "\n".join(lines) + "\n"

    def port_copy(self, value, offset):
        if value.kind == "record":
            args = []
            for field in self.fields(value):
                expression, offset = self.port_copy(field.value, offset)
                args.append(
                    ("" if value.ordered else identifier(field.name) + ": ")
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
        for name, typ, field in self.members(value):
            fields.append(f"  final {typ} {name};")
            default = self.field_default(field)
            args.append(
                f"this.{name}"
                if typ.endswith("?")
                else f"this.{name} = {default}"
                if default
                else f"required this.{name}"
            )
            if field.name in ports:
                callback, _ = ports[field.name]
                key = f"(raw.{native_ports.constant(value.native, field.name)} & 0xffffffff)"
                decoded, offset = [], 1
                for parameter in callback.parameters:
                    if parameter.name == callback.context:
                        continue
                    expression, offset = self.port_copy(parameter.value, offset)
                    decoded.append(expression)
                invoke = f"value.{name}{'!' if field.value.nullable else ''}({', '.join(decoded)})"
                delivery = (
                    f"(message) {{ if (!receiverClosed()) {{ {invoke}; }} }}"
                    if value.registration.receiver_owned
                    else f"(message) => {invoke}"
                )
                handlers.append(
                    f"      {'if (value.' + name + ' != null) ' if field.value.nullable else ''}{key}: {delivery},"
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
            if field.name in ports
            else f"{name}: {self.copy(field.value, 'source.' + field.name)}"
            for name, _, field in self.members(value)
        )
        reader = (
            f"{public} _read{public}(raw.{value.native} source) => {public}({read_args});\n"
            if disabled and value.native in self.bound.returned
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
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value, _NativeCallbackPorts roots{', bool Function() receiverClosed' if value.registration.receiver_owned else ''}) {{\n  final arena = Arena();\n  _NativeCallbackPort? port;\n  try {{\n    final result = arena<raw.{value.native}>();\n    {initialize}\n    {disabled_code}\n    port = roots.register({{\n"
            + "\n".join(handlers)
            + "\n    });\n"
            + "\n".join(writes)
            + f"\n    result.ref.{value.registration.user_data} = port.context;\n    result.ref.{value.registration.release} = Native.addressOf<NativeFunction<raw.mln_user_data_releaseFunction>>(raw.mln_adapter_dart_port_release).cast();\n    return _NativeRegistration(result, port.reject, arena.releaseAll);\n  }} catch (_) {{ port?.reject(); arena.releaseAll(); rethrow; }}\n}}\n"
        )
        return declaration, conversion + reader

    def render_deferred_registration(self, value):
        """A registration whose one callback a deferred adapter delivers.

        The adapter answers each call at once and posts a copy to a port on
        this isolate, which the transaction's roots keep until native release
        retires it. A registration that native code does not accept releases
        the port at once.
        """
        public = public_name(value.native)
        deferred = self.deferred_callback(value)
        callback = self.bound.callbacks[deferred.value.native]
        key = deferred_key(callback)
        fields, args, writes = [], [], []
        for name, typ, field in self.members(value):
            fields.append(f"  final {typ} {name};")
            default = self.field_default(field)
            args.append(
                f"this.{name}"
                if typ.endswith("?")
                else f"this.{name} = {default}"
                if default
                else f"required this.{name}"
            )
            if field.name == deferred.name:
                continue
            writes.append(
                f"    result.ref.{field.name} = {self.native(field.value, 'value.' + name)};"
            )
        name = identifier(camel(deferred.name))
        if deferred.value.nullable:
            raise Unsupported(f"{value.native}: a deferred callback must be set")
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
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value, _NativeCallbackPorts roots) {{\n  final arena = Arena();\n  _NativeCallbackPort? port;\n  try {{\n    final result = arena<raw.{value.native}>();\n    {initialize}\n"
            + "".join(line + "\n" for line in writes)
            + f"    port = roots.registerDeferred({key}, (message) => _deliver{public_name(callback.native)}(value.{name}, message));\n"
            + f"    result.ref.{deferred.name} = raw.mln_adapter_deferred_callback_function({key}).cast();\n"
            + f"    result.ref.{value.registration.user_data} = port.context;\n"
            + f"    result.ref.{value.registration.release} = Native.addressOf<NativeFunction<raw.mln_user_data_releaseFunction>>(raw.mln_adapter_deferred_callback_release).cast();\n"
            + "    return _NativeRegistration(result, port.reject, arena.releaseAll);\n  } catch (_) { port?.reject(); arena.releaseAll(); rethrow; }\n}\n"
        )
        return declaration, conversion

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
            ports = (
                ", ports"
                if self.deferred_field(self.bound.values[adapter.context])
                else ""
            )
            cases.append(
                f"    case {variant}():\n      final context = _write{context}(value.value, arena{ports});\n      final descriptor = arena<raw.{value.native}>();\n      descriptor.ref.size = sizeOf<raw.{value.native}>();\n      descriptor.ref.{callback_field} = Native.addressOf<NativeFunction<raw.{adapter.callback}Function>>(raw.{adapter.function});\n      descriptor.ref.{value.registration.user_data} = context.cast();\n      descriptor.ref.{value.registration.release} = Native.addressOf<NativeFunction<raw.mln_user_data_releaseFunction>>(raw.mln_adapter_dart_release);\n      transferred = true;\n      roots.register(context.cast(), arena.releaseAll, arena: arena);\n      return _NativeRegistration(descriptor, () => roots.reject(context.cast()));"
            )
        declarations.append("}")
        declarations.extend(children)
        conversion = (
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value, NativeCallbackReleases roots{', _NativeCallbackPorts ports' if self.registration_ports(value) else ''}) {{\n  final arena = NativeOwnedArena();\n  var transferred = false;\n  try {{\n    switch(value) {{\n"
            + "\n".join(cases)
            + "\n    }\n  } catch (_) { if (!transferred) { arena.releaseAll(); } rethrow; }\n}\n"
        )
        reader = (
            f"{public} _read{public}(raw.{value.native} source) => const {public}.empty();\n"
            if value.native in self.bound.returned
            else ""
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
            default = self.field_default(field)
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
        # The first declaration that each value renders carries its comment.
        documented = []
        for value in list(self.used.values()):
            documented.append((len(declarations), value.native))
            if value.kind == "enum":
                public = public_name(value.native)
                prefix = (
                    commonprefix([name for name, _ in value.enum_values]).rsplit(
                        "_", 1
                    )[0]
                    + "_"
                )
                members = "\n".join(
                    f"{doc(self.bound, name, '  ')}  static const {identifier(name.removeprefix(prefix).lower())} = {public}.fromRawValue({number});"
                    for name, number in value.enum_values
                )
                bitmask = value.enum_kind == "bitmask"
                base = f"_Flags<{public}>" if bitmask else "_Enum"
                operators = (
                    f"  @override {public} _of(int rawValue) => {public}.fromRawValue(rawValue);\n"
                    if bitmask
                    else ""
                )
                declarations.append(
                    f"final class {public} extends {base} {{\n  const {public}.fromRawValue(super.rawValue);\n{members}\n{operators}}}\n"
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
                if callback.deferred:
                    conversions.append(self.render_deferred_delivery(callback))
                continue
            if value.registration and self.port_callbacks(value):
                declaration, conversion = self.render_port_registration(value)
                declarations.append(declaration)
                conversions.append(conversion)
                continue
            if value.registration and self.deferred_callback(value):
                declaration, conversion = self.render_deferred_registration(value)
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
                flags.append((identifier(flag.member), flag))
            fields = "\n".join(
                doc(self.bound, f"{value.native}.{field.name}", "  ")
                + f"  final {typ} {name};"
                for name, typ, field in members
            )
            fields += "\n" + "\n".join(
                f"{doc(self.bound, flag.name, '  ')}  final bool {name};"
                for name, flag in flags
            )
            # A record whose field order is its meaning constructs positionally.
            positional = value.ordered
            args, initializers = [], []
            for name, typ, field in members:
                default = self.field_default(field)
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
            compared = ", ".join(
                [name for name, _, _ in members] + [name for name, _ in flags]
            )
            declarations.append(
                f"final class {public} extends _Value {{\n  {'const ' if not initializers else ''}{public}({signature}){' : ' + ', '.join(initializers) if initializers else ''};\n{fields}\n  @override List<Object?> get _members => [{compared}];\n}}\n"
            )
            deferred = self.deferred_field(value)
            write = [
                f"Pointer<raw.{value.native}> _write{public}({public} value, "
                + (
                    "NativeOwnedArena arena, _NativeCallbackPorts ports"
                    if deferred
                    else "Arena arena"
                )
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
            for name, typ, field in members:
                presence = field.presence
                optional = presence and presence.mask
                expression = f"value.{name}" + ("!" if optional else "")
                if optional:
                    write += [
                        f"  if (value.{name} != null) {{",
                        "    " + self.set_present(presence.mask, presence.bit),
                    ]
                native_field = f"result.ref.{field.name}"
                capture = None
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
                    # A null list leaves the zeroed pointer and count.
                    items = expression + "!" if field.value.nullable else expression
                    lines = [
                        f"  {native_field} = arena<{self.ffi(child)}>({items}.isEmpty ? 1 : {items}.length);",
                        f"  result.ref.{field.value.length} = {items}.length;",
                        f"  for (var index = 0; index < {items}.length; index++) {{ {native_field}[index] = {self.native(child, items + '[index]')}; }}",
                    ]
                    if field.value.nullable:
                        lines = [
                            f"  if ({expression} != null) {{",
                            *lines,
                            "  }",
                        ]
                    write += lines
                elif deferred and field.name == deferred[0].name:
                    write += self.write_deferred_field(field, deferred[1], expression)
                    capture = (
                        "throwInvalidState('cannot copy a registered native callback')"
                    )
                else:
                    write.append(
                        f"  {native_field} = {self.native(field.value, expression)};"
                    )
                if capture is None:
                    capture = self.copy(
                        field.value,
                        "source." + field.name,
                        "source." + str(field.value.length),
                    )
                    if field.value.kind == "array" and field.value.nullable:
                        capture = f"source.{field.name} == nullptr ? null : {capture}"
                if optional:
                    write.append("  }")
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
        ends = [start for start, _ in documented[1:]] + [len(declarations)]
        for (start, native), end in zip(documented, ends):
            if start < end:
                declarations[start] = doc(self.bound, native) + declarations[start]
        return "\n".join(declarations), "\n".join(conversions)
