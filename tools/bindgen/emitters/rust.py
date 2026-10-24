"""Generate Rust operations over the completion and handle runtimes."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import replace

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.semantic import (
    BoundApi,
    OperationPlan,
)

from ..model import Api, CType

SCALARS = {
    "int8_t": "i8",
    "int16_t": "i16",
    "uint16_t": "u16",
    "intptr_t": "isize",
    "ptrdiff_t": "isize",
    "uintptr_t": "usize",
    "bool": "bool",
    "_Bool": "bool",
    "double": "f64",
    "float": "f32",
    "uint32_t": "u32",
    "int32_t": "i32",
    "uint64_t": "u64",
    "int64_t": "i64",
    "size_t": "usize",
    "uint8_t": "u8",
}


class Unsupported(ValueError):
    pass


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The rustdoc comment of a declaration, or empty when it has none."""
    return docs.line_comment(bound.doc(native), indent)


def ctype(value: CType) -> str:
    return value.declaration or value.spelling.removeprefix("const ")


def pascal(value: str) -> str:
    return "".join(word.title() for word in value.removeprefix("mln_").split("_"))


RUST_KEYWORDS = {
    "abstract",
    "as",
    "async",
    "await",
    "become",
    "box",
    "break",
    "const",
    "continue",
    "crate",
    "do",
    "dyn",
    "else",
    "enum",
    "extern",
    "false",
    "final",
    "fn",
    "for",
    "gen",
    "if",
    "impl",
    "in",
    "let",
    "loop",
    "macro",
    "match",
    "mod",
    "move",
    "mut",
    "override",
    "priv",
    "pub",
    "ref",
    "return",
    "Self",
    "self",
    "static",
    "struct",
    "super",
    "trait",
    "true",
    "try",
    "type",
    "typeof",
    "unsafe",
    "unsized",
    "use",
    "virtual",
    "where",
    "while",
    "yield",
}
BINDGEN_RESERVED = RUST_KEYWORDS | {
    "alignof",
    "offsetof",
    "proc",
    "pure",
    "sizeof",
    "str",
    "bool",
    "f32",
    "f64",
    "usize",
    "isize",
    "u128",
    "i128",
    "u64",
    "i64",
    "u32",
    "i32",
    "u16",
    "i16",
    "u8",
    "i8",
    "_",
}


def identifier(value: str) -> str:
    if value in {"self", "Self", "super", "crate", "_"}:
        return value + "_"
    return "r#" + value if value in RUST_KEYWORDS else value


def native_identifier(value: str) -> str:
    return value + "_" if value in BINDGEN_RESERVED else value


def native_call(
    function, arguments, prefix: str = "sys::", diagnostic: str = "diagnostic"
) -> str:
    """Calls a C function, passing `diagnostic` for its diagnostic parameter."""
    if function.diagnostic:
        arguments = [*arguments, diagnostic]
    return f"{prefix}{function.name}({', '.join(arguments)})"


def checked_call(
    function, arguments, prefix: str = "sys::", runtime: str = "maplibre_core"
) -> str:
    """Checks a status-returning C call against the diagnostic the runtime lends it."""
    if not function.diagnostic:
        raise Unsupported(f"{function.name}: status return has no diagnostic")
    call = native_call(function, arguments, prefix)
    return f"{runtime}::check(|diagnostic| unsafe {{ {call} }})"


# Emitted bodies use these as crate paths and a view's callback parameter, so
# no parameter can take them.
RESERVED_LOCALS = {"callback", "sys", "convert"}


def local_name(name: str) -> str:
    """A parameter's Rust name, which is its C name."""
    name = identifier(name)
    if name in RESERVED_LOCALS:
        raise Unsupported(f"parameter {name} shadows a name the emitted body uses")
    return name


def adopt_owned(owned, raw, value_types, parent="parent"):
    public = value_types.handle_types.get(owned.handle.native)
    if not public:
        raise Unsupported(f"{owned.handle.native}: owner runtime unavailable")
    source = parent if owned.parent_parameter else "None"
    return public, f"{public}::adopt({raw}, {source})"


