"""Render the addon's C converters between Node-API values and C values.

A writer reads one JavaScript object into a C record that the caller already
zeroed, with its sizes set, so a member that the object leaves out keeps its
zero default. A reader builds a
JavaScript object from a borrowed C record. A completion decoder reads the
value of a successful completion. Converters check only the shape that the
binding owns: the JavaScript type, the integer range, and NUL termination.
A JSON value converts through the realm's `JSON.stringify` and `JSON.parse`,
with `undefined` for an empty view.

A registration or a wake that JavaScript supplies names the realm's queue, and
each of its callbacks is a trampoline that runs while the realm dispatches the
queue. The trampoline converts the callback's arguments and passes them to the
realm's callback adapter that its constant numbers, which calls the host
value. A native registration, which a wake takes as an object in place of its
function, fills the descriptor with the function pointers, context, and
release of native code in the host.
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
        # The callback types whose trampolines the realm's adapters serve.
        self.callbacks: set[str] = set()

    def merge(self, other: NativeValues) -> None:
        self.functions.update(other.functions)
        self.callbacks |= other.callbacks

    # -- Initialization -----------------------------------------------------

    def init(self, value: ValuePlan, target: str) -> list[str]:
        """Statements that start a C record at its default: zero, with the size
        of the record and of each sized record that it embeds."""
        return [f"memset(&{target}, 0, sizeof {target});", *self.sizes(value, target)]

    def sizes(self, value: ValuePlan, target: str) -> list[str]:
        if value.kind != "record":
            return []
        lines = []
        for field in value.fields:
            if field.role == "size":
                lines.append(f"{target}.{field.name} = sizeof {target};")
            elif field.value.kind == "record":
                lines += self.sizes(field.value, f"{target}.{field.name}")
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
            # An enum value converts to its C enum or flag set type, which
            # holds the carrier's range.
            ctype = (
                value.ctype.declaration
                if value.kind == "enum" and value.ctype.declaration
                else scalar.c
            )
            if scalar.kind == "int":
                return [
                    f"{{ int64_t number = 0; if (!mln_ts_read_int(call, {js}, {scalar.minimum}LL, {scalar.maximum}LL, &number, {path})) {fail}; {target} = ({ctype})number; }}"
                ]
            if scalar.kind == "double":
                return [
                    f"{{ double number = 0; if (!mln_ts_read_double(call, {js}, &number, {path})) {fail}; {target} = ({ctype})number; }}"
                ]
            reader = "mln_ts_read_u64" if scalar.kind == "u64" else "mln_ts_read_i64"
            carrier = "uint64_t" if scalar.kind == "u64" else "int64_t"
            return [
                f"{{ {carrier} number = 0; if (!{reader}(call, {js}, &number, {path})) {fail}; {target} = ({ctype})number; }}"
            ]
        if value.kind == "buffer":
            if value.buffer_form == "view":
                reader = {
                    "bytes": "mln_ts_read_bytes_view",
                    "json": "mln_ts_read_json_view",
                }.get(value.encoding, "mln_ts_read_text_view")
                return [f"if (!{reader}(call, {js}, &{target}, {path})) {fail};"]
            return [
                f"{{ const char* text = NULL; if (!mln_ts_read_text(call, {js}, &text, {path})) {fail}; {target} = text; }}"
            ]
        if value.kind == "reference" and value.unwrapped.kind == "buffer":
            return self.read(value.unwrapped, js, target, path, fail)
        if value.kind == "record" and value.registration:
            entry = self.values.registration(value)
            if entry.form == "native":
                return self.read_native(value, entry, js, target, path, fail)
            if entry.form == "wake":
                lines = [
                    f"if (!mln_ts_is_absent(call, {js})) {{",
                    f"  if (!mln_ts_read_function(call, {js}, {path})) {fail};",
                    *(f"  {line}" for line in self.register(value, js, target, fail)),
                    f"  {self.assign(entry.callbacks[0], target)}",
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
                    *self.register(value, js, target, fail),
                    self.assign(entry.callbacks[0], target),
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
        assert element is not None
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
        if value.union_member:
            # A record of only a union is the selected variant itself.
            lines = [
                f"  {line}"
                for line in self.union_writer(
                    value, value.union_member, "value", "path"
                )
            ]
            lines.append("  return true;")
            self.functions[name] = (prototype, "\n".join(lines))
            return name
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
                body = self.union_writer(value, field, "member", "child")
            elif field.value.kind == "array":
                count = field.value.length
                assert count is not None
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
                assert count is not None
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
                if field.value.kind == "record" and not field.value.registration:
                    body += self.init(field.value, target)
                body += self.read(
                    field.value, "member", target, "child", "return false"
                )
            if field.presence and field.presence.mask:
                body.append(f"out->{field.presence.mask} |= {field.presence.bit};")
            # A JSON reader takes undefined as absent itself, and null as JSON.
            json = field.value.kind == "buffer" and field.value.encoding == "json"
            if self.values.omittable(value, field) and not json:
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

    def register(self, value: ValuePlan, js: str, target: str, fail: str) -> list[str]:
        """Registers a host value with the realm, and names the realm's queue
        and the value's user data and release in the registration."""
        plan = value.registration
        assert plan is not None
        return [
            f"if (!mln_ts_read_registration(call, {js}, &{target}.{plan.user_data}, &{target}.{plan.release}, &{target}.{plan.queue})) {fail};",
        ]

    def assign(self, field: FieldPlan, target: str) -> str:
        """Sets a callback field to the trampoline of its callback type."""
        callback = self.bound.callbacks[field.value.native]
        return f"{target}.{field.name} = {self.trampoline(callback)};"

    def registration_callbacks(self, value: ValuePlan, entry) -> list[str]:
        """Checks an object's callbacks, registers it when the primary one is
        present, and sets the trampoline of each present callback.

        An absent primary callback leaves the registration unset for native
        validation.
        """
        lines = []
        present = {}
        for field in entry.callbacks:
            key = identifier(field.name)
            lines += [
                f"  if (!mln_ts_member(call, value, {c_string(key)}, &member)) return false;",
                f"  mln_ts_path(child, sizeof child, path, {c_string(key)});",
            ]
            if not field.value.nullable:
                lines.append(
                    "  if (!mln_ts_read_function(call, member, child)) return false;"
                )
                continue
            flag = present[field.name] = f"has_{field.name}"
            lines += [
                f"  bool {flag} = !mln_ts_is_absent(call, member);",
                f"  if ({flag} && !mln_ts_read_function(call, member, child)) return false;",
            ]
        body = self.register(value, "value", "(*out)", "return false")
        for field in entry.callbacks:
            assign = self.assign(field, "(*out)")
            flag = present.get(field.name)
            body.append(f"if ({flag}) {assign}" if flag else assign)
        primary = present.get(entry.callbacks[0].name)
        if primary is None:
            return lines + [f"  {line}" for line in body]
        return lines + [
            f"  if ({primary}) {{",
            *(f"    {line}" for line in body),
            "  }",
        ]

    def trampoline(self, callback) -> str:
        """The function that native code calls during dispatch for one
        callback type: it converts the arguments and passes them to the realm's
        adapter for the type. A handle that the call owns and no host value
        received is released, so native code never waits on it."""
        name = f"mln_ts_dispatch_{stem(callback.native)}"
        if name in self.functions:
            return name
        self.callbacks.add(callback.native)
        context = callback.context
        assert context is not None
        returns = callback.result.ctype.spelling
        parameters = ", ".join(
            f"{p.value.ctype.spelling} {p.name}" for p in callback.parameters
        )
        prototype = f"{returns} {name}({parameters})"
        self.functions[name] = (prototype, "")
        arguments = callback.arguments
        owned = [
            p
            for p in arguments
            if p.value.kind == "handle" and p.value.ownership == "owned"
        ]
        lines = [
            *(
                f"  (void){p.name};"
                for p in callback.parameters
                if p.name == callback.borrowed_owner
            ),
            "  napi_handle_scope scope;",
            f"  napi_env env = mln_ts_upcall_begin({context}, &scope);",
            "  bool received = false;",
            "  if (env != NULL) {",
        ]
        constant = self.callback_id(callback.native)
        if arguments:
            converted = ", ".join(self.to_js(p.value, p.name) for p in arguments)
            lines += [
                f"    napi_value argv[{len(arguments)}] = {{{converted}}};",
                f"    received = mln_ts_upcall_end({context}, scope, {constant}, {len(arguments)}, argv);",
            ]
        else:
            lines.append(
                f"    received = mln_ts_upcall_end({context}, scope, {constant}, 0, NULL);"
            )
        lines.append("  }")
        if owned:
            lines.append("  if (!received) {")
            for parameter in owned:
                handle = self.values.argument_handle(parameter)
                cleanup = self.bound.source.functions_by_name[handle.cleanup]
                extra = len(cleanup.parameters) - 1 + bool(cleanup.diagnostic)
                release = ", ".join([parameter.name, *["NULL"] * extra])
                lines.append(f"    {cleanup.name}({release});")
            lines.append("  }")
        else:
            lines.append("  (void)received;")
        if callback.result.native != "void":
            # Native code answers a dispatched call itself and ignores what
            # the trampoline returns.
            if not callback.queue_result:
                raise Unsupported(f"{callback.native}: a result needs a queue result")
            lines.append(f"  return {callback.queue_result};")
        self.functions[name] = (prototype, "\n".join(lines))
        return name

    def callback_id(self, native: str) -> str:
        """The constant that names a callback type's adapter."""
        return f"MLN_TS_CALLBACK_{stem(native).upper()}"

    def union_writer(
        self, record: ValuePlan, field: FieldPlan, js: str, path: str
    ) -> list[str]:
        """Statements that read the `{kind, value}` object `js` into a tagged
        union. A kind outside the union's arms fails."""
        assert field.presence is not None
        arms = {arm.name: arm for arm in field.value.fields}
        cases = union_kinds(field.value)
        kinds = ", ".join(c_string(kind) for _, kind, _ in cases)
        lines = [
            f"if (!mln_ts_read_record(call, {js}, {path})) return false;",
            "{",
            f"  static const char* const kinds[] = {{{kinds}}};",
            "  napi_value arm;",
            "  char arm_path[256];",
            "  (void)arm;",
            "  (void)arm_path;",
            f"  switch (mln_ts_read_kind(call, {js}, kinds, {len(cases)}, {path})) {{",
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
                    f'      if (!mln_ts_member(call, {js}, "value", &arm)) return false;',
                    f'      mln_ts_path(arm_path, sizeof arm_path, {path}, "value");',
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
                convert = {"bytes": "mln_ts_bytes", "json": "mln_ts_json"}.get(
                    value.encoding, "mln_ts_text"
                )
                expression = f"{convert}(env, ({source}).data, ({source}).size)"
                if value.nullable:
                    return f"(({source}).data == NULL ? mln_ts_undefined(env) : {expression})"
                return expression
            expression = f"mln_ts_text(env, {source}, strlen({source}))"
            return f"(({source}) == NULL ? mln_ts_undefined(env) : {expression})"
        if value.kind == "reference":
            element = value.unwrapped
            if element.kind != "record":
                return self.to_js(element, f"(*{source})")
            return f"(({source}) == NULL ? mln_ts_undefined(env) : {self.reader(element)}(env, {source}, sizeof *({source})))"
        if value.kind == "record":
            return (
                f"{self.reader(value)}(env, &({source}), {size or f'sizeof({source})'})"
            )
        if value.kind == "array" and value.element and size is not None:
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
        if value.native in self.values.bound.slices:
            # A slice record converts to its array alone.
            array = self.array_reader(value, self.values.bound.slices[value.native])
            body = f"  (void)size;\n  return {array}(env, source);"
            self.functions[name] = (prototype, body)
            return name
        if value.union_member:
            # A record of only a union converts to the selected variant.
            union = self.union_reader(value, value.union_member)
            self.functions[name] = (prototype, f"  return {union}(env, source, size);")
            return name
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
                if field.value.nullable:
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
        assert field.presence is not None
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
        assert element is not None
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
        assert element is not None
        if element.kind == "handle":
            ctype = "uint64_t"
        elif element.kind == "record" or element.buffer_form == "view":
            ctype = element.native
        elif element.kind in {"scalar", "enum"}:
            ctype = self.values.carrier(element).c
        else:
            raise Unsupported(f"{element.native}: completion value needs a decoder")
        name = (
            f"mln_ts_decode_{stem(element.native)}"
            + (
                "_json"
                if element.kind == "buffer" and element.encoding == "json"
                else ""
            )
            + ("_list" if result.kind == "array" else "")
        )
        if result.nullable:
            name += "_optional"
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
                if result.nullable
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

    def callback_ids(self) -> str:
        """The constants that name the callback adapters of the realm, which
        the TypeScript half numbers alike."""
        if not self.callbacks:
            return ""
        return (
            "/** The realm's callback adapters, by the callback type that each\n"
            " * serves. */\n"
            "enum {\n"
            + "".join(
                f"  {self.callback_id(native)} = {index},\n"
                for index, native in enumerate(sorted(self.callbacks))
            )
            + "};\n"
        )

    def prototypes(self) -> list[tuple[str, str]]:
        return [self.functions[name] for name in sorted(self.functions)]

    def render_bodies(self) -> str:
        return "".join(
            f"{prototype} {{\n{body}\n}}\n\n" for prototype, body in self.prototypes()
        )
