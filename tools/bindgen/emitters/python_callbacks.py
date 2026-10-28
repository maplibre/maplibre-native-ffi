"""Compile callback descriptors and their quiescent Python roots.

Python takes each registration in the form that `async_hosts.form` chooses,
because native threads never run Python code. Every callback that Python
supplies names the binding's dispatch queue, so it runs on the binding's
dispatcher thread: a queue registration's callbacks and a wake's callback.
Native rules answer a registration in the `rules` form, which holds no
callback, and a registration in the `native` form holds only a native
registration.

A registration that `async_hosts.accepts_native` admits also takes a native
registration, whose function pointers and context the host's native code
supplies. The record's `native` classmethod returns one as a distinct class:
the record keeps its value semantics, and the native registration is affine.
"""

from dataclasses import replace

from .. import async_hosts, docs
from ..semantic import required_callbacks
from .python_values import (
    enum_type,
    field_name,
    native_name,
    public_name,
    rust_field,
    scalar_type,
)


def hosted(api, plan):
    """The callback fields that Python code supplies, each of which the
    dispatcher thread runs."""
    if async_hosts.form(api, plan) in {"queue", "wake"}:
        return async_hosts.callback_fields(plan)
    return []


def plain_fields(plan):
    descriptor = plan.registration
    callbacks = {*descriptor.callbacks, descriptor.user_data, descriptor.release}
    return replace(
        plan,
        registration=None,
        fields=tuple(f for f in plan.fields if f.name not in callbacks),
    )


def ffi_type(value):
    if value.kind == "reference" and value.element:
        return ("*const " if value.ctype.pointee.const else "*mut ") + ffi_type(
            value.element
        )
    if value.kind == "scalar":
        return "()" if value.native == "void" else scalar_type(value)
    if value.kind == "native_pointer":
        return "*mut std::ffi::c_void"
    if value.kind == "buffer" and value.ctype.pointee:
        return "*const std::ffi::c_char"
    if value.kind == "enum":
        return enum_type(value)
    return "sys::" + value.native


def validate(values, plan):
    descriptor = plan.registration
    plain = plain_fields(plan)
    form = async_hosts.form(values.api, plan)
    if form is None:
        values.fail(
            plan, "registration requires queue delivery, a wake, or native rules"
        )
    if form == "wake" and not descriptor.queue:
        values.fail(plan, "a wake that runs Python code requires a queue member")
    if async_hosts.accepts_native(values.api, plan):
        # A native registration carries only the C registration's own fields.
        if any(f.public for f in plain.fields):
            values.fail(plan, "a native registration has no place for other fields")
        if {"cls", "native"} & set(descriptor.callbacks):
            values.fail(plan, "a callback field collides with the native classmethod")
        if any(public_name(n) == native_name(plan) for n in values.api.public_values):
            values.fail(plan, "the native registration class collides with a record")
    for field in plain.fields:
        if field.public:
            values.supported(field.value, input=True)
    for field in hosted(values.api, plan):
        callback = values.api.callbacks[field.value.native]
        if not callback.context:
            values.fail(plan, "callback requires an explicit context parameter")
        if any(
            public_name(n) == public_name(callback.native)
            for n in values.api.public_values
        ):
            values.fail(plan, "the callback protocol collides with a record")
        for parameter in callback.arguments:
            if parameter.direction != "in":
                values.fail(plan, "a dispatched callback has no output")
            values.supported(parameter.value)
        if callback.result.native != "void" and not callback.queue_result:
            values.fail(plan, "a dispatched callback's result needs a queue result")


def native_input_lines(plan):
    """Copies a native registration's addresses unchanged into the C record."""
    descriptor = plan.registration
    lines = [
        f'if let Some(native) = storage.native_registration(value, "{plan.native}")? {{',
        f"raw.{rust_field(descriptor.user_data)} = native.user_data as _;",
        f"raw.{rust_field(descriptor.release)} = unsafe {{ std::mem::transmute::<usize, _>(native.release) }};",
    ]
    lines += [
        f"raw.{rust_field(name)} = unsafe {{ std::mem::transmute::<usize, _>(native.callbacks[{index}]) }};"
        for index, name in enumerate(descriptor.callbacks)
    ]
    return [*lines, "return Ok(raw);", "}"]


