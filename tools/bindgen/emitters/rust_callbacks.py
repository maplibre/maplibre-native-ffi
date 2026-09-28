"""Retained callback descriptors and quiescent native release adapters."""

from .rust import Unsupported, identifier, native_identifier
from .rust_dynamic_values import decode, encode


def native_type(values, ctype):
    if ctype.pointee:
        return ("*const " if ctype.pointee.const else "*mut ") + native_type(
            values, ctype.pointee
        )
    native = ctype.declaration or ctype.spelling.removeprefix("const ")
    if native == "void":
        return "std::ffi::c_void"
    if native == "char":
        return "std::ffi::c_char"
    if native in values.bound.values:
        value = values.bound.values[native]
        if value.kind == "scalar":
            return values.scalar(value)
    from .rust import SCALARS

    return SCALARS.get(native, "maplibre_native_ffi_sys::" + native)


def callbacks(values, value):
    return [
        (field, values.bound.callbacks[field.value.native])
        for field in value.fields
        if field.name in value.registration.callbacks
    ]


def validate(values, value):
    for field, callback in callbacks(values, value):
        if any(
            parameter.direction != "in" and not response(parameter.value)
            for parameter in callback.parameters
        ):
            raise Unsupported("callback output requires scoped response metadata")
        for parameter in callback.parameters:
            if parameter.name != callback.context:
                values.check(parameter.value)
        if callback.result.ctype.kind != "void":
            values.check(callback.result)
    for field in value.fields:
        if (
            field.name
            not in {
                *value.registration.callbacks,
                value.registration.user_data,
                value.registration.release,
            }
            and field.role == "value"
        ):
            values.check(field.value)


def add(values, value):
    validate(values, value)
    values.used[value.native] = value
    for field, callback in callbacks(values, value):
        for parameter in callback.parameters:
            if parameter.name != callback.context:
                values.add(parameter.value)
        if callback.result.ctype.kind != "void":
            values.add(callback.result)
    for field in value.fields:
        if (
            field.name
            not in {
                *value.registration.callbacks,
                value.registration.user_data,
                value.registration.release,
            }
            and field.role == "value"
        ):
            values.add(field.value)


