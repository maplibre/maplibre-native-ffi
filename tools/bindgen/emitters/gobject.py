"""Generate the GObject library's C sources.

The library is C with introspection annotations. `g-ir-scanner` reads the
headers that this emitter writes into the `.gir` and `.typelib`, and `vapigen`
reads the `.gir` into the `.vapi`, so these annotations decide what GJS,
PyGObject, Vala, and every other GObject consumer sees.

A `GError` domain carries `mln_status`, whose codes are the raw status values,
so a status that these headers do not name survives. Each enum is a `GEnum`
and each bitmask a `GFlags`, whose values the C compiler checks against the C
constants. Each value is a boxed type, as `gobject_values` describes.

A handle is a final `GObject` class. A handle whose release waits has
`release_async` and `release_finish`, and its object's `dispose` calls the C
dispose unless the handle was released. Any other handle's object releases it
when disposed, unless its `release` already did. A handle that a call creates
at once and readies later, such as a map, has `ready_async` and `ready_finish`.

Each command, query, and operation is an `_async` and `_finish` pair, which
finishes on the thread-default main context of the thread that started it. A
command started without a callback passes no completion. Every other call is
synchronous, and fails with a `GError`.

A function that serves only some of the library's platforms compiles only for
them.
"""

from __future__ import annotations

import textwrap

from tools.bindgen import async_hosts, docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Api, Lowered, Unsupported
from tools.bindgen.protocol import (
    COMPLETION_RESULT,
    RUNTIME_EVENT_SOURCE,
    RUNTIME_EVENT_SOURCE_PREFIX,
    STATUS,
    is_completion,
)
from tools.bindgen.semantic import (
    BoundApi,
    DisposeSupport,
    FieldPlan,
    HandlePlan,
    OperationPlan,
    ParameterPlan,
    ValuePlan,
    enum_member_prefix,
)

from . import gobject_values
from .gobject_values import (
    CALL,
    HEADER,
    MACRO,
    SOURCE,
    SYMBOL,
    THREADS,
    Function,
    Values,
    accessor,
    cast_macro,
    check_macro,
    comment,
    constant_name,
    define_name,
    fail_value,
    guarded,
    symbol_name,
    type_macro,
    type_name,
    without_const,
    wrap,
)

DIRECTORY = gobject_values.DIRECTORY
# The library builds for the platforms whose GTK it serves.
TARGETS = frozenset({"linux", "macos"})
PLATFORM_CONDITIONS = {"linux": "defined(__linux__)", "macos": "defined(__APPLE__)"}
GINT_MIN, GINT_MAX, GUINT_MAX = -(2**31), 2**31 - 1, 2**32 - 1
# The names that a generated function's own parameters take, which a C
# parameter of the same name gives up.
RESERVED = frozenset({"self", "callback", "user_data", "error", "result"})


def nick(value: ValuePlan, member: str) -> str:
    prefix = enum_member_prefix([name for name, _ in value.enum_values], value.native)
    return member.removeprefix(prefix).lower().replace("_", "-")


def flags(value: ValuePlan) -> bool:
    return value.enum_kind == "bitmask"


def error_codes(value: ValuePlan) -> list[tuple[str, int]]:
    """The status constants that a `GError` carries: every one but success."""
    return [(name, number) for name, number in value.enum_values if number != 0]


def enum_values(value: ValuePlan) -> list[tuple[str, int]]:
    return error_codes(value) if value.native == STATUS else list(value.enum_values)


def receiver_type(plan: OperationPlan) -> str | None:
    """The handle type that an operation is a method of, or None."""
    receiver = next((p for p in plan.inputs if p.name == plan.receiver), None)
    if receiver is None or receiver.value.kind != "handle":
        return None
    return receiver.value.native


def method_name(plan: OperationPlan) -> str:
    """A handle's method, the factory that creates one, or a function."""
    owner = receiver_type(plan) or plan.factory
    if owner is not None:
        return f"{symbol_name(owner)}_{plan.member}"
    return f"{SYMBOL}_{plan.member}"


def guard(plan: OperationPlan) -> str | None:
    """The preprocessor condition of an operation that serves only some of
    the library's platforms."""
    platforms = served(plan.function, TARGETS)
    if platforms is None:
        return None
    return " || ".join(PLATFORM_CONDITIONS[p] for p in sorted(platforms))


def parameter_name(name: str) -> str:
    return f"{name}_value" if name in RESERVED else name


class Call:
    """One operation's C call as it is built: the public parameters, the
    statements that convert them, and the native arguments by C name."""

    def __init__(self, plan: OperationPlan) -> None:
        self.plan = plan
        self.parameters: list[tuple[str, str]] = []
        self.annotations: dict[str, str] = {}
        self.checks = ""
        self.setup = ""
        self.arguments: dict[str, str] = {}
        self.uses_call = False

    def native_call(self, extra: dict[str, str] | None = None) -> str:
        arguments = {**self.arguments, **(extra or {})}
        names = [p.name for p in self.plan.function.parameters]
        missing = [n for n in names if n not in arguments]
        if missing:
            raise Unsupported(
                f"GObject: {self.plan.name}: no argument for {', '.join(missing)}"
            )
        args = [arguments[n] for n in names]
        if self.plan.function.diagnostic:
            args.append("&diagnostic_")
        return f"{self.plan.name}({', '.join(args)})"