def input_source(plan, values):
    descriptor = plan.registration
    lines = values.input_lines(plain_fields(plan))
    if async_hosts.accepts_native(values.api, plan):
        lines += native_input_lines(plan)
    names = [field.name for field in hosted(values.api, plan)]
    if names:
        for name in names:
            lines.append(f'let {name}_enabled = !value.getattr("{name}")?.is_none();')
        lines.append("if " + " || ".join(name + "_enabled" for name in names) + " {")
        methods = ", ".join(
            f'value.getattr("_invoke_{name}")?.unbind()' for name in names
        )
        lines.append(
            f"raw.{descriptor.user_data} = storage.register_callbacks(vec![{methods}]);"
        )
        lines.append(f"raw.{descriptor.release} = Some(generated_release_callbacks);")
        lines.append(f"raw.{descriptor.queue} = generated_dispatch_queue()?;")
        for name in names:
            lines.append(
                f"raw.{name} = if {name}_enabled {{ Some(generated_callback_{plan.native}_{name}) }} else {{ None }};"
            )
        lines.append("}")
    if not any("storage" in line for line in lines):
        lines.insert(0, "let _ = storage;")
    return (
        f"fn generated_input_{plan.native}<'py>(value: &Bound<'py, PyAny>, storage: &mut GeneratedInputStorage<'py>) -> PyResult<sys::{plan.native}> {{\n    "
        + "\n    ".join(lines)
        + "\n    Ok(raw)\n}\n"
    )


def sources(values, plan):
    from .python import OWNERS, disposal, owned_native

    rust, python = [], []
    fields, methods = [], []
    for index, field in enumerate(hosted(values.api, plan)):
        name = field.name
        callback = values.api.callbacks[field.value.native]
        parameters = list(callback.arguments)
        # A handle the callback owns arrives in its generated owner, which a
        # callback that raises closes, so native never waits on it.
        owned = [
            p
            for p in parameters
            if p.value.kind == "handle" and p.value.ownership == "owned"
        ]
        required = not field.value.nullable
        fields.append(
            f"    {name}: {public_name(callback.native)}"
            + ("" if required else " | None = None")
        )
        signature = ", ".join(["self"] + [p.name for p in parameters])
        arguments = ", ".join(
            p.name if p in owned else values.facade_copy(p.value, p.name)
            for p in parameters
        )
        wraps = "".join(
            f"        {p.name} = _adopt_value({p.name}, {OWNERS[p.value.native]!r})\n"
            for p in owned
        )
        invoke = f"return callback({arguments})"
        if owned:
            closes = "".join(f"            {p.name}.close()\n" for p in owned)
            invoke = f"try:\n            return callback({arguments})\n        except BaseException:\n{closes}            raise"
        methods.append(
            f"    def _invoke_{name}({signature}):\n        callback = self.{name}\n"
            + ("" if required else "        assert callback is not None\n")
            + f"{wraps}        {invoke}\n"
        )
        native_parameters = ", ".join(
            f"{'_' if p.name == callback.borrowed_owner else ''}{rust_field(p.name)}: {ffi_type(p.value)}"
            for p in callback.parameters
        )

        def copy_parameter(parameter, owned=owned):
            if parameter in owned:
                return f"{rust_field(parameter.name)}_owner"
            return values.copy(parameter.value, rust_field(parameter.name))

        copied = ", ".join(copy_parameter(p) for p in parameters)
        # A handle the callback owns is adopted before anything else can fail,
        # and one the body never reaches, as during interpreter shutdown, is
        # released natively, so native never waits on it.
        scope = "".join(
            f"let {rust_field(p.name)}_owner = {owned_native(p.value, rust_field(p.name))}; "
            f"{rust_field(p.name)}_adopted.set(true); "
            f"let {rust_field(p.name)}_owner = {rust_field(p.name)}_owner?;"
            for p in owned
        )
        adoption_flags = "".join(
            f"let {rust_field(p.name)}_adopted = std::cell::Cell::new(false);"
            for p in owned
        )
        unadopted = "".join(
            f"if !{rust_field(p.name)}_adopted.get() {{ unsafe {{ {disposal(values.api, p.value.handle, p.value.handle.release, rust_field(p.name))} }}; }} "
            for p in owned
        )
        if len(parameters) == 1:
            copied += ","
        # Native answers a dispatched call itself and ignores what it returns,
        # so the callback returns nothing and its call returns that answer.
        answer = "()" if callback.result.native == "void" else result_literal(callback)
        body = (
            f"let _result = callback.bind(py).call1(({copied}))?;\n        Ok({answer})"
        )
        returns = (
            ""
            if callback.result.native == "void"
            else f" -> {ffi_type(callback.result)}"
        )
        rust.append(f"""unsafe extern "C" fn generated_callback_{plan.native}_{name}({native_parameters}){returns} {{
    {adoption_flags}
    let outcome = generated_invoke(|| {{ {unadopted}{"" if unadopted and answer == "()" else answer} }}, |py| {{
        {scope}
        let Some(callback) = (unsafe {{ generated_get_callback(py, {callback.context}, {index}) }}) else {{ return Ok({answer}); }};
        {body}
    }});
    outcome
}}
""")
    native_class = ""
    if async_hosts.accepts_native(values.api, plan):
        native_method, native_class = native_sources(values, plan)
        methods.append(native_method)
    doc = values.api.doc(plan.native)
    if doc and not fields:
        # Native rules or a native registration answer this registration, so
        # no Python code runs for it, on the dispatcher thread or elsewhere.
        doc = replace(doc, threads=())
    record_rust, record_python = values.record_sources(
        plain_fields(plan),
        doc=doc,
        extra_fields=fields,
        extra_methods="\n" + "\n".join(methods),
        # Native never returns callbacks, so a copy of a registration takes
        # them from its caller.
        copied=plan.native in values.api.returned,
        registration=True,
        required=[
            f.name for f in required_callbacks(plan) if f in hosted(values.api, plan)
        ],
    )
    rust.append(record_rust)
    python.append(record_python + native_class)
    return "\n".join(rust), "\n".join(python)


