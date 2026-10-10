"""Generate Rust operations over the completion and handle runtimes."""

from __future__ import annotations

from collections import defaultdict
from dataclasses import replace

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api
from tools.bindgen.semantic import BoundApi, OperationPlan, view_support

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


# Locals that a generated operation defines besides its parameters.
LOCALS = {"call", "parent", "callback", "value", "future", "sys", "convert"}


def local_name(name: str) -> str:
    """A parameter's Rust name, kept apart from the operation's own locals."""
    name = identifier(name)
    return name + "_" if name in LOCALS else name


def adopt_owned(owned, raw, value_types, parent="parent"):
    public = value_types.handle_types.get(owned.handle.native)
    if not public:
        raise Unsupported(f"{owned.handle.native}: owner runtime unavailable")
    source = parent if owned.parent_parameter else "None"
    return public, f"{public}::adopt({raw}, {source})"


def result_rule(plan: OperationPlan, value_types) -> tuple[str, str]:
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
    # A value that its Rust type alone cannot copy converts element by element.
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
    """A counted read keeps a borrowed view or copied storage live for the call."""
    from .rust_dynamic_values import dynamic

    def borrowed(value):
        target = value.element if value.kind == "reference" else value
        return "owner" in {value.lifetime, target.lifetime} or dynamic(target)

    return bool(
        plan.receiver
        and not plan.consumes
        and (plan.view or any(borrowed(output.value) for output in plan.outputs))
    )