def result_rule(
    plan: OperationPlan, value_types, parent: str = "parent"
) -> tuple[str, str]:
    """The completion's public result type and the converter that copies it."""
    from .rust_dynamic_values import decode

    result = plan.result
    if result is None:
        return "()", "completion::unit"
    if plan.completion and plan.completion.result_owner:
        public, adopted = adopt_owned(
            plan.completion.result_owner,
            f"completion::copy_value::<sys::{result.native}>(result)?",
            value_types,
            parent,
        )
        return public, f"move |result| {adopted}"
    if result.ownership != "borrowed":
        raise Unsupported("completion result requires resolved ownership transfer")
    array = result.kind == "array"
    value = result.element if array else replace(result, nullable=False)
    value_types.add(value, "out")
    public = value_types.public(value)
    raw = SCALARS.get(value.native, "sys::" + value.native)
    copied = decode(value_types, value, "value")
    rule = (
        ("optional_list" if result.nullable else "list")
        if array
        else ("optional" if result.nullable else "value")
    )
    public = f"Vec<{public}>" if array else public
    public = f"Option<{public}>" if result.nullable else public
    if copied == "unsafe { from_native(value) }?":
        return public, f"completion::{rule}::<{raw}, _>"
    copy = {
        "value": "copy_value",
        "optional": "optional_value",
        "list": "copy_slice",
        "optional_list": "optional_slice",
    }[rule]
    return (
        public,
        f"|result| {{ let value = completion::{copy}::<{raw}>(result)?; Ok({_map(rule, copied)}) }}",
    )


def _map(rule: str, copied: str) -> str:
    each = f"|value| -> Result<_> {{ Ok({copied}) }}"
    return {
        "value": copied,
        "optional": f"value.map({each}).transpose()?",
        "list": f"value.into_iter().map({each}).collect::<Result<Vec<_>>>()?",
        "optional_list": f"value.map(|value| value.into_iter().map({each}).collect::<Result<Vec<_>>>()).transpose()?",
    }[rule]


def reads_receiver(plan: OperationPlan) -> bool:
    """An immediate receiver call copies borrowed output without a native view scope."""
    from .rust_dynamic_values import dynamic

    def borrowed(value):
        target = value.unwrapped
        return "owner" in {value.lifetime, target.lifetime} or dynamic(target)

    return bool(
        plan.receiver
        and not plan.consumes
        and not plan.view
        and plan.execution == "immediate"
        and any(
            borrowed(output.value)
            for output in plan.outputs
            if not output.sized_by and not output.result_length
        )
    )


def counted_owner(bound: BoundApi, native: str) -> bool:
    """A release must not overlap a copy from this owner's borrowed output."""
    return any(
        reads_receiver(plan)
        and any(
            p.name == plan.receiver and p.value.native == native for p in plan.inputs
        )
        for plan in bound.operations
    )