def declaration(values, value):
    name = values.name(value)
    fields, writes, captures, trampolines = [], [], [], []
    for field, callback in callbacks(values, value):
        public = identifier(field.name)
        locals_by_name = {
            parameter.name: f"binding_arg_{index}"
            for index, parameter in enumerate(callback.parameters)
        }
        args = [
            parameter
            for parameter in callback.parameters
            if parameter.name != callback.context
        ]
        returns = callback.result.ctype.kind != "void"
        status_result = callback.result.native == "mln_status"
        output = (
            "crate::Result<()>"
            if status_result
            else values.public(callback.result)
            if returns
            else "()"
        )
        fields.append(
            f"    pub {public}: Option<std::sync::Arc<dyn Fn({', '.join(callback_parameter_type(values, p) for p in args)}) -> {output} + Send + Sync + 'static>>,"
        )
        writes.append(
            f"        raw.{native_identifier(field.name)} = self.{public}.as_ref().map(|_| Self::{public}_trampoline as _);"
        )
        captures.append(f"            {public}: None,")
        raw_args = ", ".join(
            f"{locals_by_name[p.name]}: {native_type(values, p.value.ctype)}"
            for p in callback.parameters
        )
        raw_output = native_type(values, callback.result.ctype) if returns else "()"
        convert = ", ".join(
            callback_parameter_capture(values, p, locals_by_name[p.name]) for p in args
        )
        result = (
            f"callback({convert})?; Ok(maplibre_native_ffi_sys::MLN_STATUS_OK)"
            if status_result
            else f"let result = callback({convert}); Ok(result.to_native())"
            if returns
            else f"callback({convert}); Ok(())"
        )
        fallback = (
            f"maplibre_native_ffi_sys::{callback.failure}"
            if returns and callback.failure not in {None, "contain"}
            else "Default::default()"
        )
        policy = callback.reentry_policy
        owner = "0"
        if policy:
            owner_param = next(
                p for p in callback.parameters if p.name == policy.owner_parameter
            )
            owner = locals_by_name[owner_param.name] + (
                ".0" if owner_param.value.kind == "handle" else " as usize as u64"
            )
        operations = (
            ", ".join('"' + operation + '"' for operation in policy.operations)
            if policy
            else ""
        )
        scope = (
            f"let _policy = crate::callback::PolicyScope::enter(&[{operations}], {owner});"
            if callback.reentry != "allow"
            else ""
        )
        decision_setup = ""
        decision_scope = ""
        finish = f"match result {{ Ok(Ok(value)) => value, {'Ok(Err(error)) => crate::resource::status_for_error(&error),' if status_result else ''} _ => {fallback} }}"
        if callback.decision:
            decision = callback.decision
            decision_scope, scope = scope, ""
            parameter = locals_by_name[decision.parameter]
            decision_setup = f"let request_state = match unsafe {{ crate::resource::ResourceRequestHandleState::new({parameter}, crate::generated::{decision.handle.native.removeprefix('mln_').upper()}_FUNCTIONS) }} {{ Ok(state) => state, Err(_) => return {fallback} }};"
            finish = f"match result {{ Ok(Ok(value)) if value == maplibre_native_ffi_sys::{decision.accept} => request_state.finish_provider_decision(crate::resource::ResourceProviderDecision::Handle), Ok(Ok(value)) if value == maplibre_native_ffi_sys::{decision.pass_through} => request_state.finish_provider_decision(crate::resource::ResourceProviderDecision::PassThrough), _ => request_state.finish_provider_exception() }}"
        trampolines.append(f"""    unsafe extern "C" fn {public}_trampoline({raw_args}) -> {raw_output} {{
        {decision_scope}
        {decision_setup}
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| -> crate::Result<{raw_output}> {{
            {scope}
            let state = unsafe {{ &*{locals_by_name[callback.context]}.cast::<Self>() }};
            let callback = state.{public}.as_ref().ok_or_else(|| crate::Error::invalid_argument("missing callback"))?;
            {result}
        }}));
        {finish}
    }}""")
    hidden = {
        *value.registration.callbacks,
        value.registration.user_data,
        value.registration.release,
    }
    for field in value.fields:
        if field.role == "presence_mask":
            writes.insert(0, f"        raw.{native_identifier(field.name)} = 0;")
        if field.name in hidden or field.role in {
            "reserved",
            "presence_mask",
            "count",
            "tag",
        }:
            continue
        raw = f"raw.{native_identifier(field.name)}"
        if field.role == "size":
            writes.insert(
                0,
                f"        {raw} = std::mem::size_of::<maplibre_native_ffi_sys::{value.native}>() as _;",
            )
            continue
        public = identifier(field.name)
        optional = field.presence and field.presence.mask
        typ = values.public(field.value)
        fields.append(
            f"    pub {public}: {'Option<' + typ + '>' if optional else typ},"
        )
        copied = decode(values, field.value, raw)
        if optional:
            mask, bit = field.presence.mask, field.presence.bit
            present = (
                f"raw.{mask} & maplibre_native_ffi_sys::{bit} != 0"
                if bit
                else f"raw.{mask}"
            )
            mark = (
                f"raw.{mask} |= maplibre_native_ffi_sys::{bit}"
                if bit
                else f"raw.{mask} = true"
            )
            writes.append(
                f"        if let Some(item) = &self.{public} {{ {mark}; {raw} = {encode(values, field.value, 'item')}; }}"
            )
            captures.append(
                f"            {public}: if {present} {{ Some({copied}) }} else {{ None }},"
            )
        else:
            writes.append(
                f"        {raw} = {encode(values, field.value, '&self.' + public)};"
            )
            captures.append(f"            {public}: {copied},")
    constructors = []
    for index, (field, callback) in enumerate(callbacks(values, value)):
        arguments = [p for p in callback.parameters if p.name != callback.context]
        signature = ", ".join(callback_parameter_type(values, p) for p in arguments)
        result = (
            "crate::Result<()>"
            if callback.result.native == "mln_status"
            else values.public(callback.result)
            if callback.result.ctype.kind != "void"
            else "()"
        )
        constructors.append(
            f"    pub fn with_{field.name}<F>(mut self, callback: F) -> Self where F: Fn({signature}) -> {result} + Send + Sync + 'static {{ self.{identifier(field.name)} = Some(std::sync::Arc::new(callback)); self }}"
        )
        if index == 0:
            constructors.append(
                f"    pub fn new<F>(callback: F) -> Self where F: Fn({signature}) -> {result} + Send + Sync + 'static {{ Self::default().with_{field.name}(callback) }}"
            )
    initial = (
        f"unsafe {{ maplibre_native_ffi_sys::{value.default}() }}"
        if value.default
        else "unsafe { std::mem::zeroed() }"
    )
    callbacks_empty = " && ".join(
        f"raw.{native_identifier(field.name)}.is_none()"
        for field, _ in callbacks(values, value)
    )
    default_impl = (
        f'impl Default for {name} {{ fn default() -> Self {{ unsafe {{ Self::from_native(maplibre_native_ffi_sys::{value.default}()).expect("native default has no callbacks") }} }} }}'
        if value.default
        else ""
    )
    return f'''#[derive(Clone{", Default" if not value.default else ""})]
pub struct {name} {{
{chr(10).join(fields)}
}}
{default_impl}
impl std::fmt::Debug for {name} {{ fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {{ f.debug_struct("{name}").finish_non_exhaustive() }} }}
impl {name} {{
{chr(10).join(constructors)}
    pub fn to_native(&self, arena: &mut crate::input::InputArena) -> crate::Result<maplibre_native_ffi_sys::{value.native}> {{
        let mut raw: maplibre_native_ffi_sys::{value.native} = {initial};
{chr(10).join(writes)}
        if !({callbacks_empty}) {{
            raw.{native_identifier(value.registration.user_data)} = unsafe {{ arena.registration(self.clone(), Self::release_callback) }};
            raw.{native_identifier(value.registration.release)} = Some(Self::release_callback);
        }}
        Ok(raw)
    }}
    /// Captures an empty callback descriptor returned by its native constructor.
    ///
    /// # Safety
    /// Borrowed fields must remain valid for this conversion.
    pub unsafe fn from_native(raw: maplibre_native_ffi_sys::{value.native}) -> crate::Result<Self> {{
        if !({callbacks_empty}) {{ return Err(crate::Error::invalid_argument("foreign callbacks cannot be adopted")); }}
        Ok(Self {{
{chr(10).join(captures)}
        }})
    }}
    unsafe extern "C" fn release_callback(user_data: *mut std::ffi::c_void) {{
        let _policy = crate::callback::PolicyScope::enter(&[], 0);
        let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {{ drop(Box::from_raw(user_data.cast::<Self>())) }}));
    }}
{chr(10).join(trampolines)}
}}
'''


