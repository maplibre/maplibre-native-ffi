"""Retained callback descriptors, scoped responses, and decision handles."""

from .rust import Unsupported, identifier, native_call, native_identifier
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

    return SCALARS.get(native, "sys::" + native)


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
                values.add(parameter.value, "out")
        if callback.result.ctype.kind != "void":
            values.add(callback.result, "in")
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
            values.add(
                field.value, "both" if value.native in values.bound.returned else "in"
            )


def reentry(callback, locals_by_name):
    """The reentry contract that host code runs under during `callback`."""
    if callback.reentry == "allow":
        return "None"
    policy = callback.reentry_policy
    if not policy:
        return "Some((&[], 0))"
    owner_parameter = next(
        p for p in callback.parameters if p.name == policy.owner_parameter
    )
    owner = locals_by_name[owner_parameter.name] + (
        ".0" if owner_parameter.value.kind == "handle" else " as usize as u64"
    )
    operations = ", ".join(f'"{operation}"' for operation in policy.operations)
    return f"Some((&[{operations}], {owner}))"


def callback_output(values, callback):
    """The host callback's Rust result and the C value that it returns."""
    if callback.result.ctype.kind == "void":
        return "()", "()", "value"
    if callback.status:
        return "Result<()>", f"sys::{callback.result.native}", "value"
    result = callback.result
    public = values.public(result)
    raw = native_type(values, result.ctype)
    if result.kind == "scalar":
        return public, raw, "value"
    if result.kind == "enum":
        return public, raw, "value.to_native()"
    raise Unsupported(f"{callback.native}: callback result requires a native copy")


def trampoline(values, callback, name, host, state="Self"):
    """One `unsafe extern "C"` function that runs a registration's callback.

    `host` is the host callback, read from `state`, the registration state of
    type `state`.
    """
    locals_by_name = {
        parameter.name: identifier(parameter.name) + "_"
        if identifier(parameter.name) in {"state", "callback", "value"}
        else identifier(parameter.name)
        for parameter in callback.parameters
    }
    arguments = [p for p in callback.parameters if p.name != callback.context]
    raw_arguments = ", ".join(
        f"{locals_by_name[p.name]}: {native_type(values, p.value.ctype)}"
        for p in callback.parameters
    )
    copies = ", ".join(
        callback_parameter_capture(values, p, locals_by_name[p.name]) for p in arguments
    )
    public, raw, converted = callback_output(values, callback)
    returns = "" if raw == "()" else f" -> {raw}"
    context = locals_by_name[callback.context]
    host = f"{host}({copies})"
    if callback.status:
        failure = (
            f"sys::{callback.failure}"
            if callback.failure not in {None, "contain"}
            else "Default::default()"
        )
        run = f"callback::invoke_status({reentry(callback, locals_by_name)}, {failure}, || {host})"
    else:
        fallback = (
            "()"
            if raw == "()"
            else f"sys::{callback.failure}"
            if callback.failure not in {None, "contain"}
            and not callback.failure.isdigit()
            else callback.failure
            if callback.failure and callback.failure.isdigit()
            else "Default::default()"
        )
        body = (
            f"{{ {host}; Ok(()) }}"
            if raw == "()"
            else f"{{ let value = {host}; Ok({converted}) }}"
        )
        run = f"callback::invoke({reentry(callback, locals_by_name)}, {fallback}, || {body})"
    return (
        f'unsafe extern "C" fn {name}({raw_arguments}){returns} {{\n'
        f"    // SAFETY: native passes the registration that this trampoline's\n"
        f"    // descriptor transferred.\n"
        f"    let state = unsafe {{ callback::state::<{state}>({context}) }};\n"
        f"    {run}\n"
        "}"
    ), public