class Emitter:
    def __init__(self, bound: BoundApi) -> None:
        self.bound = bound
        self.symbol = symbols(bound)
        self.values = Values(bound, self.symbol)
        self.generated: list[str] = []
        self.support: list[str] = []
        self.unsupported: dict[str, str] = {}
        # The functions of each handle, and of the module, by owner.
        self.functions: dict[str | None, list[Function]] = {}
        self.statics: list[str] = []
        self.handle_names: list[str] = []

    def doc_lines(self, native: str, extra: tuple[str, ...] = ()) -> list[str]:
        return self.values.doc_lines(native, extra)

    # Enums and flags.

    def enums(self) -> dict[str, str]:
        values = sorted(
            (v for v in self.bound.public_values.values() if v.kind == "enum"),
            key=lambda v: (v.native != STATUS, v.native),
        )
        header, source = [], []
        for value in values:
            header.append(self.enum_declaration(value))
            source.append(self.enum_definition(value))
        guard_ = f"{MACRO}_ENUMS_H"
        return {
            f"{SOURCE}/maplibre-native-ffi-enums.h": HEADER
            + f"#ifndef {guard_}\n#define {guard_}\n\n"
            + "#include <glib-object.h>\n\nG_BEGIN_DECLS\n\n"
            + "\n".join(header)
            + "\nG_END_DECLS\n\n#endif\n",
            f"{SOURCE}/maplibre-native-ffi-enums.c": HEADER
            + '#include "maplibre-native-ffi-enums.h"\n\n'
            + "#include <maplibre_native_c.h>\n\n"
            + "\n".join(source),
        }

    def fits(self, value: ValuePlan) -> bool:
        """Whether every constant of an enum fits a `GEnum`, or of a bitmask
        a `GFlags`."""
        low, high = (0, GUINT_MAX) if flags(value) else (GINT_MIN, GINT_MAX)
        return all(low <= number <= high for _, number in enum_values(value))

    def enum_declaration(self, value: ValuePlan) -> str:
        name = type_name(value.native)
        members = enum_values(value)
        if not self.fits(value):
            return self.wide_enum_declaration(value)
        out = ""
        if value.native == STATUS:
            out += comment(
                [
                    f"{MACRO}_ERROR:",
                    "",
                    *textwrap.wrap(
                        "The error domain of the library's failures. Each code is"
                        f" a failing status, which `{name}` names, and a code"
                        " that it doesn't name is a status that a later version"
                        " of the native library added.",
                        docs.WIDTH - 3,
                    ),
                ]
            )
            out += f"#define {MACRO}_ERROR ({SYMBOL}_error_quark())\n"
            out += f"GQuark {SYMBOL}_error_quark(void);\n\n"
        lines = [f"{name}:"]
        for member, _ in members:
            text = self.values.summary(member)
            lines += wrap(f"@{constant_name(value, member)}: {text}".rstrip())
        lines += ["", *self.doc_lines(value.native)]
        out += comment(lines)
        out += "typedef enum {\n"
        for member, number in members:
            literal = f"0x{number:08x}u" if number > GINT_MAX else str(number)
            out += f"  {constant_name(value, member)} = {literal},\n"
        out += f"}} {name};\n\n"
        out += f"GType {symbol_name(value.native)}_get_type(void) G_GNUC_CONST;\n"
        out += f"#define {type_macro(value.native)} ({symbol_name(value.native)}_get_type())\n"
        return out

    def wide_enum_declaration(self, value: ValuePlan) -> str:
        """An enum whose constants a `GEnum` or `GFlags` can't hold: an
        integer type of the enum's width, and a constant for each value."""
        name = type_name(value.native)
        carrier = self.values.gtype(
            ValuePlan(
                kind="scalar",
                native=value.scalar_carrier or "uint64_t",
                ctype=value.enum_underlying or value.ctype,
                scalar_carrier=value.scalar_carrier,
            )
        )
        out = comment([f"{name}:", "", *self.doc_lines(value.native)])
        out += f"typedef {carrier} {name};\n\n"
        for member, number in enum_values(value):
            constant = constant_name(value, member)
            literal = (
                f"G_GUINT64_CONSTANT({number})"
                if number >= 0
                else f"G_GINT64_CONSTANT({number})"
            )
            out += comment([f"{constant}:", "", *self.doc_lines(member)])
            out += f"#define {constant} (({name}){literal})\n\n"
        return out

    def enum_definition(self, value: ValuePlan) -> str:
        name = type_name(value.native)
        members = enum_values(value)
        out = ""
        if value.native == STATUS:
            # G_DEFINE_QUARK would stringify the name that the formatter spaces.
            out += (
                f"GQuark {SYMBOL}_error_quark(void) {{\n"
                '  return g_quark_from_static_string("maplibre-native-ffi-error-quark");\n'
                "}\n\n"
            )
        out += "".join(
            f"G_STATIC_ASSERT((gint64){constant_name(value, member)} == (gint64){member});\n"
            for member, _ in members
        )
        if not self.fits(value):
            return out
        kind = "FLAGS" if flags(value) else "ENUM"
        entries = ",\n".join(
            f'  G_DEFINE_ENUM_VALUE({constant_name(value, member)}, "{nick(value, member)}")'
            for member, _ in members
        )
        out += f"\nG_DEFINE_{kind}_TYPE(\n  {name}, {symbol_name(value.native)},\n{entries}\n)\n"
        return out

    # Constants.

    def constants(self) -> tuple[str, str]:
        header, source = "", ""
        for constant in self.bound.public_constants:
            name = define_name(constant)
            if constant.value > GINT_MAX or constant.value < GINT_MIN:
                literal = (
                    f"G_GUINT64_CONSTANT({constant.value})"
                    if constant.value >= 0
                    else f"G_GINT64_CONSTANT({constant.value})"
                )
            else:
                literal = str(constant.value)
            header += comment([f"{name}:", "", *self.doc_lines(constant.name)])
            header += f"#define {name} {literal}\n\n"
            source += f"G_STATIC_ASSERT({name} == {constant.name});\n"
        return header, source

    # Operations.

    def lower_operations(self) -> None:
        registrations = []
        for plan in self.bound.operations:
            # A call on a callback response has no caller unless a host
            # callback receives the response.
            if plan.scoped_receiver:
                continue
            try:
                for parameter in (*plan.inputs, *plan.outputs):
                    self.values.use(parameter.value)
                self.values.use(plan.result)
            except Unsupported as error:
                self.unsupported[plan.name] = f"{plan.function.location}: {error}"
        for value in self.values.used.values():
            if value.registration:
                registrations.append(value)
        skipped = async_hosts.rule_answered_operations(self.bound, registrations)
        for plan in self.bound.operations:
            if plan.name in skipped:
                continue
            try:
                self.lower(plan)
            except Unsupported as error:
                self.unsupported[plan.name] = f"{plan.function.location}: {error}"

    def owner(self, plan: OperationPlan) -> str | None:
        return receiver_type(plan) or plan.factory

    def add(self, plan: OperationPlan, *functions: Function) -> None:
        condition = guard(plan)
        for function in functions:
            function.guard = condition
        self.functions.setdefault(self.owner(plan), []).extend(functions)
        self.generated.append(plan.name)

    def lower(self, plan: OperationPlan) -> None:
        if plan.scoped_receiver:
            raise Unsupported("a call on a callback response")
        if isinstance(plan.support, DisposeSupport):
            self.add(plan, self.dispose(plan.support.handle, plan))
            return
        if plan.support is not None:
            self.support.append(plan.name)
            return
        match plan.shape:
            case "discard":
                # The object's dispose runs it for a frame that its owner
                # dropped.
                self.generated.append(plan.name)
            case "release_async":
                self.add(plan, *self.release_async(plan))
            case "release":
                self.add(plan, self.release(plan))
            case "attach":
                self.add(plan, self.attach(plan))
            case "command" | "query" | "run":
                self.add(plan, *self.asynchronous(plan))
            case "drain":
                self.add(plan, self.drain(plan))
            case "view":
                self.add(plan, self.immediate(plan, view=True))
            case _:
                self.add(plan, self.immediate(plan))

    # Inputs.

    def receiver(self, call: Call, fail: str) -> None:
        plan = call.plan
        owner = receiver_type(plan)
        if owner is None:
            return
        handle = self.bound.handles[owner]
        call.parameters.append((f"{type_name(owner)}*", "self"))
        call.checks += f"  g_return_val_if_fail({check_macro(owner)}(self), {fail});\n"
        live = "self->handle"
        if plan.after_release:
            live = "(self->handle != MLN_HANDLE_NULL ? self->handle : self->released_handle)"
        assert plan.receiver is not None
        call.arguments[plan.receiver] = live
        del handle

    def inputs(self, call: Call, fail: str) -> None:
        plan = call.plan
        sizes = {
            p.value.length
            for p in (*plan.inputs, *plan.outputs)
            if p.value.length and p.value.kind == "buffer"
        }
        counted = {
            p.value.length: p
            for p in plan.inputs
            if p.value.kind == "array" and p.value.length
        }
        for parameter in plan.inputs:
            if parameter.name == plan.receiver and receiver_type(plan):
                continue
            if parameter.name in sizes:
                continue
            if parameter.name in counted:
                continue
            self.input(call, parameter, fail, counted)

    def input(
        self,
        call: Call,
        parameter: ParameterPlan,
        fail: str,
        counted: dict[str, ParameterPlan],
    ) -> None:
        values = self.values
        value = parameter.value
        native = parameter.name
        name = parameter_name(native)
        kind = value.kind
        nullable = " (nullable)" if value.nullable else ""
        if kind == "handle":
            assert value.handle is not None
            call.parameters.append((f"{type_name(value.native)}*", name))
            if value.nullable:
                call.annotations[name] = "(nullable): the handle, or %NULL"
            else:
                call.checks += f"  g_return_val_if_fail({check_macro(value.native)}({name}), {fail});\n"
            call.arguments[native] = (
                f"({name} != NULL ? {name}->handle : MLN_HANDLE_NULL)"
            )
            return
        if kind in {"scalar", "enum", "native_pointer"}:
            call.parameters.append((values.gtype(value), name))
            call.arguments[native] = values.to_c(value, name)
            return
        if values.text(value) or values.bytes_(value):
            text = values.text(value)
            call.parameters.append(("const char*" if text else "GBytes*", name))
            call.annotations[name] = (
                f"{nullable.strip()}: the UTF-8 text".lstrip(": ")
                if text
                else f"{nullable.strip()}: the bytes".lstrip(": ")
            )
            view = (
                f"maplibre_native_ffi_text_view({name})"
                if text
                else f"maplibre_native_ffi_bytes_view({name})"
            )
            if value.buffer_form == "view":
                call.arguments[native] = view
            elif value.length is not None:
                local = f"{name}_view_"
                call.setup += f"  mln_buffer_view {local} = {view};\n"
                call.arguments[native] = f"{local}.data"
                call.arguments[value.length] = f"{local}.size"
            else:
                call.arguments[native] = name
            return
        if kind == "array":
            self.array_input(call, parameter, name, counted)
            return
        if kind in {"record", "reference"}:
            record = value.unwrapped
            if record.kind not in {"record", "union"}:
                raise Unsupported(f"{native}: a reference to a {record.kind}")
            type_ = type_name(record.native)
            prefix = symbol_name(record.native)
            call.parameters.append((f"const {type_}*", name))
            local = f"{name}_native_"
            call.setup += f"  {record.native} {local};\n"
            call.uses_call = True
            if value.nullable:
                call.annotations[name] = "(nullable): the value, or %NULL"
                call.setup += f"  if ({name} != NULL) {prefix}_to_native({name}, &{local}, &{CALL});\n"
                call.arguments[native] = (
                    f"({name} != NULL ? &{local} : NULL)"
                    if kind == "reference"
                    else local
                )
            else:
                call.checks += f"  g_return_val_if_fail({name} != NULL, {fail});\n"
                call.setup += f"  {prefix}_to_native({name}, &{local}, &{CALL});\n"
                call.arguments[native] = f"&{local}" if kind == "reference" else local
            return
        raise Unsupported(f"{native}: a {kind} input")

    def array_input(
        self,
        call: Call,
        parameter: ParameterPlan,
        name: str,
        counted: dict[str, ParameterPlan],
    ) -> None:
        values = self.values
        value = parameter.value
        element = value.element
        assert element is not None
        native = parameter.name
        nullable = " (nullable)" if value.nullable else ""
        local = f"{name}_native_"
        call.uses_call = True
        if values.text(element) and element.buffer_form == "view":
            call.parameters.append(("const char* const*", name))
            call.annotations[name] = f"(array zero-terminated=1){nullable}: the texts"
            count = f"{name}_count_"
            call.setup += (
                f"  guint {count} = {name} != NULL ? g_strv_length((char**){name}) : 0;\n"
                f"  mln_buffer_view* {local} = maplibre_native_ffi_call_alloc(&{CALL}, {count}, sizeof *{local});\n"
                f"  for (guint index = 0; index < {count}; index++) {{\n"
                f"    {local}[index] = maplibre_native_ffi_text_view({name}[index]);\n  }}\n"
            )
            call.arguments[native] = local
            if value.length:
                call.arguments[value.length] = count
            return
        if value.length is None or value.length not in {
            p.name for p in call.plan.inputs
        }:
            raise Unsupported(f"{native}: an array without a count input")
        length = next(p for p in call.plan.inputs if p.name == value.length)
        count = parameter_name(length.name)
        count_type = values.gtype(length.value)
        if element.kind == "record":
            type_ = type_name(element.native)
            prefix = symbol_name(element.native)
            call.parameters.append((f"{type_}* const*", name))
            call.parameters.append((count_type, count))
            call.annotations[name] = f"(array length={count}){nullable}: the items"
            call.setup += (
                f"  {element.native}* {local} = maplibre_native_ffi_call_alloc(&{CALL}, {count}, sizeof *{local});\n"
                f"  for (gsize index = 0; index < (gsize){count}; index++) {{\n"
                f"    {prefix}_to_native({name}[index], &{local}[index], &{CALL});\n  }}\n"
            )
        elif element.kind in {"scalar", "enum"}:
            call.parameters.append((f"const {values.gtype(element)}*", name))
            call.parameters.append((count_type, count))
            call.annotations[name] = f"(array length={count}){nullable}: the items"
            spelled = without_const(ctype_of(element))
            call.setup += (
                f"  {spelled}* {local} = maplibre_native_ffi_call_alloc(&{CALL}, {count}, sizeof *{local});\n"
                f"  for (gsize index = 0; index < (gsize){count}; index++) {{\n"
                f"    {local}[index] = {values.to_c(element, f'{name}[index]')};\n  }}\n"
            )
        else:
            raise Unsupported(f"{native}: an array of {element.kind}")
        call.arguments[native] = local
        call.arguments[length.name] = values.to_c(length.value, count)

    # Outputs.

    def outputs(self, call: Call) -> list[tuple[str, str, str, str]]:
        """Declare each output's storage and return what the call returns:
        each output's type, its expression after the call, its return
        annotation, and the statements that free it on a later failure."""
        plan = call.plan
        values = self.values
        returned = []
        owned = {o.parameter: o for o in plan.owned_outputs}
        lengths = {p.result_length for p in plan.outputs if p.result_length}
        for output in plan.outputs:
            if output.name in lengths:
                continue
            value = output.value
            local = f"{output.name}_"
            if output.name in owned:
                handle = owned[output.name].handle
                parent = owned[output.name].parent_parameter
                parent_object = "NULL"
                if parent is not None:
                    parent_object = (
                        "self" if parent == plan.receiver else parameter_name(parent)
                    )
                call.setup += f"  {handle.native} {local} = MLN_HANDLE_NULL;\n"
                call.arguments[output.name] = f"&{local}"
                ready = "ready_" if plan.shape == "attach" else "NULL"
                wrapped = f"{symbol_name(handle.native)}_wrap({local}, {parent_object}, FALSE, {ready})"
                nullable = (
                    plan.absence is not None and plan.absence.output.name == output.name
                )
                if nullable:
                    wrapped = f"({local} != MLN_HANDLE_NULL ? {wrapped} : NULL)"
                returned.append(
                    (
                        f"{type_name(handle.native)}*",
                        wrapped,
                        "(transfer full)"
                        + (" (nullable)" if nullable else "")
                        + ": the object",
                        "",
                    )
                )
                continue
            if output.result_length:
                returned.append(
                    ("char*", "text_value_", "(transfer full): the text", "")
                )
                continue
            if value.kind == "array":
                element = value.element
                assert element is not None
                if output.sized_by is None:
                    raise Unsupported(f"{output.name}: an output array without a size")
                count = call.arguments[value.length] if value.length else None
                if count is None:
                    raise Unsupported(f"{output.name}: an output array without a count")
                count_local = f"{output.name}_count_"
                call.setup += f"  gsize {count_local} = (gsize)({count});\n"
                spelled = (
                    element.native
                    if element.kind == "record"
                    else without_const(ctype_of(element))
                )
                call.setup += f"  {spelled}* {local} = maplibre_native_ffi_call_alloc(&{CALL}, {count_local}, sizeof *{local});\n"
                call.uses_call = True
                call.arguments[output.name] = local
                if element.kind == "record":
                    prefix = symbol_name(element.native)
                    expression = f"{output.name}_items_"
                    convert = (
                        f"  GPtrArray* {expression} = g_ptr_array_new_full((guint){count_local}, (GDestroyNotify){prefix}_free);\n"
                        f"  for (gsize index = 0; index < {count_local}; index++) {{\n"
                        f"    g_ptr_array_add({expression}, {prefix}_from_native(&{local}[index]));\n  }}\n"
                    )
                    returned.append(
                        (
                            "GPtrArray*",
                            expression,
                            f"(transfer full) (element-type {type_name(element.native)}): the items",
                            convert,
                        )
                    )
                else:
                    gtype = values.gtype(element)
                    expression = f"{output.name}_items_"
                    convert = (
                        f"  GArray* {expression} = g_array_sized_new(FALSE, TRUE, sizeof({gtype}), (guint){count_local});\n"
                        f"  for (gsize index = 0; index < {count_local}; index++) {{\n"
                        f"    {gtype} item = {values.from_c(element, f'{local}[index]')};\n"
                        f"    g_array_append_val({expression}, item);\n  }}\n"
                    )
                    returned.append(
                        (
                            "GArray*",
                            expression,
                            f"(transfer full) (element-type {gtype}): the items",
                            convert,
                        )
                    )
                continue
            if value.kind != "reference" or value.element is None:
                raise Unsupported(f"{output.name}: a {value.kind} output")
            element = value.element
            if element.kind in {"scalar", "enum"}:
                call.setup += f"  {without_const(ctype_of(element))} {local} = 0;\n"
                call.arguments[output.name] = f"&{local}"
                returned.append(
                    (
                        values.gtype(element),
                        values.from_c(element, local),
                        "the value",
                        "",
                    )
                )
                continue
            if element.kind == "record":
                call.setup += f"  {element.native} {local};\n  memset(&{local}, 0, sizeof {local});\n"
                call.setup += values.size_line(element, f"(&{local})")
                call.arguments[output.name] = f"&{local}"
                values.use(element)
                returned.append(
                    (
                        f"{type_name(element.native)}*",
                        f"{symbol_name(element.native)}_from_native(&{local})",
                        "(transfer full): the value",
                        "",
                    )
                )
                continue
            raise Unsupported(f"{output.name}: a reference to a {element.kind}")
        return returned

    # Shapes.

    def immediate(self, plan: OperationPlan, view: bool = False) -> Function:
        """A synchronous call, and a view's call inside its owner's scope."""
        call = Call(plan)
        values = self.values
        # The return type is known only after the outputs, so checks use a
        # placeholder that the return type replaces.
        fail = "FAIL_"
        self.receiver(call, fail)
        self.inputs(call, fail)
        returned = self.outputs(call)
        text = plan.caller_sized_text
        result = plan.result
        if result is not None and not returned:
            if result.kind in {"scalar", "enum", "native_pointer"}:
                returned.append(
                    (
                        values.gtype(result),
                        values.from_c(result, "result_"),
                        "the result",
                        "",
                    )
                )
            else:
                raise Unsupported(f"a {result.kind} result")
        throws = plan.status
        if len(returned) == 1:
            returns, expression, annotation, convert = returned[0]
            outs: list[tuple[str, str, str, str]] = []
        else:
            returns = "gboolean" if throws or returned else "void"
            expression, annotation, convert = "TRUE", "", ""
            outs = returned
            if not throws and not returned:
                expression = ""
        if not throws and not returned and result is None:
            returns = "void"
        out_parameters = []
        for index, (type_, value_expression, out_annotation, out_convert) in enumerate(
            outs
        ):
            name = f"out_{index}" if True else ""
            out_name = self.out_name(plan, index)
            out_parameters.append((f"{type_}*", out_name))
            call.annotations[out_name] = (
                f"(out) {out_annotation}"
                if out_annotation.startswith("(")
                else f"(out): {out_annotation}"
            )
            convert += (
                out_convert
                + f"  if ({out_name} != NULL) *{out_name} = {value_expression};\n"
            )
            del name
        call.parameters += out_parameters
        if throws:
            call.parameters.append(("GError**", "error"))
        failing = fail_value(returns)
        checks = call.checks.replace(fail, failing)
        if throws:
            checks += (
                f"  g_return_val_if_fail(error == NULL || *error == NULL, {failing});\n"
            )
        if returns == "void":
            checks = checks.replace(
                "g_return_val_if_fail(", "g_return_if_fail("
            ).replace(", );", ");")
        body = checks
        body += f"  MaplibreNativeFfiCall {CALL};\n  maplibre_native_ffi_call_init(&{CALL});\n"
        body += call.setup.replace(fail, failing)
        if throws:
            body += f"  if (!maplibre_native_ffi_call_check(&{CALL}, error)) return {failing};\n"
        else:
            body += f"  maplibre_native_ffi_call_check(&{CALL}, NULL);\n"
        if plan.function.diagnostic:
            body += "  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};\n"
        accepted = "TRUE"
        if text is not None:
            body += self.caller_sized(plan, call, text)
            accepted = "status_ == MLN_STATUS_OK"
        elif view:
            assert plan.view is not None
            body += (
                "  void* scope_ = NULL;\n"
                f"  mln_status status_ = {plan.view.begin}(self->handle, &scope_, &diagnostic_);\n"
                f"  if (status_ == MLN_STATUS_OK) {{\n"
                f"    status_ = {call.native_call()};\n"
                f"    {plan.view.end}(scope_);\n  }}\n"
            )
            accepted = "status_ == MLN_STATUS_OK"
        elif throws:
            body += f"  mln_status status_ = {call.native_call()};\n"
            accepted = "status_ == MLN_STATUS_OK"
        elif result is not None:
            body += (
                f"  {without_const(ctype_of(result))} result_ = {call.native_call()};\n"
            )
        else:
            body += f"  {call.native_call()};\n"
        for registration in plan.registrations:
            if registration.accepted_unless:
                accepted += f" && !{registration.accepted_unless}_"
        consumes = self.consumes(plan)
        body += consumes[0]
        body += f"  maplibre_native_ffi_call_end(&{CALL}, {accepted});\n"
        body += consumes[1]
        if throws:
            body += f"  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) return {failing};\n"
        body += convert
        if returns != "void":
            body += f"  return {expression};\n"
        doc = self.doc_lines(plan.name)
        identifier = ""
        if plan.factory and receiver_type(plan) is None:
            identifier = "(constructor)"
        return Function(
            self.function_name(plan),
            returns,
            call.parameters,
            body,
            doc=doc,
            annotations=call.annotations,
            identifier_annotation=identifier,
            returns_doc=annotation
            or (
                "%TRUE on success, or %FALSE with `error` set"
                if returns == "gboolean"
                else ""
            ),
        )

    def out_name(self, plan: OperationPlan, index: int) -> str:
        names = [
            p.name
            for p in plan.outputs
            if not any(o.result_length == p.name for o in plan.outputs)
        ]
        return names[index] if index < len(names) else f"out_{index}"

    def function_name(self, plan: OperationPlan) -> str:
        if plan.view is not None:
            return f"{symbol_name(plan.view.owner.native)}_get_{plan.view.stem}"
        return method_name(plan)

    def consumes(self, plan: OperationPlan) -> tuple[str, str]:
        """The statements before and after the call's end that retire the
        receiver of a call that consumes it."""
        owner = receiver_type(plan)
        if owner is None or not plan.consumes:
            return "", ""
        handle = self.bound.handles[owner]
        if plan.answers:
            # The answer ends the handle whatever it returns. A failure may
            # come before native code, so the handle is released too.
            return (
                (
                    f"  {handle.native} consumed_ = self->handle;\n"
                    "  self->handle = MLN_HANDLE_NULL;\n"
                ),
                f"  if (status_ != MLN_STATUS_OK) {handle.release}(consumed_);\n",
            )
        if plan.consumes == "always":
            return "  self->handle = MLN_HANDLE_NULL;\n", ""
        return (
            "  if (status_ == MLN_STATUS_OK) self->handle = MLN_HANDLE_NULL;\n",
            "",
        )

    def caller_sized(self, plan: OperationPlan, call: Call, text: ParameterPlan) -> str:
        """Calls again with more storage until the text fits."""
        capacity = text.value.length
        assert capacity is not None and text.result_length is not None
        call_text = call.native_call(
            {
                text.name: "text_",
                capacity: "capacity_",
                text.result_length: "&length_",
            }
        )
        return (
            "  size_t capacity_ = 256;\n  size_t length_ = 0;\n  char* text_ = NULL;\n"
            "  mln_status status_;\n"
            "  for (;;) {\n"
            "    text_ = g_realloc(text_, capacity_);\n"
            f"    status_ = {call_text};\n"
            "    if (status_ != MLN_STATUS_OK || length_ < capacity_) break;\n"
            "    capacity_ = length_ + 1;\n  }\n"
            "  char* text_value_ = status_ == MLN_STATUS_OK ? g_strndup(text_, length_) : NULL;\n"
            "  g_free(text_);\n"
        )

    def release(self, plan: OperationPlan) -> Function:
        """An immediate release, which the object's dispose also calls."""
        owner = receiver_type(plan)
        assert owner is not None
        function = self.immediate(plan)
        if owner in self.bound.borrowed_handles:
            fail = fail_value(function.returns)
            function.body = function.body.replace(
                f"  MaplibreNativeFfiCall {CALL};",
                "  if (self->borrowed) {\n"
                + (
                    "    maplibre_native_ffi_set_borrowed_error(error);\n"
                    if plan.status
                    else ""
                )
                + f"    return{' ' + fail if fail else ''};\n  }}\n"
                f"  MaplibreNativeFfiCall {CALL};",
                1,
            )
        function.doc = self.doc_lines(
            plan.name,
            (
                (
                    "The object calls this when it is disposed, unless it was"
                    " released. Later calls fail as on any released handle."
                ),
            ),
        )
        return function

    def dispose(self, handle: HandlePlan, plan: OperationPlan) -> Function:
        name, prefix = type_name(handle.native), symbol_name(handle.native)
        borrowed = (
            "  if (self->borrowed) return;\n"
            if handle.native in self.bound.borrowed_handles
            else ""
        )
        return Function(
            f"{prefix}_dispose",
            "void",
            [(f"{name}*", "self")],
            f"  g_return_if_fail({check_macro(handle.native)}(self));\n"
            + borrowed
            + f"  {handle.native} handle = self->handle;\n"
            "  self->handle = MLN_HANDLE_NULL;\n"
            # A released handle no longer belongs to the object, and a dispose
            # can't report a failure.
            "  if (handle != MLN_HANDLE_NULL) {\n"
            f"    (void){plan.name}(handle, NULL);\n  }}\n",
            doc=self.doc_lines(
                plan.name,
                (
                    (
                        "The object calls this when it is disposed, unless its"
                        " handle was released. Later calls do nothing."
                    ),
                ),
            ),
        )

    def task_start(
        self, plan: OperationPlan, start: str, call: Call, discardable: bool
    ) -> str:
        source = "self" if receiver_type(plan) else "NULL"
        task = f"  GTask* task_ = g_task_new({source}, NULL, callback, user_data);\n  g_task_set_source_tag(task_, {start});\n"
        if discardable:
            task = (
                "  GTask* task_ = NULL;\n  if (callback != NULL) {\n"
                + textwrap.indent(task.replace("  GTask* task_ =", "  task_ ="), "  ")
                + "  }\n"
            )
        return task

    def async_parameters(self, call: Call, what: str) -> None:
        call.parameters += [
            ("GAsyncReadyCallback", "callback"),
            ("gpointer", "user_data"),
        ]
        call.annotations["callback"] = (
            f"(scope async) (nullable): the callback to call when {what} finishes"
        )
        call.annotations["user_data"] = "the data to pass to `callback`"

    def finish(
        self,
        plan: OperationPlan,
        start: str,
        returns: str,
        body: str,
        doc: str,
        returns_doc: str,
    ) -> Function:
        owner = receiver_type(plan)
        parameters = []
        source = "NULL"
        if owner is not None:
            parameters.append((f"{type_name(owner)}*", "self"))
            source = "self"
        parameters += [("GAsyncResult*", "result"), ("GError**", "error")]
        failing = fail_value(returns)
        return Function(
            start.removesuffix("_async") + "_finish",
            returns,
            parameters,
            f"  g_return_val_if_fail(g_task_is_valid(result, {source}), {failing});\n"
            + body,
            doc=[doc],
            annotations={"result": "the result that the callback received"},
            returns_doc=returns_doc,
        )

    def decoder(self, plan: OperationPlan) -> tuple[str, str, str, str, str]:
        """How a query's result is copied and returned: the decoder and
        destroy function, the finish function's return type, its body, and
        its return annotation."""
        result = plan.result
        values = self.values
        if plan.shape == "command":
            return (
                "maplibre_native_ffi_command_decode",
                "(GDestroyNotify)maplibre_native_ffi_command_completion_free",
                "MaplibreNativeFfiCommandCompletion*",
                "  return g_task_propagate_pointer(G_TASK(result), error);\n",
                "(transfer full): the command's completion, or %NULL with `error` set",
            )
        if result is None:
            return (
                "NULL",
                "NULL",
                "gboolean",
                "  return g_task_propagate_boolean(G_TASK(result), error);\n",
                "%TRUE when the call finished, or %FALSE with `error` set",
            )
        decode = f"{plan.name}_decode"
        nullable = " (nullable)" if result.nullable else ""
        guard_null = "  if (result->value == NULL) return NULL;\n"
        if result.kind in {"scalar", "enum"}:
            gtype = values.gtype(result)
            spelled = without_const(ctype_of(result))
            self.statics.append(
                guarded(
                    guard(plan),
                    f"static gpointer {decode}(const mln_completion_result* result) {{\n"
                    + guard_null
                    + f"  {gtype}* value = g_new0({gtype}, 1);\n"
                    f"  *value = {values.from_c(result, f'*(const {spelled}*)result->value')};\n"
                    "  return value;\n}\n",
                )
                + "\n"
            )
            return (
                decode,
                "g_free",
                gtype,
                (
                    f"  {gtype}* value = g_task_propagate_pointer(G_TASK(result), error);\n"
                    f"  if (value == NULL) return {fail_value(gtype) or '0'};\n"
                    f"  {gtype} copy = *value;\n  g_free(value);\n  return copy;\n"
                ),
                "the result, or 0 with `error` set",
            )
        if values.text(result) or values.bytes_(result):
            if result.buffer_form != "view":
                raise Unsupported("a result that is not a buffer view")
            text = values.text(result)
            copy = (
                "maplibre_native_ffi_text_new(*view)"
                if text
                else "maplibre_native_ffi_bytes_new(view->data, view->size)"
            )
            self.statics.append(
                guarded(
                    guard(plan),
                    f"static gpointer {decode}(const mln_completion_result* result) {{\n"
                    + guard_null
                    + "  const mln_buffer_view* view = result->value;\n"
                    f"  return {copy};\n}}\n",
                )
                + "\n"
            )
            return (
                decode,
                "g_free" if text else "(GDestroyNotify)g_bytes_unref",
                "char*" if text else "GBytes*",
                "  return g_task_propagate_pointer(G_TASK(result), error);\n",
                f"(transfer full){nullable}: the {'text' if text else 'bytes'}",
            )
        if result.kind == "record":
            prefix = symbol_name(result.native)
            values.use(result)
            self.statics.append(
                guarded(
                    guard(plan),
                    f"static gpointer {decode}(const mln_completion_result* result) {{\n"
                    + guard_null
                    + f"  return {prefix}_from_native(result->value);\n}}\n",
                )
                + "\n"
            )
            return (
                decode,
                f"(GDestroyNotify){prefix}_free",
                f"{type_name(result.native)}*",
                "  return g_task_propagate_pointer(G_TASK(result), error);\n",
                f"(transfer full){nullable}: the value",
            )
        if result.kind == "array":
            element = result.element
            assert element is not None
            if element.kind == "record":
                prefix = symbol_name(element.native)
                values.use(element)
                item = f"{prefix}_from_native((const {element.native}*)(const void*)((const char*)result->value + index * result->value_size))"
                free = f"(GDestroyNotify){prefix}_free"
            elif values.text(element) and element.buffer_form == "view":
                item = "maplibre_native_ffi_text_new(*(const mln_buffer_view*)(const void*)((const char*)result->value + index * result->value_size))"
                free = "g_free"
            else:
                raise Unsupported(f"a result array of {element.kind}")
            self.statics.append(
                guarded(
                    guard(plan),
                    f"static gpointer {decode}(const mln_completion_result* result) {{\n"
                    + (guard_null if result.nullable else "")
                    + f"  GPtrArray* items = g_ptr_array_new_full((guint)result->value_count, {free});\n"
                    "  for (size_t index = 0; index < result->value_count; index++) {\n"
                    f"    g_ptr_array_add(items, {item});\n  }}\n"
                    "  return items;\n}\n",
                )
                + "\n"
            )
            element_type = (
                type_name(element.native) if element.kind == "record" else "utf8"
            )
            return (
                decode,
                "(GDestroyNotify)g_ptr_array_unref",
                "GPtrArray*",
                "  return g_task_propagate_pointer(G_TASK(result), error);\n",
                f"(transfer full) (element-type {element_type}){nullable}: the items",
            )
        raise Unsupported(f"a {result.kind} result")

    def asynchronous(self, plan: OperationPlan) -> list[Function]:
        """A command, query, or operation as an `_async` and `_finish` pair."""
        assert plan.completion is not None
        if plan.outputs:
            raise Unsupported("an asynchronous call with outputs")
        call = Call(plan)
        self.receiver(call, "FAIL_")
        self.inputs(call, "FAIL_")
        start = f"{method_name(plan)}_async"
        discardable = plan.completion.discardable
        self.async_parameters(call, "the call")
        decode, destroy, returns, finish_body, returns_doc = self.decoder(plan)
        body = call.checks.replace(
            "g_return_val_if_fail(", "g_return_if_fail("
        ).replace(", FAIL_);", ");")
        body += self.task_start(plan, start, call, discardable)
        body += self.async_call(plan, call, decode, destroy, discardable)
        doc = self.doc_lines(plan.name)
        if discardable:
            doc += [
                "",
                *wrap(
                    "Without `callback`, the call passes no completion, and"
                    " nothing reports whether it succeeds."
                ),
            ]
        start_function = Function(
            start, "void", call.parameters, body, doc=doc, annotations=call.annotations
        )
        return [
            start_function,
            self.finish(
                plan,
                start,
                returns,
                finish_body,
                f"Finishes a call that `{start}` started.",
                returns_doc,
            ),
        ]

    def async_call(
        self,
        plan: OperationPlan,
        call: Call,
        decode: str,
        destroy: str,
        discardable: bool,
        release: bool = False,
    ) -> str:
        """Converts the inputs, starts the call with a completion that returns
        the task, and returns any failure through the task."""
        task = "task_ != NULL" if discardable else "TRUE"
        body = f"  MaplibreNativeFfiCall {CALL};\n  maplibre_native_ffi_call_init(&{CALL});\n"
        body += call.setup.replace("FAIL_", "")
        body += (
            "  GError* error_ = NULL;\n"
            f"  if (!maplibre_native_ffi_call_check(&{CALL}, &error_)) {{\n"
            "    if (task_ != NULL) {\n      g_task_return_error(task_, error_);\n"
            "      g_object_unref(task_);\n    } else {\n      g_error_free(error_);\n    }\n"
            "    return;\n  }\n"
            "  mln_completion completion_;\n"
            "  const mln_completion* completion_argument_ = NULL;\n"
            f"  if ({task}) {{\n"
            f"    if (!maplibre_native_ffi_completion_init(&completion_, task_, {decode}, {destroy}, &error_)) {{\n"
            f"      maplibre_native_ffi_call_end(&{CALL}, FALSE);\n"
            "      g_task_return_error(task_, error_);\n      g_object_unref(task_);\n      return;\n    }\n"
            "    completion_argument_ = &completion_;\n  }\n"
            "  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};\n"
        )
        completion = next(
            p.name for p in plan.function.parameters if is_completion(p.type)
        )
        body += f"  mln_status status_ = {call.native_call({completion: 'completion_argument_'})};\n"
        body += f"  maplibre_native_ffi_call_end(&{CALL}, status_ == MLN_STATUS_OK);\n"
        if release:
            body += "  if (status_ == MLN_STATUS_OK) {\n"
            body += "    self->released_handle = self->handle;\n"
            body += "    self->handle = MLN_HANDLE_NULL;\n  }\n"
        body += (
            "  if (status_ != MLN_STATUS_OK && completion_argument_ != NULL) {\n"
            "    maplibre_native_ffi_completion_reject(&completion_);\n"
            "    g_task_return_error(task_, maplibre_native_ffi_error_new(status_, &diagnostic_));\n  }\n"
            "  if (task_ != NULL) g_object_unref(task_);\n"
        )
        return body

    def release_async(self, plan: OperationPlan) -> list[Function]:
        owner = receiver_type(plan)
        assert owner is not None and plan.completion is not None
        if plan.outputs or plan.result:
            raise Unsupported("a release with outputs")
        call = Call(plan)
        self.receiver(call, "FAIL_")
        self.inputs(call, "FAIL_")
        start = f"{symbol_name(owner)}_release_async"
        self.async_parameters(call, "the release")
        body = call.checks.replace(
            "g_return_val_if_fail(", "g_return_if_fail("
        ).replace(", FAIL_);", ");")
        body += self.task_start(plan, start, call, False)
        if owner in self.bound.borrowed_handles:
            body += (
                "  if (self->borrowed) {\n    GError* borrowed_ = NULL;\n"
                "    maplibre_native_ffi_set_borrowed_error(&borrowed_);\n"
                "    g_task_return_error(task_, borrowed_);\n    g_object_unref(task_);\n"
                "    return;\n  }\n"
            )
        body += self.async_call(plan, call, "NULL", "NULL", False, release=True)
        return [
            Function(
                start,
                "void",
                call.parameters,
                body,
                doc=self.doc_lines(plan.name),
                annotations=call.annotations,
            ),
            self.finish(
                plan,
                start,
                "gboolean",
                "  return g_task_propagate_boolean(G_TASK(result), error);\n",
                f"Finishes a release that `{start}` started.",
                "%TRUE when the handle retired, or %FALSE with `error` set",
            ),
        ]

    def attach(self, plan: OperationPlan) -> Function:
        """A call that creates its handle at once and readies it later: it
        returns the object, whose `ready_async` waits for the completion."""
        assert plan.completion is not None
        if len(plan.completion.immediate_owners) != 1 or plan.result:
            raise Unsupported("an attach with other than one handle")
        call = Call(plan)
        self.receiver(call, "NULL")
        self.inputs(call, "NULL")
        returned = self.outputs(call)
        if len(returned) != 1:
            raise Unsupported("an attach with other outputs")
        returns, expression, annotation, _ = returned[0]
        call.parameters.append(("GError**", "error"))
        completion = next(
            p.name for p in plan.function.parameters if is_completion(p.type)
        )
        body = call.checks
        body += "  g_return_val_if_fail(error == NULL || *error == NULL, NULL);\n"
        body += f"  MaplibreNativeFfiCall {CALL};\n  maplibre_native_ffi_call_init(&{CALL});\n"
        body += call.setup
        body += f"  if (!maplibre_native_ffi_call_check(&{CALL}, error)) return NULL;\n"
        body += (
            "  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();\n"
            "  mln_completion completion_;\n"
            "  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {\n"
            f"    maplibre_native_ffi_call_end(&{CALL}, FALSE);\n"
            "    maplibre_native_ffi_ready_unref(ready_);\n    return NULL;\n  }\n"
            "  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};\n"
            f"  mln_status status_ = {call.native_call({completion: '&completion_'})};\n"
            f"  maplibre_native_ffi_call_end(&{CALL}, status_ == MLN_STATUS_OK);\n"
            "  if (status_ != MLN_STATUS_OK) {\n"
            "    maplibre_native_ffi_completion_reject(&completion_);\n"
            "    maplibre_native_ffi_ready_unref(ready_);\n"
            "    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);\n"
            "    return NULL;\n  }\n"
            f"  {returns} object_ = {expression};\n"
            "  maplibre_native_ffi_ready_unref(ready_);\n"
            "  return object_;\n"
        )
        handle = plan.completion.immediate_owners[0].handle
        ready = f"{symbol_name(handle.native)}_ready_async"
        return Function(
            method_name(plan),
            returns,
            call.parameters,
            body,
            doc=self.doc_lines(
                plan.name,
                (
                    (
                        f"The object is ready once `{ready}` finishes, which fails"
                        " with the errors that the completion reports."
                    ),
                ),
            ),
            annotations=call.annotations,
            returns_doc=annotation.replace(
                ": the object", ": the new object, or %NULL with `error` set"
            ),
        )

    def drain(self, plan: OperationPlan) -> Function:
        """A drain that returns a copy of the batch's slice, empty when nothing
        is queued, and releases the batch."""
        drain = plan.drain
        assert drain is not None
        call = Call(plan)
        self.receiver(call, "NULL")
        self.inputs(call, "NULL")
        batch_handle = drain.batch.value.unwrapped
        view = drain.view.value.unwrapped
        slice_ = drain.slice
        element = slice_.value.element
        assert element is not None
        self.values.use(element)
        prefix = symbol_name(element.native)
        release = self.bound.handles[batch_handle.native].release
        call.parameters.append(("GError**", "error"))
        item = f"&view_.{slice_.name}[index]"
        if slice_.value.stride:
            item = f"(const {element.native}*)(const void*)((const char*)view_.{slice_.name} + index * view_.{slice_.value.stride})"
        body = call.checks
        body += "  g_return_val_if_fail(error == NULL || *error == NULL, NULL);\n"
        body += (
            f"  {batch_handle.native} batch_ = MLN_HANDLE_NULL;\n"
            f"  {view.native} view_;\n  memset(&view_, 0, sizeof view_);\n"
            + self.values.size_line(view, "(&view_)")
            + "  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};\n"
            f"  mln_status status_ = {call.native_call({drain.batch.name: '&batch_', drain.view.name: '&view_'})};\n"
            "  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) return NULL;\n"
            f"  GPtrArray* items_ = g_ptr_array_new_with_free_func((GDestroyNotify){prefix}_free);\n"
            "  if (batch_ != MLN_HANDLE_NULL) {\n"
            f"    for (size_t index = 0; index < view_.{slice_.value.length}; index++) {{\n"
            f"      g_ptr_array_add(items_, {prefix}_from_native({item}));\n    }}\n"
            f"    {release}(batch_);\n  }}\n"
            "  return items_;\n"
        )
        self.generated.append(release)
        return Function(
            method_name(plan),
            "GPtrArray*",
            call.parameters,
            body,
            doc=self.doc_lines(plan.name),
            annotations=call.annotations,
            returns_doc=f"(transfer full) (element-type {type_name(element.native)}): the items, or %NULL with `error` set",
        )

    # Handles.

    def handles(self) -> tuple[str, str, str]:
        header, source, private = "", "", ""
        structs = ""
        handles = [
            h
            for name, h in sorted(self.bound.public_handles.items())
            if name not in self.bound.slices
        ]
        handles = [h for h in handles if self.handle_used(h)]
        self.handle_names = [h.native for h in handles]
        for handle in handles:
            name, prefix = type_name(handle.native), symbol_name(handle.native)
            structs += (
                f"struct _{name} {{\n  GObject parent_instance;\n  {handle.native} handle;\n"
                f"  {handle.native} released_handle;\n"
                "  // The object of the handle's parent, which outlives it.\n"
                "  GObject* parent;\n"
                "  gboolean borrowed;\n"
                "  MaplibreNativeFfiReady* ready;\n};\n\n"
            )
            private += (
                f"G_GNUC_INTERNAL {name}* {prefix}_wrap(\n"
                f"  {handle.native} handle, gpointer parent, gboolean borrowed,\n"
                "  MaplibreNativeFfiReady* ready\n);\n"
            )
        for handle in handles:
            h, s = self.handle(handle)
            header += h
            source += s
        return header, structs + "".join(self.statics) + source, private

    def handle_used(self, handle: HandlePlan) -> bool:
        if handle.native not in self.bound.handles:
            return False
        return any(
            self.owner(plan) == handle.native
            or any(o.handle.native == handle.native for o in plan.owned_outputs)
            or any(
                p.value.kind == "handle" and p.value.native == handle.native
                for p in plan.inputs
            )
            for plan in self.bound.operations
            if plan.name in self.generated
        ) or any(
            p.value.unwrapped.kind == "handle"
            and p.value.unwrapped.native == handle.native
            for callback in self.values.callbacks.values()
            for p in callback.arguments
        )

    def handle(self, handle: HandlePlan) -> tuple[str, str]:
        native = handle.native
        name, prefix = type_name(native), symbol_name(native)
        upper = prefix.removeprefix(SYMBOL + "_").upper()
        functions = list(self.functions.get(native, []))
        readied = native in self.bound.readied_handles
        functions.insert(
            0,
            Function(
                f"{prefix}_get_id",
                "guint64",
                [(f"{name}*", "self")],
                f"  g_return_val_if_fail({check_macro(native)}(self), 0);\n"
                "  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle : self->released_handle);\n",
                doc=[
                    "The handle's ID, which runtime events name as their source.",
                ],
                returns_doc="the ID",
            ),
        )
        if readied:
            functions += self.ready(handle)
        functions += self.event_source(handle)
        header = comment([f"{name}:", "", *self.doc_lines(native)])
        header += f"#define {type_macro(native)} ({prefix}_get_type())\n"
        header += (
            f"G_DECLARE_FINAL_TYPE({name}, {prefix}, {MACRO}, {upper}, GObject)\n\n"
        )
        header += "".join(f.declaration() for f in functions)
        cleanup = handle.cleanup
        release = handle.release
        if (
            cleanup is not None
            and cleanup == handle.dispose
            and handle.dispose != release
        ):
            dispose_call = f"    {prefix}_dispose(self);\n"
        elif cleanup is not None and handle.discard:
            dispose_call = f"    (void){cleanup}(self->handle, NULL);\n"
        else:
            release_function = self.bound.source.functions_by_name[release]
            arguments = ["self->handle"]
            arguments += ["NULL"] * (len(release_function.parameters) - 1)
            if release_function.diagnostic:
                arguments.append("NULL")
            dispose_call = f"    (void){release}({', '.join(arguments)});\n"
        borrowed = (
            "!self->borrowed && " if native in self.bound.borrowed_handles else ""
        )
        source = (
            f"G_DEFINE_FINAL_TYPE({name}, {prefix}, G_TYPE_OBJECT)\n\n"
            # A finalized object that its owner never released still holds
            # its handle, which the cleanup tears down in any state.
            f"static void {prefix}_object_dispose(GObject* object) {{\n"
            f"  {name}* self = {cast_macro(native)}(object);\n"
            f"  if ({borrowed}self->handle != MLN_HANDLE_NULL) {{\n"
            + dispose_call
            + "    self->handle = MLN_HANDLE_NULL;\n  }\n"
            "  g_clear_object(&self->parent);\n"
            "  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);\n"
            f"  G_OBJECT_CLASS({prefix}_parent_class)->dispose(object);\n}}\n\n"
            f"static void {prefix}_class_init({name}Class* klass) {{\n"
            f"  G_OBJECT_CLASS(klass)->dispose = {prefix}_object_dispose;\n}}\n\n"
            f"static void {prefix}_init({name}* self) {{\n"
            f"  self->handle = MLN_HANDLE_NULL;\n  self->released_handle = MLN_HANDLE_NULL;\n}}\n\n"
            f"{name}* {prefix}_wrap(\n"
            f"  {native} handle, gpointer parent, gboolean borrowed,\n"
            "  MaplibreNativeFfiReady* ready\n) {\n"
            f"  {name}* self = g_object_new({type_macro(native)}, NULL);\n"
            "  self->handle = handle;\n"
            "  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;\n"
            "  self->borrowed = borrowed;\n"
            "  if (ready != NULL) {\n"
            "    // The wrapper takes its own reference, which the caller keeps.\n"
            "    self->ready = maplibre_native_ffi_ready_ref(ready);\n  }\n"
            "  return self;\n}\n\n"
        )
        source += "".join(f.definition() for f in functions)
        return header, source

    def event_source(self, handle: HandlePlan) -> list[Function]:
        """Whether a runtime event comes from the handle, for a handle that
        events name as their source.

        A handle's ID needs all 64 bits, which a binding that holds numbers as
        doubles, such as GJS, can't compare.
        """
        if handle.native not in self.bound.event_sources:
            return []
        found = next(
            (
                (record, member)
                for record in self.values.used.values()
                for member in record.fields
                if member.value.kind == "enum"
                and member.value.native == RUNTIME_EVENT_SOURCE
            ),
            None,
        )
        if found is None:
            return []
        record, kind = found
        source = kind.name.removesuffix("_type")
        if not any(f.name == source for f in record.fields):
            return []
        name, prefix = type_name(handle.native), symbol_name(handle.native)
        event, event_prefix = type_name(record.native), symbol_name(record.native)
        constant = constant_name(
            kind.value,
            RUNTIME_EVENT_SOURCE_PREFIX + handle.stem.upper(),
        )
        return [
            Function(
                f"{prefix}_is_event_source",
                "gboolean",
                [(f"{name}*", "self"), (f"const {event}*", "event")],
                f"  g_return_val_if_fail({check_macro(handle.native)}(self), FALSE);\n"
                "  g_return_val_if_fail(event != NULL, FALSE);\n"
                f"  return {event_prefix}_get_{accessor(record, kind.name)}(event) == {constant} &&\n"
                f"    {event_prefix}_get_{accessor(record, source)}(event) == {prefix}_get_id(self);\n",
                doc=[
                    *wrap(
                        "Whether a runtime event comes from this handle. A handle's"
                        f" ID needs all of its 64 bits, so compare with this, rather"
                        f" than with `{prefix}_get_id`, where numbers are doubles, as"
                        " in JavaScript."
                    )
                ],
                annotations={"event": "the event"},
                returns_doc="whether the event's source is this handle",
            )
        ]

    def ready(self, handle: HandlePlan) -> list[Function]:
        name, prefix = type_name(handle.native), symbol_name(handle.native)
        start = f"{prefix}_ready_async"
        return [
            Function(
                start,
                "void",
                [
                    (f"{name}*", "self"),
                    ("GAsyncReadyCallback", "callback"),
                    ("gpointer", "user_data"),
                ],
                f"  g_return_if_fail({check_macro(handle.native)}(self));\n"
                "  GTask* task_ = g_task_new(self, NULL, callback, user_data);\n"
                f"  g_task_set_source_tag(task_, {start});\n"
                "  maplibre_native_ffi_ready_wait(self->ready, task_);\n"
                "  g_object_unref(task_);\n",
                doc=[
                    *wrap(
                        "Waits until the handle is ready, as the call that created"
                        " it reports. A handle that a callback lends is ready at once."
                    ),
                    "",
                    *wrap(THREADS),
                ],
                annotations={
                    "callback": "(scope async) (nullable): the callback to call when the handle is ready",
                    "user_data": "the data to pass to `callback`",
                },
            ),
            Function(
                f"{prefix}_ready_finish",
                "gboolean",
                [
                    (f"{name}*", "self"),
                    ("GAsyncResult*", "result"),
                    ("GError**", "error"),
                ],
                "  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);\n"
                "  return g_task_propagate_boolean(G_TASK(result), error);\n",
                doc=[f"Finishes a wait that `{start}` started."],
                annotations={"result": "the result that the callback received"},
                returns_doc="%TRUE when the handle is ready, or %FALSE with `error` set",
            ),
        ]

    def module_functions(self) -> tuple[str, str]:
        functions = self.functions.get(None, [])
        return (
            "".join(f.declaration() for f in functions),
            "".join(f.definition() for f in functions),
        )


