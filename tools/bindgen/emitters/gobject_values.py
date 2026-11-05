"""Generate the GObject library's values: records, unions, registrations, and
the callbacks that registrations carry.

A record of plain values, which zeroed storage represents, is a transparent
boxed struct whose members a host reads and writes. Every other record, such
as an option bag with a presence mask, a record that holds text, arrays, or
bytes, a registration, or a tagged union, is an opaque boxed type with
accessors, since introspection can represent neither its presence nor its
ownership. A record that holds only a tagged union and its tag is that union,
under the record's name.

Each type converts to its C record within the scope of one call, which owns
the storage that the C record borrows and the native registrations that it
hands over, and copies from a C record that native code lends.
"""

from __future__ import annotations

import re
import textwrap
from dataclasses import dataclass, field

from tools.bindgen import async_hosts, docs
from tools.bindgen.model import Unsupported
from tools.bindgen.names import pascal
from tools.bindgen.protocol import STATUS
from tools.bindgen.semantic import (
    BoundApi,
    CallbackPlan,
    FieldPlan,
    ValuePlan,
    enum_member_prefix,
    plain_value,
    public_stem,
)

DIRECTORY = "bindings/gobject"
SOURCE = f"{DIRECTORY}/src"
NAMESPACE = "MaplibreNativeFfi"
SYMBOL = "maplibre_native_ffi"
MACRO = "MAPLIBRE_NATIVE_FFI"
HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"
THREADS = "Runs on the thread-default main context of the thread that started the call."
CALL = "call_"

GLIB_SCALARS = {
    "bool": "gboolean",
    "_Bool": "gboolean",
    "float": "gfloat",
    "double": "gdouble",
    "char": "gchar",
    "signed char": "gint8",
    "unsigned char": "guint8",
    "int8_t": "gint8",
    "uint8_t": "guint8",
    "short": "gshort",
    "unsigned short": "gushort",
    "int16_t": "gint16",
    "uint16_t": "guint16",
    "int": "gint",
    "unsigned int": "guint",
    "int32_t": "gint32",
    "uint32_t": "guint32",
    "long": "glong",
    "unsigned long": "gulong",
    "long long": "gint64",
    "unsigned long long": "guint64",
    "int64_t": "gint64",
    "uint64_t": "guint64",
    "size_t": "gsize",
    "ptrdiff_t": "gssize",
    "intptr_t": "gintptr",
    "uintptr_t": "guintptr",
}
BOOLEANS = frozenset({"bool", "_Bool"})
# Each record's own functions, which a member's accessor must not take.
RECORD_FUNCTIONS = frozenset({"new", "copy", "free", "get_type", "to_options"})


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


def define_name(constant) -> str:
    """The GObject macro of a C constant macro."""
    return f"{MACRO}_" + constant.name.removeprefix("MLN_")


def accessor(record: ValuePlan, member: str) -> str:
    """The stem of a member's accessors, which a member named for a function
    every boxed type has, such as `type`, names with its record's noun:
    `get_event_type`."""
    if member not in {"type", "new", "copy", "free"}:
        return member
    words = [w for w in public_stem(record.native).split("_") if w != "info"]
    return f"{words[-1]}_{member}"


def ctype(value: ValuePlan) -> str:
    """The C spelling of a value's declared type."""
    return value.ctype.spelling


def without_const(spelling: str) -> str:
    return spelling.removeprefix("const ").strip()


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
    # The preprocessor condition of a function that serves only some platforms.
    guard: str | None = None

    def prototype(self) -> str:
        parameters = ", ".join(f"{t} {n}" for t, n in self.parameters) or "void"
        return f"{self.returns} {self.name}({parameters})"

    def comment(self) -> str:
        head = f"{self.name}:"
        if self.identifier_annotation:
            head += f" {self.identifier_annotation}"
        lines = [head]
        # g-ir-scanner reads an annotation only from a single line.
        for _, parameter in self.parameters:
            if parameter in self.annotations:
                lines.append(f"@{parameter}: {self.annotations[parameter]}")
        lines += ["", *(self.doc or [f"Calls `{self.name}`."])]
        if self.returns_doc:
            lines += ["", f"Returns: {self.returns_doc}"]
        return comment(lines)

    def declaration(self) -> str:
        return guarded(self.guard, self.comment() + self.prototype() + ";\n") + "\n"

    def definition(self) -> str:
        return guarded(self.guard, f"{self.prototype()} {{\n{self.body}}}\n") + "\n"


def guarded(condition: str | None, text: str) -> str:
    if condition is None:
        return text
    return f"#if {condition}\n{text}#endif\n"


def wrap(text: str) -> list[str]:
    """An annotation's lines, whose continuations gtk-doc reads indented."""
    return textwrap.wrap(
        text,
        docs.WIDTH - 3,
        subsequent_indent="  ",
        break_long_words=False,
        break_on_hyphens=False,
    )


def paragraph(text: str) -> list[str]:
    """A paragraph's lines."""
    return textwrap.wrap(
        text, docs.WIDTH - 3, break_long_words=False, break_on_hyphens=False
    )


# Between a slash and an asterisk, which together open or close a C comment.
COMMENT_MARK = re.compile(r"(?<=/)(?=\*)|(?<=\*)(?=/)")


def comment(lines: list[str]) -> str:
    """A gtk-doc comment, which must not open or close a comment inside."""
    body = "".join(
        f" * {COMMENT_MARK.sub(' ', line)}".rstrip() + "\n" for line in lines
    )
    return f"/**\n{body} */\n"


def fail_value(returns: str) -> str:
    """What a function returns when it fails."""
    if returns == "void":
        return ""
    if returns.endswith("*"):
        return "NULL"
    if returns == "gboolean":
        return "FALSE"
    if returns.startswith(NAMESPACE):
        return f"({returns})0"
    return "0"