def operation(plan: OperationPlan, value_types, *, discard=False) -> tuple[str, str]:
    from .rust_dynamic_values import decode, dynamic, encode

    function = plan.function
    if plan.consumes == "always" and function.return_type.kind != "void":
        raise Unsupported(
            "status-returning unconditional consumption requires a commit-on-error adapter"
        )
    if plan.scoped_receiver:
        scoped = next(
            p.value.element for p in plan.inputs if p.name == plan.scoped_receiver
        )
        value_types.add(scoped)
        return "global", ""
    consuming = bool(plan.consumes)
    receiver_plan = next(
        (
            parameter.value
            for parameter in plan.inputs
            if parameter.name == plan.receiver
        ),
        None,
    )
    receiver = (
        (
            receiver_plan.element.native
            if receiver_plan.kind == "reference"
            else receiver_plan.native
        )
        if receiver_plan
        else None
    )
    if receiver is not None and receiver not in value_types.owners:
        raise Unsupported(f"receiver {receiver} needs a generated runtime adapter")
    module = value_types.owners[receiver] if receiver else "global"
    read = reads_receiver(plan)
    parameters = function.parameters[1:] if receiver else function.parameters
    names = {parameter.name: local_name(parameter.name) for parameter in parameters}
    # Parameters keep their C names, so a local the body binds steps aside.
    taken = {*names.values(), *(local_name(p.name) for p in function.parameters[:1])}

    def body_local(name: str) -> str:
        while name in taken:
            name += "_"
        return name

    call_local, value_local, future_local, parent_local = (
        body_local("call"),
        body_local("value"),
        body_local("future"),
        body_local("parent"),
    )
    execution = plan.execution
    if execution not in {
        "command",
        "query",
        "operation",
        "immediate",
        "snapshot",
        "lifecycle",
        "event_batch",
    }:
        raise Unsupported(f"execution {execution} requires another ownership skeleton")
    if receiver:
        target = local_name(function.parameters[0].name)
        pointer_receiver = bool(function.parameters[0].type.pointee)
        args = [f"&mut {target}" if pointer_receiver else target]
        closure = f"mut {target}" if pointer_receiver else target
    else:
        target, args, closure = None, [], "_"
    signature = ["&self"] if receiver else []
    prelude, setup, outputs = [], [], []
    uses_call = False
    owner_plans = [
        *plan.owned_outputs,
        *(
            [plan.completion.result_owner]
            if plan.completion and plan.completion.result_owner
            else []
        ),
    ]
    parent_line = None
    if any(owner.parent_parameter for owner in owner_plans):
        parent_line = f"let {parent_local} = self.inner.parent();"
        prelude.append(parent_line)
    input_plans = {parameter.name: parameter for parameter in plan.inputs}
    output_plans = {parameter.name: parameter for parameter in plan.outputs}
    lengths = {}
    for parameter in plan.inputs:
        value = parameter.value
        if (
            value.kind in {"array", "buffer"}
            and value.length
            and value.length != "nul"
            and not value.length.isdigit()
        ):
            source = names[parameter.name]
            lengths[value.length] = (
                f"{source}.map_or(0, |value| value.len())"
                if value.nullable or value.optional == "empty"
                else f"{source}.len()"
            )

    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    text_parameters = (
        {text.name, text.value.length, text.result_length} if text else set()
    )

    for parameter in parameters:
        native, local = ctype(parameter.type), names[parameter.name]
        pointee = parameter.type.pointee
        parameter_plan = input_plans.get(parameter.name)
        value_plan = parameter_plan.value if parameter_plan else None
        if parameter.name in text_parameters:
            args.append(local)
            if parameter.name == text.name:
                prelude.append(f"let mut {local} = Vec::new();")
                outputs.append(("Vec<u8>", local))
        elif parameter.name in lengths:
            # Counts precede the conversions, which shadow their slices.
            prelude.append(f"let {local} = convert::count({lengths[parameter.name]})?;")
            args.append(local)
        elif plan.completion and parameter.name == plan.completion.parameter:
            args.append(local)
        elif value_plan and value_plan.kind == "enum":
            value_types.add(value_plan)
            signature.append(f"{local}: {value_types.public(value_plan)}")
            args.append(
                f"{local}.bits()"
                if value_plan.enum_kind == "bitmask"
                else f"{local}.as_raw()"
            )
        elif value_plan and value_plan.kind == "array" and value_plan.element:
            element = value_plan.element
            value_types.add(element, "in")
            public = value_types.public(element)
            signature.append(
                f"{local}: &[&str]"
                if element.kind == "buffer" and element.encoding == "utf8"
                else f"{local}: &[{public}]"
            )
            if value_plan.length and value_plan.length.isdigit():
                prelude.append(
                    f'if {local}.len() != {value_plan.length} {{ return Err(Error::invalid_argument("{parameter.name} requires {value_plan.length} elements")); }}'
                )
            setup.append(f"let {local} = {call_local}.array({local})?;")
            uses_call = True
            args.append(local)
        elif value_plan and value_plan.kind == "handle":
            public = value_types.handle_types.get(value_plan.native)
            if not public:
                raise Unsupported(f"{value_plan.native}: owner runtime unavailable")
            signature.append(f"{local}: &{public}")
            prelude.append(f"let {local} = {local}.inner.native();")
            args.append(local)
        elif value_plan and value_plan.kind in {"scalar", "native_pointer"}:
            signature.append(f"{local}: {value_types.public(value_plan)}")
            args.append(local)
        elif native in SCALARS:
            signature.append(f"{local}: {SCALARS[native]}")
            args.append(local)
        elif (
            value_plan
            and value_plan.kind == "buffer"
            and value_plan.length != "nul"
            and value_plan.buffer_form != "view"
        ):
            public = "&str" if value_plan.encoding == "utf8" else "&[u8]"
            optional = value_plan.nullable or value_plan.optional == "empty"
            signature.append(
                f"{local}: {'Option<' + public + '>' if optional else public}"
            )
            setup.append(f"let {local} = {encode(value_types, value_plan, local)};")
            args.append(local)
        elif value_plan and value_plan.buffer_form == "view":
            encoding = value_plan.encoding
            if encoding not in {"utf8", "json", "bytes"}:
                raise Unsupported(f"parameter {parameter.name} needs buffer encoding")
            optional = value_plan.nullable or value_plan.optional == "empty"
            public = "&str" if encoding == "utf8" else "&[u8]"
            signature.append(
                f"{local}: {'Option<' + public + '>' if optional else public}"
            )
            setup.append(f"let {local} = {call_local}.input(&{local})?;")
            uses_call = True
            args.append(local)
        elif (
            pointee
            and ctype(pointee) == "char"
            and pointee.const
            and value_plan.encoding == "utf8"
        ):
            signature.append(f"{local}: &str")
            setup.append(f"let {local} = {call_local}.input({local})?;")
            uses_call = True
            args.append(local)
        elif (
            value_plan
            and value_plan.kind == "reference"
            and value_plan.element.kind == "buffer"
        ):
            element = value_plan.element
            public = "&str" if element.encoding == "utf8" else "&[u8]"
            signature.append(
                f"{local}: {'Option<' + public + '>' if value_plan.nullable else public}"
            )
            setup.append(
                f"let {local} = {call_local}.optional_reference({local})?;"
                if value_plan.nullable
                else f"let {local} = {call_local}.reference({local})?;"
            )
            uses_call = True
            args.append(local)
        elif (
            native in value_types.bound.values
            and value_types.bound.values[native].kind == "record"
        ):
            value = value_types.bound.values[native]
            value_types.add(value, "in")
            public = value_types.public(value)
            signature.append(
                f"{local}: &{public}" if dynamic(value) else f"{local}: {public}"
            )
            setup.append(f"let {local} = {call_local}.input(&{local})?;")
            uses_call = True
            args.append(local)
        elif (
            pointee
            and ctype(pointee) in value_types.bound.values
            and value_types.bound.values[ctype(pointee)].kind == "record"
            and parameter_plan is not None
            and parameter_plan.direction == "in"
            and value_plan.kind == "reference"
        ):
            value = value_types.bound.values[ctype(pointee)]
            value_types.add(value, "in")
            public = f"{'' if value.registration else '&'}{value_types.public(value)}"
            if value_plan.nullable:
                signature.append(f"{local}: Option<{public}>")
                setup.append(
                    f"let {local} = {call_local}.optional_reference({local}.as_ref())?;"
                )
            else:
                signature.append(f"{local}: {public}")
                setup.append(f"let {local} = {call_local}.reference(&{local})?;")
            uses_call = True
            args.append(local)
        elif parameter.name in output_plans:
            output = output_plans[parameter.name]
            value = output.value
            if output.sized_by:
                element = value.element
                value_types.add(element, "out")
                raw_type = SCALARS.get(ctype(pointee), "sys::" + ctype(pointee))
                initial = (
                    f"unsafe {{ sys::{element.default}() }}"
                    if element.default
                    else "unsafe { std::mem::zeroed() }"
                )
                prelude.append(
                    f"let mut {local}: Vec<{raw_type}> = (0..{lengths[value.length]}).map(|_| {initial}).collect();"
                )
                if element.kind == "record" and any(
                    field.role == "size" for field in element.fields
                ):
                    prelude.append(
                        f"for value in &mut {local} {{ value.size = std::mem::size_of::<{raw_type}>() as _; }}"
                    )
                args.append(f"{local}.as_mut_ptr()")
                copied = decode(value_types, element, "item")
                outputs.append(
                    (
                        f"Vec<{value_types.public(element)}>",
                        f"{local}.into_iter().map(|item| Ok({copied})).collect::<Result<Vec<_>>>()?",
                    )
                )
                continue
            if value.kind == "reference":
                value = value.element
            if value.kind == "handle":
                owned = next(
                    (
                        owner
                        for owner in plan.owned_outputs
                        if owner.parameter == parameter.name
                    ),
                    None,
                )
                if not owned:
                    raise Unsupported("handle output lacks ownership transfer")
                public, converted = adopt_owned(
                    owned, local, value_types, f"{parent_local}.clone()"
                )
                prelude.append(f"let mut {local} = sys::{value.native}(0);")
                args.append(f"&mut {local}")
                outputs.append((public, converted + "?"))
                continue
            value_types.add(value, "out")
            public = value_types.public(value)
            if dynamic(value) and not read:
                raise Unsupported("borrowed output record needs its native owner scope")
            raw_type = ctype(pointee)
            initial = (
                f"unsafe {{ sys::{value.default}() }}"
                if value.default
                else "Default::default()"
                if value.kind in {"scalar", "enum"}
                else "unsafe { std::mem::zeroed() }"
            )
            prelude.append(
                f"let mut {local}: {SCALARS.get(raw_type, 'sys::' + raw_type)} = {initial};"
            )
            if value.kind == "record" and any(
                field.role == "size" for field in value.fields
            ):
                prelude.append(
                    f"{local}.size = std::mem::size_of::<sys::{raw_type}>() as _;"
                )
            args.append(f"&mut {local}")
            outputs.append((public, decode(value_types, value, local)))
        else:
            raise Unsupported(
                f"parameter {parameter.name}: {parameter.type.spelling} needs ownership/shape lowering"
            )
    method = plan.member
    if method in {"native", "submit_command", "submit_query", "drop", "adopt"} or (
        method == "close" and not consuming
    ):
        raise Unsupported("method name is reserved by the handle runtime")
    method = identifier(method)
    if plan.completion:
        closure = (
            f"|{closure}, {local_name(plan.completion.parameter)}, out_diagnostic|"
        )
    elif function.return_type.kind != "void" and not plan.result:
        closure = f"|{closure}, out_diagnostic|"
    else:
        closure = f"|{closure}|"
    native = native_call(function, args, diagnostic="out_diagnostic")
    if text:
        storage, capacity, length = (
            names[text.name],
            names[text.value.length],
            names[text.result_length],
        )
        native = f"convert::caller_sized_text(&mut {storage}, |{storage}, {capacity}, {length}| {native})"
    native = f"unsafe {{ {native} }}"
    public = (
        outputs[0][0]
        if len(outputs) == 1
        else "(" + ", ".join(item[0] for item in outputs) + ")"
    )
    output = (
        outputs[0][1]
        if len(outputs) == 1
        else "(" + ", ".join(item[1] for item in outputs) + ")"
    )
    if not plan.completion:
        if function.return_type.kind == "void":
            ending = [f"{call_local}.run({closure} {native});", f"Ok({output})"]
        elif plan.result:
            value_types.add(plan.result, "out")
            public = value_types.public(plan.result)
            output = (
                f"convert::native_default({value_local})"
                if plan.result.kind == "record" and plan.result.default == function.name
                else decode(value_types, plan.result, value_local)
            )
            ending = [
                f"let {value_local} = {call_local}.run({closure} {native});",
                f"Ok({output})",
            ]
        else:
            if not function.diagnostic:
                raise Unsupported(f"{function.name}: status return has no diagnostic")
            declined = next(
                (r.accepted_unless for r in plan.registrations if r.accepted_unless),
                None,
            )
            ending = [
                f"{call_local}.status_registered({closure} {{ let status = {native}; (status, !{names[declined]}) }})?;"
                if declined
                else f"{call_local}.status({closure} {native})?;",
                f"Ok({output})",
            ]
            if plan.absence:
                if parent_line:
                    prelude.remove(parent_line)
                ending = [
                    f"if !{call_local}.status_unless(sys::{plan.absence.status}, {closure} {native})? {{",
                    "    return Ok(None);",
                    "}",
                    *([parent_line] if parent_line else []),
                    f"Ok(Some({output}))",
                ]
                public = f"Option<{public}>"
    else:
        if discard:
            completed = "()"
            expression = f"{call_local}.discard({closure} {native})"
        elif execution == "command":
            if plan.result is not None:
                raise Unsupported("command receipt cannot discard a value payload")
            completed = "NativeFuture<CommandCompletion>"
            expression = f"{call_local}.command({closure} {native})"
        else:
            result, converter = result_rule(plan, value_types, parent_local)
            completed = f"NativeFuture<{result}>"
            expression = f"{call_local}.complete({closure} {native}, {converter})"
        if outputs:
            public = "(" + ", ".join([*(item[0] for item in outputs), completed]) + ")"
            converted = ", ".join([*(item[1] for item in outputs), future_local])
            ending = [f"let {future_local} = {expression}?;", f"Ok(({converted}))"]
        else:
            public = completed
            ending = [expression]
    text = "\n".join(ending)
    if text.count(f"{parent_local}.clone()") == 1:
        ending = [
            line.replace(f"{parent_local}.clone()", parent_local) for line in ending
        ]
    generic = ""
    if plan.view:
        handle = plan.view.owner
        prelude.insert(
            0,
            f"unsafe {{ {call_local}.view(sys::{handle.view_begin}, sys::{handle.view_end}) }}?;",
        )
        uses_call = True
        ending[-1] = f"let {value_local} = {output};"
        ending.append(f"Ok(callback(&{value_local}))")
        signature.append(f"callback: impl FnOnce(&{public}) -> R")
        generic, public = "<R>", "R"
    mutable = "mut " if uses_call or not plan.completion else ""
    if consuming:
        if not receiver:
            raise Unsupported("consumption requires an owned receiver")
        if outputs:
            raise Unsupported("consumption with outputs requires an owner transaction")
        if ending[0].startswith(f"{call_local}.status("):
            ending = [ending[0].removesuffix("?;")]
        body = [
            *prelude,
            f"self.inner.close(|{target}| {{",
            f"    let {mutable}{call_local} = Call::new({target}, None);",
            *("    " + line for line in [*setup, *ending]),
            "})",
        ]
    else:
        receiver_self = "self.0" if discard else "self"
        body = [
            f"let {mutable}{call_local} = {receiver_self}.inner.{'read()?' if read else 'call()'};"
            if receiver
            else f"let {mutable}{call_local} = Call::global();"
        ]
        if plan.owned_outputs and receiver is None:
            body.append("maplibre_core::validate_abi_version()?;")
        body += [*prelude, *setup, *ending]
    returns = f" -> Result<{public}>"
    # A call with no status and an infallible conversion returns its value.
    if (
        not consuming
        and body[-1].startswith("Ok(")
        and not any("?" in line for line in body)
    ):
        body[-1] = body[-1].removeprefix("Ok(").removesuffix(")")
        returns = f" -> {public}"
        if public == "()":
            body.pop()
            returns = ""

    def has_pointer(value):
        return (
            False
            if value.registration
            else value.kind == "native_pointer"
            or bool(value.element and has_pointer(value.element))
            or any(has_pointer(field.value) for field in value.fields if field.public)
        )

    unsafe_input = any(
        has_pointer(parameter.value)
        for parameter in plan.inputs
        if parameter.name != plan.receiver and not parameter.value.registration
    )
    safety_doc = (
        "    ///\n    /// # Safety\n    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.\n"
        if unsafe_input
        else ""
    )
    code = (
        doc(value_types.bound, function.name, "    ")
        + safety_doc
        + f"    pub {'unsafe ' if unsafe_input else ''}fn {method}{generic}({', '.join(signature)}){returns} {{\n"
        + "\n".join("        " + line for line in body)
        + "\n    }\n"
    )
    return module, code


