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
        if field.role not in {"size", "reserved", "count", "presence_mask"}:
            values.supported(field.value, input=True)
    for group in plain.presence_groups:
        if len(group.fields) > 1:
            values.supported(values.group_value(plain, group), input=True)
    for name in descriptor.callbacks:
        field = next(f for f in plan.fields if f.name == name)
        callback = values.api.callbacks[field.value.native]
        if not callback.context:
            values.fail(plan, "callback requires an explicit context parameter")
        for parameter in callback.parameters:
            if parameter.name == callback.context:
                continue
            if callback.decision and parameter.name == callback.decision.parameter:
                continue
            response = (
                parameter.value.element
                if parameter.value.kind == "reference"
                else parameter.value
            )
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
    release_callback = next(
        f.value.native for f in plan.fields if f.name == descriptor.release
    )
    release_fn = (
        "generated_release_callbacks_no_reentry"
        if values.api.callbacks[release_callback].reentry == "forbid"
        else "generated_release_callbacks"
    )
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
    rust, python = [], []
    descriptor = plan.registration
    fields, methods, copies = [], [], []
    for index, name in enumerate(descriptor.callbacks):
        field = next(f for f in plan.fields if f.name == name)
        callback = values.api.callbacks[field.value.native]
        parameters = [p for p in callback.parameters if p.name != callback.context]
        types = ", ".join(
            "ResourceRequestHandle"
            if callback.decision and p.name == callback.decision.parameter
            else values.type(p.value)
            for p in parameters
        )
        result_type = (
            "None" if callback.result.native == "void" else values.type(callback.result)
        )
        fields.append(f"    {name}: Callable[[{types}], {result_type}] | None = None")
        signature = ", ".join(["self"] + [p.name for p in parameters])
        arguments = ", ".join(
            f"_wrap_response({p.name}, 'ResourceRequestHandle')"
            if callback.decision and p.name == callback.decision.parameter
            else values.facade_copy(p.value, p.name)
            for p in parameters
        )
        methods.append(
            f"    def _invoke_{name}({signature}):\n        callback = self.{name}\n        assert callback is not None\n        return callback({arguments})\n"
        )
        copies.append(f'dict.set_item("{name}", py.None())?;')
        copies.append(
            f'if value.{rust_field(name)}.is_some() {{ return Err(native_error("native callback cannot be copied into a Python closure")); }}'
        )
        native_parameters = ", ".join(
            f"{rust_field(p.name)}: {ffi_type(p.value)}" for p in callback.parameters
        )

        def copy_parameter(parameter, callback=callback):
            if callback.decision and parameter.name == callback.decision.parameter:
                return "Py::new(py, ResourceRequestHandle { state: ManuallyDrop::new(Arc::clone(&decision_state)), cancel_root: Mutex::new(None) })?"
            value = (
                parameter.value.element
                if parameter.value.kind == "reference"
                else parameter.value
            )
            if value.response:
                return f"Py::new(py, {public_name(value.native)}Scope {{ native: {rust_field(parameter.name)} as usize, scope: callback_scope.scope() }})?"
            return values.copy(parameter.value, rust_field(parameter.name))

        copied = ", ".join(copy_parameter(p) for p in parameters)
        scope = (
            "let callback_scope = GeneratedCallbackGuard::new();"
            if any(
                (p.value.element if p.value.kind == "reference" else p.value).response
                for p in parameters
            )
            else ""
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
            functions = f"maplibre_core::resource::ResourceRequestHandleFns::new(sys::{decision.complete}, sys::{decision.cancelled}, sys::{decision.cancel_registration}, sys::{decision.handle.release}, sys::{decision.wait_retired})"
            decision_setup = f"let decision_state = match unsafe {{ maplibre_core::resource::ResourceRequestHandleState::new({decision.parameter}, {functions}) }} {{ Ok(state) => state, Err(_) => return sys::{decision.pass_through} }};"
            failure = "decision_state.finish_provider_decision(maplibre_core::ResourceProviderDecision::PassThrough)"
            result = f"let decision = result.extract::<u32>()?; Ok(decision_state.finish_provider_decision(if decision == sys::{decision.accept} {{ maplibre_core::ResourceProviderDecision::Handle }} else {{ maplibre_core::ResourceProviderDecision::PassThrough }}))"
        policy_guard = ""
        if callback.reentry_policy:
            policy = callback.reentry_policy
            owner = next(
                p.value for p in callback.parameters if p.name == policy.owner_parameter
            )
            identity = (
                f"{policy.owner_parameter} as usize as u64"
                if owner.kind == "reference"
                else f"maplibre_core::handle::NativeHandle::to_raw({policy.owner_parameter})"
            )
            operations = ", ".join(
                '"' + operation + '"' for operation in policy.operations
            )
            policy_guard = f"let _policy = GeneratedCallbackPolicy::enter(&[{operations}], {identity});"
        rust.append(f"""unsafe extern "C" fn generated_callback_{plan.native}_{name}({native_parameters}) -> {ffi_type(callback.result)} {{
    {"let _reentry = GeneratedCallbackPolicy::enter(&[], 0);" if callback.reentry == "forbid" else ""}
    {policy_guard}
    {decision_setup}
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {{
        Python::try_attach(|py| -> PyResult<{ffi_type(callback.result)}> {{
            {scope}
            let Some(callback) = (unsafe {{ generated_get_callback(py, {callback.context}, {index}) }}) else {{ return Ok({failure}); }};
            let {"_result" if callback.result.native == "void" else "result"} = callback.bind(py).call1(({copied}))?;
            {result}
        }}).unwrap_or(Ok({failure}))
    }}));
    match result {{
        Ok(Ok(result)) => result,
        Ok(Err(error)) => {{ Python::try_attach(|py| error.write_unraisable(py, None)); {failure} }},
        Err(_) => {failure},
    }}
}}
""")
    record_rust, record_python = values.record_sources(
        plain_fields(plan),
        extra_fields=fields,
        extra_copies=copies,
        extra_public_copies=[f"{name}=raw[{name!r}]" for name in descriptor.callbacks],
        extra_methods="\n" + "\n".join(methods),
    )
    rust.append(record_rust)
    python.append(record_python)
    return "\n".join(rust), "\n".join(python)


def direct_operation(plan, values):
    from ..semantic import FieldPlan, RegistrationDescriptorPlan, ValuePlan
    from .python import OWNERS, unsupported

    registration = plan.direct_registrations[0]
    parameter = next(p for p in plan.inputs if p.name == registration.callback)
    callback = values.api.callbacks[parameter.value.native]
    receiver = next((p for p in plan.inputs if p.name == plan.receiver), None)
    if registration.owner_release:
        decision = next(
            (
                c.decision
                for c in values.api.callbacks.values()
                if c.decision
                and c.decision.handle.release == registration.owner_release
            ),
            None,
        )
        if (
            not decision
            or len(callback.parameters) != 1
            or callback.result.native != "void"
        ):
            raise unsupported(
                plan.function,
                "direct owner callback needs a matching decision protocol",
            )
        owner = OWNERS[receiver.value.native]
        name = plan.name.removeprefix(
            receiver.value.native.removesuffix("_handle") + "_"
        )
        policy = callback.reentry_policy
        operations = (
            ", ".join('"' + operation + '"' for operation in policy.operations)
            if policy
            else ""
        )
        guard = (
            f"let _policy = GeneratedCallbackPolicy::enter(&[{operations}], callback_owner);"
            if policy
            else ""
        )
        native = f"""    fn {name}(&self, py: Python<'_>, callback: Py<PyAny>) -> PyResult<bool> {{
        let callback_owner = maplibre_core::handle::NativeHandle::to_raw(self.state.issued_handle());
        generated_check_operation("{plan.name}", callback_owner)?;
        if !callback.bind(py).is_callable() {{ return Err(invalid_argument_error("callback must be callable")); }}
        let root = Arc::new(Mutex::new(Some(callback)));
        let weak = Arc::downgrade(&root);
        let inline = self.state.register_cancel_callback(Box::new(move || {{
            {guard}
            if let Some(root) = weak.upgrade() {{
                let callback = root.lock().unwrap_or_else(|p| p.into_inner()).take();
                if let Some(callback) = callback {{
                    Python::try_attach(|py| {{
                        if let Err(error) = callback.bind(py).call0() {{ error.write_unraisable(py, None); }}
                    }});
                }}
            }}
        }})).map_err(map_error)?;
        let cancelled = inline.is_some();
        drop(inline);
        if !cancelled {{
            *self.cancel_root.lock().unwrap_or_else(|p| p.into_inner()) = Some(root);
        }}
        Ok(cancelled)
    }}
"""
        facade = f"""    def {name}(self, callback: Callable[[], None]) -> bool:
        return self._native.{name}(callback)
"""
        return (
            owner,
            native,
            facade,
            f"    def {name}(self, callback: Callable[[], None]) -> bool: ...\n",
            None,
        )
    if receiver or not registration.release_callback:
        raise unsupported(
            plan.function, "direct registration requires native release notification"
        )
    descriptor = ValuePlan(
        kind="record",
        native=plan.name + "_registration",
        ctype=parameter.value.ctype,
        fields=(FieldPlan(registration.callback, parameter.value),),
        registration=RegistrationDescriptorPlan(
            (registration.callback,),
            registration.user_data,
            registration.release_callback,
        ),
    )
    validate(values, descriptor)
    values.records[descriptor.native] = descriptor
    name = plan.name.removeprefix("mln_")
    arguments = {
        registration.callback: "native_callback",
        registration.user_data: "context",
        registration.release_callback: "release",
    }
    call = ", ".join(arguments[p.name] for p in plan.function.parameters)
    native = f'''#[pyfunction]
fn {name}(py: Python<'_>, callback: &Bound<'_, PyAny>) -> PyResult<()> {{
    generated_check_reentry()?;
    let storage = &mut GeneratedInputStorage::default();
    let enabled = !callback.getattr("{registration.callback}")?.is_none();
    let context = if enabled {{ storage.register_callbacks(vec![callback.getattr("_invoke_{registration.callback}")?.unbind()]) }} else {{ std::ptr::null_mut() }};
    let native_callback: sys::{parameter.value.native} = if enabled {{ Some(generated_callback_{descriptor.native}_{registration.callback}) }} else {{ None }};
    let release = if enabled {{ Some({"generated_release_callbacks_no_reentry" if values.api.callbacks[next(p.value.native for p in plan.inputs if p.name == registration.release_callback)].reentry == "forbid" else "generated_release_callbacks"} as unsafe extern "C" fn(*mut c_void)) }} else {{ None }};
    let status = unsafe {{ generated_native_call(py, || sys::{plan.name}({call})) }};
    maplibre_core::check(status).map_err(map_error)?;
    storage.accept_callbacks();
    Ok(())
}}
'''
    parameter_types = ", ".join(
        values.type(p.value) for p in callback.parameters if p.name != callback.context
    )
    result_type = (
        "None" if callback.result.native == "void" else values.type(callback.result)
    )
    public_type = f"Callable[[{parameter_types}], {result_type}] | None"
    facade = f"""def {name}(callback: {public_type} = None) -> None:
    return _native.{name}({public_name(descriptor.native)}(callback))
"""
    return (
        "",
        native,
        facade,
        f"def {name}(callback: {public_type} = None) -> None: ...\n",
        None,
    )
