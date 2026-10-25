"""Retained callback descriptors and scoped responses."""

from .rust import Unsupported, doc, identifier, native_call, native_identifier
from .rust_dynamic_values import decode, encode, optional


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
        for parameter in callback.arguments:
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
        for parameter in callback.arguments:
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
        return (
            public,
            raw,
            "value.bits()" if result.enum_kind == "bitmask" else "value.as_raw()",
        )
    raise Unsupported(f"{callback.native}: callback result requires a native copy")


def failure_value(callback):
    """The Rust value of a callback's `failure=`: a literal, such as a scalar
    result's integer or boolean, or else a `sys` constant."""
    failure = callback.failure
    if failure in {None, "contain"}:
        return "Default::default()"
    if callback.result.kind == "scalar" or failure.lstrip("-").isdigit():
        return failure
    return f"sys::{failure}"


def callback_doc(summary, callback, indent):
    """A callback field's doc, which names the arguments that a closure bound
    cannot, and how a status callback's error reaches native."""
    paragraphs = []
    if callback.arguments:
        names = ", ".join(f"`{p.name}`" for p in callback.arguments)
        paragraphs.append([f"The callback receives ({names})."])
    if callback.status:
        paragraphs.append(
            [
                "An `Err` of `Error::Status` with a failure status answers native",
                "that status. Any other error, or a panic, reaches the reporter and",
                f"answers `{callback.failure}`.",
            ]
        )
    lines = [summary] if summary else []
    for paragraph in paragraphs:
        if lines:
            lines.append(f"{indent}///\n")
        lines.extend(f"{indent}/// {line}\n" for line in paragraph)
    return "".join(lines)


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
    if callback.borrowed_owner:
        locals_by_name[callback.borrowed_owner] = (
            "_" + locals_by_name[callback.borrowed_owner]
        )
    arguments = callback.arguments
    raw_arguments = ", ".join(
        f"{locals_by_name[p.name]}: {native_type(values, p.value.ctype)}"
        for p in callback.parameters
    )
    # An owned handle is adopted before the other arguments convert, so a
    # conversion that fails drops its owner, which releases the handle.
    owned = [
        p
        for p in arguments
        if p.value.kind == "handle" and p.value.ownership == "owned"
    ]
    adoptions = "".join(
        f"let {locals_by_name[p.name]} = {callback_parameter_capture(values, p, locals_by_name[p.name])}; "
        for p in owned
    )
    copies = ", ".join(
        locals_by_name[p.name]
        if p in owned
        else callback_parameter_capture(values, p, locals_by_name[p.name])
        for p in arguments
    )
    public, raw, converted = callback_output(values, callback)
    returns = "" if raw == "()" else f" -> {raw}"
    context = locals_by_name[callback.context]
    if callback.status:
        # The outer result carries a failure to convert the arguments, and the
        # inner one host code's result.
        host = f"{adoptions}Ok({host}({copies}))"
        run = f'callback::invoke_status("{callback.native}", {failure_value(callback)}, || {{ {host} }})'
    else:
        host = f"{adoptions}{host}({copies})"
        body = (
            f"{{ {host}; Ok(()) }}"
            if raw == "()"
            else f"{{ let value = {host}; Ok({converted}) }}"
        )
        fallback = "()" if raw == "()" else failure_value(callback)
        run = f'callback::invoke("{callback.native}", {fallback}, || {body})'
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
        arguments = callback.arguments
        signature = ", ".join(callback_parameter_type(values, p) for p in arguments)
        code, output = trampoline(
            values,
            callback,
            f"{public}_trampoline",
            f"callback::require(&state.{public})?",
        )
        trampolines.append(code)
        returns = "" if output == "()" else f" -> {output}"
        function = f"Fn({signature}){returns} + Send + Sync + 'static"
        fields.append(
            f"{callback_doc(doc(values.bound, f'{value.native}.{field.name}', '    '), callback, '    ')}    pub {public}: Option<std::sync::Arc<dyn {function}>>,"
        )
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
        if field.role == "count":
            # A registration's arrays, such as provider routes, carry counts
            # like any other record's.
            array = next(
                (f for f in value.fields if f.value.length == field.name), None
            )
            if array is None:
                raise Unsupported(f"{value.native}.{field.name}: count has no array")
            source = f"self.{identifier(array.name)}"
            length = (
                f"{source}.as_ref().map_or(0, |items| items.len())"
                if optional(array.value) or array.presence
                else f"{source}.len()"
            )
            writes.append(f"{place} = convert::count({length})?;")
            continue
        if not field.public or field.name in value.registration.callbacks:
            continue
        public = identifier(field.name)
        masked = field.presence and field.presence.mask
        typ = values.public(field.value)
        fields.append(
            f"{doc(values.bound, f'{value.native}.{field.name}', '    ')}    pub {public}: {'Option<' + typ + '>' if masked else typ},"
        )
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
        f"impl Default for {name} {{ fn default() -> Self {{ maplibre_core::require_abi_version(); convert::native_default(unsafe {{ sys::{value.default}() }}) }} }}\n"
        if value.default
        else ""
    )
    user_data = native_identifier(value.registration.user_data)
    release = native_identifier(value.registration.release)
    return (
        doc(values.bound, value.native)
        + f"#[derive(Clone{', Default' if not value.default else ''})]\n"
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


def response(value):
    return (
        value.element
        if value.kind == "reference" and value.element.response
        else value
        if value.response
        else None
    )


def lent_text(value):
    """Whether a callback argument is text that C lends for the call, which the
    host borrows rather than copies."""
    return (
        value.kind == "buffer"
        and value.encoding == "utf8"
        and not optional(value)
        and (value.length == "nul" or value.buffer_form == "view")
    )


def callback_parameter_type(values, parameter):
    scoped = response(parameter.value)
    if scoped:
        return "&mut " + values.public(scoped)
    return "&str" if lent_text(parameter.value) else values.public(parameter.value)


def callback_parameter_capture(values, parameter, local=None):
    local = local or identifier(parameter.name)
    scoped = response(parameter.value)
    if scoped:
        return f"&mut {values.name(scoped)}::new({local})?"
    if lent_text(parameter.value):
        borrow = (
            "borrow_c_string"
            if parameter.value.length == "nul"
            else "borrow_string_view"
        )
        return f"unsafe {{ maplibre_core::string::{borrow}({local}) }}?"
    if parameter.value.kind == "handle" and parameter.value.ownership == "owned":
        return f"{values.public(parameter.value)}::adopt({local}, None)?"
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
                if (
                    parameter.value.buffer_form == "view"
                    or parameter.value.length == "nul"
                ) and "let arena = &mut InputArena::default();" not in setup:
                    setup.insert(0, "let arena = &mut InputArena::default();")
                setup.append(
                    f"let native_{local} = {encode(values, parameter.value, local)};"
                )
                args.append(f"native_{local}")
            else:
                raise Unsupported(
                    f"response parameter {method_name}.{parameter.name} requires conversion"
                )
        method = identifier(method_name.removeprefix(value.native + "_"))
        if not plan.function.diagnostic:
            raise Unsupported(f"{plan.function.name}: status return has no diagnostic")
        call = native_call(plan.function, args, diagnostic="out_diagnostic")
        methods.append(
            doc(values.bound, method_name, "    ")
            + f"    pub fn {method}({', '.join(signature)}) -> Result<()> {{ {' '.join(setup)} maplibre_core::check(|out_diagnostic| unsafe {{ {call} }}) }}"
        )
    summary = doc(values.bound, value.native)
    return f"""/// A native response borrowed only for one host callback.
{"///" + chr(10) + summary if summary else ""}#[derive(Debug)]
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
