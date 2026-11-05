"""Generate the GObject library's C sources.

The library is C with introspection annotations. `g-ir-scanner` reads the
headers that this emitter writes into the `.gir` and `.typelib`, and `vapigen`
reads the `.gir` into the `.vapi`, so these annotations decide what GJS,
PyGObject, Vala, and every other GObject consumer sees.

A `GError` domain carries `mln_status`, whose codes are the raw status values,
so a status that these headers do not name survives. Each enum is a `GEnum`
and each bitmask a `GFlags`, whose values the C compiler checks against the C
constants. A handle is a final `GObject` class. A handle whose release waits
has `release_async` and `release_finish`, and its object's `dispose` calls the
C dispose unless the handle was released. An option record that holds text is
an opaque boxed type with accessors, since introspection can't write its
members.

The emitter generates the declarations in `SLICE` and leaves out the rest of
the API until it covers every shape; until then the generator-wide coverage
checks leave it out too. A declaration of the slice that it cannot lower fails
generation.
"""

from __future__ import annotations

import re
import textwrap
from dataclasses import dataclass, field, replace

from tools.bindgen import async_hosts, docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.model import Api, Lowered, Unsupported
from tools.bindgen.names import pascal
from tools.bindgen.protocol import STATUS
from tools.bindgen.semantic import (
    BoundApi,
    DisposeSupport,
    FieldPlan,
    HandlePlan,
    OperationPlan,
    ValuePlan,
    enum_member_prefix,
    public_stem,
)

DIRECTORY = "bindings/gobject"
SOURCE = f"{DIRECTORY}/src"
NAMESPACE = "MaplibreNativeFfi"
SYMBOL = "maplibre_native_ffi"
MACRO = "MAPLIBRE_NATIVE_FFI"
# The library builds for the platforms whose GTK it serves.
TARGETS = frozenset({"linux", "macos"})
HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"
# The handles whose operations the library generates so far.
SLICE = ("mln_runtime",)
THREADS = "Runs on the thread-default main context of the thread that started the call."
GINT_MIN, GINT_MAX, GUINT_MAX = -(2**31), 2**31 - 1, 2**32 - 1


def type_name(native: str) -> str:
    """The GObject type of a C declaration: `MaplibreNativeFfiRuntime`."""
    if native == STATUS:
        return f"{NAMESPACE}Error"
    return NAMESPACE + pascal(public_stem(native))


def symbol_name(native: str) -> str:
    """The function prefix of a C declaration's type: `maplibre_native_ffi_runtime`."""
    if native == STATUS:
        return f"{SYMBOL}_error"
    return f"{SYMBOL}_{public_stem(native)}"


def type_macro(native: str) -> str:
    """The macro that names a type's GType: `MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME`."""
    return f"{MACRO}_TYPE_{symbol_name(native).removeprefix(SYMBOL + '_').upper()}"


def cast_macro(native: str) -> str:
    """The macro that casts to a class: `MAPLIBRE_NATIVE_FFI_RUNTIME`."""
    return symbol_name(native).upper()


def check_macro(native: str) -> str:
    """The macro that checks an instance's class: `MAPLIBRE_NATIVE_FFI_IS_RUNTIME`."""
    return f"{MACRO}_IS_" + cast_macro(native).removeprefix(f"{MACRO}_")


def constant_name(value: ValuePlan, member: str) -> str:
    """The GObject constant of a C enum constant. A status's constant is a code
    of the error domain: `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT`."""
    if value.native == STATUS:
        return f"{MACRO}_ERROR_" + member.removeprefix("MLN_STATUS_")
    return f"{MACRO}_" + member.removeprefix("MLN_")


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
    """A handle's method, or the factory that creates one, by its C member."""
    return f"{symbol_name(receiver_type(plan) or plan.factory or '')}_{plan.member}"