def lower(api: Api | BoundApi) -> tuple[dict[str, str], list[str], dict[str, str]]:
    chunks, generated, unsupported = defaultdict(list), [], {}
    bound = compile_api(api)
    from . import rust_owners
    from .rust_values import Values

    value_types = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            value_types.add(value)
    owners = rust_owners.owned_handles(bound)
    value_types.owners = {native: rust_owners.module_name(native) for native in owners}
    value_types.handle_types = {
        native: rust_owners.owner_name(native) for native in owners
    }
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        function = plan.function
        previous_values = dict(value_types.used)
        previous_directions = {
            name: set(known) for name, known in value_types.directions.items()
        }
        try:
            module, code = operation(plan, value_types)
            if code:
                chunks[module].append(code)
            generated.append(function.name)
        except Unsupported as error:
            value_types.used = previous_values
            value_types.directions = previous_directions
            unsupported[function.name] = f"{function.location}: {error}"
    marker = "// Generated from C headers by tools/bindgen. Do not edit.\n"
    root = "crates/maplibre-native-ffi/src/generated"
    files = {}
    for native, handle in owners.items():
        module = rust_owners.module_name(native)
        body = chunks.pop(module, [])
        files[f"{root}/{module}.rs"] = (
            marker
            + "use super::*;\n\n"
            + rust_owners.declaration(bound, handle)
            + (
                f"\nimpl {rust_owners.owner_name(native)} {{\n"
                + "\n".join(body)
                + "}\n"
                if body
                else ""
            )
        )
        commands = [
            p
            for p in bound.operations
            if p.name in generated
            and p.completion
            and p.completion.discardable
            and any(i.name == p.receiver and i.value.native == native for i in p.inputs)
        ]
        if commands:
            owner = rust_owners.owner_name(native)
            view = f"{owner}Discarding"
            files[f"{root}/{module}.rs"] += (
                f"\n/// Submits [`{owner}`] commands without a completion. Each call returns\n"
                "/// the same acceptance status as its completing form, and native logs a\n"
                "/// command that later fails as a warning through the log handler.\n"
                f"pub struct {view}<'a>(&'a {owner});\n"
                f"impl {owner} {{ pub fn discarding(&self) -> {view}<'_> {{ {view}(self) }} }}\n"
                f"impl {view}<'_> {{\n"
                + "\n".join(
                    operation(p, value_types, discard=True)[1] for p in commands
                )
                + "}\n"
            )
    modules = [rust_owners.module_name(native) for native in owners]
    if global_body := chunks.pop("global", []):
        files[f"{root}/global.rs"] = (
            marker + "use super::*;\n\n" + "\n".join(global_body)
        )
        modules.append("global")
    assert not chunks, f"operations without an owner module: {sorted(chunks)}"
    declarations = [value_types.render()]
    files[f"{root}/values.rs"] = marker + "use super::*;\n\n" + "\n".join(declarations)
    files[f"{root}/mod.rs"] = marker + rust_owners.module_index(
        owners, ["values", *modules]
    )
    from . import rust_sys

    files["crates/maplibre-native-ffi-core/src/status.rs"] = status_source(bound)
    sys_declarations = rust_sys.declarations(bound)
    files[rust_sys.PATH] = sys_declarations.render()
    for name, reason in sys_declarations.unsupported.items():
        if name in generated:
            generated.remove(name)
        unsupported.setdefault(name, reason)
    return files, generated, unsupported


