"""Render the addon's C converters between Node-API values and C values.

A writer reads one JavaScript object into a C record that the caller already
initialized from the record's native default, or from zero with its size set,
so a member that the object leaves out keeps that default. A reader builds a
JavaScript object from a borrowed C record. A completion decoder reads the
value of a drained completion, and the payload reader converts the copied
arguments of each other drained delivery. Converters check only the shape that
the binding owns: the JavaScript type, the integer range, and NUL termination.

A registration that a delivery queue carries names the realm's queue and a
token in place of its callbacks; a wake posts its token to the realm through
the addon's wake shim. A native registration, which a wake takes as an object
in place of its function, fills the descriptor with the function pointers,
context, and release of native code in the host.
"""

from __future__ import annotations

from ..semantic import FieldPlan, ValuePlan
from .typescript_values import (
    Unsupported,
    Values,
    identifier,
    is_counted,
    public_name,
    union_kinds,
)


def c_string(text: str) -> str:
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def stem(native: str) -> str:
    return native.removeprefix("mln_").replace(" ", "_").replace("*", "").strip("_")


class NativeValues:
    """The C converters that the generated export functions call."""

    def __init__(self, values: Values):
        self.values = values
        self.bound = values.bound
        self.functions: dict[str, tuple[str, str]] = {}
        # The completion decoders that the decoder table lists.
        self.decoders: set[str] = set()

    def merge(self, other: NativeValues) -> None:
        self.functions.update(other.functions)
        self.decoders |= other.decoders

    # -- Initialization -----------------------------------------------------

    def init(self, value: ValuePlan, target: str) -> list[str]:
        """Statements that start a C record from its native default."""
        if value.kind != "record":
            return [f"memset(&{target}, 0, sizeof {target});"]
        if value.default:
            return [f"{target} = {value.default}();"]
        lines = [f"memset(&{target}, 0, sizeof {target});"]
        size = next((f for f in value.fields if f.role == "size"), None)
        if size:
            lines.append(f"{target}.{size.name} = sizeof {target};")
        return lines

    # -- JavaScript to C ----------------------------------------------------

    def read(
        self, value: ValuePlan, js: str, target: str, path: str, fail: str
    ) -> list[str]:
        """Statements that read `js` into the C lvalue `target`."""
        if value.kind in {"scalar", "enum", "handle", "native_pointer"}:
            scalar = self.values.carrier(value)
            if value.kind == "native_pointer":
                return [
                    f"{{ uint64_t number = 0; if (!mln_ts_read_u64(call, {js}, &number, {path})) {fail}; {target} = ({value.ctype.spelling})(uintptr_t)number; }}"
                ]
            if scalar.kind == "bool":
                return [
                    f"{{ bool flag = false; if (!mln_ts_read_bool(call, {js}, &flag, {path})) {fail}; {target} = flag; }}"
                ]
            if scalar.kind == "int":
                return [
                    f"{{ int64_t number = 0; if (!mln_ts_read_int(call, {js}, {scalar.minimum}LL, {scalar.maximum}LL, &number, {path})) {fail}; {target} = ({scalar.c})number; }}"
                ]
            if scalar.kind == "double":
                return [
                    f"{{ double number = 0; if (!mln_ts_read_double(call, {js}, &number, {path})) {fail}; {target} = ({scalar.c})number; }}"
                ]
            reader = "mln_ts_read_u64" if scalar.kind == "u64" else "mln_ts_read_i64"
            carrier = "uint64_t" if scalar.kind == "u64" else "int64_t"
            return [
                f"{{ {carrier} number = 0; if (!{reader}(call, {js}, &number, {path})) {fail}; {target} = ({scalar.c})number; }}"
            ]
        if value.kind == "buffer":
            if value.buffer_form == "view":
                reader = (
                    "mln_ts_read_bytes_view"
                    if value.encoding == "bytes"
                    else "mln_ts_read_text_view"
                )
                return [f"if (!{reader}(call, {js}, &{target}, {path})) {fail};"]
            return [
                f"{{ const char* text = NULL; if (!mln_ts_read_text(call, {js}, &text, {path})) {fail}; {target} = text; }}"
            ]
        if value.kind == "reference" and value.element.kind == "buffer":
            return self.read(value.element, js, target, path, fail)
        if value.kind == "record" and value.registration:
            entry = self.values.registration(value)
            plan = value.registration
            if entry.form == "native":
                return self.read_native(value, entry, js, target, path, fail)
            if entry.form == "wake":
                field = entry.callbacks[0]
                lines = [
                    f"if (!mln_ts_read_wake(call, {js}, (void**)&{target}.{plan.user_data}, {path})) {fail};",
                    f"if ({target}.{plan.user_data} != NULL) {{",
                    f"  {target}.{field.name} = mln_ts_wake;",
                    f"  {target}.{plan.release} = mln_ts_wake_release;",
                    "}",
                ]
                if not entry.native:
                    return lines
                native = self.read_native(value, entry, js, target, path, fail)
                return [
                    f"if (mln_ts_is_object(call, {js})) {{",
                    *(f"  {line}" for line in native),
                    "} else {",
                    *(f"  {line}" for line in lines),
                    "}",
                ]
            if entry.form == "queue" and entry.function:
                return [
                    f"if (!mln_ts_read_function(call, {js}, {path})) {fail};",
                    *self.register(value, js, target, path, fail),
                ]
        if value.kind == "record":
            return [
                f"if (!mln_ts_read_record(call, {js}, {path})) {fail};",
                f"if (!{self.writer(value)}(call, {js}, &{target}, {path})) {fail};",
            ]
        raise Unsupported(f"{value.native}: no C reader")

    def read_native(
        self, value: ValuePlan, entry, js: str, target: str, path: str, fail: str
    ) -> list[str]:
        """Statements that take a native registration into the descriptor
        `target`: its callbacks, then its context and its release."""
        if not entry.native:
            raise Unsupported(f"{value.native}: no native registration")
        kind = c_string(public_name(value.native))
        count = len(entry.native)
        return [
            "{",
            f"  uint64_t fields[{count}];",
            f"  if (!mln_ts_read_native_registration(call, {js}, {kind}, {count - 2}, fields, {path})) {fail};",
            *(
                f"  {target}.{field.name} = ({field.value.ctype.spelling})(uintptr_t)fields[{index}];"
                for index, field in enumerate(entry.native)
            ),
            "}",
        ]

    def list_writer(self, element: ValuePlan) -> str:
        """Reads a JavaScript array into C storage that lives for the call."""
        ctype = element.native
        name = f"mln_ts_write_list_{stem(ctype)}"
        if name in self.functions:
            return name
        prototype = f"bool {name}(mln_ts_call* call, napi_value value, size_t maximum, {ctype}** out, size_t* out_count, const char* path)"
        self.functions[name] = (prototype, "")
        read = self.init(element, "items[index]") + self.read(
            element, "element", "items[index]", "item_path", "return false"
        )
        lines = [
            f"  {ctype}* items = NULL;",
            "  if (!mln_ts_read_list(call, value, sizeof *items, maximum, (void**)&items, out_count, path)) return false;",
            "  for (size_t index = 0; index < *out_count; index++) {",
            "    napi_value element;",
            "    char item_path[256];",
            "    if (!mln_ts_element(call, value, index, &element)) return false;",
            "    mln_ts_index_path(item_path, sizeof item_path, path, index);",
            *[f"    {line}" for line in read],
            "  }",
            "  *out = items;",
            "  return true;",
        ]
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def count_maximum(self, count: ValuePlan) -> str:
        scalar = self.values.carrier(count)
        if scalar.kind == "int" and scalar.maximum < 0xFFFFFFFF:
            return str(scalar.maximum)
        return "UINT32_MAX"

    def read_list(
        self,
        value: ValuePlan,
        js: str,
        target: str,
        count: str,
        count_value: ValuePlan,
        path: str,
        fail: str,
    ) -> list[str]:
        element = value.element
        return [
            "{",
            f"  {element.native}* items = NULL;",
            "  size_t count = 0;",
            f"  if (!{self.list_writer(element)}(call, {js}, {self.count_maximum(count_value)}, &items, &count, {path})) {fail};",
            f"  {target} = items;",
            f"  {count} = ({count_value.ctype.spelling})count;",
            "}",
        ]

    def read_sized(
        self,
        value: ValuePlan,
        js: str,
        target: str,
        count: str,
        count_value: ValuePlan,
        path: str,
        fail: str,
    ) -> list[str]:
        bytes_ = "true" if value.encoding == "bytes" else "false"
        return [
            "{",
            "  const void* data = NULL;",
            "  size_t size = 0;",
            f"  if (!mln_ts_read_sized(call, {js}, {bytes_}, {self.count_maximum(count_value)}, &data, &size, {path})) {fail};",
            f"  {target} = data;",
            f"  {count} = ({count_value.ctype.spelling})size;",
            "}",
        ]

    def field_by_name(self, record: ValuePlan, name: str) -> FieldPlan:
        return next(f for f in record.fields if f.name == name)

    def writer(self, value: ValuePlan) -> str:
        name = f"mln_ts_write_{stem(value.native)}"
        if name in self.functions:
            return name
        prototype = f"bool {name}(mln_ts_call* call, napi_value value, {value.native}* out, const char* path)"
        self.functions[name] = (prototype, "")
        lines = ["  napi_value member;", "  char child[256];", "  (void)child;"]
        callbacks = ()
        if value.registration:
            entry = self.values.registration(value)
            callbacks = entry.callbacks
            if entry.form == "queue":
                lines += self.registration_callbacks(value, entry)
        for field in self.values.fields(value):
            if field in callbacks:
                continue
            key = identifier(field.name)
            lines.append(
                f"  if (!mln_ts_member(call, value, {c_string(key)}, &member)) return false;"
            )
            lines.append(f"  mln_ts_path(child, sizeof child, path, {c_string(key)});")
            target = f"out->{field.name}"
            if field.value.kind == "union":
                body = self.union_writer(value, field)
            elif field.value.kind == "array":
                count = field.value.length
                body = self.read_list(
                    field.value,
                    "member",
                    target,
                    f"out->{count}",
                    self.field_by_name(value, count).value,
                    "child",
                    "return false",
                )
            elif is_counted(field.value):
                count = field.value.length
                body = self.read_sized(
                    field.value,
                    "member",
                    target,
                    f"out->{count}",
                    self.field_by_name(value, count).value,
                    "child",
                    "return false",
                )
            else:
                body = []
                if (
                    field.value.kind == "record"
                    and not field.value.registration
                    and not value.default
                ):
                    body += self.init(field.value, target)
                body += self.read(
                    field.value, "member", target, "child", "return false"
                )
            if field.presence and field.presence.mask:
                body.append(f"out->{field.presence.mask} |= {field.presence.bit};")
            if self.values.omittable(value, field):
                lines.append("  if (!mln_ts_is_absent(call, member)) {")
                lines += [f"    {line}" for line in body]
                lines.append("  }")
            else:
                lines += [f"  {line}" for line in body]
        for flag in value.mask_flags:
            key = identifier(flag.member)
            lines += [
                f"  if (!mln_ts_member(call, value, {c_string(key)}, &member)) return false;",
                f"  mln_ts_path(child, sizeof child, path, {c_string(key)});",
                "  if (!mln_ts_is_absent(call, member)) {",
                "    bool flag = false;",
                "    if (!mln_ts_read_bool(call, member, &flag, child)) return false;",
                f"    if (flag) out->{flag.mask} |= {flag.name};",
                "  }",
            ]
        lines.append("  return true;")
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def register(
        self, value: ValuePlan, js: str, target: str, path: str, fail: str
    ) -> list[str]:
        """Registers a host value with the realm and names its queue in place
        of the registration's callbacks."""
        plan = value.registration
        return [
            f"if (!mln_ts_read_registration(call, {js}, &{target}.{plan.user_data}, &{target}.{plan.queue}, {path})) {fail};",
        ]

    def registration_callbacks(self, value: ValuePlan, entry) -> list[str]:
        """Checks an object's callbacks and registers it when the primary one
        is present.

        A queue replaces the primary callback, so its absence leaves the queue
        unset for native validation. Another absent callback drops the
        deliveries that would reach it.
        """
        primary = entry.callbacks[0].value.nullable
        lines = ["  bool present = true;"] if primary else []
        for index, field in enumerate(entry.callbacks):
            key = identifier(field.name)
            lines += [
                f"  if (!mln_ts_member(call, value, {c_string(key)}, &member)) return false;",
                f"  mln_ts_path(child, sizeof child, path, {c_string(key)});",
            ]
            check = "if (!mln_ts_read_function(call, member, child)) return false;"
            if not field.value.nullable:
                lines.append(f"  {check}")
                continue
            lines += [
                "  if (mln_ts_is_absent(call, member)) {",
                *(["    present = false;"] if index == 0 else []),
                "  } else {",
                f"    {check}",
                "  }",
            ]
        register = self.register(value, "value", "(*out)", "path", "return false")
        if not primary:
            return lines + [f"  {line}" for line in register]
        return lines + [
            "  if (present) {",
            *(f"    {line}" for line in register),
            "  }",
        ]

    def union_writer(self, record: ValuePlan, field: FieldPlan) -> list[str]:
        """Statements that read a `{kind, value}` object into a tagged union."""
        arms = {arm.name: arm for arm in field.value.fields}
        cases = union_kinds(field.value)
        kinds = ", ".join(c_string(kind) for _, kind, _ in cases)
        lines = [
            "if (!mln_ts_read_record(call, member, child)) return false;",
            "{",
            f"  static const char* const kinds[] = {{{kinds}}};",
            "  napi_value arm;",
            "  char arm_path[256];",
            "  (void)arm;",
            "  (void)arm_path;",
            f"  switch (mln_ts_read_kind(call, member, kinds, {len(cases)}, child)) {{",
        ]
        for index, (constant, _, name) in enumerate(cases):
            lines.append(f"    case {index}:")
            lines.append(f"      out->{field.presence.tag} = {constant};")
            if name is not None:
                arm = arms[name]
                target = f"out->{field.name}.{name}"
                read = (
                    self.init(arm.value, target) if arm.value.kind == "record" else []
                )
                read += self.read(arm.value, "arm", target, "arm_path", "return false")
                lines += [
                    '      if (!mln_ts_member(call, member, "value", &arm)) return false;',
                    '      mln_ts_path(arm_path, sizeof arm_path, child, "value");',
                    *[f"      {line}" for line in read],
                ]
            lines.append("      break;")
        lines += ["    default:", "      return false;", "  }", "}"]
        return lines

    # -- C to JavaScript ----------------------------------------------------

    def to_js(self, value: ValuePlan, source: str, size: str | None = None) -> str:
        """An expression that converts the C value `source` to a napi_value."""
        if value.kind in {"scalar", "enum", "handle", "native_pointer"}:
            if value.kind == "native_pointer":
                return f"mln_ts_u64(env, (uint64_t)(uintptr_t)({source}))"
            scalar = self.values.carrier(value)
            if scalar.kind == "bool":
                return f"mln_ts_bool(env, {source})"
            if scalar.kind in {"int", "double"}:
                return f"mln_ts_number(env, (double)({source}))"
            if scalar.kind == "u64":
                return f"mln_ts_u64(env, (uint64_t)({source}))"
            return f"mln_ts_i64(env, (int64_t)({source}))"
        if value.kind == "buffer":
            if value.buffer_form == "view":
                convert = "mln_ts_bytes" if value.encoding == "bytes" else "mln_ts_text"
                expression = f"{convert}(env, ({source}).data, ({source}).size)"
                if value.optional == "empty":
                    return (
                        f"(({source}).size == 0 ? mln_ts_undefined(env) : {expression})"
                    )
                if value.nullable:
                    return f"(({source}).data == NULL ? mln_ts_undefined(env) : {expression})"
                return expression
            expression = f"mln_ts_text(env, {source}, strlen({source}))"
            return f"(({source}) == NULL ? mln_ts_undefined(env) : {expression})"
        if value.kind == "reference":
            element = value.element
            if element.kind != "record":
                return self.to_js(element, f"(*{source})")
            return f"(({source}) == NULL ? mln_ts_undefined(env) : {self.reader(element)}(env, {source}, sizeof *({source})))"
        if value.kind == "record":
            return (
                f"{self.reader(value)}(env, &({source}), {size or f'sizeof({source})'})"
            )
        if value.kind == "array" and size is not None:
            return f"{self.items_reader(value.element)}(env, {source}, {size})"
        raise Unsupported(f"{value.native}: no C to JavaScript conversion")

    def items_reader(self, element: ValuePlan) -> str:
        """Converts `count` contiguous C elements to a JavaScript array."""
        ctype = element.native
        name = f"mln_ts_read_items_{stem(ctype)}"
        if name in self.functions:
            return name
        prototype = (
            f"napi_value {name}(napi_env env, const {ctype}* items, size_t count)"
        )
        self.functions[name] = (prototype, "")
        lines = [
            "  napi_value result = mln_ts_array(env, count);",
            "  if (result == NULL) return NULL;",
            "  for (size_t index = 0; index < count; index++) {",
            f"    napi_value element = {self.to_js(element, 'items[index]')};",
            "    if (element == NULL) return NULL;",
            "    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;",
            "  }",
            "  return result;",
        ]
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def reader(self, value: ValuePlan) -> str:
        name = f"mln_ts_read_{stem(value.native)}"
        if name in self.functions:
            return name
        prototype = f"napi_value {name}(napi_env env, const {value.native}* source, size_t size)"
        self.functions[name] = (prototype, "")
        lines = [
            "  (void)size;",
            "  napi_value result = mln_ts_object(env);",
            "  if (result == NULL) return NULL;",
        ]
        fields = (
            self.values.registration(value).members
            if value.registration
            else self.values.fields(value)
        )
        for field in fields:
            if field.value.registration or field.value.kind == "callback":
                # Native code never returns a callback; a copy leaves it unset.
                continue
            key = c_string(identifier(field.name))
            guard = None
            if field.value.kind == "union":
                expression = f"{self.union_reader(value, field)}(env, source, size)"
            elif field.value.kind == "array":
                expression = f"{self.array_reader(value, field)}(env, source)"
                if field.value.nullable:
                    guard = f"source->{field.name} != NULL"
            elif is_counted(field.value):
                convert = (
                    "mln_ts_bytes" if field.value.encoding == "bytes" else "mln_ts_text"
                )
                expression = f"{convert}(env, source->{field.name}, (size_t)source->{field.value.length})"
                if field.value.nullable or field.value.optional:
                    guard = f"source->{field.name} != NULL"
            else:
                expression = self.to_js(field.value, f"source->{field.name}")
            line = f"if (!mln_ts_set(env, result, {key}, {expression})) return NULL;"
            if field.presence and field.presence.mask:
                guard = f"(source->{field.presence.mask} & {field.presence.bit}) != 0"
            if guard:
                line = f"if ({guard}) {{ {line} }}"
            lines.append(f"  {line}")
        for flag in value.mask_flags:
            lines.append(
                f"  if (!mln_ts_set(env, result, {c_string(identifier(flag.member))}, mln_ts_bool(env, (source->{flag.mask} & {flag.name}) != 0))) return NULL;"
            )
        lines.append("  return result;")
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def union_reader(self, record: ValuePlan, field: FieldPlan) -> str:
        name = f"mln_ts_read_{stem(record.native)}_{field.name}"
        if name in self.functions:
            return name
        prototype = f"napi_value {name}(napi_env env, const {record.native}* source, size_t size)"
        tag = field.presence.tag
        lines = [
            "  napi_value result = mln_ts_object(env);",
            "  if (result == NULL) return NULL;",
            f"  switch (source->{tag}) {{",
        ]
        arms = {arm.name: arm for arm in field.value.fields}
        for constant, kind, arm in union_kinds(field.value):
            lines.append(f"    case {constant}:")
            lines.append(
                f'      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, {c_string(kind)}, {len(kind)}))) return NULL;'
            )
            if arm is not None:
                expression = self.to_js(arms[arm].value, f"source->{field.name}.{arm}")
                lines.append(
                    f'      if (!mln_ts_set(env, result, "value", {expression})) return NULL;'
                )
            lines.append("      return result;")
        lines += [
            "    default:",
            '      if (!mln_ts_set(env, result, "kind", mln_ts_text(env, "unknown", 7))) return NULL;',
            f'      if (!mln_ts_set(env, result, "tag", mln_ts_number(env, (double)source->{tag}))) return NULL;',
            "      // The bytes of the whole record, as far as its stride reaches.",
            '      if (!mln_ts_set(env, result, "bytes", mln_ts_bytes(env, source, size))) return NULL;',
            "      return result;",
            "  }",
        ]
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def array_reader(self, record: ValuePlan, field: FieldPlan) -> str:
        name = f"mln_ts_read_{stem(record.native)}_{field.name}"
        if name in self.functions:
            return name
        value = field.value
        element = value.element
        ctype = element.native
        prototype = f"napi_value {name}(napi_env env, const {record.native}* source)"
        stride = (
            f"(size_t)source->{value.stride}" if value.stride else f"sizeof({ctype})"
        )
        convert = (
            f"{self.reader(element)}(env, item, stride)"
            if element.kind == "record"
            else self.to_js(element, "(*item)")
        )
        lines = [
            f"  size_t count = (size_t)source->{value.length};",
            f"  size_t stride = {stride};",
            f"  if (count > 0 && stride < sizeof({ctype})) {{",
            '    napi_throw_range_error(env, NULL, "native record stride is too small");',
            "    return NULL;",
            "  }",
            "  napi_value result = mln_ts_array(env, count);",
            "  if (result == NULL) return NULL;",
            "  for (size_t index = 0; index < count; index++) {",
            f"    const {ctype}* item = (const {ctype}*)((const char*)source->{field.name} + index * stride);",
            f"    napi_value element = {convert};",
            "    if (element == NULL) return NULL;",
        ]
        lines += [
            "    if (!mln_ts_set_index(env, result, (uint32_t)index, element)) return NULL;",
            "  }",
            "  return result;",
        ]
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def decoder(self, result: ValuePlan) -> str:
        """A completion decoder for a function's result value."""
        element = result.element if result.kind == "array" else result
        if element.kind == "handle":
            ctype = "uint64_t"
        elif element.kind == "record" or element.buffer_form == "view":
            ctype = element.native
        elif element.kind in {"scalar", "enum"}:
            ctype = self.values.carrier(element).c
        else:
            raise Unsupported(f"{element.native}: completion value needs a decoder")
        name = f"mln_ts_decode_{stem(element.native)}" + (
            "_list" if result.kind == "array" else ""
        )
        if result.nullable or result.optional:
            name += "_optional"
        self.decoders.add(name)
        if name in self.functions:
            return name
        prototype = (
            f"napi_value {name}(napi_env env, const mln_completion_result* result)"
        )
        convert = self.to_js(element, f"*(const {ctype}*)value", "stride")
        if result.kind == "array":
            lines = [
                "  size_t stride = result->value_size;",
                f"  if (result->value_count > 0 && stride < sizeof({ctype})) {{",
                '    napi_throw_range_error(env, NULL, "native completion stride is too small");',
                "    return NULL;",
                "  }",
            ]
            if result.nullable:
                lines.append(
                    "  if (result->value == NULL) return mln_ts_undefined(env);"
                )
            lines += [
                "  napi_value array = mln_ts_array(env, result->value_count);",
                "  if (array == NULL) return NULL;",
                "  for (size_t index = 0; index < result->value_count; index++) {",
                "    const char* value = (const char*)result->value + index * stride;",
                f"    if (!mln_ts_set_index(env, array, (uint32_t)index, {convert})) return NULL;",
                "  }",
                "  return array;",
            ]
        else:
            absent = (
                "mln_ts_undefined(env)"
                if result.nullable or result.optional
                else "mln_ts_missing_value(env)"
            )
            lines = [
                f"  if (result->value == NULL || result->value_count == 0) return {absent};",
                f"  if (result->value_size < sizeof({ctype})) {{",
                '    napi_throw_range_error(env, NULL, "native completion stride is too small");',
                "    return NULL;",
                "  }",
                "  const char* value = (const char*)result->value;",
                "  size_t stride = result->value_size;",
                "  (void)stride;",
                f"  return {convert};",
            ]
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def decoder_key(self, name: str) -> str:
        """The member of the TypeScript `Decoder` table that names a
        completion decoder: its name without the runtime prefix."""
        return identifier(name.removeprefix("mln_ts_decode_"))

    def decoder_keys(self) -> list[tuple[str, int]]:
        """Each completion decoder's member and its index in the addon's
        decoder table."""
        return [
            (self.decoder_key(name), index)
            for index, name in enumerate(sorted(self.decoders))
        ]

    # -- Deliveries ---------------------------------------------------------

    def payload_reader(self, callbacks) -> str:
        """Converts the copied arguments of each drained delivery other than
        a completion, by its type, or returns undefined for a type that no
        registration of this binding receives."""
        name = "mln_ts_delivery_payload"
        prototype = f"napi_value {name}(napi_env env, const mln_delivery* delivery)"
        lines = ["  switch (delivery->type) {"]
        seen = set()
        for callback in sorted(callbacks, key=lambda c: c.delivery.constant):
            delivery = callback.delivery
            if delivery.constant in seen:
                continue
            seen.add(delivery.constant)
            record = self.bound.values[delivery.record]
            lines += [
                f"    case {delivery.constant}:",
                f"      return {self.reader(record)}(env, &delivery->payload.{delivery.payload_member}, sizeof delivery->payload.{delivery.payload_member});",
            ]
        lines += ["    default:", "      return mln_ts_undefined(env);", "  }"]
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def decoder_table(self) -> str:
        names = sorted(self.decoders)
        return (
            "const mln_ts_decoder mln_ts_decoders[] = {\n"
            + "".join(f"  {name},\n" for name in names)
            + ("" if names else "  NULL,\n")
            + "};\n"
            + f"const size_t mln_ts_decoder_count = {len(names)};\n"
        )

    def prototypes(self) -> list[tuple[str, str]]:
        return [self.functions[name] for name in sorted(self.functions)]

    def render_bodies(self) -> str:
        return (
            "".join(
                f"{prototype} {{\n{body}\n}}\n\n"
                for prototype, body in self.prototypes()
            )
            + self.decoder_table()
        )
