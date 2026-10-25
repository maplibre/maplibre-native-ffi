"""Lower resolved operations to calls into the Zig operation runtime.

A generated operation names its native function, its receiver access, and the
binding value for each remaining native parameter. bindings/zig/src/call.zig
passes native ids, encodes inputs,
retains callback roots, and decodes outputs and completion results.
"""

from dataclasses import replace

from ..names import camel, pascal
from ..semantic import output_member
from .zig import (
    DIAGNOSTIC_PARAMETER,
    failure,
    identifier,
)
from .zig_dynamic_values import dynamic, encoding

RESERVED = {
    "allocator",
    "diagnostic",
    "call",
    "c",
    "std",
    "status",
    "completion",
    "owner",
    "callbacks",
    "marshal",
    "diagnostics",
    "started",
    "released",
    "out",
    "text",
}


def element(value):
    return value.unwrapped


def validate_input(function, name, value):
    if value.kind == "handle" or value.registration:
        return
    if value.element:
        validate_input(function, name, value.element)
    for field in value.fields:
        if field.public:
            validate_input(function, name + "." + field.name, field.value)


def access(plan):
    if plan.scoped_receiver:
        return "scoped"
    if not plan.receiver:
        return "none"
    return "handle"


def owned(values, value):
    """The public type of a value, wrapped in OwnedValue when it copies storage."""
    public = values.public(value)
    return f"OwnedValue({public})" if dynamic(value) else public


def copier(plan, values):
    """The call.zig copier for a completion's result, and its value type."""
    result = plan.result
    if plan.execution == "command":
        return "call.command", "completion.CommandCompletion"
    if result is None:
        return "call.unit", "void"
    values.add(result)
    if result.nullable and result.optional == "empty":
        raise failure(plan.function, "result is both nullable and optional")
    content = replace(result, nullable=False, optional=None)
    if result.kind == "array":
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
        raise failure(plan.function, "empty completion requires a copier")
    return copy, public


def function_name(plan, values, *, discard=False):
    """The public function's name: the C name without `mln_`, or the member
    name on a command's discarding view.

    A dispose that is also its handle's release keeps the C name for the call
    that passes no completion, and takes the release's name, as `mapRelease`
    has, for the call that passes one.
    """
    optional = plan.completion and plan.completion.optional
    if optional and not discard:
        receiver = next(p.value.native for p in plan.inputs if p.name == plan.receiver)
        return camel(f"{values.bound.handles[receiver].stem}_{plan.member}")
    if discard and not optional:
        return camel(plan.member)
    return camel(plan.name.removeprefix("mln_"))


def method_name(plan, *, discard=False):
    """A method's name: its member name, or `dispose` for the call that passes
    no completion to a dispose that is also its handle's release."""
    if discard and plan.completion and plan.completion.optional:
        return "dispose"
    return camel(plan.member)


