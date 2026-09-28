"""Generate Rust operations using the stable completion and handle runtimes."""

from __future__ import annotations

from collections import defaultdict

from tools.bindgen.compiler import compile_api
from tools.bindgen.semantic import BoundApi, OperationPlan

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
OWNERS = {
    "mln_geojson_source_data": ("geojson", "GeoJsonSourceDataHandle"),
    "mln_acquired_frame": ("frame", "AcquiredFrameHandle"),
    "mln_map": ("map", "MapHandle"),
    "mln_map_projection": ("projection", "MapProjectionHandle"),
    "mln_runtime": ("runtime", "RuntimeHandle"),
    "mln_render_session": ("render", "RenderSessionHandle"),
}


HANDLE_TYPES = {native: "crate::" + owner for native, (_, owner) in OWNERS.items()}
HANDLE_TYPES["mln_geojson_source_data"] = "crate::GeoJsonSourceDataHandle"


def adopt_owned(owned, raw, value_types):
    public = value_types.handle_types.get(owned.handle.native)
    if not public:
        raise Unsupported(f"{owned.handle.native}: owner runtime unavailable")
    parent = ", binding_parent" if owned.parent_parameter else ""
    return public, f"{public}::from_native({raw}{parent})"


class Unsupported(ValueError):
    pass


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


def result_rule(
    api: Api, plan: OperationPlan, value_types
) -> tuple[str, str, set[str]]:
    from dataclasses import replace

    from .rust_dynamic_values import decode

    result = plan.result
    if result is None:
        return "()", "crate::completion::unit", set()
    if plan.completion and plan.completion.result_owner:
        public, adopted = adopt_owned(
            plan.completion.result_owner, "value", value_types
        )
        return (
            public,
            f"move |result| {{ let value = crate::completion::copy_value::<sys::{result.native}>(result)?; {adopted} }}",
            set(),
        )
    if result.ownership != "borrowed":
        raise Unsupported("completion result requires resolved ownership transfer")
    array = result.kind == "array"
    value = result.element if array else replace(result, nullable=False)
    value_types.add(value)
    public = value_types.public(value).replace(
        "crate::generated::", "maplibre_core::generated::"
    )
    raw = SCALARS.get(value.native, "sys::" + value.native)
    copied = decode(value_types, value, "value").replace("crate::", "maplibre_core::")
    convert = f"Ok({copied})"
    if array:
        public = f"Vec<{public}>"
        converter = f"|result| {{ crate::completion::copy_slice::<{raw}>(result)?.into_iter().map(|value| -> Result<_> {{ {convert} }}).collect::<Result<Vec<_>>>() }}"
        if result.nullable:
            return (
                f"Option<{public}>",
                f"|result| {{ if result.value.is_null() {{ return Ok(None); }} ({converter})(result).map(Some) }}",
                set(),
            )
        return public, converter, set()
    if result.nullable:
        return (
            f"Option<{public}>",
            f"|result| {{ crate::completion::optional_value::<{raw}>(result)?.map(|value| -> Result<_> {{ {convert} }}).transpose() }}",
            set(),
        )
    return (
        public,
        f"|result| {{ let value = crate::completion::copy_value::<{raw}>(result)?; {convert} }}",
        set(),
    )


def owner_access(value_types, native, expression):
    module, _ = value_types.owners[native]
    return (
        f"{expression}.native()?"
        if module in {"frame", "geojson"} or module.startswith("owned_")
        else f"{expression}.inner.native()?"
    )