@dataclass
class Function:
    """One generated function: its declaration's documentation and annotated
    prototype for the header, and its body for the source."""

    name: str
    returns: str
    parameters: list[tuple[str, str]]
    body: str
    doc: list[str] = field(default_factory=list)
    annotations: dict[str, str] = field(default_factory=dict)
    returns_doc: str = ""
    identifier_annotation: str = ""

    def prototype(self) -> str:
        parameters = ", ".join(f"{t} {n}" for t, n in self.parameters) or "void"
        return f"{self.returns} {self.name}({parameters})"

    def comment(self) -> str:
        head = f"{self.name}:"
        if self.identifier_annotation:
            head += f" {self.identifier_annotation}"
        lines = [head]
        for _, parameter in self.parameters:
            if parameter in self.annotations:
                lines.append(f"@{parameter}: {self.annotations[parameter]}")
        lines += ["", *(self.doc or [f"Calls `{self.name}`."])]
        if self.returns_doc:
            lines += ["", f"Returns: {self.returns_doc}"]
        return comment(lines)


# Between a slash and an asterisk, which together open or close a C comment.
COMMENT_MARK = re.compile(r"(?<=/)(?=\*)|(?<=\*)(?=/)")


def comment(lines: list[str]) -> str:
    """A gtk-doc comment, which must not open or close a comment inside."""
    body = "".join(
        f" * {COMMENT_MARK.sub(' ', line)}".rstrip() + "\n" for line in lines
    )
    return f"/**\n{body} */\n"