def generate(api: Api | BoundApi) -> dict[str, str]:
    return lower(api)[0]


def coverage(api: Api | BoundApi) -> dict:
    bound = compile_api(api)
    _, generated, unsupported = lower(bound)
    return {
        "generated": generated,
        "unsupported": unsupported,
    }


def status_source(bound):
    values = bound.values["mln_status"].enum_values
    variants = [
        (
            pascal(key.removeprefix("MLN_STATUS_").lower()),
            number,
            key.removeprefix("MLN_STATUS_"),
        )
        for key, number in values
    ]
    return (
        "// Generated from C headers by tools/bindgen. Do not edit.\n"
        + doc(bound, "mln_status")
        + "#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]\npub enum Status {\n"
        + "".join(
            f"{doc(bound, 'MLN_STATUS_' + label, '    ')}    {name},\n"
            for name, _, label in variants
        )
        + "    Unknown(i32),\n}\nimpl Status {\n"
        + "    pub fn from_raw(raw: i32) -> Self { match raw {\n"
        + "".join(
            f"        {number} => Self::{name},\n" for name, number, _ in variants
        )
        + "        value => Self::Unknown(value),\n    } }\n"
        + "    pub fn as_raw(self) -> i32 { match self {\n"
        + "".join(
            f"        Self::{name} => {number},\n" for name, number, _ in variants
        )
        + "        Self::Unknown(value) => value,\n    } }\n}\n"
        + "impl Default for Status { fn default() -> Self { Self::from_raw(0) } }\n"
        + "impl std::fmt::Display for Status { fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result { match self {\n"
        + "".join(
            f'        Self::{name} => f.write_str("{label}"),\n'
            for name, _, label in variants
        )
        + '        Self::Unknown(value) => write!(f, "UNKNOWN({value})"),\n    } } }\n'
    )