def operation(api: Api, plan: OperationPlan, value_types) -> tuple[str, str, set[str]]:
    from .rust_dynamic_values import dynamic

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
        if any(
            callback.decision
            and callback.decision.handle.native == receiver_value.native
            for callback in value_types.bound.callbacks.values()
        ):
            value_types.add(receiver_value)
            return "global", "", set()
    if plan.scoped_receiver:
        scoped = next(
            p.value.element for p in plan.inputs if p.name == plan.scoped_receiver
        )
        value_types.add(scoped)
        return "global", "", set()
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
    module, _owner = value_types.owners[receiver] if receiver else ("global", None)
    native_receiver = owner_access(value_types, receiver, "self") if receiver else ""
    parameters = function.parameters[1:] if receiver else function.parameters
    locals_by_name = {
        parameter.name: f"binding_arg_{index}"
        for index, parameter in enumerate(function.parameters)
    }
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
    values = set()
    args, signature, setup, outputs = (
        (
            ["&mut native" if function.parameters[0].type.pointee else "native"]
            if receiver
            else []
        ),
        (["&self"] if receiver else []),
        [],
        [],
    )
    arena = False
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
    if parents:
        parent = next(iter(parents))
        source = "self" if parent == plan.receiver else locals_by_name[parent]
        setup.append(f"let binding_parent = std::sync::Arc::clone(&{source}.inner);")
    direct_arguments = {}
    for index, registration in enumerate(plan.direct_registrations):
        from .rust_direct import add

        public = add(value_types, plan, registration)
        local = locals_by_name[registration.callback]
        signature.append(f"{local}: Option<maplibre_core::generated::{public}>")
        callback_type = next(
            parameter.value.native
            for parameter in plan.inputs
            if parameter.name == registration.callback
        )
        setup.append(
            f"let binding_registration_{index} = maplibre_core::generated::{callback_type.removeprefix('mln_')}_registration({local}, &mut arena);"
        )
        for part, parameter in enumerate(
            (
                registration.callback,
                registration.user_data,
                registration.release_callback,
            )
        ):
            direct_arguments[parameter] = f"binding_registration_{index}.{part}"
        arena = True
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
            local = locals_by_name[parameter.name]
            lengths[value.length] = (
                f"{local}.as_ref().map_or(0, |value| value.len())"
                if value.nullable or value.optional == "empty"
                else f"{local}.len()"
            )

    for parameter_index, parameter in enumerate(parameters):
        native, local = ctype(parameter.type), locals_by_name[parameter.name]
        pointee = parameter.type.pointee
        parameter_plan = input_plans.get(parameter.name)
        value_plan = parameter_plan.value if parameter_plan else None
        if parameter.name in direct_arguments:
            args.append(direct_arguments[parameter.name])
        elif parameter.name in lengths:
            setup.append(
                f'let {local} = {lengths[parameter.name]}.try_into().map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;'
            )
            args.append(local)
        elif pointee and ctype(pointee) == "mln_completion":
            args.append("completion")
        elif value_plan and value_plan.kind == "enum":
            value_types.add(value_plan)
            public = value_types.public(value_plan).replace(
                "crate::generated::", "maplibre_core::generated::"
            )
            signature.append(f"{local}: {public}")
            args.append(f"{local}.to_native()")
        elif value_plan and value_plan.kind == "array" and value_plan.element:
            element = value_plan.element
            value_types.add(element)
            public = value_types.public(element).replace(
                "crate::generated::", "maplibre_core::generated::"
            )
            signature.append(
                f"{local}: &[&str]"
                if element.kind == "buffer" and element.encoding == "utf8"
                else f"{local}: &[{public}]"
            )
            if value_plan.length and value_plan.length.isdigit():
                setup.append(
                    f'if {local}.len() != {value_plan.length} {{ return Err(crate::Error::new(crate::ErrorKind::InvalidArgument, None, "{parameter.name} requires {value_plan.length} elements")); }}'
                )
            from .rust_dynamic_values import encode

            conversion = encode(value_types, element, "value").replace(
                "crate::", "maplibre_core::"
            )
            arena |= dynamic(element)
            setup.append(
                f"let {local}: Vec<_> = {local}.iter().map(|value| -> Result<_> {{ Ok({conversion}) }}).collect::<Result<_>>()?;"
            )
            args.append(f"{local}.as_ptr()")
        elif value_plan and value_plan.kind == "handle":
            public = value_types.handle_types.get(value_plan.native)
            if not public:
                raise Unsupported(f"{value_plan.native}: owner runtime unavailable")
            signature.append(f"{local}: &{public}")
            access = owner_access(value_types, value_plan.native, local)
            setup.append(f"let {local}_native = {access};")
            args.append(f"{local}_native")
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
            and native != "mln_buffer_view"
        ):
            from .rust_dynamic_values import encode

            public = "&str" if value_plan.encoding == "utf8" else "&[u8]"
            signature.append(
                f"{local}: {'Option<' + public + '>' if value_plan.nullable or value_plan.optional == 'empty' else public}"
            )
            setup.append(
                f"let {local}_native = {encode(value_types, value_plan, local).replace('crate::', 'maplibre_core::')};"
            )
            args.append(f"{local}_native")
        elif native == "mln_buffer_view":
            from .rust_dynamic_values import encode

            encoding = value_plan.encoding
            if (
                encoding not in {"utf8", "json", "bytes"}
                or value_plan.lifetime != "call"
            ):
                raise Unsupported(
                    f"parameter {parameter.name} needs buffer encoding and call lifetime"
                )
            optional = value_plan.nullable or value_plan.optional == "empty"
            public = "&str" if encoding == "utf8" else "&[u8]"
            signature.append(
                f"{local}: {'Option<' + public + '>' if optional else public}"
            )
            setup.append(
                f"let {local} = {encode(value_types, value_plan, local).replace('crate::', 'maplibre_core::')};"
            )
            args.append(local)
        elif (
            pointee
            and ctype(pointee) == "char"
            and pointee.const
            and value_plan.encoding == "utf8"
            and value_plan.lifetime == "call"
        ):
            signature.append(f"{local}: &str")
            setup.append(f"let {local} = maplibre_core::string::c_string({local})?;")
            args.append(f"{local}.as_ptr()")
        elif (
            value_plan
            and value_plan.kind == "reference"
            and value_plan.element.kind == "buffer"
        ):
            from .rust_dynamic_values import encode

            element = value_plan.element
            public = "&str" if element.encoding == "utf8" else "&[u8]"
            signature.append(
                f"{local}: {'Option<' + public + '>' if value_plan.nullable else public}"
            )
            arena |= element.length == "nul"
            converted = encode(value_types, element, "value").replace(
                "crate::", "maplibre_core::"
            )
            if value_plan.nullable:
                setup.append(
                    f"let {local} = {local}.map(|value| -> Result<_> {{ Ok({converted}) }}).transpose()?;"
                )
                args.append(f"{local}.as_ref().map_or(std::ptr::null(), |value| value)")
            else:
                setup.append(f"let value = {local}; let {local} = {converted};")
                args.append(f"&{local}")
        elif (
            native in value_types.bound.values
            and value_types.bound.values[native].kind == "record"
        ):
            value_types.add(value_types.bound.values[native])
            signature.append(
                f"{local}: &maplibre_core::generated::{pascal(native)}"
                if dynamic(value_types.bound.values[native])
                else f"{local}: maplibre_core::generated::{pascal(native)}"
            )
            arena |= dynamic(value_types.bound.values[native])
            args.append(
                f"{local}.to_native(&mut arena)?"
                if dynamic(value_types.bound.values[native])
                else f"{local}.to_native()"
            )
        elif (
            pointee
            and ctype(pointee) in value_types.bound.values
            and value_types.bound.values[ctype(pointee)].kind == "record"
            and parameter_plan is not None
            and parameter_plan.direction == "in"
            and value_plan.kind == "reference"
        ):
            value = value_types.bound.values[ctype(pointee)]
            value_types.add(value)
            public = f"{'' if value.registration else '&'}maplibre_core::generated::{pascal(value.native)}"
            arena |= dynamic(value)
            conversion = (
                "value.to_native(&mut arena)" if dynamic(value) else "value.to_native()"
            )
            if value_plan.nullable:
                signature.append(f"{local}: Option<{public}>")
                setup.append(
                    f"let {local} = {local}.map(|value| {conversion}){'.transpose()?' if dynamic(value) else ''};"
                )
                args.append(f"{local}.as_ref().map_or(std::ptr::null(), |value| value)")
            else:
                signature.append(f"{local}: {public}")
                setup.append(
                    f"let {local} = {local}.to_native({'&mut arena' if dynamic(value) else ''}){'?' if dynamic(value) else ''};"
                )
                args.append(f"&{local}")
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
                public, converted = adopt_owned(owned, local, value_types)
                setup.append(f"let mut {local} = sys::{value.native}(0);")
                args.append(f"&mut {local}")
                outputs.append((public, converted + "?"))
                continue
            value_types.add(value)
            public = value_types.public(value).replace(
                "crate::generated::", "maplibre_core::generated::"
            )
            if dynamic(value):
                if not module.startswith("owned_"):
                    raise Unsupported(
                        "borrowed output record needs its native owner scope"
                    )
                initial = (
                    f"unsafe {{ sys::{value.default}() }}"
                    if value.default
                    else "unsafe { std::mem::zeroed() }"
                )
                if value.kind == "record" and any(
                    field.role == "size" for field in value.fields
                ):
                    initial = f"{{ let mut value: sys::{value.native} = {initial}; value.size = std::mem::size_of::<sys::{value.native}>() as _; value }}"
            elif value.kind == "record":
                initial = f"{public}::default().to_native()"
            else:
                initial = "Default::default()"
            raw_type = ctype(pointee)
            setup.append(
                f"let mut {local}: {SCALARS.get(raw_type, 'sys::' + raw_type)} = {initial};"
            )
            args.append(f"&mut {local}")
            from .rust_dynamic_values import decode

            converted = decode(value_types, value, local).replace(
                "crate::", "maplibre_core::"
            )
            outputs.append((public, converted))
        else:
            raise Unsupported(
                f"parameter {parameter.name}: {parameter.type.spelling} needs ownership/shape lowering"
            )
    if arena:
        setup.insert(0, "let mut arena = maplibre_core::input::InputArena::default();")
    method = function.name.removeprefix(
        receiver + "_" if receiver else "mln_"
    ).removeprefix("mln_")
    if method in {"native", "submit_command", "submit_query", "drop"} or (
        method == "close" and not consuming
    ):
        raise Unsupported("method name is reserved by the handle runtime")
    method = identifier(method)
    call = f"sys::{function.name}({', '.join(args)})"
    if not plan.completion:
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
        body = [
            *([f"let native = {native_receiver};"] if receiver else []),
            *setup,
            f"unsafe {{ {call} }};"
            if function.return_type.kind == "void"
            else f"maplibre_core::check(unsafe {{ {call} }})?;",
            *(
                ["arena.accept_registrations();"]
                if plan.registrations or plan.direct_registrations
                else []
            ),
            f"Ok({output})",
        ]
        if plan.result and not plan.completion:
            from .rust_dynamic_values import decode

            value_types.add(plan.result)
            public = value_types.public(plan.result).replace(
                "crate::generated::", "maplibre_core::generated::"
            )
            copied = decode(value_types, plan.result, "value").replace(
                "crate::", "maplibre_core::"
            )
            body = [*setup, f"let value = unsafe {{ {call} }};", f"Ok({copied})"]
    else:
        if module == "projection":
            raise Unsupported(
                "completion receiver/output combination needs another runtime adapter"
            )
        if execution == "command":
            if plan.result is not None:
                raise Unsupported("command receipt cannot discard a value payload")
            public = "NativeFuture<crate::CommandCompletion>"
            expression = (
                f"crate::completion::submit_command(|completion| unsafe {{ {call} }})"
            )
        else:
            result, converter, result_values = result_rule(api, plan, value_types)
            values.update(result_values)
            public = f"NativeFuture<{result}>"
            expression = f"crate::completion::submit(|completion| unsafe {{ {call} }}, {converter})"
        native_setup = [f"let native = {native_receiver};"] if receiver else []
        if outputs:
            public = "(" + ", ".join([*(item[0] for item in outputs), public]) + ")"
            converted = ", ".join([*(item[1] for item in outputs), "submitted"])
            body = [
                *native_setup,
                *setup,
                f"let submitted = {expression}?;",
                *(
                    ["arena.accept_registrations();"]
                    if plan.registrations or plan.direct_registrations
                    else []
                ),
                f"Ok(({converted}))",
            ]
        elif plan.registrations or plan.direct_registrations:
            body = [
                *native_setup,
                *setup,
                f"let submitted = {expression};",
                "if submitted.is_ok() { arena.accept_registrations(); }",
                "submitted",
            ]
        else:
            body = [*native_setup, *setup, expression]
    if module.startswith("owned_") and not consuming:
        body[0:1] = [
            "let binding_read = self.handle.read_handle()?;",
            "let native = binding_read.native;",
        ]
    if consuming:
        if not receiver:
            raise Unsupported("consumption requires an owned receiver")
        handle = (
            "self.handle"
            if module == "frame" or module.startswith("owned_") or module == "geojson"
            else "self.inner.handle"
        )
        # Input conversion can fail before the close reservation is acquired.
        call_body = [
            line
            for line in body
            if line not in setup and not line.startswith("let native = ")
        ]
        fallback = (
            "crate::completion::ready(())" if plan.completion else "Default::default()"
        )
        body = [
            *setup,
            f"let result = {handle}.close_with(|{'mut ' if function.parameters[0].type.pointee else ''}native| {{",
            *["    " + line for line in call_body],
            "})?;",
            f"Ok(result.unwrap_or_else(|| {fallback}))",
        ]
    admission = (
        f'maplibre_core::callback::check("{function.name}", '
        + ("native.0" if receiver else "0")
        + ")?;"
    )
    if not consuming:
        body.insert(
            2 if module.startswith("owned_") else 1 if receiver else 0, admission
        )
    if plan.owned_outputs and receiver is None:
        body.insert(1, "maplibre_core::validate_abi_version()?;")
    generic = ""
    if plan.view:
        handle = plan.view.owner
        body.insert(
            2,
            f"let _scope = unsafe {{ maplibre_core::handle::NativeViewScope::begin(native, sys::{handle.view_begin}, sys::{handle.view_end}) }}?;",
        )
        body[-1] = f"let value = {output}; Ok(callback(&value))"
        signature.append(f"callback: impl FnOnce(&{public}) -> R")
        generic, public = "<R>", "R"

    def has_pointer(value):
        return (
            False
            if value.registration
            else value.kind == "native_pointer"
            or bool(value.element and has_pointer(value.element))
            or any(
                has_pointer(field.value)
                for field in value.fields
                if field.role not in {"reserved", "context"}
            )
        )

    unsafe_input = any(
        has_pointer(parameter.value)
        for parameter in plan.inputs
        if parameter.name != plan.receiver
        and parameter.name not in direct_arguments
        and not parameter.value.registration
    )
    safety_doc = (
        "    /// # Safety\n    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.\n"
        if unsafe_input
        else ""
    )
    code = (
        safety_doc
        + f"    /// Calls `{function.name}` using its header execution and ownership contract.\n"
        f"    pub {'unsafe ' if unsafe_input else ''}fn {method}{generic}({', '.join(signature)}) -> Result<{public}> {{\n"
        "        // SAFETY: input storage lives through submission; callback values are copied before return.\n"
        + "\n".join("        " + line for line in body)
        + "\n    }\n"
    )
    return module, code, values