def response(value):
    return (
        value.element
        if value.kind == "reference" and value.element.response
        else value
        if value.response
        else None
    )


def callback_parameter_type(values, parameter):
    scoped = response(parameter.value)
    return "&mut " + values.public(scoped) if scoped else values.public(parameter.value)


def callback_parameter_capture(values, parameter, local=None):
    local = local or identifier(parameter.name)
    scoped = response(parameter.value)
    if scoped:
        return f'&mut {values.name(scoped)} {{ raw: std::ptr::NonNull::new({local}).ok_or_else(|| crate::Error::invalid_argument("null callback response"))?, lifetime: std::marker::PhantomData }}'
    if parameter.value.kind == "handle":
        return f"{values.name(parameter.value)} {{ state: std::sync::Arc::clone(&request_state), not_sync: std::marker::PhantomData }}"
    return decode(values, parameter.value, local)


def response_declaration(values, value):
    methods = []
    for name in value.response.methods:
        plan = values.bound.operations_by_name[name]
        receiver = plan.scoped_receiver
        args, signature = [], ["&mut self"]
        counts = {
            p.value.length: p.name
            for p in plan.inputs
            if p.value.kind in {"buffer", "array"}
            and p.value.length
            and p.value.length != "nul"
        }
        for parameter in plan.inputs:
            local = identifier(parameter.name)
            if parameter.name == receiver:
                args.append("self.raw.as_ptr()")
            elif parameter.name in counts:
                args.append(
                    f'{identifier(counts[parameter.name])}.len().try_into().map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?'
                )
            elif parameter.value.kind == "buffer":
                public = "&str" if parameter.value.encoding == "utf8" else "&[u8]"
                signature.append(f"{local}: {public}")
                args.append(encode(values, parameter.value, local))
            else:
                raise Unsupported(
                    f"response parameter {name}.{parameter.name} requires conversion"
                )
        method = identifier(name.removeprefix(value.native + "_"))
        methods.append(
            f'    pub fn {method}({", ".join(signature)}) -> crate::Result<()> {{ crate::callback::check("{name}", self.raw.as_ptr() as usize as u64)?; crate::check(unsafe {{ maplibre_native_ffi_sys::{name}({", ".join(args)}) }}) }}'
        )
    return f"""/// A native response borrowed only for one host callback.
#[derive(Debug)]
pub struct {values.name(value)}<'a> {{
    raw: std::ptr::NonNull<maplibre_native_ffi_sys::{value.native}>,
    lifetime: std::marker::PhantomData<&'a mut maplibre_native_ffi_sys::{value.native}>,
}}
impl {values.name(value)}<'_> {{
{chr(10).join(methods)}
}}
"""


