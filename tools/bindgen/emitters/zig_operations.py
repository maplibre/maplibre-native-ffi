"""Lower resolved operations to calls into the Zig operation runtime.

A generated operation names its native function, its receiver access, and the
binding value for each remaining native parameter. bindings/zig/src/call.zig
admits the call, holds the receiver and handle arguments, encodes inputs,
retains callback roots, and decodes outputs and completion results.
"""

from dataclasses import replace

from .zig import (
    DIAGNOSTIC_PARAMETER,
    DIAGNOSTIC_PREAMBLE,
    camel,
    failure,
    identifier,
    status_call,
)
from .zig_dynamic_values import dynamic

# Names that a generated operation body refers to besides its parameters.
RESERVED = {
    "allocator",
    "diagnostic",
    "call",
    "c",
    "std",
    "status",
    "completion",
    "owner",
    "callback",
    "marshal",
    "diagnostics",
    "started",
    "out",
    "text",
}


def element(value):
    return value.element if value.kind == "reference" else value


def validate_input(function, name, value):
    if value.kind == "handle" or value.registration:
        return
    if value.lifetime != "call":
        raise failure(
            function, f"parameter {name}: retained input requires a lifetime adapter"
        )
    if value.item_buffer:
        raise failure(
            function,
            f"parameter {name}: packed item-buffer inputs require an encoding adapter",
        )
    if value.element:
        validate_input(function, name, value.element)
    for field in value.fields:
        if field.role not in {"reserved", "count", "size", "presence_mask", "tag"}:
            validate_input(function, name + "." + field.name, field.value)


def access(plan, values, decision):
    if plan.scoped_receiver:
        return "scoped"
    if not plan.receiver:
        return "none"
    if plan.receiver_access == "issued":
        return "issued"
    if plan.consumes:
        return "close"
    if decision:
        return "complete"
    borrowed = any(
        dynamic(element(p.value)) and element(p.value).kind != "handle"
        for p in plan.outputs
    )
    return "borrow" if borrowed else "lease"


def owned(values, value):
    """The public type of a value, wrapped in OwnedValue when it copies storage."""
    public = values.public(value)
    return f"OwnedValue({public})" if dynamic(value) else public


def parent_of(plan, owner):
    """The call.Parent that an adopted handle retains."""
    parent = owner.parent_parameter
    if parent is None:
        return ".none"
    if parent == plan.receiver:
        return ".receiver"
    # Arguments are the parameters after the receiver, without the completion.
    arguments = [
        p.name
        for p in plan.function.parameters
        if p.name != (plan.receiver or plan.scoped_receiver)
        and not (plan.completion and p.name == plan.completion.parameter)
    ]
    return f".{{ .argument = {arguments.index(parent)} }}"


def copier(plan, values):
    """The call.zig copier for a completion's result, and its value type."""
    result = plan.result
    if plan.execution == "command":
        return "call.command", "completion.CommandCompletion"
    if result is None:
        return "call.unit", "void"
    values.add(result)
    if plan.completion.result_owner:
        handle = values.public(result)
        where = parent_of(plan, plan.completion.result_owner)
        return f"call.handle({handle}, c.{result.native}, {where})", handle
    if result.nullable and result.optional == "empty":
        raise failure(plan.function, "result is both nullable and optional")
    content = replace(result, nullable=False, optional=None)
    if result.kind == "array":
        if result.stride or result.item_buffer:
            raise failure(plan.function, "strided array results need a record copy")
        item = result.element
        values.add(item)
        copy = f"call.slice({values.public(item)}, {values.native_type(item)})"
        public = f"OwnedValue([]const {values.public(item)})"
    else:
        public = owned(values, content)
        copy = f"call.value({public}, {values.native_type(content)})"
    if result.nullable:
        return f"call.orNull({copy})", "?" + public
    if result.optional == "empty":
        return f"call.orEmpty({copy}, {values.native_type(content)})", "?" + public
    return copy, public


