"""Native-retired direct callback registrations for Rust."""

from .rust import Unsupported, pascal
from .rust_callbacks import (
    callback_parameter_capture,
    callback_parameter_type,
    native_type,
)


def add(values, plan, registration):
    if not registration.release_callback:
        raise Unsupported("direct callback requires owner-managed retirement")
    callback_parameter = next(p for p in plan.inputs if p.name == registration.callback)
    callback = values.bound.callbacks[callback_parameter.value.native]
    if callback.decision or callback.reentry_policy:
        raise Unsupported("direct callback requires a decision owner")
    for parameter in callback.parameters:
        if parameter.name != callback.context:
            values.add(parameter.value)
    if callback.result.ctype.kind != "void":
        values.add(callback.result)
    values.direct_callbacks[callback.native] = (callback, plan, registration)
    return pascal(callback.native)


def declaration(values, callback, plan, registration):
    name = pascal(callback.native)
    locals_by_name = {
        parameter.name: f"binding_arg_{index}"
        for index, parameter in enumerate(callback.parameters)
    }
    args = [p for p in callback.parameters if p.name != callback.context]
    arguments = ", ".join(callback_parameter_type(values, p) for p in args)
    output = (
        "()" if callback.result.ctype.kind == "void" else values.public(callback.result)
    )
    raw_output = (
        "()"
        if callback.result.ctype.kind == "void"
        else native_type(values, callback.result.ctype)
    )
    raw_arguments = ", ".join(
        f"{locals_by_name[p.name]}: {native_type(values, p.value.ctype)}"
        for p in callback.parameters
    )
    copies = ", ".join(
        callback_parameter_capture(values, p, locals_by_name[p.name]) for p in args
    )
    result = "value" if callback.result.kind == "scalar" else "value.to_native()"
    fallback = (
        callback.failure
        if callback.failure and callback.failure.isdigit()
        else "Default::default()"
    )
    release_type = next(
        p.value.native for p in plan.inputs if p.name == registration.release_callback
    )
    scope = (
        "let _policy = crate::callback::PolicyScope::enter(&[], 0);"
        if callback.reentry == "forbid"
        else ""
    )
    return f"""pub type {name} = std::sync::Arc<dyn Fn({arguments}) -> {output} + Send + Sync + 'static>;
pub fn {callback.native.removeprefix("mln_")}_registration(callback: Option<{name}>, arena: &mut crate::input::InputArena) -> (maplibre_native_ffi_sys::{callback.native}, *mut std::ffi::c_void, maplibre_native_ffi_sys::{release_type}) {{
    unsafe extern "C" fn invoke({raw_arguments}) -> {raw_output} {{
        {scope}
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<{raw_output}> {{
            let callback = unsafe {{ &*{locals_by_name[callback.context]}.cast::<{name}>() }};
            let value = callback({copies});
            Ok({result})
        }}));
        match result {{ Ok(Ok(value)) => value, _ => {fallback} }}
    }}
    unsafe extern "C" fn release(context: *mut std::ffi::c_void) {{
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {{ drop(Box::from_raw(context.cast::<{name}>())); }}));
    }}
    match callback {{ Some(callback) => (Some(invoke), unsafe {{ arena.registration(callback, release) }}, Some(release)), None => (None, std::ptr::null_mut(), None) }}
}}
"""