def operation(plan: OperationPlan, value_types) -> tuple[str, str]:
    from .rust_dynamic_values import decode, dynamic, encode

    function = plan.function
    if plan.consumes == "always" and function.return_type.kind != "void":
        raise Unsupported(
            "status-returning unconditional consumption requires a commit-on-error adapter"
        )
    if plan.receiver:
        receiver_value = next(
            parameter.value
            for parameter in plan.inputs
            if parameter.name == plan.receiver
        )
        if receiver_value.native in value_types.bound.decisions:
            value_types.add(receiver_value)
            return "global", ""
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
    execution = plan.execution
    if execution not in {
        "command",
        "query",
        "operation",
        "immediate",
        "snapshot",
        "lifecycle",
        "render_driver",
        "event_batch",
    }:
        raise Unsupported(f"execution {execution} requires another ownership skeleton")
    # The closure that makes the C call names the receiver after its C
    # parameter, and passes a pointer receiver by address.
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
    parents = {
        owner.parent_parameter for owner in owner_plans if owner.parent_parameter
    }
    if len(parents) > 1:
        raise Unsupported("operation transfers owners from distinct parents")
    parent_line = None
    if parents:
        parent = next(iter(parents))
        source = "self" if parent == plan.receiver else names[parent]
        parent_line = f"let parent = {source}.inner.parent();"
        prelude.append(parent_line)
    direct_arguments = {}
    for registration in plan.direct_registrations:
        from .rust_direct import add

        public = add(value_types, plan, registration)
        callback = names[registration.callback]
        signature.append(f"{callback}: Option<{public}>")
        callback_type = next(
            parameter.value.native
            for parameter in plan.inputs
            if parameter.name == registration.callback
        )
        parts = [
            registration.callback,
            registration.user_data,
            registration.release_callback,
        ]
        setup.append(
            f"let ({', '.join(names[part] for part in parts)}) = {callback_type.removeprefix('mln_')}_registration({callback}, call.arena());"
        )
        direct_arguments.update((part, names[part]) for part in parts)
        uses_call = True
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

    for parameter in parameters:
        native, local = ctype(parameter.type), names[parameter.name]
        pointee = parameter.type.pointee
        parameter_plan = input_plans.get(parameter.name)
        value_plan = parameter_plan.value if parameter_plan else None
        if parameter.name in direct_arguments:
            args.append(direct_arguments[parameter.name])
        elif parameter.name in lengths:
            # Counts precede the conversions, which shadow their slices.
            prelude.append(f"let {local} = convert::count({lengths[parameter.name]})?;")
            args.append(local)
        elif plan.completion and parameter.name == plan.completion.parameter:
            args.append(local)
        elif value_plan and value_plan.kind == "enum":
            value_types.add(value_plan)
            signature.append(f"{local}: {value_types.public(value_plan)}")
            args.append(f"{local}.to_native()")
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
            setup.append(f"let {local} = call.array({local})?;")
            uses_call = True
            args.append(local)
        elif value_plan and value_plan.kind == "handle":
            public = value_types.handle_types.get(value_plan.native)
            if not public:
                raise Unsupported(f"{value_plan.native}: owner runtime unavailable")
            signature.append(f"{local}: &{public}")
            prelude.append(f"let {local} = {local}.inner.native()?;")
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
            setup.append(f"let {local} = call.input(&{local})?;")
            uses_call = True
            args.append(local)
        elif (
            pointee
            and ctype(pointee) == "char"
            and pointee.const
            and value_plan.encoding == "utf8"
        ):
            signature.append(f"{local}: &str")
            setup.append(f"let {local} = call.input({local})?;")
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
                f"let {local} = call.optional_reference({local})?;"
                if value_plan.nullable
                else f"let {local} = call.reference({local})?;"
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
            setup.append(f"let {local} = call.input(&{local})?;")
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
                    f"let {local} = call.optional_reference({local}.as_ref())?;"
                )
            else:
                signature.append(f"{local}: {public}")
                setup.append(f"let {local} = call.reference(&{local})?;")
            uses_call = True
            args.append(local)
        elif parameter.name in output_plans:
            value = output_plans[parameter.name].value
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
                    owned, local, value_types, "parent.clone()"
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
    native = f"unsafe {{ {native_call(function, args, diagnostic='out_diagnostic')} }}"
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
            ending = [f"call.run({closure} {native});", f"Ok({output})"]
        elif plan.result:
            value_types.add(plan.result, "out")
            public = value_types.public(plan.result)
            output = decode(value_types, plan.result, "value")
            ending = [f"let value = call.run({closure} {native});", f"Ok({output})"]
        else:
            if not function.diagnostic:
                raise Unsupported(f"{function.name}: status return has no diagnostic")
            ending = [f"call.status({closure} {native})?;", f"Ok({output})"]
            if plan.absence:
                # The parent joins only an output that native published.
                if parent_line:
                    prelude.remove(parent_line)
                ending = [
                    f"if !call.status_unless(sys::{plan.absence.status}, {closure} {native})? {{",
                    "    return Ok(None);",
                    "}",
                    *([parent_line] if parent_line else []),
                    f"Ok(Some({output}))",
                ]
                public = f"Option<{public}>"
    else:
        if execution == "command":
            if plan.result is not None:
                raise Unsupported("command receipt cannot discard a value payload")
            completed = "NativeFuture<CommandCompletion>"
            expression = f"call.command({closure} {native})"
        else:
            result, converter = result_rule(plan, value_types)
            completed = f"NativeFuture<{result}>"
            expression = f"call.complete({closure} {native}, {converter})"
        if outputs:
            public = "(" + ", ".join([*(item[0] for item in outputs), completed]) + ")"
            converted = ", ".join([*(item[1] for item in outputs), "future"])
            ending = [f"let future = {expression}?;", f"Ok(({converted}))"]
        else:
            public = completed
            ending = [expression]
    # The one owner that takes the parent moves it rather than cloning it.
    text = "\n".join(ending)
    if text.count("parent.clone()") == 1:
        ending = [line.replace("parent.clone()", "parent") for line in ending]
    generic = ""
    if plan.view:
        handle = plan.view.owner
        prelude.insert(
            0,
            f"unsafe {{ call.view(sys::{handle.view_begin}, sys::{handle.view_end}) }}?;",
        )
        uses_call = True
        ending[-1] = f"let value = {output};"
        ending.append("Ok(callback(&value))")
        signature.append(f"callback: impl FnOnce(&{public}) -> R")
        generic, public = "<R>", "R"
    # Status and run calls borrow the call, and submissions consume it.
    mutable = "mut " if uses_call or not plan.completion else ""
    if consuming:
        if not receiver:
            raise Unsupported("consumption requires an owned receiver")
        if outputs:
            raise Unsupported("consumption with outputs requires an owner transaction")
        if ending[0].startswith("call.status("):
            ending = [ending[0].removesuffix("?;")]
        consume = "release" if plan.completion else "close"
        body = [
            *prelude,
            f"self.inner.{consume}(|{target}| {{",
            f"    let {mutable}call = Call::new({target}, None);",
            *("    " + line for line in [*setup, *ending]),
            "})",
        ]
    else:
        operation_name = f'"{function.name}"'
        body = [
            f"let {mutable}call = self.inner.{'read' if read else 'call'}({operation_name})?;"
            if receiver
            else f"let {mutable}call = Call::global({operation_name})?;"
        ]
        if plan.owned_outputs and receiver is None:
            body.append("maplibre_core::validate_abi_version()?;")
        body += [*prelude, *setup, *ending]

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
        if parameter.name != plan.receiver
        and parameter.name not in direct_arguments
        and not parameter.value.registration
    )
    safety_doc = (
        "    ///\n    /// # Safety\n    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.\n"
        if unsafe_input
        else ""
    )
    code = (
        doc(value_types.bound, function.name, "    ")
        + safety_doc
        + f"    pub {'unsafe ' if unsafe_input else ''}fn {method}{generic}({', '.join(signature)}) -> Result<{public}> {{\n"
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
    value_types.direct_callbacks = {}
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
    modules = [rust_owners.module_name(native) for native in owners]
    if global_body := chunks.pop("global", []):
        files[f"{root}/global.rs"] = (
            marker + "use super::*;\n\n" + "\n".join(global_body)
        )
        modules.append("global")
    assert not chunks, f"operations without an owner module: {sorted(chunks)}"
    from .rust_callbacks import decision_table

    decision_tables = [
        decision_table(bound, decision, pascal(native))
        for native, decision in sorted(bound.decisions.items())
    ]
    from .rust_direct import declaration as direct_declaration

    declarations = [
        value_types.render(),
        *decision_tables,
        *(
            direct_declaration(value_types, *registration)
            for registration in value_types.direct_callbacks.values()
        ),
    ]
    files[f"{root}/values.rs"] = marker + "use super::*;\n\n" + "\n".join(declarations)
    files[f"{root}/mod.rs"] = marker + rust_owners.module_index(
        owners, ["values", *modules]
    )
    from . import rust_sys

    sys_declarations = rust_sys.declarations(bound)
    files[rust_sys.PATH] = sys_declarations.render()
    # An operation calls its C function through the -sys declaration.
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
        "support": view_support(bound, generated),
        "unsupported": unsupported,
    }