def operation(plan, api, values):
    function = plan.function
    if plan.consumes == "always" and function.return_type.kind != "void":
        raise failure(
            function,
            "status-returning unconditional consumption requires a commit-on-error adapter",
        )
    decision = next(
        (
            cb.decision
            for cb in values.bound.callbacks.values()
            if cb.decision and cb.decision.complete == plan.name
        ),
        None,
    )
    if plan.view:
        return view_operation(plan, values)
    if plan.direct_registrations:
        from .zig_direct import operation as direct

        return direct(plan, values)
    inputs = {p.name: p.value for p in plan.inputs}
    outputs = {p.name: p.value for p in plan.outputs}
    owned_outputs = {p.parameter: p for p in plan.owned_outputs}
    receiver_name = plan.receiver or plan.scoped_receiver
    names = {}
    for parameter in function.parameters:
        public = parameter.name
        while public in RESERVED or public in names.values():
            public += "_input"
        names[parameter.name] = identifier(public)
    length_sources = {
        v.length: n
        for n, v in inputs.items()
        if v.kind in {"array", "buffer"}
        and v.length
        and v.length != "nul"
        and not v.length.isdigit()
    }
    signature, arguments, results = [], [], []
    needs_allocator = False
    completion = plan.completion
    for parameter in function.parameters:
        name = parameter.name
        local = names[name]
        if completion and name == completion.parameter:
            continue
        if name in length_sources:
            source = names[length_sources[name]]
            counted = next(v for v in inputs.values() if v.length == name)
            optional = counted.nullable or counted.optional == "empty"
            arguments.append(f"call.len({source})" if optional else f"{source}.len")
            continue
        if name in outputs and not (name == plan.receiver and plan.consumes):
            value = element(outputs[name])
            values.add(value)
            label = identifier(name.removeprefix("out_"))
            if name in owned_outputs:
                results.append((label, values.public(value)))
                where = parent_of(plan, owned_outputs[name])
                arguments.append(f"call.adopt({values.public(value)}, {where})")
                continue
            if completion:
                raise failure(function, "completion immediate output needs ownership")
            public = owned(values, value)
            needs_allocator |= dynamic(value)
            results.append((label, public))
            sized = value.kind == "record" and any(
                field.role == "size" for field in value.fields
            )
            arguments.append(f"call.{'sizedOut' if sized else 'out'}({public})")
            continue
        value = inputs.get(name)
        if value is None:
            raise failure(function, f"parameter {name} has no resolved input")
        validate_input(function, name, value)
        target = element(value)
        values.add(target if target.kind == "handle" else value)
        signature.append(
            f"{local}: {values.public(target if target.kind == 'handle' else value)}"
        )
        if name == receiver_name:
            continue
        if value.kind == "reference" and value.element.response:
            arguments.append(local + ".native")
            continue
        if target.kind != "handle":
            needs_allocator |= (
                value.kind == "reference"
                or dynamic(value)
                and value.kind != "buffer"
                or value.kind == "buffer"
                and value.length == "nul"
            )
        if value.kind == "buffer" and value.length == "nul":
            arguments.append(
                f"if ({local}) |text| call.cString(text) else null"
                if value.nullable or value.optional == "empty"
                else f"call.cString({local})"
            )
            continue
        arguments.append(local)
    mode = access(plan, values, decision)
    receiver = names[receiver_name] if receiver_name else "{}"
    diagnostic = "diagnostic" if function.diagnostic else "null"
    args = ".{ " + ", ".join(arguments) + " }" if arguments else ".{}"
    head = f'"{plan.name}", .{mode}, {receiver}'
    if completion:
        if not function.diagnostic:
            raise failure(function, "completion start requires a diagnostic parameter")
        copy, value_type = copier(plan, values)
        needs_allocator |= "OwnedValue" in value_type
        allocator = "allocator" if needs_allocator else "null"
        submit = f"call.submit({head}, {copy}, {allocator}, {diagnostic}, {args})"
        future = f"completion.Future({value_type})"
        if results:
            fields = ", ".join(f"{label}: {typ}" for label, typ in results)
            return_type = f"struct {{ {fields}, ready: {future} }}"
            picks = (
                [f".{results[0][0]} = started.outputs"]
                if len(results) == 1
                else [
                    f".{label} = started.outputs[{index}]"
                    for index, (label, _) in enumerate(results)
                ]
            )
            body = [
                f"const started = try {submit};",
                "return .{ " + ", ".join(picks) + ", .ready = started.ready };",
            ]
        else:
            return_type = future
            body = [f"return {submit};"]
    elif function.return_type.kind == "void" or (
        plan.result and function.return_type.spelling != "mln_status"
    ):
        result_type = "void"
        if plan.result:
            values.add(plan.result)
            result_type = owned(values, plan.result)
            needs_allocator |= dynamic(plan.result)
        allocator = "allocator" if needs_allocator else "null"
        return_type = result_type
        body = [f"return call.direct({head}, {result_type}, {allocator}, {args});"]
    else:
        if not function.diagnostic:
            raise failure(function, "status result requires a diagnostic parameter")
        if len(results) > 1 and any("OwnedValue" in typ for _, typ in results):
            raise failure(function, "several outputs that copy storage need one arena")
        allocator = "allocator" if needs_allocator else "null"
        invoke = f"call.invoke({head}, {allocator}, {diagnostic}, {args})"
        if len(results) > 1:
            return_type = (
                "struct { "
                + ", ".join(f"{label}: {typ}" for label, typ in results)
                + " }"
            )
            body = [
                f"const out = try {invoke};",
                "return .{ "
                + ", ".join(
                    f".{label} = out[{index}]"
                    for index, (label, _) in enumerate(results)
                )
                + " };",
            ]
        else:
            return_type = results[0][1] if results else "void"
            body = [f"return {invoke};"]
    if needs_allocator:
        signature.insert(0, "allocator: std.mem.Allocator")
    if function.diagnostic:
        signature.append(DIAGNOSTIC_PARAMETER)
    code = (
        f"pub fn {camel(plan.name.removeprefix('mln_'))}({', '.join(signature)}) status.Error!{return_type} {{\n    "
        + "\n    ".join(body)
        + "\n}\n"
    )
    return code, "global", "", None


def view_operation(plan, values):
    view = plan.view
    functions = values.bound.source.functions_by_name
    begin = status_call(functions[view.owner.view_begin], ["lease.native", "&token"])
    get = status_call(plan.function, ["lease.native", "&raw"])
    preamble = "\n    ".join(DIAGNOSTIC_PREAMBLE)
    value = plan.outputs[0].value.element
    values.add(value)
    handle_type = values.public(
        next(p.value for p in plan.inputs if p.name == plan.receiver)
    )
    typ = values.public(value)
    code = f'''pub fn {camel(plan.name.removeprefix("mln_"))}(comptime Result: type, handle: {handle_type}, context: anytype, comptime use: *const fn (@TypeOf(context), {typ}) anyerror!Result, {DIAGNOSTIC_PARAMETER}) anyerror!Result {{
    {preamble}
    try callback.check("{plan.name}", handle.raw);
    const lease = try handle.lease();
    defer lease.release();
    var token: ?*anyopaque = null;
    {begin}
    defer c.{view.owner.view_end}(token);
    var raw: c.{value.native} = std.mem.zeroes(c.{value.native});
    raw.size = @sizeOf(c.{value.native});
    {get}
    return use(context, {typ}.fromNative(raw));
}}
'''
    return code, "global", "", None