def native_sources(values, plan):
    """The record's `native` classmethod, and the class that it returns."""
    descriptor = plan.registration
    public, native = public_name(plan.native), native_name(plan)
    callbacks = [field_name(name) for name in descriptor.callbacks]
    context = [field_name(descriptor.user_data), field_name(descriptor.release)]
    parameters = ", ".join(
        ["cls", *(f"{name}: int" for name in callbacks)]
        + [f"{name}: int = 0" for name in context]
    )
    addresses = ", ".join(callbacks) + ("," if len(callbacks) == 1 else "")
    method = (
        f"    @classmethod\n    def native({parameters}) -> {native}:\n"
        + docs.docstring(
            None,
            values.symbol,
            "        ",
            (
                (
                    "A native registration, whose function pointers, context, "
                    "and release the host's native code supplies as addresses. "
                    "Zero is null. The binding copies each address unchanged, "
                    "and native code validates them."
                ),
            ),
        )
        + "        from . import _native\n\n"
        + f"        return {native}._from_registration(\n"
        + f'            _native._NativeRegistration("{plan.native}", ({addresses}), {", ".join(context)})\n'
        + "        )\n"
    )
    cls = (
        f"\n\nclass {native}:\n"
        + docs.docstring(
            None,
            values.symbol,
            "    ",
            (
                f"A native `{public}` from `{public}.native`, accepted wherever a `{public}` is.",
                (
                    "The first call that carries it consumes it. Native code "
                    "releases the context of a call that it accepts, and the "
                    "binding releases the context of a call that native code "
                    "does not accept. A second call raises an invalid-state "
                    "error."
                ),
            ),
        )
        + '\n    __slots__ = ("_registration",)\n\n'
        + "    def __init__(self):\n"
        + f'        raise TypeError("native registrations come from {public}.native")\n\n'
        + "    @classmethod\n    def _from_registration(cls, registration):\n"
        + "        value = cls.__new__(cls)\n"
        + "        value._registration = registration\n"
        + "        return value\n"
    )
    return method, cls


def callback_types(api, records) -> set[str]:
    """The callback types that Python code supplies to the registrations among
    `records`, each of which a Protocol presents."""
    return {
        field.value.native
        for plan in records
        if plan.registration
        for field in hosted(api, plan)
    }


def protocol_sources(values):
    """The Protocol of each callback type that a registration holds, whose
    positional-only parameters keep the C names, so any callable of the same
    shape satisfies it. Native answers a dispatched call itself, so every
    callback returns None."""
    from .python import OWNERS

    natives = sorted(callback_types(values.api, values.records.values()))
    classes = []
    for native in natives:
        callback = values.api.callbacks[native]
        parameters = "".join(
            f", {field_name(p.name)}: "
            + (
                OWNERS[p.value.native]
                if p.value.kind == "handle" and p.value.ownership == "owned"
                else values.type(p.value)
            )
            for p in callback.arguments
        )
        classes.append(
            f"class {public_name(native)}(Protocol):\n"
            + docs.docstring(values.api.doc(native), values.symbol, "    ")
            + f"\n    def __call__(self{parameters}, /) -> None: ...\n"
        )
    return "\n\n".join(classes)


def result_literal(callback):
    """The answer that native gives a dispatched call, as Rust spells it."""
    answer = callback.queue_result
    return "sys::" + answer if answer.startswith("MLN_") else answer