def declaration(values, value):
    name = values.name(value)
    raw = f"sys::{value.native}"
    fields, writes, copies, trampolines, constructors = [], [], [], [], []
    for index, (field, callback) in enumerate(callbacks(values, value)):
        public = identifier(field.name)
        arguments = [p for p in callback.parameters if p.name != callback.context]
        signature = ", ".join(callback_parameter_type(values, p) for p in arguments)
        code, output = trampoline(
            values,
            callback,
            f"{public}_trampoline",
            f"callback::require(&state.{public})?",
        )
        if callback.decision:
            code = decision_trampoline(values, callback, f"{public}_trampoline", public)
        trampolines.append(code)
        function = f"Fn({signature}) -> {output} + Send + Sync + 'static"
        fields.append(f"    pub {public}: Option<std::sync::Arc<dyn {function}>>,")
        writes.append(
            f"raw.{native_identifier(field.name)} = self.{public}.as_ref().map(|_| Self::{public}_trampoline as _);"
        )
        copies.append(f"{public}: None,")
        constructors.append(
            f"pub fn with_{field.name}<F>(mut self, callback: F) -> Self where F: {function} {{ self.{public} = Some(std::sync::Arc::new(callback)); self }}"
        )
        if index == 0:
            constructors.append(
                f"pub fn new<F>(callback: F) -> Self where F: {function} {{ Self::default().with_{field.name}(callback) }}"
            )
    for field in value.fields:
        place = f"raw.{native_identifier(field.name)}"
        if field.role == "presence_mask":
            writes.insert(0, f"{place} = 0;")
        if field.role == "size":
            writes.insert(0, f"{place} = std::mem::size_of::<{raw}>() as _;")
            continue
        if not field.public or field.name in value.registration.callbacks:
            continue
        public = identifier(field.name)
        masked = field.presence and field.presence.mask
        typ = values.public(field.value)
        fields.append(f"    pub {public}: {'Option<' + typ + '>' if masked else typ},")
        copied = decode(values, field.value, place)
        if masked:
            from .rust_dynamic_values import masked_field

            write, copy = masked_field(values, field, place, public)
            writes.append(write)
            copies.append(copy)
        else:
            writes.append(f"{place} = {encode(values, field.value, 'self.' + public)};")
            copies.append(f"{public}: {copied},")
    initial = (
        f"unsafe {{ sys::{value.default}() }}"
        if value.default
        else "unsafe { std::mem::zeroed() }"
    )
    empty = " && ".join(
        f"raw.{native_identifier(field.name)}.is_none()"
        for field, _ in callbacks(values, value)
    )
    default = (
        f"impl Default for {name} {{ fn default() -> Self {{ convert::native_default(unsafe {{ sys::{value.default}() }}) }} }}\n"
        if value.default
        else ""
    )
    user_data = native_identifier(value.registration.user_data)
    release = native_identifier(value.registration.release)
    return (
        f"#[derive(Clone{', Default' if not value.default else ''})]\n"
        f"pub struct {name} {{\n"
        + "\n".join(fields)
        + "\n}\n"
        + default
        + f'impl std::fmt::Debug for {name} {{ fn fmt(&self, f: &mut std::fmt::Formatter<\'_>) -> std::fmt::Result {{ f.debug_struct("{name}").finish_non_exhaustive() }} }}\n'
        + f"impl {name} {{\n"
        + "".join(f"    {line}\n" for line in constructors)
        + "\n".join(trampolines)
        + "\n}\n"
        + f"impl ToNative<{raw}> for {name} {{\n"
        + f"    fn to_native(&self, arena: &mut InputArena) -> Result<{raw}> {{\n"
        + f"        let mut raw: {raw} = {initial};\n"
        + "".join(f"        {line}\n" for line in writes)
        + f"        if !({empty}) {{\n"
        + "            // SAFETY: the release reclaims exactly this state.\n"
        + f"            raw.{user_data} = unsafe {{ arena.registration(self.clone(), callback::release::<Self>) }};\n"
        + f"            raw.{release} = Some(callback::release::<Self>);\n"
        + "        }\n        Ok(raw)\n    }\n}\n"
        + (
            f"impl FromNative<{raw}> for {name} {{\n"
            + "    /// Copies a native default, whose callbacks are unset.\n"
            + f"    unsafe fn from_native(raw: {raw}) -> Result<Self> {{\n"
            + "        Ok(Self {\n"
            + "".join(f"            {line}\n" for line in copies)
            + "        })\n    }\n}\n"
            if value.native in values.bound.returned
            else ""
        )
    )