def operation(plan, values, *, discard=False, method=False, taken=frozenset()):
    """A function, or with `method` a method of its receiver's type, which
    takes the receiver as `self`. `taken` names the declarations of that
    type, which no parameter may shadow."""
    function = plan.function
    if plan.consumes == "always" and function.return_type.kind != "void":
        raise failure(
            function,
            "status-returning unconditional consumption requires a commit-on-error adapter",
        )
    if plan.view:
        return view_operation(plan, values, method)
    if plan.drain:
        return drain_operation(plan, values, method)
    inputs = {p.name: p.value for p in plan.inputs}
    outputs = {p.name: p.value for p in plan.outputs}
    owned_outputs = {p.parameter: p for p in plan.owned_outputs}
    receiver_name = plan.receiver or plan.scoped_receiver
    names = {}
    for parameter in function.parameters:
        public = parameter.name
        while public in RESERVED or public in taken or public in names.values():
            public += "_input"
        names[parameter.name] = identifier(public)
    if method:
        names[receiver_name] = "self"
    length_sources = {
        v.length: n
        for n, v in inputs.items()
        if v.kind in {"array", "buffer"}
        and v.length
        and v.length != "nul"
        and not v.length.isdigit()
    }
    # Caller-sized text marks its storage, capacity, and length, which the
    # call fills in and passes again with enough storage when the text did
    # not fit.
    text = plan.caller_sized_text
    text_markers = (
        {
            text.name: "call.text()",
            text.value.length: "call.textCapacity()",
            text.result_length: "call.textLength()",
        }
        if text
        else {}
    )
    signature, arguments, results = [], [], []
    infallible = False
    needs_allocator = False
    completion = plan.completion
    for parameter in function.parameters:
        name = parameter.name
        local = names[name]
        if completion and name == completion.parameter:
            continue
        if name in text_markers:
            if name == text.name:
                needs_allocator = True
                results.append(
                    (identifier(output_member(name)), "OwnedValue([]const u8)")
                )
            arguments.append(text_markers[name])
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
            label = identifier(output_member(name))
            if name in owned_outputs:
                results.append((label, values.public(value)))
                arguments.append(f"call.adopt({values.public(value)})")
                continue
            if completion:
                raise failure(function, "completion immediate output needs ownership")
            public = owned(values, value)
            needs_allocator |= dynamic(value)
            results.append((label, public))
            sized_by = next(p.sized_by for p in plan.outputs if p.name == name)
            if sized_by:
                # The call allocates the native elements from its input arena.
                needs_allocator = True
                arguments.append(
                    f"call.outArray({values.public(value.element)}, call.len({names[sized_by]}))"
                )
                continue
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
        # A consuming call closes the caller's value, so it takes a pointer.
        pointer = "*" if name == receiver_name and plan.consumes else ""
        signature.append(
            f"{local}: {pointer}{values.public(target if target.kind == 'handle' else value)}"
        )
        if name == receiver_name:
            continue
        if value.kind == "reference" and value.element.response:
            arguments.append(local + ".native")
            continue
        if target.kind != "handle":
            # The call encodes a value native reads through a pointer in its
            # own storage, so only what that value holds can allocate.
            needs_allocator |= encoding(
                value.element if value.kind == "reference" else value
            )[0]
        if value.kind == "buffer" and value.length == "nul":
            arguments.append(
                f"if ({local}) |text| call.cString(text) else null"
                if value.nullable or value.optional == "empty"
                else f"call.cString({local})"
            )
            continue
        arguments.append(local)
    mode = access(plan)
    receiver = names[receiver_name] if receiver_name else "{}"
    if plan.consumes:
        receiver += ".*"
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
        if discard:
            return_type = "void"
            body = [f"return call.discard({head}, {allocator}, {diagnostic}, {args});"]
        elif results:
            # A call that creates an owner returns it with the completion that
            # reports when it is ready, as map creation and every attach do.
            created = len(results) == 1 and plan.owned_outputs
            return_type = values.result(
                "New" + results[0][1]
                if created
                else pascal(plan.name.removeprefix("mln_")) + "Result",
                [*results, ("ready", future)],
                f"A new `{results[0][1]}`, and the future that completes once it"
                " is ready."
                if created
                else "The outputs of a call, and the future of its completion.",
            )
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
    elif function.return_type.kind == "void" or (plan.result and not plan.status):
        result_type = "void"
        if plan.result:
            values.add(plan.result)
            result_type = owned(values, plan.result)
            needs_allocator |= dynamic(plan.result)
        allocator = "allocator" if needs_allocator else "null"
        return_type = result_type
        body = [f"return call.direct({head}, {result_type}, {allocator}, {args});"]
        # A call that passes only handle ids and copies no storage can't fail,
        # so it calls native directly and returns no error.
        if (
            not needs_allocator
            and mode != "scoped"
            and all(
                inputs.get(p.name) and inputs[p.name].kind == "handle"
                for p in function.parameters
            )
        ):
            native_call = f"c.{function.name}({', '.join(names[p.name] + '.raw' for p in function.parameters)})"
            body = [
                f"{native_call};"
                if result_type == "void"
                else f"return {values.capture(plan.result, native_call)};"
            ]
            infallible = True
            if mode == "none":
                # A call without a handle can be a program's first, and with
                # no error channel it panics on another C ABI version.
                body.insert(0, "status.requireAbiVersion();")
    else:
        if not function.diagnostic:
            raise failure(function, "status result requires a diagnostic parameter")
        if len(results) > 1 and any("OwnedValue" in typ for _, typ in results):
            raise failure(function, "several outputs that copy storage need one arena")
        allocator = "allocator" if needs_allocator else "null"
        invoke = f"call.invoke({head}, {allocator}, {diagnostic}, {args})"
        if len(results) > 1:
            return_type = values.result(
                pascal(plan.name.removeprefix("mln_")) + "Result",
                results,
                f"The outputs of `{camel(plan.name.removeprefix('mln_'))}`.",
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
        elif any(r.accepted_unless for r in plan.registrations):
            return_type = "bool"
            body = [
                f"return call.invokeDeclinable({head}, {allocator}, {diagnostic}, {args});"
            ]
        elif plan.absence:
            absent = f"c.{plan.absence.status}"
            return_type = "?" + results[0][1]
            body = [
                f"return call.invokeUnless({head}, {allocator}, {diagnostic}, {absent}, {args});"
            ]
        else:
            return_type = results[0][1] if results else "void"
            body = [f"return {invoke};"]
    if plan.consumes:
        body = consume(
            function, names[receiver_name], body, return_type, infallible, plan.release
        )
    if needs_allocator:
        signature.insert(0, "allocator: std.mem.Allocator")
    if function.diagnostic:
        signature.append(DIAGNOSTIC_PARAMETER)
    if method:
        receiver_parameter = next(
            i for i, p in enumerate(signature) if p.startswith("self:")
        )
        signature.insert(0, signature.pop(receiver_parameter))
    name = (
        method_name(plan, discard=discard)
        if method
        else function_name(plan, values, discard=discard)
    )
    if discard and not completion.optional:
        receiver_parameter = next(
            i
            for i, p in enumerate(signature)
            if p.startswith(names[receiver_name] + ":")
        )
        receiver_type = signature[receiver_parameter].split(": ", 1)[1]
        signature.pop(receiver_parameter)
        signature.insert(0, f"self: {receiver_type}Discarding")
        body.insert(0, f"const {receiver} = self.receiver;")
    code = (
        f"pub fn {name}({', '.join(signature)}) {'' if infallible else 'status.Error!'}{return_type} {{\n    "
        + "\n    ".join(body)
        + "\n}\n"
    )
    return code


def consume(function, receiver, body, return_type, infallible=False, release=True):
    """Close the receiver once native accepts its release. A synchronous release
    of a closed receiver does nothing; an asynchronous one, and an answer that
    consumes its receiver, reach native, which reports a stale handle."""
    if not release:
        if len(body) != 1 or not body[0].startswith("return "):
            raise failure(function, "an answer with outputs needs an owner transaction")
        answer = body[0].removeprefix("return ").removesuffix(";")
        if return_type != "void":
            return [
                f"const answered = try {answer};",
                f"{receiver}.closed = true;",
                "return answered;",
            ]
        return [f"try {answer};", f"{receiver}.closed = true;"]
    if infallible and return_type == "void":
        return [
            f"if ({receiver}.closed) return;",
            body[0],
            f"{receiver}.closed = true;",
        ]
    if len(body) != 1 or not body[0].startswith("return "):
        raise failure(
            function, "consumption with outputs requires an owner transaction"
        )
    release = body[0].removeprefix("return ").removesuffix(";")
    if return_type == "void":
        return [
            f"if ({receiver}.closed) return;",
            f"try {release};",
            f"{receiver}.closed = true;",
        ]
    return [
        f"const released = try {release};",
        f"{receiver}.closed = true;",
        "return released;",
    ]


def view_operation(plan, values, method=False):
    view = plan.view
    value = view.output.value.element
    values.add(value)
    handle_type = values.public(
        next(p.value for p in plan.inputs if p.name == plan.receiver)
    )
    typ = values.public(value)
    # `use` is a `*const fn (@TypeOf(context), View) E!Result`, and the view
    # returns its result with the binding's errors and its own.
    name = method_name(plan) if method else camel(plan.name.removeprefix("mln_"))
    handle = "self" if method else "handle"
    return f'''pub fn {name}({handle}: {handle_type}, context: anytype, comptime use: anytype, {DIAGNOSTIC_PARAMETER}) call.ViewResult(@TypeOf(use)) {{
    return call.scopedView("{plan.name}", "{view.begin}", "{view.end}", {typ}, {handle}, context, use, diagnostic);
}}
'''


def drain_operation(plan, values, method=False):
    """A drain, which returns a copy of its batch's slice and releases the
    batch, so no owner holds it."""
    drain = plan.drain
    function = plan.function
    receiver = plan.receiver
    if not function.diagnostic or [p.name for p in function.parameters] != [
        *([receiver] if receiver else []),
        drain.batch.name,
        drain.view.name,
    ]:
        raise failure(function, "a drain takes its receiver, batch, and view")
    array = drain.slice.value
    values.add(array)
    item = values.public(array.element)
    batch = element(drain.batch.value)
    release = values.bound.handles[batch.native].release
    if values.bound.source.functions_by_name[release].return_type.kind != "void":
        raise failure(function, "a drained batch's release must not fail")
    signature = ["allocator: std.mem.Allocator", DIAGNOSTIC_PARAMETER]
    if receiver:
        handle = values.public(next(p.value for p in plan.inputs if p.name == receiver))
        signature.insert(0, f"{'self' if method else identifier(receiver)}: {handle}")
    stride = f'"{array.stride}"' if array.stride else "null"
    arguments = [
        f'"{plan.name}"',
        f".{access(plan)}",
        ("self" if method else identifier(receiver)) if receiver else "{}",
        item,
        f'"{drain.slice.name}"',
        f'"{array.length}"',
        stride,
        f"c.{release}",
        "allocator",
        "diagnostic",
        f"c.{plan.absence.status}",
    ]
    name = method_name(plan) if method else camel(plan.name.removeprefix("mln_"))
    return (
        f"pub fn {name}({', '.join(signature)}) status.Error!OwnedValue([]const {item}) {{\n"
        f"    return call.drain({', '.join(arguments)});\n"
        "}\n"
    )