def decision_declaration(values, value):
    decision = next(
        callback.decision
        for callback in values.bound.callbacks.values()
        if callback.decision and callback.decision.handle.native == value.native
    )
    complete = values.bound.operations_by_name[decision.complete]
    response = next(
        parameter.value.element
        for parameter in complete.inputs
        if parameter.name != complete.receiver
    )
    name = values.name(value)
    prefix = value.native.removesuffix("_handle") + "_"
    method = lambda operation: identifier(operation.removeprefix(prefix))
    registration = cancel_registration(values, decision, method)
    return f'''#[derive(Debug)]
pub struct {name} {{
    state: std::sync::Arc<crate::resource::ResourceRequestHandleState>,
    not_sync: std::marker::PhantomData<std::cell::Cell<()>>,
}}
impl {name} {{
    pub fn {method(decision.complete)}(&self, response: &{values.public(response)}) -> crate::Result<()> {{
        let native = self.state.native_for_call()?;
        crate::callback::check("{decision.complete}", native.0)?;
        let mut arena = crate::input::InputArena::default();
        let response = response.to_native(&mut arena)?;
        self.state.complete_with(|handle| crate::check(unsafe {{ maplibre_native_ffi_sys::{decision.complete}(handle, &response) }}))
    }}
    pub fn {method(decision.cancelled)}(&self) -> crate::Result<bool> {{
        let native = self.state.native_for_call()?;
        crate::callback::check("{decision.cancelled}", native.0)?;
        let mut cancelled = false;
        crate::check(unsafe {{ maplibre_native_ffi_sys::{decision.cancelled}(native, &mut cancelled) }})?;
        Ok(cancelled)
    }}
    pub fn {method(decision.cancel_registration)}(&self, callback: impl FnOnce() + Send + 'static) -> crate::Result<bool> {{
        let native = self.state.native_for_call()?;
        crate::callback::check("{decision.cancel_registration}", native.0)?;
        self.state.{method(decision.cancel_registration)}(Box::new(callback))
    }}
    pub fn {method(decision.wait_retired)}(&self) -> crate::Result<()> {{
        let native = self.state.issued_handle();
        crate::callback::check("{decision.wait_retired}", native.0)?;
        crate::check(unsafe {{ maplibre_native_ffi_sys::{decision.wait_retired}(native) }})
    }}
    pub fn close(&self) -> crate::Result<()> {{
        crate::callback::check("{decision.handle.release}", self.state.issued_handle().0)?;
        self.state.close(); Ok(())
    }}
}}
{registration}'''