def decision_trampoline(values, callback, name, public):
    """A provider trampoline, which turns the host's decision into ownership of
    the request handle it was given."""
    decision = callback.decision
    locals_by_name = {
        parameter.name: identifier(parameter.name) for parameter in callback.parameters
    }
    arguments = [p for p in callback.parameters if p.name != callback.context]
    raw_arguments = ", ".join(
        f"{locals_by_name[p.name]}: {native_type(values, p.value.ctype)}"
        for p in callback.parameters
    )
    copies = ", ".join(
        callback_parameter_capture(values, p, locals_by_name[p.name]) for p in arguments
    )
    raw = native_type(values, callback.result.ctype)
    fallback = (
        f"sys::{callback.failure}"
        if callback.failure not in {None, "contain"}
        else "Default::default()"
    )
    functions = decision_table_name(decision)
    return f"""unsafe extern "C" fn {name}({raw_arguments}) -> {raw} {{
    let _policy = callback::enter({reentry(callback, locals_by_name)});
    // SAFETY: native lends the callback this live decision handle.
    let Ok(request_state) = (unsafe {{ maplibre_core::decision::DecisionHandleState::new({locals_by_name[decision.parameter]}, {functions}) }}) else {{
        return {fallback};
    }};
    // SAFETY: native passes the registration that this trampoline's
    // descriptor transferred.
    let state = unsafe {{ callback::state::<Self>({locals_by_name[callback.context]}) }};
    match callback::invoke(None, None, || Ok(Some(callback::require(&state.{public})?({copies}).to_native()))) {{
        Some(sys::{decision.accept}) => request_state.finish_decision(true),
        Some(sys::{decision.pass_through}) => request_state.finish_decision(false),
        _ => request_state.finish_exception(),
    }}
}}"""


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
        return f"&mut {values.name(scoped)}::new({local})?"
    if parameter.value.kind == "handle":
        return f"{values.name(parameter.value)} {{ state: std::sync::Arc::clone(&request_state), not_sync: std::marker::PhantomData }}"
    return decode(values, parameter.value, local)


def response_declaration(values, value):
    name = values.name(value)
    raw = f"sys::{value.native}"
    methods = []
    for method_name in value.response.methods:
        plan = values.bound.operations_by_name[method_name]
        receiver = plan.scoped_receiver
        args, signature, setup = [], ["&mut self"], []
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
                # The checked closure returns a status, so fallible conversions precede it.
                setup.append(
                    f"let {local} = convert::count({identifier(counts[parameter.name])}.len())?;"
                )
                args.append(local)
            elif parameter.value.kind == "buffer":
                public = "&str" if parameter.value.encoding == "utf8" else "&[u8]"
                signature.append(f"{local}: {public}")
                args.append(encode(values, parameter.value, local))
            else:
                raise Unsupported(
                    f"response parameter {method_name}.{parameter.name} requires conversion"
                )
        method = identifier(method_name.removeprefix(value.native + "_"))
        if not plan.function.diagnostic:
            raise Unsupported(f"{plan.function.name}: status return has no diagnostic")
        call = native_call(plan.function, args, diagnostic="out_diagnostic")
        methods.append(
            f'    pub fn {method}({", ".join(signature)}) -> Result<()> {{ maplibre_core::callback::check("{method_name}", self.raw.as_ptr() as usize as u64)?; {" ".join(setup)} maplibre_core::check(|out_diagnostic| unsafe {{ {call} }}) }}'
        )
    return f"""/// A native response borrowed only for one host callback.
#[derive(Debug)]
pub struct {name}<'a> {{
    raw: std::ptr::NonNull<{raw}>,
    lifetime: std::marker::PhantomData<&'a mut {raw}>,
}}
impl {name}<'_> {{
    fn new(raw: *mut {raw}) -> Result<Self> {{
        let raw = std::ptr::NonNull::new(raw).ok_or_else(|| Error::invalid_argument("null callback response"))?;
        Ok(Self {{ raw, lifetime: std::marker::PhantomData }})
    }}
{chr(10).join(methods)}
}}
"""