def lower(api: Api | BoundApi) -> tuple[dict[str, str], list[str], dict[str, str]]:
    chunks, generated, unsupported, values = defaultdict(list), [], {}, set()
    bound = compile_api(api)
    api = bound.source
    from .rust_values import Values

    value_types = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            value_types.add(value)
    value_types.direct_callbacks = {}
    value_types.owners = dict(OWNERS)
    value_types.handle_types = dict(HANDLE_TYPES)
    owned_declarations = []
    for handle in bound.handles.values():
        if (
            handle.native in HANDLE_TYPES
            or handle.parent
            or any(
                callback.decision and callback.decision.handle.native == handle.native
                for callback in bound.callbacks.values()
            )
        ):
            continue
        dispose = bound.operations_by_name.get(handle.dispose)
        if dispose is None:
            continue
        if (
            len(dispose.function.parameters) != 1
            or dispose.function.return_type.kind != "void"
        ):
            continue
        owner = pascal(handle.native) + "Handle"
        module = "owned_" + handle.native
        value_types.owners[handle.native] = (module, owner)
        value_types.handle_types[handle.native] = "crate::" + owner
        owned_declarations.append(
            f'#[derive(Debug)] pub struct {owner} {{ handle: crate::handle::ConcurrentNativeHandle<sys::{handle.native}> }}\nimpl {owner} {{ pub(crate) fn from_native(raw: sys::{handle.native}) -> Result<Self> {{ Ok(Self {{ handle: unsafe {{ crate::handle::ConcurrentNativeHandle::from_handle(raw, "{handle.native}") }}? }}) }} fn native(&self) -> Result<sys::{handle.native}> {{ maplibre_core::callback::check("", 0)?; self.handle.live_handle().ok_or_else(|| crate::handle::closed_handle_error("{owner}")) }} }}\nimpl Drop for {owner} {{ fn drop(&mut self) {{ self.handle.finalize_with(|raw| {{ unsafe {{ sys::{handle.dispose}(raw) }}; Ok(()) }}); }} }}'
        )
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        function = plan.function
        previous_values = dict(value_types.used)
        try:
            module, code, records = operation(api, plan, value_types)
            chunks[module].append(code)
            generated.append(function.name)
            values.update(records)
        except Unsupported as error:
            value_types.used = previous_values
            unsupported[function.name] = f"{function.location}: {error}"
    marker = "// Generated from C headers by tools/bindgen. Do not edit.\n"
    files = {
        (
            f"crates/maplibre-native-ffi/src/{module}/generated.rs"
            if module != "frame"
            else "crates/maplibre-native-ffi/src/render/frame_generated.rs"
        ): marker
        + "use super::*;\n\n"
        + (
            f"impl {next(owner for name, owner in value_types.owners.values() if name == module)} {{\n"
            if module != "global"
            else ""
        )
        + "\n".join(body)
        + ("}\n" if module != "global" else "")
        for module, body in chunks.items()
    }
    owned_bodies = [
        files.pop(path).removeprefix(marker + "use super::*;\n\n")
        for path in list(files)
        if "/owned_" in path
    ]
    files["crates/maplibre-native-ffi/src/owned_generated.rs"] = (
        marker + "use super::*;\n" + "\n".join(owned_declarations + owned_bodies)
    )
    disposers = [
        f"#[doc(hidden)]\npub unsafe fn {handle.native.removeprefix('mln_')}_dispose(native: maplibre_native_ffi_sys::{handle.native}) -> crate::Result<()> {{ crate::check(unsafe {{ maplibre_native_ffi_sys::{handle.dispose}(native) }}) }}\n"
        for handle in bound.handles.values()
        if handle.dispose in bound.operations_by_name
        and bound.operations_by_name[handle.dispose].function.return_type.kind != "void"
    ]
    decisions = {
        callback.decision.handle.native: callback.decision
        for callback in bound.callbacks.values()
        if callback.decision
    }
    decision_tables = [
        f"pub const {native.removeprefix('mln_').upper()}_FUNCTIONS: crate::resource::ResourceRequestHandleFns = unsafe {{ crate::resource::ResourceRequestHandleFns::new(maplibre_native_ffi_sys::{decision.complete}, maplibre_native_ffi_sys::{decision.cancelled}, maplibre_native_ffi_sys::{decision.cancel_registration}, maplibre_native_ffi_sys::{decision.handle.release}, maplibre_native_ffi_sys::{decision.wait_retired}) }};\n"
        for native, decision in sorted(decisions.items())
    ]
    from .rust_direct import declaration as direct_declaration

    declarations = [
        value_types.render(),
        *disposers,
        *decision_tables,
        *(
            direct_declaration(value_types, *registration)
            for registration in value_types.direct_callbacks.values()
        ),
    ]
    files["crates/maplibre-native-ffi-core/src/generated.rs"] = marker + "\n".join(
        declarations
    )
    return files, generated, unsupported


def generate(api: Api | BoundApi) -> dict[str, str]:
    return lower(api)[0]


def coverage(api: Api | BoundApi) -> dict:
    _, generated, unsupported = lower(api)
    return {"generated": generated, "unsupported": unsupported}
