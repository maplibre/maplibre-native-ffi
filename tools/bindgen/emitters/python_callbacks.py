"""Compile callback descriptors and their quiescent Python roots."""

from dataclasses import replace

from .python_values import public_name, rust_field, scalar_type


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
    return "sys::" + value.native


def validate(values, plan):
    descriptor = plan.registration
    plain = plain_fields(plan)
    for field in plain.fields:
        if field.public:
            values.supported(field.value, input=True)
    for name in descriptor.callbacks:
        field = next(f for f in plan.fields if f.name == name)
        callback = values.api.callbacks[field.value.native]
        if not callback.context:
            values.fail(plan, "callback requires an explicit context parameter")
        for parameter in callback.arguments:
            if callback.decision and parameter.name == callback.decision.parameter:
                continue
            response = parameter.value.unwrapped
            if parameter.direction != "in" and not response.response:
                values.fail(plan, "callback output requires a scoped response")
            values.supported(parameter.value)
        if callback.result.native != "void":
            values.supported(callback.result, input=True)


def input_source(plan, values):
    descriptor = plan.registration
    lines = values.input_lines(plain_fields(plan))
    for name in descriptor.callbacks:
        lines.append(f'let {name}_enabled = !value.getattr("{name}")?.is_none();')
    lines.append(
        "if " + " || ".join(name + "_enabled" for name in descriptor.callbacks) + " {"
    )
    methods = ", ".join(
        f'value.getattr("_invoke_{name}")?.unbind()' for name in descriptor.callbacks
    )
    lines.append(
        f"raw.{descriptor.user_data} = storage.register_callbacks(vec![{methods}]);"
    )
    release_fn = "generated_release_callbacks"
    lines.append(f"raw.{descriptor.release} = Some({release_fn});")
    for name in descriptor.callbacks:
        lines.append(
            f"raw.{name} = if {name}_enabled {{ Some(generated_callback_{plan.native}_{name}) }} else {{ None }};"
        )
    lines.append("}")
    return (
        f"fn generated_input_{plan.native}<'py>(value: &Bound<'py, PyAny>, storage: &mut GeneratedInputStorage<'py>) -> PyResult<sys::{plan.native}> {{\n    "
        + "\n    ".join(lines)
        + "\n    Ok(raw)\n}\n"
    )