def decision_table_name(decision) -> str:
    """The generated constant that holds one decision protocol's functions."""
    return f"{decision.handle.stem.upper()}_DECISION"


def decision_reentry(bound, decision) -> tuple[str, ...]:
    """The operations a decision handle's cancellation notification may call."""
    plan = bound.operations_by_name[decision.cancel_registration]
    (registration,) = plan.direct_registrations
    callback = bound.callbacks[
        next(p.value.native for p in plan.inputs if p.name == registration.callback)
    ]
    return callback.reentry_policy.operations if callback.reentry_policy else ()


def decision_table(bound, decision, owner: str) -> str:
    """Declare the function table that one decision protocol's state uses."""
    handle = decision.handle.native
    reentry = ", ".join(f'"{name}"' for name in decision_reentry(bound, decision))
    return (
        f"pub(crate) const {decision_table_name(decision)}: "
        f"maplibre_core::decision::DecisionHandleFns<sys::{handle}> = unsafe {{ "
        f"maplibre_core::decision::DecisionHandleFns::new("
        f'"{owner}", sys::{decision.accept}, sys::{decision.pass_through}, '
        f"sys::{decision.handle.release}, sys::{decision.cancel_registration}, "
        f"&[{reentry}]) }};\n"
    )


def decision_declaration(values, value):
    decision = values.bound.decisions[value.native]
    complete = values.bound.operations_by_name[decision.complete]
    functions = values.bound.source.functions_by_name

    def checked(operation, *arguments):
        function = functions[operation]
        if not function.diagnostic:
            raise Unsupported(f"{operation}: status return has no diagnostic")
        call = native_call(function, arguments, diagnostic="out_diagnostic")
        return f"maplibre_core::check(|out_diagnostic| unsafe {{ {call} }})"

    response_value = next(
        parameter.value.element
        for parameter in complete.inputs
        if parameter.name != complete.receiver
    )
    name = values.name(value)

    def method(operation):
        return identifier(values.bound.operations_by_name[operation].member)

    cancel_registration(values, decision)
    return f'''#[derive(Debug)]
pub struct {name} {{
    state: std::sync::Arc<maplibre_core::decision::DecisionHandleState<sys::{decision.handle.native}>>,
    not_sync: std::marker::PhantomData<std::cell::Cell<()>>,
}}
impl {name} {{
    pub fn {method(decision.complete)}(&self, response: &{values.public(response_value)}) -> Result<()> {{
        let native = self.state.native_for_call()?;
        maplibre_core::callback::check("{decision.complete}", native.0)?;
        let mut arena = InputArena::default();
        let response = response.to_native(&mut arena)?;
        self.state.complete_with(|handle| {checked(decision.complete, "handle", "&response")})
    }}
    pub fn {method(decision.cancelled)}(&self) -> Result<bool> {{
        let native = self.state.native_for_call()?;
        maplibre_core::callback::check("{decision.cancelled}", native.0)?;
        let mut cancelled = false;
        {checked(decision.cancelled, "native", "&mut cancelled")}?;
        Ok(cancelled)
    }}
    pub fn {method(decision.cancel_registration)}(&self, callback: impl FnOnce() + Send + 'static) -> Result<bool> {{
        let native = self.state.native_for_call()?;
        maplibre_core::callback::check("{decision.cancel_registration}", native.0)?;
        self.state.register_cancel(Box::new(callback))
    }}
    pub fn {method(decision.wait_retired)}(&self) -> Result<()> {{
        let native = self.state.issued_handle();
        maplibre_core::callback::check("{decision.wait_retired}", native.0)?;
        {checked(decision.wait_retired, "native")}
    }}
    pub fn close(&self) -> Result<()> {{
        maplibre_core::callback::check("{decision.handle.release}", self.state.issued_handle().0)?;
        self.state.close();
        Ok(())
    }}
}}
'''


def cancel_registration(values, decision):
    """Check that the cancel registration is the one-shot request notification
    whose root native releases, which the core runtime registers."""
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
