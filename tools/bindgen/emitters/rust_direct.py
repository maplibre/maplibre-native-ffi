"""Native-retired direct callback registrations for Rust."""

from .rust import Unsupported, doc, pascal
from .rust_callbacks import callback_parameter_type, trampoline


def add(values, plan, registration):
    if not registration.release_callback:
        raise Unsupported("direct callback requires owner-managed retirement")
    callback_parameter = next(p for p in plan.inputs if p.name == registration.callback)
    callback = values.bound.callbacks[callback_parameter.value.native]
    if callback.decision or callback.reentry_policy:
        raise Unsupported("direct callback requires a decision owner")
    for parameter in callback.parameters:
        if parameter.name != callback.context:
            values.add(parameter.value, "out")
    if callback.result.ctype.kind != "void":
        values.add(callback.result, "in")
    values.direct_callbacks[callback.native] = (callback, plan, registration)
    return pascal(callback.native)


def declaration(values, callback, plan, registration):
    name = pascal(callback.native)
    arguments = ", ".join(
        callback_parameter_type(values, p)
        for p in callback.parameters
        if p.name != callback.context
    )
    code, output = trampoline(values, callback, "invoke", "state", name)
    release_type = next(
        p.value.native for p in plan.inputs if p.name == registration.release_callback
    )
    return f"""{doc(values.bound, callback.native)}pub type {name} = std::sync::Arc<dyn Fn({arguments}) -> {output} + Send + Sync + 'static>;
pub(crate) fn {callback.native.removeprefix("mln_")}_registration(callback: Option<{name}>, arena: &mut InputArena) -> (sys::{callback.native}, *mut std::ffi::c_void, sys::{release_type}) {{
    {code}
    match callback {{
        // SAFETY: the release reclaims exactly this state.
        Some(callback) => (Some(invoke), unsafe {{ arena.registration(callback, callback::release::<{name}>) }}, Some(callback::release::<{name}>)),
        None => (None, std::ptr::null_mut(), None),
    }}
}}
"""