class Emitter:
    def __init__(self, bound: BoundApi) -> None:
        self.bound = bound
        self.symbol = symbols(bound)
        self.generated: list[str] = []

    # Documentation.

    def doc_lines(self, native: str, extra: tuple[str, ...] = ()) -> list[str]:
        doc = self.bound.doc(native)
        if doc is not None and extra:
            doc = replace(doc, notes=doc.notes + extra)
        return docs.markdown_lines(
            doc, docs.WIDTH - 3, self.symbol, errors="It fails with these errors:"
        )

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
        guard = f"{MACRO}_ENUMS_H"
        return {
            f"{SOURCE}/maplibre-native-ffi-enums.h": HEADER
            + f"#ifndef {guard}\n#define {guard}\n\n"
            + "#include <glib-object.h>\n\nG_BEGIN_DECLS\n\n"
            + "\n".join(header)
            + "\nG_END_DECLS\n\n#endif\n",
            f"{SOURCE}/maplibre-native-ffi-enums.c": HEADER
            + '#include "maplibre-native-ffi-enums.h"\n\n'
            + "#include <maplibre_native_c.h>\n\n"
            + "\n".join(source),
        }

    def enum_declaration(self, value: ValuePlan) -> str:
        name = type_name(value.native)
        members = enum_values(value)
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
            member_doc = self.bound.doc(member)
            text = ""
            if member_doc is not None:
                text = " ".join(docs.markdown_lines(member_doc, 1000, self.symbol)[:1])
            lines += textwrap.wrap(
                f"@{constant_name(value, member)}: {text}".rstrip(),
                docs.WIDTH - 3,
                subsequent_indent="  ",
                break_long_words=False,
                break_on_hyphens=False,
            )
        lines += ["", *self.doc_lines(value.native)]
        out += comment(lines)
        out += "typedef enum {\n"
        for member, number in members:
            if flags(value):
                if not 0 <= number <= GUINT_MAX:
                    raise Unsupported(
                        f"GObject: {value.native}: {member} does not fit GFlags"
                    )
                literal = f"0x{number:08x}u" if number > GINT_MAX else str(number)
            else:
                if not GINT_MIN <= number <= GINT_MAX:
                    raise Unsupported(
                        f"GObject: {value.native}: {member} does not fit GEnum"
                    )
                literal = str(number)
            out += f"  {constant_name(value, member)} = {literal},\n"
        out += f"}} {name};\n\n"
        out += f"GType {symbol_name(value.native)}_get_type(void) G_GNUC_CONST;\n"
        out += f"#define {type_macro(value.native)} ({symbol_name(value.native)}_get_type())\n"
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
        kind = "FLAGS" if flags(value) else "ENUM"
        entries = ",\n".join(
            f'  G_DEFINE_ENUM_VALUE({constant_name(value, member)}, "{nick(value, member)}")'
            for member, _ in members
        )
        out += f"\nG_DEFINE_{kind}_TYPE(\n  {name}, {symbol_name(value.native)},\n{entries}\n)\n"
        return out

    # Records.

    def record_fields(self, value: ValuePlan) -> list[FieldPlan]:
        fields = [f for f in value.fields if f.role != "size"]
        for member in fields:
            if not (
                member.role == "value"
                and member.value.kind == "buffer"
                and member.value.encoding == "utf8"
            ):
                raise Unsupported(
                    f"GObject: {value.native}.{member.name}: only text members"
                    " are supported yet"
                )
        return fields

    def records(self, natives: list[str]) -> dict[str, str]:
        header, source = [], []
        for native in sorted(natives):
            value = self.bound.values[native]
            h, s = self.record(value)
            header.append(h)
            source.append(s)
        guard = f"{MACRO}_VALUES_H"
        return {
            f"{SOURCE}/maplibre-native-ffi-values.h": HEADER
            + f"#ifndef {guard}\n#define {guard}\n\n"
            + "#include <glib-object.h>\n\nG_BEGIN_DECLS\n\n"
            + "\n".join(header)
            + "\nG_END_DECLS\n\n#endif\n",
            f"{SOURCE}/maplibre-native-ffi-values.c": HEADER
            + '#include "maplibre-native-ffi-values.h"\n\n'
            + "#include <string.h>\n\n"
            + '#include "maplibre-native-ffi-values-private.h"\n\n'
            + "\n".join(source),
            f"{SOURCE}/maplibre-native-ffi-values-private.h": HEADER
            + f"#ifndef {MACRO}_VALUES_PRIVATE_H\n#define {MACRO}_VALUES_PRIVATE_H\n\n"
            + "#include <maplibre_native_c.h>\n\n"
            + '#include "maplibre-native-ffi-values.h"\n\n'
            + "".join(
                f"G_GNUC_INTERNAL void {symbol_name(n)}_to_native(\n"
                f"  const {type_name(n)}* self, {n}* out\n);\n"
                for n in sorted(natives)
            )
            + "\n#endif\n",
        }

    def record(self, value: ValuePlan) -> tuple[str, str]:
        name, prefix = type_name(value.native), symbol_name(value.native)
        fields = self.record_fields(value)
        functions = [
            Function(
                f"{prefix}_new",
                f"{name}*",
                [],
                f"  {name}* self = g_new0({name}, 1);\n"
                + "".join(f'  self->{f.name} = g_strdup("");\n' for f in fields)
                + "  return self;\n",
                doc=[f"Creates a `{name}` that selects every default."],
                identifier_annotation="(constructor)",
                returns_doc="(transfer full): the new value",
            ),
            Function(
                f"{prefix}_copy",
                f"{name}*",
                [(f"const {name}*", "self")],
                f"  g_return_val_if_fail(self != NULL, NULL);\n"
                f"  {name}* copy = g_new0({name}, 1);\n"
                + "".join(
                    f"  copy->{f.name} = g_strdup(self->{f.name});\n" for f in fields
                )
                + "  return copy;\n",
                doc=["Copies the value."],
                returns_doc="(transfer full): the copy",
            ),
            Function(
                f"{prefix}_free",
                "void",
                [(f"{name}*", "self")],
                "  if (self == NULL) return;\n"
                + "".join(f"  g_free(self->{f.name});\n" for f in fields)
                + "  g_free(self);\n",
                doc=["Frees the value."],
                annotations={"self": "(transfer full) (nullable): the value"},
            ),
        ]
        for member in fields:
            documented = f"{value.native}.{member.name}"
            functions.append(
                Function(
                    f"{prefix}_get_{member.name}",
                    "const char*",
                    [(f"const {name}*", "self")],
                    f"  g_return_val_if_fail(self != NULL, NULL);\n"
                    f"  return self->{member.name};\n",
                    doc=self.doc_lines(documented) or [f"Reads `{member.name}`."],
                    returns_doc="(transfer none): the text",
                )
            )
            functions.append(
                Function(
                    f"{prefix}_set_{member.name}",
                    "void",
                    [(f"{name}*", "self"), ("const char*", "value")],
                    f"  g_return_if_fail(self != NULL);\n"
                    f"  g_return_if_fail(value != NULL);\n"
                    f"  char* previous = self->{member.name};\n"
                    f"  self->{member.name} = g_strdup(value);\n"
                    f"  g_free(previous);\n",
                    doc=self.doc_lines(documented) or [f"Sets `{member.name}`."],
                    annotations={"value": "the UTF-8 text"},
                )
            )
        header = comment([f"{name}:", "", *self.doc_lines(value.native)])
        header += f"typedef struct _{name} {name};\n\n"
        header += f"GType {prefix}_get_type(void) G_GNUC_CONST;\n"
        header += f"#define {type_macro(value.native)} ({prefix}_get_type())\n\n"
        header += "".join(f.comment() + f.prototype() + ";\n\n" for f in functions)
        source = f"struct _{name} {{\n"
        source += "".join(f"  char* {f.name};\n" for f in fields)
        source += "};\n\n"
        source += (
            f"G_DEFINE_BOXED_TYPE({name}, {prefix}, {prefix}_copy, {prefix}_free)\n\n"
        )
        source += "".join(f"{f.prototype()} {{\n{f.body}}}\n\n" for f in functions)
        source += (
            f"void {prefix}_to_native(const {name}* self, {value.native}* out) {{\n"
        )
        source += "  memset(out, 0, sizeof *out);\n  out->size = sizeof *out;\n"
        for member in fields:
            source += (
                f"  out->{member.name}.data = self->{member.name};\n"
                f"  out->{member.name}.size = strlen(self->{member.name});\n"
            )
        source += "}\n"
        return header, source

    # Handles.

    def handles(self) -> tuple[dict[str, str], set[str]]:
        header, source, records = [], [], set()
        for native in SLICE:
            handle = self.bound.handles.get(native)
            if handle is None:
                continue
            h, s, reached = self.handle(handle)
            header.append(h)
            source.append(s)
            records |= reached
        guard = f"{MACRO}_HANDLES_H"
        files = {
            f"{SOURCE}/maplibre-native-ffi-handles.h": HEADER
            + f"#ifndef {guard}\n#define {guard}\n\n"
            + "#include <gio/gio.h>\n\n"
            + '#include "maplibre-native-ffi-values.h"\n\n'
            + "G_BEGIN_DECLS\n\n"
            + "\n".join(header)
            + "G_END_DECLS\n\n#endif\n",
            f"{SOURCE}/maplibre-native-ffi-handles.c": HEADER
            + '#include "maplibre-native-ffi-handles.h"\n\n'
            + "#include <maplibre_native_c.h>\n\n"
            + '#include "maplibre-native-ffi-private.h"\n'
            + '#include "maplibre-native-ffi-values-private.h"\n\n'
            + "\n".join(source),
        }
        return files, records

    def handle(self, handle: HandlePlan) -> tuple[str, str, set[str]]:
        native = handle.native
        name, prefix = type_name(native), symbol_name(native)
        upper = prefix.removeprefix(SYMBOL + "_").upper()
        plans = [
            plan
            for plan in self.bound.operations
            if plan.factory == native or receiver_type(plan) == native
        ]
        functions: list[Function] = []
        records: set[str] = set()
        dispose = None
        for plan in plans:
            if isinstance(plan.support, DisposeSupport):
                dispose = plan
                functions.append(self.dispose(handle, plan))
            elif plan.name == handle.release and handle.release_waits:
                functions += self.release_async(handle, plan)
            elif plan.factory == native:
                function, reached = self.factory(handle, plan)
                functions.append(function)
                records |= reached
            else:
                continue
            self.generated.append(plan.name)
        if not handle.release_waits or dispose is None:
            raise Unsupported(
                f"GObject: {native}: only a handle whose release waits is supported yet"
            )
        header = comment([f"{name}:", "", *self.doc_lines(native)])
        header += f"#define {type_macro(native)} ({prefix}_get_type())\n"
        header += (
            f"G_DECLARE_FINAL_TYPE({name}, {prefix}, {MACRO}, {upper}, GObject)\n\n"
        )
        header += "".join(f.comment() + f.prototype() + ";\n\n" for f in functions)
        source = (
            f"struct _{name} {{\n  GObject parent_instance;\n  {native} handle;\n}};\n\n"
            f"G_DEFINE_FINAL_TYPE({name}, {prefix}, G_TYPE_OBJECT)\n\n"
            # A finalized object that its owner never released still holds
            # its handle, which the C dispose tears down in any state.
            f"static void {prefix}_object_dispose(GObject* object) {{\n"
            f"  {prefix}_dispose({cast_macro(native)}(object));\n"
            f"  G_OBJECT_CLASS({prefix}_parent_class)->dispose(object);\n}}\n\n"
            f"static void {prefix}_class_init({name}Class* klass) {{\n"
            f"  G_OBJECT_CLASS(klass)->dispose = {prefix}_object_dispose;\n}}\n\n"
            f"static void {prefix}_init({name}* self) {{\n"
            f"  self->handle = MLN_HANDLE_NULL;\n}}\n\n"
        )
        source += "".join(f"{f.prototype()} {{\n{f.body}}}\n\n" for f in functions)
        return header, source, records

    def factory(
        self, handle: HandlePlan, plan: OperationPlan
    ) -> tuple[Function, set[str]]:
        name = type_name(handle.native)
        owned = plan.owned_outputs
        if (
            plan.execution != "immediate"
            or plan.completion
            or len(owned) != 1
            or len(plan.outputs) != 1
            or plan.registrations
        ):
            raise Unsupported(f"GObject: {plan.name}: this factory's shape")
        parameters: list[tuple[str, str]] = []
        annotations: dict[str, str] = {}
        setup, arguments, records = "", [], set()
        for parameter in plan.inputs:
            value = parameter.value
            element = value.element
            if value.kind != "reference" or element is None or element.kind != "record":
                raise Unsupported(
                    f"GObject: {plan.name}: {parameter.name} needs a lowering"
                )
            records.add(element.native)
            record = type_name(element.native)
            parameters.append((f"const {record}*", parameter.name))
            annotations[parameter.name] = (
                "(nullable): the options, or %NULL for every default"
                if value.nullable
                else "the options"
            )
            local = f"native_{parameter.name}"
            setup += f"  {element.native} {local}_value;\n"
            setup += f"  const {element.native}* {local} = NULL;\n"
            guard = ""
            if value.nullable:
                guard = f"if ({parameter.name} != NULL) "
            else:
                setup += f"  g_return_val_if_fail({parameter.name} != NULL, NULL);\n"
            setup += (
                f"  {guard}{{\n"
                f"    {symbol_name(element.native)}_to_native({parameter.name}, &{local}_value);\n"
                f"    {local} = &{local}_value;\n  }}\n"
            )
            arguments.append(local)
        parameters.append(("GError**", "error"))
        body = (
            "  g_return_val_if_fail(error == NULL || *error == NULL, NULL);\n"
            + setup
            + f"  {handle.native} handle = MLN_HANDLE_NULL;\n"
            + "  mln_diagnostic diagnostic = {.size = sizeof diagnostic};\n"
            + f"  mln_status status = {plan.name}({', '.join([*arguments, '&handle', '&diagnostic'])});\n"
            + f"  if (!{SYMBOL}_check(status, &diagnostic, error)) return NULL;\n"
            + f"  {name}* self = g_object_new({type_macro(handle.native)}, NULL);\n"
            + "  self->handle = handle;\n  return self;\n"
        )
        return (
            Function(
                method_name(plan),
                f"{name}*",
                parameters,
                body,
                doc=self.doc_lines(plan.name),
                annotations=annotations,
                identifier_annotation="(constructor)",
                returns_doc=f"(transfer full): the new `{name}`, or %NULL with `error` set",
            ),
            records,
        )

    def release_async(self, handle: HandlePlan, plan: OperationPlan) -> list[Function]:
        name, prefix = type_name(handle.native), symbol_name(handle.native)
        if plan.inputs[1:] or plan.outputs or plan.result or not plan.completion:
            raise Unsupported(f"GObject: {plan.name}: this release's shape")
        start = f"{prefix}_release_async"
        finish = f"{prefix}_release_finish"
        start_body = (
            f"  g_return_if_fail({check_macro(handle.native)}(self));\n"
            f"  GTask* task = g_task_new(self, NULL, callback, user_data);\n"
            f"  g_task_set_source_tag(task, {start});\n"
            "  mln_completion completion;\n"
            "  GError* error = NULL;\n"
            f"  if (!{SYMBOL}_completion_init(&completion, task, &error)) {{\n"
            "    g_task_return_error(task, error);\n"
            "    g_object_unref(task);\n"
            "    return;\n  }\n"
            "  mln_diagnostic diagnostic = {.size = sizeof diagnostic};\n"
            f"  mln_status status = {plan.name}(self->handle, &completion, &diagnostic);\n"
            "  if (status == MLN_STATUS_OK) {\n"
            "    self->handle = MLN_HANDLE_NULL;\n"
            "  } else {\n"
            f"    {SYMBOL}_completion_reject(&completion);\n"
            f"    g_task_return_error(task, {SYMBOL}_error_new(status, &diagnostic));\n"
            "  }\n"
            "  g_object_unref(task);\n"
        )
        finish_body = (
            "  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);\n"
            "  return g_task_propagate_boolean(G_TASK(result), error);\n"
        )
        return [
            Function(
                start,
                "void",
                [
                    (f"{name}*", "self"),
                    ("GAsyncReadyCallback", "callback"),
                    ("gpointer", "user_data"),
                ],
                start_body,
                doc=self.doc_lines(plan.name),
                annotations={
                    "callback": "(scope async) (nullable): the callback to call when the release finishes",
                    "user_data": "the data to pass to `callback`",
                },
            ),
            Function(
                finish,
                "gboolean",
                [
                    (f"{name}*", "self"),
                    ("GAsyncResult*", "result"),
                    ("GError**", "error"),
                ],
                finish_body,
                doc=[f"Finishes a release that `{start}` started."],
                annotations={"result": "the result that the callback received"},
                returns_doc="%TRUE when the handle retired, or %FALSE with `error` set",
            ),
        ]

    def dispose(self, handle: HandlePlan, plan: OperationPlan) -> Function:
        name, prefix = type_name(handle.native), symbol_name(handle.native)
        return Function(
            f"{prefix}_dispose",
            "void",
            [(f"{name}*", "self")],
            f"  g_return_if_fail({check_macro(handle.native)}(self));\n"
            f"  {handle.native} handle = self->handle;\n"
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


def lower(api: Api | BoundApi) -> Lowered:
    bound = compile_api(api, TARGETS)
    emitter = Emitter(bound)
    files = emitter.enums()
    handles, records = emitter.handles()
    files.update(handles)
    files.update(emitter.records(sorted(records)))
    return Lowered(files, {}, emitter.generated)


def symbols(bound: BoundApi) -> docs.Symbol:
    """How the GObject library spells the C declarations that its
    documentation names."""
    return bound.symbols("gobject", _symbols)


def _symbols(bound: BoundApi) -> docs.Symbol:
    # A declaration that the library doesn't generate yet keeps its C name, so
    # the text that names it stays.
    slice_operations = {
        plan.name
        for plan in bound.operations
        if plan.factory in SLICE or receiver_type(plan) in SLICE
    }
    reached = {
        parameter.value.element.native
        for plan in bound.operations
        if plan.name in slice_operations
        for parameter in plan.inputs
        if parameter.value.element is not None
    }

    def type_(native: str) -> str | None:
        value = bound.values.get(native)
        if native in SLICE or native in reached:
            return type_name(native)
        if value is not None and value.kind == "enum":
            return type_name(native)
        return native

    def operation(plan: OperationPlan) -> str | None:
        # The C text that names a function writes its parentheses.
        if plan.name not in slice_operations:
            return plan.name
        receiver = receiver_type(plan)
        if receiver is not None:
            handle = bound.handles[receiver]
            if plan.name == handle.release and handle.release_waits:
                return f"{symbol_name(receiver)}_release_async"
            if plan.name == handle.dispose:
                return f"{symbol_name(receiver)}_dispose"
        return method_name(plan)

    def constant(value: ValuePlan, member: str) -> str | None:
        if value.native == STATUS and dict(value.enum_values)[member] == 0:
            return None
        return constant_name(value, member)

    def field_(value: ValuePlan, member: FieldPlan) -> str | None:
        if value.native not in reached:
            return f"{value.native}.{member.name}"
        return f"{symbol_name(value.native)}_set_{member.name}()"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field_,
        threads=async_hosts.threads(bound, THREADS),
    )