def ctype_of(value: ValuePlan) -> str:
    return value.ctype.spelling


def lower(api: Api | BoundApi) -> Lowered:
    bound = compile_api(api, TARGETS)
    emitter = Emitter(bound)
    files = emitter.enums()
    emitter.lower_operations()
    emitter.unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    if emitter.unsupported:
        return Lowered(
            {}, emitter.unsupported, emitter.generated, tuple(emitter.support)
        )
    try:
        files.update(render(emitter))
    except Unsupported as error:
        raise Unsupported(f"GObject: {error}") from error
    return Lowered(files, {}, emitter.generated, tuple(emitter.support))


def render(emitter: Emitter) -> dict[str, str]:
    handles_header, handles_source, handles_private = emitter.handles()
    values_header, values_source, values_private = emitter.values.render(
        emitter.handle_names
    )
    constants_header, constants_source = emitter.constants()
    module_header, module_source = emitter.module_functions()
    guard_ = f"{MACRO}_VALUES_H"
    files = {
        f"{SOURCE}/maplibre-native-ffi-values.h": HEADER
        + f"#ifndef {guard_}\n#define {guard_}\n\n"
        + "#include <glib-object.h>\n\n"
        + '#include "maplibre-native-ffi-enums.h"\n\n'
        + "G_BEGIN_DECLS\n\n"
        + constants_header
        + values_header
        + "G_END_DECLS\n\n#endif\n",
        f"{SOURCE}/maplibre-native-ffi-values.c": HEADER
        + '#include "maplibre-native-ffi-values.h"\n\n'
        + "#include <maplibre_native_c.h>\n#include <string.h>\n\n"
        + '#include "maplibre-native-ffi-handles-private.h"\n'
        + '#include "maplibre-native-ffi-private.h"\n'
        + '#include "maplibre-native-ffi-values-private.h"\n\n'
        + constants_source
        + "\n"
        + values_source,
        f"{SOURCE}/maplibre-native-ffi-values-private.h": HEADER
        + f"#ifndef {MACRO}_VALUES_PRIVATE_H\n#define {MACRO}_VALUES_PRIVATE_H\n\n"
        + "#include <maplibre_native_c.h>\n\n"
        + '#include "maplibre-native-ffi-private.h"\n'
        + '#include "maplibre-native-ffi-values.h"\n\n'
        + "G_BEGIN_DECLS\n\n"
        + values_private
        + "\nG_END_DECLS\n\n#endif\n",
        f"{SOURCE}/maplibre-native-ffi-handles.h": HEADER
        + f"#ifndef {MACRO}_HANDLES_H\n#define {MACRO}_HANDLES_H\n\n"
        + "#include <gio/gio.h>\n\n"
        + '#include "maplibre-native-ffi-command-completion.h"\n'
        + '#include "maplibre-native-ffi-values.h"\n\n'
        + "G_BEGIN_DECLS\n\n"
        + handles_header
        + module_header
        + "G_END_DECLS\n\n#endif\n",
        f"{SOURCE}/maplibre-native-ffi-handles-private.h": HEADER
        + f"#ifndef {MACRO}_HANDLES_PRIVATE_H\n#define {MACRO}_HANDLES_PRIVATE_H\n\n"
        + "#include <maplibre_native_c.h>\n\n"
        + '#include "maplibre-native-ffi-handles.h"\n'
        + '#include "maplibre-native-ffi-private.h"\n\n'
        + "G_BEGIN_DECLS\n\n"
        + handles_private
        + "\nG_END_DECLS\n\n#endif\n",
        f"{SOURCE}/maplibre-native-ffi-handles.c": HEADER
        + '#include "maplibre-native-ffi-handles.h"\n\n'
        + "#include <maplibre_native_c.h>\n#include <maplibre_native_c/plugin.h>\n#include <string.h>\n\n"
        + '#include "maplibre-native-ffi-handles-private.h"\n'
        + '#include "maplibre-native-ffi-private.h"\n'
        + '#include "maplibre-native-ffi-values-private.h"\n\n'
        + handles_source
        + module_source,
    }
    return files