def sources(values, plan):
    from .python import OWNERS, owned_native

    rust, python = [], []
    descriptor = plan.registration
    fields, methods = [], []
    for index, name in enumerate(descriptor.callbacks):
        field = next(f for f in plan.fields if f.name == name)
        callback = values.api.callbacks[field.value.native]
        parameters = list(callback.arguments)
        decision_owner = (
            OWNERS[callback.decision.handle.native] if callback.decision else None
        )
        # A handle the callback owns arrives in its generated owner, which a
        # callback that raises closes, so native never waits on it.
        owned = [
            p
            for p in parameters
            if p.value.kind == "handle"
            and p.value.ownership == "owned"
            and not (callback.decision and p.name == callback.decision.parameter)
        ]
        types = ", ".join(
            decision_owner
            if callback.decision and p.name == callback.decision.parameter
            else OWNERS[p.value.native]
            if p in owned
            else values.type(p.value)
            for p in parameters
        )
        result_type = (
            "None" if callback.result.native == "void" else values.type(callback.result)
        )
        fields.append(f"    {name}: Callable[[{types}], {result_type}] | None = None")
        signature = ", ".join(["self"] + [p.name for p in parameters])
        arguments = ", ".join(
            f"_wrap_response({p.name}, {decision_owner!r})"
            if callback.decision and p.name == callback.decision.parameter
            else p.name
            if p in owned
            else values.facade_copy(p.value, p.name)
            for p in parameters
        )
        wraps = "".join(
            f"        {p.name} = _wrap_response({p.name}, {OWNERS[p.value.native]!r})\n"
            for p in owned
        )
        invoke = f"return callback({arguments})"
        if owned:
            closes = "".join(f"            {p.name}.close()\n" for p in owned)
            invoke = f"try:\n            return callback({arguments})\n        except BaseException:\n{closes}            raise"
        methods.append(
            f"    def _invoke_{name}({signature}):\n        callback = self.{name}\n        assert callback is not None\n{wraps}        {invoke}\n"
        )
        native_parameters = ", ".join(
            f"{'_' if p.name == callback.borrowed_owner else ''}{rust_field(p.name)}: {ffi_type(p.value)}"
            for p in callback.parameters
        )

        def copy_parameter(
            parameter, callback=callback, owner=decision_owner, owned=owned
        ):
            if callback.decision and parameter.name == callback.decision.parameter:
                return "decision_owner.clone_ref(py)"
            if parameter in owned:
                return f"{rust_field(parameter.name)}_owner"
            value = parameter.value.unwrapped
            if value.response:
                return f"Py::new(py, {public_name(value.native)}Scope {{ native: {rust_field(parameter.name)} as usize, scope: callback_scope.scope() }})?"
            return values.copy(parameter.value, rust_field(parameter.name))

        copied = ", ".join(copy_parameter(p) for p in parameters)
        scope = (
            "let callback_scope = GeneratedCallbackGuard::new();"
            if any((p.value.unwrapped).response for p in parameters)
            else ""
        )
        # A handle the callback owns is adopted before anything else can fail,
        # and one the body never reaches, as during interpreter shutdown, is
        # released natively, so native never waits on it.
        scope += "".join(
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
            f"if !{rust_field(p.name)}_adopted.get() {{ unsafe {{ sys::{p.value.handle.release}({rust_field(p.name)}) }}; }} "
            for p in owned
        )
        if len(parameters) == 1:
            copied += ","
        result = (
            "Ok(())"
            if callback.result.native == "void"
            else f"result.extract::<{ffi_type(callback.result)}>()"
        )
        failure = (
            "()"
            if callback.result.native == "void"
            else (
                "sys::" + callback.failure
                if callback.failure.startswith("MLN_")
                else callback.failure
            )
        )
        decision_setup = ""
        if callback.decision:
            decision = callback.decision
            decision_setup = "let mut decision_state = None;"
            scope = (
                f"let decision_owner = unsafe {{ {decision_owner}::adopt(py, {decision.parameter}, Vec::new()) }}?;"
                f"decision_state = Some(Arc::clone(&decision_owner.cast_bound::<{decision_owner}>(py).unwrap().borrow().state));"
                + scope
            )
            result = "result.extract::<u32>()"
        returns = (
            ""
            if callback.result.native == "void"
            else f" -> {ffi_type(callback.result)}"
        )
        rust.append(f"""unsafe extern "C" fn generated_callback_{plan.native}_{name}({native_parameters}){returns} {{
    {"let _scope = maplibre_core::callback::CallbackScope::enter();" if callback.decision else ""}
    {decision_setup}
    {adoption_flags}
    let outcome = generated_invoke(|| {{ {unadopted}{"" if unadopted and failure == "()" else failure} }}, |py| {{
        {scope}
        let Some(callback) = (unsafe {{ generated_get_callback(py, {callback.context}, {index}) }}) else {{ return Ok({failure}); }};
        let {"_result" if callback.result.native == "void" else "result"} = callback.bind(py).call1(({copied}))?;
        {result}
    }});
    {f"if outcome != sys::{callback.decision.accept} {{ if let Some(state) = decision_state {{ state.lock().unwrap_or_else(|p| p.into_inner()).handle.hand_off(); }} }}" if callback.decision else ""}
    outcome
}}
""")
    record_rust, record_python = values.record_sources(
        plain_fields(plan),
        extra_fields=fields,
        extra_methods="\n" + "\n".join(methods),
        # Native never returns callbacks, so a copy of a registration's
        # default leaves them at None.
        copied=plan.native in values.api.returned,
        registration=True,
    )
    rust.append(record_rust)
    python.append(record_python)
    return "\n".join(rust), "\n".join(python)
