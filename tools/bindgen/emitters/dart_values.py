"""Render Dart value ownership and conversions from resolved semantic plans."""

from __future__ import annotations

from dataclasses import replace

from .. import async_hosts, docs
from ..managed_contracts import DART_RESERVED
from ..names import camel, pascal
from ..semantic import BoundApi, ValuePlan, enum_member_prefix

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
        # The owners whose calls root callback registrations.
        self.root_owners: set[str] = set()
        self.used: dict[str, ValuePlan] = {
            name: value
            for name, value in bound.public_values.items()
            if value.kind == "enum"
        }

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
            if callback.delivery:
                self.check_delivery(callback)
                self.used[value.native] = value
                return
            if not self.wake_callback(callback):
                raise Unsupported(
                    "callback must answer on its native thread, which Dart cannot run"
                )
            self.used[value.native] = value
            return
        if value.registration and (
            self.wake(value) or self.queue_callbacks(value) or self.rules_only(value)
        ):
            for field in self.fields(value):
                self.check(field.value)
            self.used[value.native] = value
            return
        if value.registration:
            raise Unsupported(
                f"{value.native}: registration requires queue delivery, a wake, "
                "or native rules"
            )
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

    # How each registration reaches this isolate; see async_hosts.py.
    def wake_callback(self, callback):
        return async_hosts.wake_callback(callback)

    def wake(self, value):
        return async_hosts.is_wake(self.bound, value)

    def queue_callbacks(self, value):
        return async_hosts.queue_callbacks(self.bound, value)

    def omitted(self, field):
        return async_hosts.omitted(self.bound, field)

    def rules_only(self, value):
        return async_hosts.rules_only(self.bound, value)

    def check_delivery(self, callback):
        """Accept a delivered callback whose copied arguments Dart can decode."""
        for parameter in callback.arguments:
            if self.delivered_owner(callback, parameter):
                if parameter.value.native not in self.bound.public_handles:
                    raise Unsupported("delivered handle requires a generated owner")
                continue
            if parameter.value.kind == "handle":
                raise Unsupported("delivered handle argument requires an owner rule")
            if parameter.value.kind == "reference" and (
                not parameter.value.element or parameter.value.element.kind != "record"
            ):
                raise Unsupported("delivered reference argument requires a record")
            self.check(parameter.value)

    def fields(self, value):
        controls = {
            f.name
            for f in value.fields
            if f.role
            in {
                "size",
                "reserved",
                "presence_mask",
                "count",
                "tag",
                "stride",
                "delivery_queue",
            }
        }
        controls |= {
            f.presence.mask for f in value.fields if f.presence and f.presence.mask
        }
        if value.registration:
            controls |= {value.registration.user_data, value.registration.release}
            controls |= {f.name for f in value.fields if self.omitted(f)}
        controls |= {f.value.length for f in value.fields if f.value.kind == "array"}
        if value.default:
            controls |= {f.name for f in value.fields if self.left_disabled(f.value)}
        return tuple(f for f in value.fields if f.name not in controls)

    def left_disabled(self, value):
        return async_hosts.left_disabled(self.bound, value)

    def public(self, value):
        # An owned handle names its owner class, as a borrowed one does;
        # adopting it is the caller's job.
        self.check(
            replace(value, ownership="value") if value.kind == "handle" else value
        )
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
        if self.wake(value):
            field = next(
                f for f in value.fields if f.name in value.registration.callbacks
            )
            return f"registrations.add(registrations.roots.wake({expression}.{identifier(field.name)}))"
        roots = ", registrations.roots" if self.queue_callbacks(value) else ""
        return f"registrations.add(_prepare{public_name(value.native)}({expression}{roots}))"

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
            if (
                self.wake(value)
                or self.rules_only(value)
                or self.queue_callbacks(value)
            ):
                return f"const {public}()"
            raise Unsupported(f"{value.native}: copied registration needs a default")
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
                # Only a record that ends in a tagged union keeps the raw bytes
                # an unknown variant forwards.
                raw_record = (
                    f", rawRecord: () => {pointer}.cast<Uint8>().asTypedList({parent}.{value.stride})"
                    if any(f.value.kind == "union" for f in value.element.fields)
                    and len(self.fields(value.element)) > 1
                    else ""
                )
                item = f"_read{public_name(value.element.native)}({item_expression}{raw_record})"
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
        if bit not in self.bound.enum_numbers:
            raise Unsupported(f"{bit}: no C enum declares this constant")
        return f"raw.{bit}"

    def present(self, mask, bit):
        return f"(source.{mask} & {self.enum_constant(bit)}) != 0"

    def set_present(self, mask, bit):
        return f"result.ref.{mask} |= {self.enum_constant(bit)};"

    def field_default(self, field):
        return self.default_expression(field.value)

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
            if value.default or any(f.value.kind == "union" for f in value.fields):
                return None
            for _name, typ, field in self.members(value):
                if typ.endswith("?"):
                    continue
                default = self.field_default(field)
                if default is None:
                    return None
            return f"const {public_name(value.native)}()"
        return None

    def delivered_owner(self, callback, parameter):
        """Whether a delivered argument is a handle that the host takes from
        the batch: a decision handle, or one the callback owns."""
        return (
            callback.decision and parameter.name == callback.decision.parameter
        ) or (parameter.value.kind == "handle" and parameter.value.ownership == "owned")

    def render_delivery(self, callback):
        """Decode one queued delivery and run the host callback with it.

        MapLibre already has native code's answer, so a callback error reaches
        the registering zone instead of the native caller. The owner takes a
        delivered handle from the batch before the callback runs; closing it
        after a failure releases the unanswered request, which native then
        settles: a provider request fails, and a transform keeps its URL.
        """
        public = public_name(callback.native)
        owned = next(
            (p for p in callback.arguments if self.delivered_owner(callback, p)),
            None,
        )
        member = callback.delivery.payload_member
        lines = [
            f"void _deliver{public}({public} callback, int batch, int index, Pointer<raw.mln_delivery> delivery) {{",
            *(
                [f"  final payload = delivery.ref.payload.{member};"]
                if callback.arguments
                else []
            ),
        ]
        decoded = []
        body = []
        for parameter in callback.arguments:
            if parameter is owned:
                owner, native = owner_names(parameter.value.handle)
                body += [
                    "    _check(raw.mln_delivery_batch_take(batch, index, nativeDiagnostic));",
                    f"    final adopted = owner = {owner}._({native}(payload.{parameter.name}));",
                ]
                decoded.append("adopted")
            else:
                decoded.append(self.copy(parameter.value, f"payload.{parameter.name}"))
        if owned:
            owner, _ = owner_names(owned.value.handle)
            lines += [
                f"  {owner}? owner;",
                "  try {",
                *body,
                f"    callback({', '.join(decoded)});",
                "  } catch (_) {",
                "    owner?.close();",
                "    rethrow;",
                "  }",
            ]
        else:
            lines.append(f"  callback({', '.join(decoded)});")
        lines.append("}")
        return "\n".join(lines) + "\n"

    def registration_members(self, value, skip=()):
        """The declaration fields, constructor arguments, initializers, and
        native writes of a registration's public members other than skip."""
        fields, args, initializers, writes = [], [], [], []
        for name, typ, field in self.members(value):
            fields.append(
                doc(self.bound, f"{value.native}.{field.name}", "  ")
                + f"  final {typ} {name};"
            )
            if field.value.kind == "array":
                args.append(f"{typ} {name} = const []")
                initializers.append(f"{name} = List.unmodifiable({name})")
            else:
                default = self.field_default(field)
                args.append(
                    f"this.{name}"
                    if typ.endswith("?")
                    else f"this.{name} = {default}"
                    if default
                    else f"required this.{name}"
                )
            if field.name in skip:
                continue
            expression = "value." + name
            if field.value.kind == "array":
                child = field.value.element
                writes += [
                    f"    result.ref.{field.name} = arena<{self.ffi(child)}>({expression}.isEmpty ? 1 : {expression}.length);",
                    f"    result.ref.{field.value.length} = {expression}.length;",
                    f"    for (var index = 0; index < {expression}.length; index++) {{ result.ref.{field.name}[index] = {self.native(child, expression + '[index]')}; }}",
                ]
            elif field.presence and field.presence.mask:
                writes.append(
                    f"    if ({expression} != null) {{ {self.set_present(field.presence.mask, field.presence.bit)} result.ref.{field.name} = {self.native(field.value, expression + '!')}; }}"
                )
            else:
                writes.append(
                    f"    result.ref.{field.name} = {self.native(field.value, expression)};"
                )
        return fields, args, initializers, writes

    def registration_class(self, value, fields, args, initializers):
        public = public_name(value.native)
        constructor = (
            f"  {public}({{{', '.join(args)}}}) : {', '.join(initializers)};"
            if initializers
            else f"  const {public}({{{', '.join(args)}}});"
        )
        return f"final class {public} {{\n{constructor}\n" + "\n".join(fields) + "\n}"

    def registration_initializer(self, value):
        initialize = (
            f"result.ref = raw.{value.default}();"
            if value.default
            else f"result.ref.size = sizeOf<raw.{value.native}>();"
            if any(f.role == "size" for f in value.fields)
            else ""
        )
        return f"    {initialize}\n" if initialize else ""

    def render_queue_registration(self, value):
        """A registration whose callbacks a delivery queue carries.

        Dart sets the isolate's delivery queue in place of the callbacks, with
        a token as user_data. Native code answers each call and queues a copy,
        which the queue's wake delivers to this isolate, where an unset
        callback drops it. A queue replaces the primary callback, so its
        absence leaves the queue unset for native validation. The registering
        owner roots the registration until native code retires it, and a
        registration that native code rejects retires at once.
        """
        public = public_name(value.native)
        fields = self.queue_callbacks(value)
        names = {field.name for field in fields}
        members, args, initializers, writes = self.registration_members(
            value, skip=names
        )
        handlers = []
        for index, field in enumerate(fields):
            callback = self.bound.callbacks[field.value.native]
            name = identifier(field.name)
            invoke = f"_deliver{public_name(callback.native)}(value.{name}{'!' if field.value.nullable else ''}, batch, index, delivery)"
            invoke = f"=> {invoke}"
            handlers.append(
                (
                    "if (value." + name + " != null) "
                    if field.value.nullable and index > 0
                    else ""
                )
                + f"raw.{callback.delivery.constant}: (batch, index, delivery) {invoke},"
            )
        disabled = (
            "    if ("
            + f"value.{identifier(fields[0].name)} == null"
            + ") { return _NativeRegistration(result, () {}, arena.releaseAll); }\n"
            if fields[0].value.nullable
            else ""
        )
        conversion = (
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value, _CallbackRoots roots) {{\n  final arena = Arena();\n  DeliveryRegistration? registration;\n  try {{\n    final result = arena<raw.{value.native}>();\n"
            + self.registration_initializer(value)
            + "".join(line + "\n" for line in writes)
            + disabled
            + "    final registered = registration = roots.registerQueued({"
            + " ".join(handlers)
            + "});\n"
            + f"    result.ref.{value.registration.queue} = isolateDeliveryQueue.handle;\n"
            + f"    result.ref.{value.registration.user_data} = Pointer.fromAddress(registered.token);\n"
            + "    return _NativeRegistration(result, registered.reject, arena.releaseAll);\n"
            + "  } catch (_) {\n    registration?.reject();\n    arena.releaseAll();\n    rethrow;\n  }\n}\n"
        )
        reader = (
            f"{public} _read{public}(raw.{value.native} source) => {public}("
            + ", ".join(
                f"{name}: {self.copy(field.value, 'source.' + field.name)}"
                for name, _, field in self.members(value)
                if field.name not in names
            )
            + ");\n"
            if value.native in self.bound.returned
            else ""
        )
        return (
            self.registration_class(value, members, args, initializers),
            conversion + reader,
        )

    def render_wake(self, value):
        """The declaration of a wake, which _CallbackRoots.wake
        materializes."""
        members, args, initializers, _writes = self.registration_members(
            value, skip=set(value.registration.callbacks)
        )
        return self.registration_class(value, members, args, initializers)

    def render_rules_registration(self, value):
        """A registration whose callbacks Dart leaves unset, so the native
        rules that it carries answer each call. Native code copies the rules,
        so the registration holds nothing after the call."""
        public = public_name(value.native)
        fields, args, initializers, writes = self.registration_members(value)
        conversion = (
            f"_NativeRegistration<raw.{value.native}> _prepare{public}({public} value) {{\n  final arena = Arena();\n  try {{\n    final result = arena<raw.{value.native}>();\n"
            + self.registration_initializer(value)
            + "".join(line + "\n" for line in writes)
            + "    return _NativeRegistration(result, () {}, arena.releaseAll);\n  } catch (_) {\n    arena.releaseAll();\n    rethrow;\n  }\n}\n"
        )
        reader = (
            f"{public} _read{public}(raw.{value.native} source) => {public}();\n"
            if value.native in self.bound.returned
            else ""
        )
        return (
            self.registration_class(value, fields, args, initializers),
            conversion + reader,
        )

    def render_union(self, value, union):
        public = public_name(value.native)
        declarations = [f"sealed class {public} {{", f"  const {public}._();"]
        children, writes, reads = [], [], []
        for arm in union.value.fields:
            name = identifier(arm.name)
            variant = public + pascal(arm.name)
            typ = self.public(arm.value)
            declarations.append(
                doc(self.bound, f"{union.value.native}.{arm.name}", "  ")
                + f"  const factory {public}.{name}({typ} value) = {variant};"
            )
            children.append(
                f"final class {variant} extends {public} {{\n  const {variant}(this.value) : super._();\n  final {typ} value;\n  @override bool operator ==(Object other) => other is {variant} && other.value == value;\n  @override int get hashCode => value.hashCode;\n}}"
            )
            tag = str(self.bound.enum_number(arm.presence.variant))
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
            f"Pointer<raw.{value.native}> _write{public}({public} value, Allocator arena) {{\n  final result = arena<raw.{value.native}>();\n{initialize}\n  switch(value) {{\n"
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
            fields.append(
                doc(self.bound, f"{value.native}.{field.name}", "  ")
                + f"  final {typ} {name};"
            )
            writes.append(
                f"  result.ref.{field.name} = {self.native(field.value, 'value.' + name)};"
            )
            reads.append(
                f"    {name}: {self.copy(field.value, 'source.' + field.name)},"
            )
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
                doc(self.bound, f"{union.value.native}.{arm.name}")
                + f"final class {variant} extends {payload} {{\n  const {variant}(this.value) : super._();\n  final {typ} value;\n}}"
            )
            tag = self.bound.enum_number(arm.presence.variant)
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
            f"Pointer<raw.{value.native}> _write{public}({public} value, Allocator arena) {{\n  final result = arena<raw.{value.native}>();\n{initialize}\n"
            + "\n".join(writes)
            + f"\n  switch(value.{identifier(union.name)}) {{\n"
            + "\n".join(cases)
            + f"\n  }}\n  return result;\n}}\n{public} _read{public}(raw.{value.native} source, {{Uint8List Function()? rawRecord}}) => {public}(\n"
            + "\n".join(reads)
            + "\n);\n"
        )
        return "\n".join(declarations), conversion

    def render(self):
        declarations, conversions = [], []
        documented = []
        for value in list(self.used.values()):
            documented.append((len(declarations), value.native))
            if value.kind == "enum":
                public = public_name(value.native)
                prefix = enum_member_prefix(name for name, _ in value.enum_values)
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
                if not bitmask:
                    cases = "\n".join(
                        f"{number} => '{public}.{identifier(name.removeprefix(prefix).lower())}',"
                        for number, name in {
                            number: name for name, number in value.enum_values
                        }.items()
                    )
                    operators += f"  @override String toString() => switch (rawValue) {{ {cases} _ => '{public}($rawValue)', }};\n"
                declarations.append(
                    f"final class {public} extends {base} {{\n  const {public}.fromRawValue(super.rawValue);\n{members}\n{operators}}}\n"
                )
                continue
            public = public_name(value.native)
            if value.kind == "callback":
                callback = self.bound.callbacks[value.native]
                parameters = ", ".join(self.public(p.value) for p in callback.arguments)
                declarations.append(f"typedef {public} = void Function({parameters});")
                if callback.delivery:
                    conversions.append(self.render_delivery(callback))
                continue
            if value.registration and self.wake(value):
                declarations.append(self.render_wake(value))
                continue
            if value.registration and self.queue_callbacks(value):
                declaration, conversion = self.render_queue_registration(value)
                declarations.append(declaration)
                conversions.append(conversion)
                continue
            if value.registration:
                declaration, conversion = self.render_rules_registration(value)
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
                if value.native not in self.bound.inputs:
                    conversion = conversion[
                        conversion.index(f"{public} _read{public}(") :
                    ]
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
            args, initializers = [], []
            for name, typ, field in members:
                if value.default and not typ.endswith("?"):
                    args.append(f"{typ}? {name}")
                    fallback = f"{name} ?? {camel(value.default.removeprefix('mln_'))}().{name}"
                    expression = (
                        f"List.unmodifiable({fallback})"
                        if typ.startswith("List<")
                        else f"Uint8List.fromList({fallback}).asUnmodifiableView()"
                        if typ == "Uint8List"
                        else fallback
                    )
                    initializers.append(f"{name} = {expression}")
                    continue
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
                        if typ.endswith("?")
                        else f"this.{name} = {default}"
                        if default
                        else f"required this.{name}"
                    )
            args += [f"this.{name} = false" for name, _ in flags]
            signature = ", ".join(args)
            signature = "{" + signature + "}"
            compared = ", ".join(
                [name for name, _, _ in members] + [name for name, _ in flags]
            )
            declarations.append(
                f"final class {public} extends _Value {{\n  {'const ' if not initializers and not value.default else ''}{public}({signature}){' : ' + ', '.join(initializers) if initializers else ''};\n{fields}\n  @override List<Object?> get _members => [{compared}];\n}}\n"
            )
            write = [
                f"Pointer<raw.{value.native}> _write{public}({public} value, Allocator arena"
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
            conversions.append(
                ("\n".join(write) if value.native in self.bound.inputs else "")
                + f"\n{public} _read{public}(raw.{value.native} source) => {public}(\n"
                + "\n".join(read)
                + "\n);\n"
            )
        ends = [start for start, _ in documented[1:]] + [len(declarations)]
        for (start, native), end in zip(documented, ends):
            if start < end:
                declarations[start] = doc(self.bound, native) + declarations[start]
        return "\n".join(declarations), "\n".join(conversions)