def cancel_registration(values, decision, method):
    """Emit the one-shot cancel registration whose root native releases."""
    plan = values.bound.operations_by_name[decision.cancel_registration]
    if len(plan.direct_registrations) != 1:
        raise Unsupported(f"{plan.name}: cancel registration needs one callback")
    registration = plan.direct_registrations[0]
    if not registration.release_callback or not registration.accepted_unless:
        raise Unsupported(
            f"{plan.name}: cancel registration needs a native release and report"
        )
    callback = values.bound.callbacks[
        next(p.value.native for p in plan.inputs if p.name == registration.callback)
    ]
    if (
        [p.name for p in callback.parameters] != [callback.context]
        or callback.result.ctype.kind != "void"
        or not callback.reentry_policy
        or not callback.reentry_policy.registration_owner
    ):
        raise Unsupported(
            f"{plan.name}: cancel callback must be a request notification"
        )
    allowed = ", ".join(
        f'"{operation}"' for operation in callback.reentry_policy.operations
    )
    arguments = {
        plan.receiver: "handle",
        registration.callback: "Some(invoke)",
        registration.user_data: "user_data",
        registration.release_callback: "Some(release)",
        registration.accepted_unless: "&mut cancelled",
    }
    call = ", ".join(arguments[p.name] for p in plan.function.parameters)
    return f"""impl crate::resource::ResourceRequestHandleState {{
    /// Registers a callback that runs at most once when MapLibre cancels the
    /// request, returning whether the request was already cancelled.
    ///
    /// An accepted registration transfers the callback to the C API, which
    /// releases it once it can no longer run. A rejected registration or an
    /// already cancelled request drops the callback unrun before returning.
    pub fn {method(decision.cancel_registration)}(&self, callback: Box<dyn FnOnce() + Send + 'static>) -> crate::Result<bool> {{
        type Registration = (u64, Option<Box<dyn FnOnce() + Send + 'static>>);
        unsafe extern "C" fn invoke(user_data: *mut std::ffi::c_void) {{
            // SAFETY: native passes the registration it owns and invokes it at
            // most once, before its release.
            let (owner, callback) = unsafe {{ &mut *user_data.cast::<Registration>() }};
            let _policy = crate::callback::PolicyScope::enter(&[{allowed}], *owner);
            if let Some(callback) = callback.take() {{
                let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(callback));
            }}
        }}
        unsafe extern "C" fn release(user_data: *mut std::ffi::c_void) {{
            let _policy = crate::callback::PolicyScope::enter(&[], 0);
            // SAFETY: native or the rejecting arena releases each registration once.
            let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| unsafe {{ drop(Box::from_raw(user_data.cast::<Registration>())) }}));
        }}
        let handle = self.native_for_call()?;
        let mut arena = crate::input::InputArena::default();
        // SAFETY: release reclaims exactly this box without unwinding.
        let user_data = unsafe {{ arena.registration::<Registration>((handle.0, Some(callback)), release) }};
        let mut cancelled = false;
        crate::check(unsafe {{ maplibre_native_ffi_sys::{plan.name}({call}) }})?;
        if !cancelled {{ arena.accept_registrations(); }}
        Ok(cancelled)
    }}
}}
"""