class Values:
    """The records, unions, registrations, and callbacks that the generated
    operations reach, and their conversions."""

    def __init__(self, bound: BoundApi, symbol: docs.Symbol) -> None:
        self.bound = bound
        self.symbol = symbol
        # The records and unions that public API reaches, in first use order.
        self.used: dict[str, ValuePlan] = {}
        # The callbacks that a host registration carries.
        self.callbacks: dict[str, CallbackPlan] = {}
        self.guards: dict[str, str | None] = {}

    # Documentation.

    def doc_lines(self, native: str, extra: tuple[str, ...] = ()) -> list[str]:
        from dataclasses import replace

        doc = self.bound.doc(native)
        if doc is not None and extra:
            doc = replace(doc, notes=doc.notes + extra)
        return docs.markdown_lines(
            doc, docs.WIDTH - 3, self.symbol, errors="It fails with these errors:"
        )

    def summary(self, native: str) -> str:
        doc = self.bound.doc(native)
        if doc is None:
            return ""
        return " ".join(docs.markdown_lines(doc, 1000, self.symbol)[:1])

    # Classification.

    def record(self, native: str) -> ValuePlan:
        return self.bound.values[native]

    def transparent(self, value: ValuePlan) -> bool:
        """Whether a record is a transparent boxed struct of plain members."""
        if value.kind != "record" or value.registration or value.union_member:
            return False
        for member in value.fields:
            if member.role in {"size", "reserved"}:
                continue
            if member.role != "value" or member.presence is not None:
                return False
            kind = member.value.kind
            if kind == "record":
                if not self.transparent(member.value):
                    return False
            elif kind not in {"scalar", "enum"} or not plain_value(member.value):
                return False
        return True

    def form(self, value: ValuePlan) -> str | None:
        return async_hosts.form(self.bound, value)

    def hosted(self, value: ValuePlan) -> list[FieldPlan]:
        """The callbacks of a registration that a host supplies."""
        if not value.registration:
            return []
        form = self.form(value)
        if form == "rules":
            return []
        if form is None:
            raise Unsupported(
                f"GObject: {value.native}: a registration without a host form"
            )
        return [
            f
            for f in async_hosts.callback_fields(value)
            if not async_hosts.omitted(self.bound, f)
        ]

    def queued(self, value: ValuePlan) -> bool:
        """Whether a registration's callbacks run when the main context
        dispatches its queue, rather than on a native thread."""
        return self.form(value) in {"queue", "wake"} and bool(
            value.registration and value.registration.queue
        )

    def members(self, value: ValuePlan) -> list[FieldPlan]:
        """A record's members that a host reads or writes."""
        hosted = {f.name for f in self.hosted(value)}
        return [
            f
            for f in value.fields
            if f.public
            and (f.value.kind != "callback" or f.name in hosted)
            and f.role != "erased"
        ]

    def union_of(self, value: ValuePlan) -> FieldPlan | None:
        """The union member that a union-like type presents: the union of a
        record that holds only it and its tag."""
        return value.union_member

    def is_input(self, native: str) -> bool:
        return native in self.bound.inputs

    # Use.

    def use(self, value: ValuePlan | None) -> None:
        """Record that public API reaches a value, and every value it holds."""
        if value is None:
            return
        if value.kind in {"reference", "array"}:
            self.use(value.element)
            return
        if value.kind == "handle" and value.handle is None:
            return
        if value.kind not in {"record", "union"}:
            return
        native = value.native
        if native in self.bound.slices:
            self.use(self.bound.slices[native].value)
            return
        if native in self.used:
            return
        self.used[native] = self.record(native)
        record = self.used[native]
        if record.kind == "record" and record.union_member:
            for variant in record.union_member.value.fields:
                self.use(variant.value)
            return
        for member in self.members(record):
            if member.value.kind == "callback":
                callback = self.bound.callbacks[member.value.native]
                self.callbacks[callback.native] = callback
                for parameter in callback.arguments:
                    self.use(parameter.value)
                continue
            self.use(member.value)
        if record.options:
            self.use(self.record(record.options))

    # Types.

    def gtype(self, value: ValuePlan) -> str:
        """The GObject type that stores a plain value."""
        if value.kind == "enum":
            return type_name(value.native)
        if value.kind == "native_pointer":
            return "gpointer"
        if value.kind == "scalar":
            carrier = value.scalar_carrier or value.native
            scalar = GLIB_SCALARS.get(carrier) or GLIB_SCALARS.get(
                without_const(value.ctype.canonical)
            )
            if scalar is None:
                raise Unsupported(f"GObject: scalar {carrier} has no GLib type")
            return scalar
        raise Unsupported(f"GObject: {value.kind} {value.native} is not a plain value")

    def boolean(self, value: ValuePlan) -> bool:
        return value.kind == "scalar" and (
            (value.scalar_carrier or value.native) in BOOLEANS
            or without_const(value.ctype.canonical) in BOOLEANS
        )

    def to_c(self, value: ValuePlan, expression: str) -> str:
        """A plain GObject value as its C type."""
        if self.boolean(value):
            return f"({expression}) != FALSE"
        if value.kind == "native_pointer":
            return f"({without_const(ctype(value))})({expression})"
        return f"({without_const(ctype(value))})({expression})"

    def from_c(self, value: ValuePlan, expression: str) -> str:
        """A plain C value as its GObject type."""
        if self.boolean(value):
            return f"({expression}) ? TRUE : FALSE"
        if value.kind == "native_pointer":
            return f"(gpointer)({expression})"
        return f"({self.gtype(value)})({expression})"

    def text(self, value: ValuePlan) -> bool:
        return value.kind == "buffer" and value.encoding in {"utf8", "json", None}

    def bytes_(self, value: ValuePlan) -> bool:
        return value.kind == "buffer" and value.encoding == "bytes"

    def element_type(self, value: ValuePlan) -> str:
        """The `element-type` of a record array."""
        assert value.element is not None
        return type_name(value.element.native)

    # Members: storage, defaults, copies, frees, accessors, and conversions.

    def storage(self, value: ValuePlan) -> str:
        """The C type that an opaque value stores a member's value as."""
        kind = value.kind
        if kind in {"scalar", "enum", "native_pointer"}:
            return self.gtype(value)
        if self.text(value):
            return "char*"
        if self.bytes_(value):
            return "GBytes*"
        if kind == "array":
            element = value.element
            assert element is not None
            if element.kind == "record":
                return "GPtrArray*"
            if self.text(element) and element.buffer_form == "view":
                return "char**"
            raise Unsupported(f"GObject: an array of {element.kind} members")
        if kind == "record":
            if self.transparent(value):
                return type_name(value.native)
            return f"{type_name(value.native)}*"
        if kind == "union":
            return f"{type_name(value.native)}*"
        raise Unsupported(f"GObject: a {kind} member")

    def default(self, member: FieldPlan, place: str) -> str:
        """The statements that set a new value's member to its default."""
        value = member.value
        if self.text(value) and not value.nullable:
            return f'  {place} = g_strdup("");\n'
        if value.kind == "array" and not value.nullable:
            element = value.element
            assert element is not None
            if element.kind == "record":
                return f"  {place} = g_ptr_array_new_with_free_func((GDestroyNotify){symbol_name(element.native)}_free);\n"
            return f"  {place} = g_new0(char*, 1);\n"
        if (
            value.kind == "record"
            and not self.transparent(value)
            and not value.union_member
        ):
            return f"  {place} = {symbol_name(value.native)}_new();\n"
        return ""

    def copy(self, value: ValuePlan, place: str, source: str) -> str:
        """The statements that deep copy a member that a shallow copy shares."""
        if self.text(value):
            return f"  {place} = g_strdup({source});\n"
        if self.bytes_(value):
            return f"  {place} = {source} != NULL ? g_bytes_ref({source}) : NULL;\n"
        if value.kind == "array":
            element = value.element
            assert element is not None
            if element.kind == "record":
                prefix = symbol_name(element.native)
                return (
                    f"  {place} = {source} != NULL ? g_ptr_array_copy({source}, "
                    f"{prefix}_copy_func, NULL) : NULL;\n"
                    f"  if ({place} != NULL) g_ptr_array_set_free_func({place}, (GDestroyNotify){prefix}_free);\n"
                )
            return f"  {place} = g_strdupv({source});\n"
        if value.kind in {"record", "union"} and not self.transparent(value):
            prefix = symbol_name(value.native)
            return f"  {place} = {source} != NULL ? {prefix}_copy({source}) : NULL;\n"
        return ""

    def free(self, value: ValuePlan, place: str) -> str:
        if self.text(value):
            return f"  g_free({place});\n"
        if self.bytes_(value):
            return f"  g_clear_pointer(&{place}, g_bytes_unref);\n"
        if value.kind == "array":
            element = value.element
            assert element is not None
            if element.kind == "record":
                return f"  g_clear_pointer(&{place}, g_ptr_array_unref);\n"
            return f"  g_strfreev({place});\n"
        if value.kind in {"record", "union"} and not self.transparent(value):
            return f"  g_clear_pointer(&{place}, {symbol_name(value.native)}_free);\n"
        return ""

    def getter(self, value: ValuePlan, place: str) -> tuple[str, str, str]:
        """A member's getter: its type, return expression, and return
        annotation."""
        kind = value.kind
        if kind in {"scalar", "enum", "native_pointer"}:
            return self.gtype(value), place, "the value"
        if self.text(value):
            nullable = " (nullable)" if value.nullable else ""
            return "const char*", place, f"(transfer none){nullable}: the text"
        if self.bytes_(value):
            return "GBytes*", place, "(transfer none) (nullable): the bytes"
        if kind == "array":
            element = value.element
            assert element is not None
            nullable = " (nullable)" if value.nullable else ""
            if element.kind == "record":
                return (
                    "GPtrArray*",
                    place,
                    f"(transfer none) (element-type {self.element_type(value)}){nullable}: the items",
                )
            return (
                "const char* const*",
                f"(const char* const*){place}",
                f"(transfer none) (array zero-terminated=1){nullable}: the texts",
            )
        name = type_name(value.native)
        if kind == "record" and self.transparent(value):
            # The member lives in the value, which a getter takes as const,
            # and a binding copies it out.
            return f"{name}*", f"({name}*)&{place}", "(transfer none): the value"
        nullable = " (nullable)" if kind == "union" or value.union_member else ""
        return f"{name}*", place, f"(transfer none){nullable}: the value"

    def setter(
        self, value: ValuePlan, place: str, name: str
    ) -> tuple[list[tuple[str, str]], dict[str, str], str]:
        """A member's setter: its parameters after the instance, their
        annotations, and its body."""
        kind = value.kind
        nullable = " (nullable)" if value.nullable else ""
        if kind in {"scalar", "enum", "native_pointer"}:
            return [(self.gtype(value), name)], {}, f"  {place} = {name};\n"
        if self.text(value):
            body = f"  char* previous = {place};\n"
            if value.nullable:
                body += f"  {place} = g_strdup({name});\n"
            else:
                body += f'  {place} = g_strdup({name} != NULL ? {name} : "");\n'
            body += "  g_free(previous);\n"
            return (
                [("const char*", name)],
                {name: f"{nullable.strip()}: the UTF-8 text".lstrip(": ")},
                body,
            )
        if self.bytes_(value):
            return (
                [("GBytes*", name)],
                {name: "(nullable): the bytes"},
                (
                    f"  GBytes* previous = {place};\n"
                    f"  {place} = {name} != NULL ? g_bytes_ref({name}) : NULL;\n"
                    "  if (previous != NULL) g_bytes_unref(previous);\n"
                ),
            )
        if kind == "array":
            element = value.element
            assert element is not None
            if element.kind == "record":
                prefix = symbol_name(element.native)
                count = f"{name}_count"
                # A plain record's array is flat, as C's is.
                flat = self.transparent(element)
                item = f"&{name}[index]" if flat else f"{name}[index]"
                body = (
                    f"  GPtrArray* previous = {place};\n"
                    f"  {place} = g_ptr_array_new_full((guint){count}, (GDestroyNotify){prefix}_free);\n"
                    f"  for (gsize index = 0; index < {count}; index++) {{\n"
                    f"    g_ptr_array_add({place}, {prefix}_copy({item}));\n  }}\n"
                    "  if (previous != NULL) g_ptr_array_unref(previous);\n"
                )
                if value.nullable:
                    body = (
                        f"  if ({name} == NULL) {{\n"
                        f"    g_clear_pointer(&{place}, g_ptr_array_unref);\n"
                        "    return;\n  }\n" + body
                    )
                array_type = (
                    f"const {type_name(element.native)}*"
                    if flat
                    else f"{type_name(element.native)}* const*"
                )
                return (
                    [(array_type, name), ("gsize", count)],
                    {name: f"(array length={count}){nullable}: the items"},
                    body,
                )
            body = f"  char** previous = {place};\n"
            if value.nullable:
                body += f"  {place} = g_strdupv((char**){name});\n"
            else:
                body += f"  {place} = {name} != NULL ? g_strdupv((char**){name}) : g_new0(char*, 1);\n"
            body += "  g_strfreev(previous);\n"
            return (
                [("const char* const*", name)],
                {name: f"(array zero-terminated=1){nullable}: the texts"},
                body,
            )
        type_ = type_name(value.native)
        prefix = symbol_name(value.native)
        if kind == "record" and self.transparent(value):
            return (
                [(f"const {type_}*", name)],
                {name: "the value"},
                f"  g_return_if_fail({name} != NULL);\n  {place} = *{name};\n",
            )
        return (
            [(f"const {type_}*", name)],
            {name: "the value"},
            (
                f"  g_return_if_fail({name} != NULL);\n"
                f"  {type_}* previous = {place};\n"
                f"  {place} = {prefix}_copy({name});\n"
                f"  if (previous != NULL) {prefix}_free(previous);\n"
            ),
        )

    def write(self, record: ValuePlan, member: FieldPlan, place: str, out: str) -> str:
        """The statements that write a member of `record`, stored at `place`,
        into the C record at `out`."""
        value = member.value
        target = f"{out}->{member.name}"
        kind = value.kind
        if kind in {"scalar", "enum", "native_pointer"}:
            return f"  {target} = {self.to_c(value, place)};\n"
        if self.text(value) or self.bytes_(value):
            view = (
                f"maplibre_native_ffi_text_view({place})"
                if self.text(value)
                else f"maplibre_native_ffi_bytes_view({place})"
            )
            if value.buffer_form == "view":
                return f"  {target} = {view};\n"
            if value.length is None:
                return f"  {target} = {place};\n"
            local = f"{member.name}_view"
            return (
                f"  {{\n    mln_buffer_view {local} = {view};\n"
                f"    {target} = {local}.data;\n"
                f"    {out}->{value.length} = {local}.size;\n  }}\n"
            )
        if kind == "array":
            element = value.element
            assert element is not None and value.length is not None
            count = f"{out}->{value.length}"
            if element.kind == "record":
                native = element.native
                return (
                    f"  if ({place} != NULL && {place}->len > 0) {{\n"
                    f"    {native}* items = maplibre_native_ffi_call_alloc(call, {place}->len, sizeof *items);\n"
                    f"    for (guint index = 0; index < {place}->len; index++) {{\n"
                    f"      {symbol_name(native)}_to_native({place}->pdata[index], &items[index], call);\n    }}\n"
                    f"    {target} = items;\n    {count} = {place}->len;\n  }}\n"
                )
            return (
                f"  if ({place} != NULL && {place}[0] != NULL) {{\n"
                f"    guint count = g_strv_length({place});\n"
                "    mln_buffer_view* items = maplibre_native_ffi_call_alloc(call, count, sizeof *items);\n"
                "    for (guint index = 0; index < count; index++) {\n"
                f"      items[index] = maplibre_native_ffi_text_view({place}[index]);\n    }}\n"
                f"    {target} = items;\n    {count} = count;\n  }}\n"
            )
        prefix = symbol_name(value.native)
        if kind == "record":
            if self.transparent(value):
                return f"  {prefix}_to_native(&{place}, &{target}, call);\n"
            return f"  if ({place} != NULL) {prefix}_to_native({place}, &{target}, call);\n"
        if kind == "union":
            tag = member.presence.tag if member.presence else value.tag
            return f"  if ({place} != NULL) {prefix}_to_native({place}, &{out}->{tag}, &{target}, call);\n"
        raise Unsupported(f"GObject: {record.native}.{member.name}: a {kind} member")

    def read(
        self,
        record: ValuePlan,
        member: FieldPlan,
        source: str,
        place: str,
        free: bool = True,
    ) -> str:
        """The statements that copy a member of the C record at `source` into
        `place`, after freeing the default there when `free`."""
        value = member.value
        kind = value.kind
        origin = f"{source}->{member.name}"
        release = self.free(value, place) if free else ""
        if kind in {"scalar", "enum", "native_pointer"}:
            return f"  {place} = {self.from_c(value, origin)};\n"
        if self.text(value):
            if value.buffer_form == "view":
                copied = (
                    f"{origin}.data != NULL ? maplibre_native_ffi_text_new({origin}) : NULL"
                    if value.nullable
                    else f"maplibre_native_ffi_text_new({origin})"
                )
            elif value.length is not None:
                copied = f"maplibre_native_ffi_text_new_sized({origin}, {source}->{value.length})"
            else:
                copied = f'g_strdup({origin} != NULL ? (const char*){origin} : "")'
            return release + f"  {place} = {copied};\n"
        if self.bytes_(value):
            if value.buffer_form == "view":
                copied = f"maplibre_native_ffi_bytes_new({origin}.data, {origin}.size)"
            else:
                assert value.length is not None
                copied = (
                    f"maplibre_native_ffi_bytes_new({origin}, {source}->{value.length})"
                )
            return release + f"  {place} = {copied};\n"
        if kind == "array":
            element = value.element
            assert element is not None and value.length is not None
            count = f"{source}->{value.length}"
            if element.kind == "record":
                native = element.native
                prefix = symbol_name(native)
                item = f"&{origin}[index]"
                if value.stride:
                    item = f"(const {native}*)(const void*)((const char*){origin} + index * {source}->{value.stride})"
                return (
                    release
                    + f"  {place} = g_ptr_array_new_full((guint){count}, (GDestroyNotify){prefix}_free);\n"
                    f"  for (size_t index = 0; index < {count}; index++) {{\n"
                    f"    g_ptr_array_add({place}, {prefix}_from_native({item}));\n  }}\n"
                )
            return (
                release + f"  {place} = g_new0(char*, {count} + 1);\n"
                f"  for (size_t index = 0; index < {count}; index++) {{\n"
                f"    {place}[index] = maplibre_native_ffi_text_new({origin}[index]);\n  }}\n"
            )
        prefix = symbol_name(value.native)
        if kind == "record":
            if self.transparent(value):
                return f"  {prefix}_read(&{place}, &{origin});\n"
            return release + f"  {place} = {prefix}_from_native(&{origin});\n"
        if kind == "union":
            tag = member.presence.tag if member.presence else value.tag
            return (
                release
                + f"  {place} = {prefix}_from_native((gint64){source}->{tag}, &{origin});\n"
            )
        raise Unsupported(f"GObject: {record.native}.{member.name}: a {kind} member")

    # Rendering.

    def ordered(self) -> list[ValuePlan]:
        """The used values, each after the values that it embeds by value."""
        order: list[str] = []

        def visit(native: str) -> None:
            if native in order:
                return
            value = self.used[native]
            for member in value.fields:
                inner = member.value
                if inner.kind == "array" and inner.element is not None:
                    inner = inner.element
                if inner.kind in {"record", "union"} and inner.native in self.used:
                    visit(inner.native)
            if value.union_member:
                for variant in value.union_member.value.fields:
                    if variant.value.kind == "record":
                        visit(variant.value.native)
            order.append(native)

        for native in sorted(self.used):
            visit(native)
        return [self.used[native] for native in order]

    def presented(self) -> list[ValuePlan]:
        """The used values that the library presents as types: each but the
        union that a union-like record presents in its place."""
        hidden = {
            value.union_member.value.native
            for value in self.used.values()
            if value.kind == "record" and value.union_member
        }
        return [v for v in self.ordered() if v.native not in hidden]

    def render(self, handles: list[str]) -> tuple[str, str, str]:
        """The public header, the source, and the private header.

        The header declares the opaque types first, then the transparent
        structs, which embed only each other, then the callbacks, whose
        parameters name both, and then every type's functions.
        """
        values = self.presented()
        # A callback's parameters name the handle classes, which the handles'
        # header defines.
        header = "".join(
            f"typedef struct _{type_name(h)} {type_name(h)};\n" for h in handles
        )
        header += "".join(self.forward(v) for v in values) + "\n"
        header += "".join(
            self.transparent_struct(v) for v in values if self.transparent(v)
        )
        header += "".join(self.callback_typedef(c) for c in self.sorted_callbacks())
        source, private = "", ""
        for value in values:
            h, s, p = self.render_value(value)
            header += h
            source += s
            private += p
        return header, source, private

    def sorted_callbacks(self) -> list[CallbackPlan]:
        return [self.callbacks[name] for name in sorted(self.callbacks)]

    def forward(self, value: ValuePlan) -> str:
        name = type_name(value.native)
        if self.transparent(value):
            return ""
        return f"typedef struct _{name} {name};\n"

    def render_value(self, value: ValuePlan) -> tuple[str, str, str]:
        if value.kind == "union":
            return self.render_union(value, value)
        if value.union_member:
            return self.render_union(value, value.union_member.value)
        if self.transparent(value):
            return self.render_transparent(value)
        return self.render_opaque(value)

    def type_comment(
        self, value: ValuePlan, members: tuple[str, ...] | list[str] = ()
    ) -> str:
        name = type_name(value.native)
        return comment([f"{name}:", *members, "", *self.doc_lines(value.native)])

    def boxed_header(self, value: ValuePlan, functions: list[Function]) -> str:
        prefix = symbol_name(value.native)
        out = f"GType {prefix}_get_type(void) G_GNUC_CONST;\n"
        out += f"#define {type_macro(value.native)} ({prefix}_get_type())\n\n"
        out += "".join(f.declaration() for f in functions)
        out += f"G_DEFINE_AUTOPTR_CLEANUP_FUNC({type_name(value.native)}, {prefix}_free)\n\n"
        return out

    def lifecycle(
        self, value: ValuePlan, new_body: str, copy_body: str, free_body: str
    ) -> list[Function]:
        """A value's constructor, without `new_body`, and its copy and free."""
        name, prefix = type_name(value.native), symbol_name(value.native)
        functions = [
            Function(
                f"{prefix}_new",
                f"{name}*",
                [],
                new_body,
                doc=[f"Creates a `{name}` whose members are zero or empty."],
                identifier_annotation="(constructor)",
                returns_doc="(transfer full): the new value",
            ),
            Function(
                f"{prefix}_copy",
                f"{name}*",
                [(f"const {name}*", "self")],
                copy_body,
                doc=["Copies the value."],
                returns_doc="(transfer full): the copy",
            ),
            Function(
                f"{prefix}_free",
                "void",
                [(f"{name}*", "self")],
                free_body,
                doc=["Frees the value."],
                annotations={"self": "(transfer full) (nullable): the value"},
            ),
        ]
        return functions if new_body else functions[1:]

    def private_prototypes(self, value: ValuePlan) -> str:
        name, prefix, native = (
            type_name(value.native),
            symbol_name(value.native),
            value.native,
        )
        if value.kind == "union":
            tag = self.tag_type(value)
            return (
                f"G_GNUC_INTERNAL void {prefix}_to_native(\n"
                f"  const {name}* self, {tag}* out_tag, {native}* out,\n"
                "  MaplibreNativeFfiCall* call\n);\n"
                f"G_GNUC_INTERNAL {name}* {prefix}_from_native(\n"
                f"  gint64 tag, const {native}* in\n);\n"
            )
        out = (
            f"G_GNUC_INTERNAL void {prefix}_to_native(\n"
            f"  const {name}* self, {native}* out, MaplibreNativeFfiCall* call\n);\n"
            f"G_GNUC_INTERNAL {name}* {prefix}_from_native(const {native}* in);\n"
            f"G_GNUC_INTERNAL gpointer {prefix}_copy_func(gconstpointer self, gpointer data);\n"
        )
        if self.transparent(value):
            out += f"G_GNUC_INTERNAL void {prefix}_read({name}* out, const {native}* in);\n"
        return out

    def copy_func(self, value: ValuePlan) -> str:
        name, prefix = type_name(value.native), symbol_name(value.native)
        return (
            f"gpointer {prefix}_copy_func(gconstpointer self, gpointer data) {{\n"
            "  (void)data;\n"
            f"  return {prefix}_copy((const {name}*)self);\n}}\n\n"
        )

    def transparent_struct(self, value: ValuePlan) -> str:
        name, native = type_name(value.native), value.native
        members = [f for f in value.fields if f.public]
        lines = []
        for member in members:
            summary = self.summary(f"{native}.{member.name}")
            lines += wrap(f"@{member.name}: {summary or member.name}")
        header = self.type_comment(value, lines)
        header += "typedef struct {\n"
        for member in members:
            header += f"  {self.storage(member.value)} {member.name};\n"
        if not members:
            # A C struct has at least one member.
            header += "  guint8 reserved;\n"
        header += f"}} {name};\n\n"
        return header

    def render_transparent(self, value: ValuePlan) -> tuple[str, str, str]:
        name, prefix, native = (
            type_name(value.native),
            symbol_name(value.native),
            value.native,
        )
        members = [f for f in value.fields if f.public]
        header = ""
        # A host creates a plain struct as it would any struct, so it has no
        # constructor, which would make Vala treat it as a class.
        functions = self.lifecycle(
            value,
            "",
            "  g_return_val_if_fail(self != NULL, NULL);\n  return g_memdup2(self, sizeof *self);\n",
            "  g_free(self);\n",
        )
        functions += self.option_conversion(value)
        header += self.boxed_header(value, functions)
        source = (
            f"G_DEFINE_BOXED_TYPE({name}, {prefix}, {prefix}_copy, {prefix}_free)\n\n"
        )
        source += "".join(f.definition() for f in functions)
        source += self.copy_func(value)
        source += (
            f"void {prefix}_to_native(\n  const {name}* self, {native}* out, MaplibreNativeFfiCall* call\n) {{\n"
            "  (void)call;\n  memset(out, 0, sizeof *out);\n"
            + self.size_line(value, "out")
            + "".join(
                self.write(value, member, f"self->{member.name}", "out")
                for member in members
            )
            + "}\n\n"
        )
        source += (
            f"void {prefix}_read({name}* out, const {native}* in) {{\n"
            + "".join(
                self.read(value, member, "in", f"out->{member.name}", free=False)
                for member in members
            )
            + "}\n\n"
        )
        source += (
            f"{name}* {prefix}_from_native(const {native}* in) {{\n"
            f"  {name}* self = g_new0({name}, 1);\n"
            f"  {prefix}_read(self, in);\n  return self;\n}}\n\n"
        )
        return header, source, self.private_prototypes(value)

    def size_line(self, value: ValuePlan, out: str) -> str:
        return "".join(
            f"  {out}->{member.name} = sizeof *{out};\n"
            for member in value.fields
            if member.role == "size"
        )

    def masks(self, value: ValuePlan) -> list[str]:
        return [f.name for f in value.fields if f.role == "presence_mask"]

    def render_opaque(self, value: ValuePlan) -> tuple[str, str, str]:
        name, prefix, native = (
            type_name(value.native),
            symbol_name(value.native),
            value.native,
        )
        members = self.members(value)
        hosted = {f.name for f in self.hosted(value)}
        data = [m for m in members if m.name not in hosted]
        callbacks = [m for m in members if m.name in hosted]
        masks = self.masks(value)
        input_ = self.is_input(native)
        struct = f"struct _{name} {{\n"
        struct += "".join(f"  guint64 {mask};\n" for mask in masks)
        struct += "".join(f"  {self.storage(m.value)} {m.name};\n" for m in data)
        struct += "".join(f"  MaplibreNativeFfiClosure* {m.name};\n" for m in callbacks)
        if not (masks or data or callbacks):
            struct += "  guint8 reserved;\n"
        struct += "};\n\n"
        new_body = f"  {name}* self = g_new0({name}, 1);\n"
        new_body += "".join(self.default(m, f"self->{m.name}") for m in data)
        new_body += "  return self;\n"
        copy_body = (
            "  g_return_val_if_fail(self != NULL, NULL);\n"
            f"  {name}* copy = g_memdup2(self, sizeof *self);\n"
        )
        copy_body += "".join(
            self.copy(m.value, f"copy->{m.name}", f"self->{m.name}") for m in data
        )
        copy_body += "".join(
            f"  maplibre_native_ffi_closure_ref(copy->{m.name});\n" for m in callbacks
        )
        copy_body += "  return copy;\n"
        free_body = "  if (self == NULL) return;\n"
        free_body += "".join(self.free(m.value, f"self->{m.name}") for m in data)
        free_body += "".join(
            f"  maplibre_native_ffi_closure_unref(self->{m.name});\n" for m in callbacks
        )
        free_body += "  g_free(self);\n"
        functions = self.lifecycle(value, new_body, copy_body, free_body)
        for member in data:
            functions += self.accessors(value, member, input_)
        for member in callbacks:
            functions.append(self.callback_setter(value, member))
        functions += self.option_conversion(value)
        header = self.type_comment(value) + self.boxed_header(value, functions)
        source = struct
        source += (
            f"G_DEFINE_BOXED_TYPE({name}, {prefix}, {prefix}_copy, {prefix}_free)\n\n"
        )
        source += "".join(f.definition() for f in functions)
        source += self.copy_func(value)
        source += "".join(
            self.trampoline(value, member, index)
            for index, member in enumerate(callbacks)
        )
        source += self.opaque_to_native(value, data, callbacks)
        source += f"{name}* {prefix}_from_native(const {native}* in) {{\n"
        source += f"  {name}* self = {prefix}_new();\n"
        if not (masks or data):
            source += "  (void)in;\n"
        source += "".join(f"  self->{mask} = (guint64)in->{mask};\n" for mask in masks)
        for member in data:
            read = self.read(value, member, "in", f"self->{member.name}")
            bit = member.presence.bit if member.presence else None
            if bit and member.presence and member.presence.mask:
                read = f"  if ((in->{member.presence.mask} & {bit}) != 0) {{\n{textwrap.indent(read, '  ')}  }}\n"
            source += read
        source += "  return self;\n}\n\n"
        return header, source, self.private_prototypes(value)

    def accessors(
        self, value: ValuePlan, member: FieldPlan, input_: bool
    ) -> list[Function]:
        name, prefix = type_name(value.native), symbol_name(value.native)
        stem = accessor(value, member.name)
        documented = f"{value.native}.{member.name}"
        doc = self.doc_lines(documented) or [f"The `{member.name}` member."]
        place = f"self->{member.name}"
        returns, expression, annotation = self.getter(member.value, place)
        default = fail_value(returns)
        guard = (
            f"  g_return_val_if_fail(self != NULL, {default});\n"
            if default
            else "  g_return_val_if_fail(self != NULL, 0);\n"
        )
        functions = [
            Function(
                f"{prefix}_get_{stem}",
                returns,
                [(f"const {name}*", "self")],
                guard + f"  return {expression};\n",
                doc=doc,
                returns_doc=annotation,
            )
        ]
        presence = member.presence
        if presence and presence.mask and presence.bit:
            functions.append(
                Function(
                    f"{prefix}_has_{stem}",
                    "gboolean",
                    [(f"const {name}*", "self")],
                    "  g_return_val_if_fail(self != NULL, FALSE);\n"
                    f"  return (self->{presence.mask} & {presence.bit}) != 0;\n",
                    doc=[f"Whether the value sets `{prefix}_set_{stem}`."]
                    if input_
                    else [f"Whether the value has `{prefix}_get_{stem}`."],
                    returns_doc="whether the member is set",
                )
            )
        if input_:
            parameters, annotations, body = self.setter(member.value, place, "value")
            if presence and presence.mask and presence.bit:
                body += f"  self->{presence.mask} |= {presence.bit};\n"
            functions.append(
                Function(
                    f"{prefix}_set_{stem}",
                    "void",
                    [(f"{name}*", "self"), *parameters],
                    "  g_return_if_fail(self != NULL);\n" + body,
                    doc=doc,
                    annotations=annotations,
                )
            )
        return functions

    def callback_setter(self, value: ValuePlan, member: FieldPlan) -> Function:
        name, prefix = type_name(value.native), symbol_name(value.native)
        callback = self.bound.callbacks[member.value.native]
        documented = f"{value.native}.{member.name}"
        doc = self.doc_lines(documented) or self.doc_lines(callback.native)
        native = self.form(value) == "native"
        skip = " (skip)" if native else ""
        return Function(
            f"{prefix}_set_{member.name}",
            "void",
            [
                (f"{name}*", "self"),
                (type_name(callback.native), "callback"),
                ("gpointer", "user_data"),
                ("GDestroyNotify", "notify"),
            ],
            "  g_return_if_fail(self != NULL);\n"
            f"  MaplibreNativeFfiClosure* previous = self->{member.name};\n"
            f"  self->{member.name} = callback != NULL ? maplibre_native_ffi_closure_new((GCallback)callback, user_data, notify) : NULL;\n"
            "  if (callback == NULL && notify != NULL) notify(user_data);\n"
            "  maplibre_native_ffi_closure_unref(previous);\n",
            doc=doc or [f"Sets the `{member.name}` callback."],
            annotations={
                "callback": "(scope notified) (closure user_data) (destroy notify) (nullable): the callback",
                "user_data": "the data to pass to `callback`",
                "notify": "the function that frees `user_data` after the last call",
            },
            identifier_annotation=skip.strip(),
        )

    def opaque_to_native(
        self, value: ValuePlan, data: list[FieldPlan], callbacks: list[FieldPlan]
    ) -> str:
        name, prefix, native = (
            type_name(value.native),
            symbol_name(value.native),
            value.native,
        )
        body = "  memset(out, 0, sizeof *out);\n" + self.size_line(value, "out")
        if not (data or callbacks or self.masks(value)):
            body = "  (void)self;\n" + body
        for mask in self.masks(value):
            mask_field = next(f for f in value.fields if f.name == mask)
            body += f"  out->{mask} = ({without_const(ctype(mask_field.value))})self->{mask};\n"
        for member in data:
            body += self.write(value, member, f"self->{member.name}", "out")
        registration = value.registration
        if registration and callbacks:
            closures = ", ".join(f"self->{m.name}" for m in callbacks)
            queue = f"&out->{registration.queue}" if self.queued(value) else "NULL"
            present = " || ".join(f"self->{m.name} != NULL" for m in callbacks)
            body += f"  if ({present}) {{\n"
            body += (
                f"    MaplibreNativeFfiClosure* const closures[] = {{{closures}}};\n"
            )
            body += f"    out->{registration.user_data} = maplibre_native_ffi_call_register(call, closures, {len(callbacks)}, {queue});\n"
            body += f"    out->{registration.release} = maplibre_native_ffi_registration_release;\n"
            for member in callbacks:
                body += f"    out->{member.name} = self->{member.name} != NULL ? {self.trampoline_name(value, member)} : NULL;\n"
            body += "  }\n"
        return (
            f"void {prefix}_to_native(\n  const {name}* self, {native}* out, MaplibreNativeFfiCall* call\n) {{\n"
            + ("  (void)call;\n" if not self.needs_call(value) else "")
            + body
            + "}\n\n"
        )

    def needs_call(self, value: ValuePlan) -> bool:
        """Whether a value's conversion uses the call's scope."""
        if value.registration and self.hosted(value):
            return True
        for member in value.fields:
            if not member.public:
                continue
            inner = member.value
            if inner.kind == "array":
                return True
            if inner.kind in {"record", "union"}:
                return True
        return False

    def option_conversion(self, value: ValuePlan) -> list[Function]:
        """The conversion of a complete record to the option bag whose members
        it sets."""
        if not value.options:
            return []
        options = self.record(value.options)
        name, prefix = type_name(value.native), symbol_name(value.native)
        target, target_prefix = type_name(options.native), symbol_name(options.native)
        body = (
            "  g_return_val_if_fail(self != NULL, NULL);\n"
            f"  {target}* options = {target_prefix}_new();\n"
        )
        transparent = self.transparent(value)
        for member in value.fields:
            if not member.public:
                continue
            stem = accessor(options, member.name)
            if transparent:
                argument = f"self->{member.name}"
                if member.value.kind == "record":
                    argument = f"&{argument}"
            else:
                argument = f"{prefix}_get_{accessor(value, member.name)}(self)"
            body += f"  {target_prefix}_set_{stem}(options, {argument});\n"
        body += "  return options;\n"
        return [
            Function(
                f"{prefix}_to_options",
                f"{target}*",
                [(f"const {name}*", "self")],
                body,
                doc=self.doc_lines(f"{value.native}#options"),
                returns_doc="(transfer full): the options",
            )
        ]

    # Unions.

    def tag_field(self, value: ValuePlan, union: ValuePlan) -> FieldPlan | None:
        """The tag member of a union-like record."""
        if value.kind != "record":
            return None
        member = value.union_member
        assert member is not None
        tag = member.presence.tag if member.presence else member.value.tag
        return next(f for f in value.fields if f.name == tag)

    def tag_type(self, union: ValuePlan) -> str:
        """The C type of a standalone union's tag, from the record that holds
        it."""
        for record in self.bound.values.values():
            for member in record.fields:
                if member.value.kind == "union" and member.value.native == union.native:
                    tag = member.presence.tag if member.presence else member.value.tag
                    found = next(f for f in record.fields if f.name == tag)
                    return without_const(ctype(found.value))
        raise Unsupported(f"GObject: {union.native}: a union without a tag")

    def empty_variant(self, value: ValuePlan, union: ValuePlan):
        empty = union.empty_variant
        if empty is None and value is not union:
            member = value.union_member
            empty = member.value.empty_variant if member else None
        return empty

    def variant_stem(self, union: ValuePlan, constant: str) -> str:
        names = [c for c, _ in self.tag_constants(union)]
        prefix = enum_member_prefix(names or [constant])
        return constant.removeprefix(prefix).lower()

    def tag_constants(self, union: ValuePlan) -> list[tuple[str, int]]:
        variants = [f.presence.variant for f in union.fields if f.presence]
        enum = next(
            (
                e
                for e in self.bound.source.enums
                if any(item.name in variants for item in e.values)
            ),
            None,
        )
        return [(item.name, item.value) for item in enum.values] if enum else []

    def render_union(self, value: ValuePlan, union: ValuePlan) -> tuple[str, str, str]:
        """A tagged union, or a record that presents one: an opaque type with
        a constructor, a predicate, and a getter for each variant."""
        name, prefix, native = (
            type_name(value.native),
            symbol_name(value.native),
            value.native,
        )
        variants = list(union.fields)
        struct = f"struct _{name} {{\n  gint64 tag;\n"
        struct += "".join(f"  {self.storage(v.value)} {v.name};\n" for v in variants)
        struct += "};\n\n"
        free_body = "  if (self == NULL) return;\n"
        free_body += "".join(self.free(v.value, f"self->{v.name}") for v in variants)
        free_body += "  g_free(self);\n"
        copy_body = (
            "  g_return_val_if_fail(self != NULL, NULL);\n"
            f"  {name}* copy = g_memdup2(self, sizeof *self);\n"
        )
        copy_body += "".join(
            self.copy(v.value, f"copy->{v.name}", f"self->{v.name}") for v in variants
        )
        copy_body += "  return copy;\n"
        functions = [
            Function(
                f"{prefix}_copy",
                f"{name}*",
                [(f"const {name}*", "self")],
                copy_body,
                doc=["Copies the value."],
                returns_doc="(transfer full): the copy",
            ),
            Function(
                f"{prefix}_free",
                "void",
                [(f"{name}*", "self")],
                free_body,
                doc=["Frees the value."],
                annotations={"self": "(transfer full) (nullable): the value"},
            ),
        ]
        input_ = self.is_input(native) or self.is_input(union.native)
        for variant in variants:
            assert variant.presence and variant.presence.variant
            constant = variant.presence.variant
            documented = f"{union.native}.{variant.name}"
            doc = self.doc_lines(documented) or [f"The `{variant.name}` variant."]
            place = f"self->{variant.name}"
            if input_:
                parameters, annotations, body = self.setter(
                    variant.value, place, "value"
                )
                check = "  g_return_if_fail(value != NULL);\n"
                precondition = ""
                if check in body:
                    body = body.replace(check, "")
                    precondition = "  g_return_val_if_fail(value != NULL, NULL);\n"
                functions.append(
                    Function(
                        f"{prefix}_new_{variant.name}",
                        f"{name}*",
                        parameters,
                        precondition + f"  {name}* self = g_new0({name}, 1);\n"
                        f"  self->tag = {constant};\n"
                        + self.default(variant, place).replace(
                            "return;", "return self;"
                        )
                        + body.replace("return;", "return self;")
                        + "  return self;\n",
                        doc=doc,
                        annotations=annotations,
                        identifier_annotation="(constructor)",
                        returns_doc="(transfer full): the new value",
                    )
                )
            functions.append(
                Function(
                    f"{prefix}_is_{variant.name}",
                    "gboolean",
                    [(f"const {name}*", "self")],
                    "  g_return_val_if_fail(self != NULL, FALSE);\n"
                    f"  return self->tag == {constant};\n",
                    doc=[f"Whether the value holds `{prefix}_get_{variant.name}`."],
                    returns_doc="whether the value holds the variant",
                )
            )
            returns, expression, annotation = self.getter(variant.value, place)
            default = fail_value(returns) or "0"
            functions.append(
                Function(
                    f"{prefix}_get_{variant.name}",
                    returns,
                    [(f"const {name}*", "self")],
                    f"  g_return_val_if_fail(self != NULL, {default});\n"
                    f"  g_return_val_if_fail(self->tag == {constant}, {default});\n"
                    f"  return {expression};\n",
                    doc=doc,
                    returns_doc=annotation,
                )
            )
        empty = self.empty_variant(value, union)
        if empty is not None:
            constant, _ = empty
            stem = self.variant_stem(union, constant)
            if input_:
                functions.append(
                    Function(
                        f"{prefix}_new_{stem}",
                        f"{name}*",
                        [],
                        f"  {name}* self = g_new0({name}, 1);\n"
                        f"  self->tag = {constant};\n  return self;\n",
                        doc=self.doc_lines(constant) or [f"The `{stem}` variant."],
                        identifier_annotation="(constructor)",
                        returns_doc="(transfer full): the new value",
                    )
                )
            functions.append(
                Function(
                    f"{prefix}_is_{stem}",
                    "gboolean",
                    [(f"const {name}*", "self")],
                    "  g_return_val_if_fail(self != NULL, FALSE);\n"
                    f"  return self->tag == {constant};\n",
                    doc=[f"Whether the value is the `{stem}` variant."],
                    returns_doc="whether the value is the variant",
                )
            )
        header = self.type_comment(value) + self.boxed_header(value, functions)
        source = struct
        source += (
            f"G_DEFINE_BOXED_TYPE({name}, {prefix}, {prefix}_copy, {prefix}_free)\n\n"
        )
        source += "".join(f.definition() for f in functions)
        # The variant writes into the union at `data` and its tag at `tag`.
        write = ""
        for variant in variants:
            assert variant.presence and variant.presence.variant
            body = self.write(union, variant, f"self->{variant.name}", "data")
            write += f"  case {variant.presence.variant}:\n{textwrap.indent(body, '  ')}    break;\n"
        write = f"  switch (self->tag) {{\n{write}  default:\n    break;\n  }}\n"
        read = ""
        for variant in variants:
            assert variant.presence and variant.presence.variant
            body = self.read(
                union, variant, "data", f"self->{variant.name}", free=False
            )
            read += f"  case {variant.presence.variant}:\n{textwrap.indent(body, '  ')}    break;\n"
        read = f"  switch (tag) {{\n{read}  default:\n    break;\n  }}\n"
        union_native = union.native
        uses_call = any(v.value.kind in {"record", "union", "array"} for v in variants)
        if value is union:
            tag = self.tag_type(union)
            source += (
                f"void {prefix}_to_native(\n  const {name}* self, {tag}* out_tag, {native}* out,\n"
                "  MaplibreNativeFfiCall* call\n) {\n"
                + ("" if uses_call else "  (void)call;\n")
                + f"  {native}* data = out;\n  memset(data, 0, sizeof *data);\n"
                f"  *out_tag = ({tag})self->tag;\n" + write + "}\n\n"
            )
            source += (
                f"{name}* {prefix}_from_native(gint64 tag, const {native}* in) {{\n"
                f"  const {native}* data = in;\n"
                f"  {name}* self = g_new0({name}, 1);\n  self->tag = tag;\n"
                + read
                + "  return self;\n}\n\n"
            )
        else:
            tag_field = self.tag_field(value, union)
            member = value.union_member
            assert tag_field is not None and member is not None
            tag = without_const(ctype(tag_field.value))
            source += (
                f"void {prefix}_to_native(\n  const {name}* self, {native}* out, MaplibreNativeFfiCall* call\n) {{\n"
                + ("" if uses_call else "  (void)call;\n")
                + "  memset(out, 0, sizeof *out);\n"
                + self.size_line(value, "out")
                + f"  out->{tag_field.name} = ({tag})self->tag;\n"
                f"  {union_native}* data = &out->{member.name};\n" + write + "}\n\n"
            )
            source += (
                f"{name}* {prefix}_from_native(const {native}* in) {{\n"
                f"  gint64 tag = (gint64)in->{tag_field.name};\n"
                f"  const {union_native}* data = &in->{member.name};\n"
                f"  {name}* self = g_new0({name}, 1);\n  self->tag = tag;\n"
                + read
                + "  return self;\n}\n\n"
            )
            source += self.copy_func(value)
        private = self.private_prototypes(value)
        if value is union:
            private += f"G_GNUC_INTERNAL gpointer {prefix}_copy_func(gconstpointer self, gpointer data);\n"
            source += self.copy_func(value)
        return header, source, private

    # Callbacks.

    def callback_parameters(self, callback: CallbackPlan) -> list[tuple[str, str, str]]:
        """A host callback's parameters before its data: their types, names,
        and annotations."""
        out = []
        for parameter in callback.arguments:
            value = parameter.value
            if parameter.direction != "in":
                raise Unsupported(
                    f"GObject: {callback.native}: an output parameter of a host callback"
                )
            unwrapped = value.unwrapped
            kind = unwrapped.kind
            if unwrapped.native in self.bound.slices:
                slice_ = self.bound.slices[unwrapped.native]
                element = slice_.value.element
                assert element is not None
                out.append(
                    (
                        "GPtrArray*",
                        parameter.name,
                        f"(element-type {type_name(element.native)}) (transfer none): the items",
                    )
                )
            elif kind in {"scalar", "enum"}:
                out.append((self.gtype(unwrapped), parameter.name, ""))
            elif self.text(unwrapped):
                out.append(("const char*", parameter.name, "(transfer none): the text"))
            elif self.bytes_(unwrapped):
                out.append(("GBytes*", parameter.name, "(transfer none): the bytes"))
            elif kind == "handle":
                assert unwrapped.handle is not None
                out.append(
                    (
                        f"{type_name(unwrapped.native)}*",
                        parameter.name,
                        "(transfer none): the handle",
                    )
                )
            elif kind in {"record", "union"}:
                # A callback's record is the callback's own copy, and Vala
                # writes a delegate's parameters without const.
                out.append(
                    (
                        f"{type_name(unwrapped.native)}*",
                        parameter.name,
                        "(transfer none): the value",
                    )
                )
            else:
                raise Unsupported(
                    f"GObject: {callback.native}: a {kind} argument of a host callback"
                )
        return out

    def callback_typedef(self, callback: CallbackPlan) -> str:
        if callback.result.native != "void":
            raise Unsupported(
                f"GObject: {callback.native}: a host callback that returns a value"
            )
        name = type_name(callback.native)
        parameters = self.callback_parameters(callback)
        lines = [f"{name}:"]
        for _, parameter, annotation in parameters:
            lines.append(f"@{parameter}: {annotation or 'the value'}")
        lines.append(
            "@user_data: (closure): the data that the callback's setter received"
        )
        lines += ["", *(self.doc_lines(callback.native) or [f"A `{name}`."])]
        arguments = ", ".join(
            [*(f"{t} {n}" for t, n, _ in parameters), "gpointer user_data"]
        )
        return comment(lines) + f"typedef void (*{name})({arguments});\n\n"

    def trampoline_name(self, value: ValuePlan, member: FieldPlan) -> str:
        return f"{public_stem(value.native)}_{member.name}_trampoline"

    def trampoline(self, value: ValuePlan, member: FieldPlan, index: int) -> str:
        """The native callback that calls the host callback of a
        registration's member, whose closure is at `index` of the
        registration's context."""
        callback = self.bound.callbacks[member.value.native]
        native_parameters = ", ".join(
            f"{p.value.ctype.spelling} {p.name}" for p in callback.parameters
        )
        context = callback.context
        setup, arguments, cleanup = "", [], ""
        for parameter in callback.arguments:
            argument = parameter.value
            unwrapped = argument.unwrapped
            name = parameter.name
            pointer = argument.kind == "reference"
            access = f"{name}->" if pointer else f"{name}."
            local = f"{name}_"
            if unwrapped.native in self.bound.slices:
                slice_ = self.bound.slices[unwrapped.native]
                element = slice_.value.element
                assert element is not None
                prefix = symbol_name(element.native)
                count = f"{access}{slice_.value.length}"
                item = f"&{access}{slice_.name}[index]"
                if slice_.value.stride:
                    item = f"(const {element.native}*)(const void*)((const char*){access}{slice_.name} + index * {access}{slice_.value.stride})"
                setup += (
                    f"  GPtrArray* {local} = g_ptr_array_new_full((guint){count}, (GDestroyNotify){prefix}_free);\n"
                    f"  for (size_t index = 0; index < {count}; index++) {{\n"
                    f"    g_ptr_array_add({local}, {prefix}_from_native({item}));\n  }}\n"
                )
                cleanup += f"  g_ptr_array_unref({local});\n"
                arguments.append(local)
            elif unwrapped.kind in {"scalar", "enum"}:
                arguments.append(
                    self.from_c(unwrapped, f"*{name}" if pointer else name)
                )
            elif self.text(unwrapped):
                if unwrapped.buffer_form == "view":
                    copied = (
                        f"maplibre_native_ffi_text_new({'*' if pointer else ''}{name})"
                    )
                else:
                    copied = f'g_strdup({name} != NULL ? (const char*){name} : "")'
                setup += f"  char* {local} = {copied};\n"
                cleanup += f"  g_free({local});\n"
                arguments.append(local)
            elif self.bytes_(unwrapped):
                setup += f"  GBytes* {local} = maplibre_native_ffi_bytes_new({access}data, {access}size);\n"
                cleanup += f"  if ({local} != NULL) g_bytes_unref({local});\n"
                arguments.append(local)
            elif unwrapped.kind == "handle":
                handle = unwrapped.handle
                assert handle is not None
                borrowed = "TRUE" if unwrapped.ownership == "borrowed" else "FALSE"
                setup += (
                    f"  {type_name(handle.native)}* {local} = "
                    f"{symbol_name(handle.native)}_wrap({name}, NULL, {borrowed}, NULL);\n"
                )
                cleanup += f"  g_object_unref({local});\n"
                arguments.append(local)
            elif unwrapped.kind == "record":
                prefix = symbol_name(unwrapped.native)
                source = name if pointer else f"&{name}"
                setup += f"  {type_name(unwrapped.native)}* {local} = {prefix}_from_native({source});\n"
                cleanup += f"  {prefix}_free({local});\n"
                arguments.append(local)
            else:
                raise Unsupported(
                    f"GObject: {callback.native}: a {unwrapped.kind} argument"
                )
        unused = "".join(
            f"  (void){p.name};\n"
            for p in callback.parameters
            if p.name != context and p not in callback.arguments
        )
        function = type_name(callback.native)
        return (
            f"static void {self.trampoline_name(value, member)}({native_parameters}) {{\n"
            + unused
            + f"  MaplibreNativeFfiClosure* closure = maplibre_native_ffi_registration_closure({context}, CLOSURE_INDEX);\n"
            + "  if (closure == NULL) return;\n"
            + setup
            + f"  (({function})(void (*)(void))closure->func)({', '.join([*arguments, 'closure->user_data'])});\n"
            + cleanup
            + "}\n\n"
        ).replace("CLOSURE_INDEX", str(index))