def symbols(bound: BoundApi) -> docs.Symbol:
    """How the GObject library spells the C declarations that its
    documentation names."""
    return bound.symbols("gobject", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    bound = compile_api(bound, TARGETS)

    def type_(native: str) -> str | None:
        if native == COMPLETION_RESULT:
            return None
        return type_name(native)

    def operation(plan: OperationPlan) -> str | None:
        # The C text that names a function writes its parentheses.
        receiver = receiver_type(plan)
        if plan.view is not None:
            return f"{symbol_name(plan.view.owner.native)}_get_{plan.view.stem}"
        if receiver is not None:
            handle = bound.handles[receiver]
            if plan.name == handle.release and handle.release_waits:
                return f"{symbol_name(receiver)}_release_async"
            if plan.name == handle.dispose:
                return f"{symbol_name(receiver)}_dispose"
            if plan.name == handle.discard:
                return None
        if plan.completion is not None and plan.shape != "attach":
            return f"{method_name(plan)}_async"
        return method_name(plan)

    def constant(value: ValuePlan, member: str) -> str | None:
        # The error domain has no code for success, which a status member
        # holds as 0.
        if value.native == STATUS and dict(value.enum_values)[member] == 0:
            return "0"
        if value.native in bound.control_enums:
            return None
        return constant_name(value, member)

    values = Values(bound, lambda name: None)

    def field_(value: ValuePlan, member: FieldPlan) -> str | None:
        if value.kind == "union":
            return f"{symbol_name(value.native)}_get_{member.name}"
        if value.kind == "record" and value.union_member:
            return None
        if values.transparent(value):
            return f"{type_name(value.native)}.{member.name}"
        if member.value.kind == "callback":
            return f"{symbol_name(value.native)}_set_{member.name}"
        verb = "set" if value.native in bound.inputs else "get"
        return f"{symbol_name(value.native)}_{verb}_{accessor(value, member.name)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field_,
        define=define_name,
        extra={
            f"{COMPLETION_RESULT}.{name}": f"MaplibreNativeFfiCommandCompletion.{name}"
            for name in ("generation", "render_update_generation")
        },
        threads=async_hosts.threads(bound, THREADS),
        fields=async_hosts.omitted_fields(bound),
    )
