"""Lower resolved operations through the shared Zig owner and completion runtimes."""

from .zig import camel, failure, identifier, status_call
from .zig_dynamic_values import decode, dynamic, encode


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
    names = {p.name: f"binding_arg_{i}" for i, p in enumerate(function.parameters)}
    owned = {p.parameter: p for p in plan.owned_outputs}
    declarations, setup, arguments, output_values = [], [], [], []
    completion = plan.completion
    needs_allocator = False
    receiver = inputs.get(plan.receiver)
    while receiver and receiver.kind == "reference":
        receiver = receiver.element
    leases = {}
    length_sources = {
        v.length: names[n]
        for n, v in inputs.items()
        if v.kind in {"array", "buffer"}
        and v.length
        and v.length != "nul"
        and not v.length.isdigit()
    }
    for parameter_index, parameter in enumerate(function.parameters):
        name = parameter.name
        local = names[name]
        value = inputs.get(name)
        if completion and name == completion.parameter:
            continue
        if name in length_sources:
            source = length_sources[name]
            value = next(v for v in inputs.values() if v.length == name)
            arguments.append(
                f"if ({source}) |items| items.len else 0"
                if value.nullable or value.optional == "empty"
                else f"{source}.len"
            )
            continue
        if name in outputs and not (name == plan.receiver and plan.consumes):
            value = outputs[name]
            if value.kind == "reference":
                value = value.element
            values.add(value)
            setup.append(
                f"var {local}: {values.native_type(value)} = std.mem.zeroes({values.native_type(value)});"
            )
            if value.kind == "record":
                for field in value.fields:
                    if field.role == "size":
                        setup.append(
                            f"{local}.{identifier(field.name)} = @sizeOf({values.native_type(value)});"
                        )
            arguments.append(f"&{local}")
            output_values.append((name, local, value))
            continue
        if value is None:
            raise failure(function, f"parameter {name} has no resolved input")
        validate_input(function, name, value)
        target = value.element if value.kind == "reference" else value
        if target.kind == "handle":
            values.add(target)
            declarations.append(f"{local}: {values.public(target)}")
            lease = local + "_lease"
            leases[name] = lease
            if plan.receiver_access == "issued" and name == plan.receiver:
                arguments.append(local + ".raw")
                continue
            if plan.consumes and name == plan.receiver:
                setup.append(
                    f"const {lease} = try {local}.beginClose() orelse "
                    + (
                        "return try completion.completed(void, {});"
                        if completion
                        else "return;"
                    )
                )
                setup.append(f"errdefer {lease}.rollback();")
                setup.append(
                    f"var {local}_native = {lease}.native;"
                    if parameter.type.kind == "pointer"
                    else f"const {local}_native = {lease}.native;"
                )
                arguments.append(
                    ("&" if parameter.type.kind == "pointer" else "")
                    + local
                    + "_native"
                )
            else:
                if decision:
                    setup.extend(
                        [
                            f"const {lease} = try {local}.beginComplete();",
                            f"errdefer {lease}.finishComplete(false);",
                        ]
                    )
                else:
                    access = (
                        "borrow"
                        if name == plan.receiver
                        and any(
                            dynamic(
                                p.value.element
                                if p.value.kind == "reference"
                                else p.value
                            )
                            and (
                                p.value.element
                                if p.value.kind == "reference"
                                else p.value
                            ).kind
                            != "handle"
                            for p in plan.outputs
                        )
                        else "lease"
                    )
                    setup.extend(
                        [
                            f"const {lease} = try {local}.{access}();",
                            f"defer {lease}.release();",
                        ]
                    )
                arguments.append(lease + ".native")
            continue
        values.add(value)
        declarations.append(f"{local}: {values.public(value)}")
        if value.kind == "reference" and value.element.response:
            arguments.append(local + ".native")
        elif value.kind == "reference":
            needs_allocator = True
            arguments.append(
                encode(values, value, local).replace("allocator", "input_allocator")
            )
        else:
            needs_allocator |= (
                dynamic(value)
                and value.kind != "buffer"
                or value.kind == "buffer"
                and value.length == "nul"
            )
            encoded = encode(values, value, local).replace(
                "allocator", "input_allocator"
            )
            arguments.append(
                f'@as(@typeInfo(@TypeOf(c.{function.name})).@"fn".params[{parameter_index}].type.?, {encoded})'
            )
    policy_owner = (
        names[plan.receiver] + ".raw"
        if plan.receiver
        else f"@intFromPtr({names[plan.scoped_receiver]}.native)"
        if plan.scoped_receiver
        else "0"
    )
    setup[:0] = [
        f'try callback.{"checkScoped" if plan.scoped_receiver else "check"}("{plan.name}", {policy_owner});',
        "var root_storage: callback.Roots = .{};",
        "const roots = &root_storage;",
        "defer roots.deinit();",
    ]
    body = []
    receiver_lease = leases.get(plan.receiver)
    # Handle calls report into the receiver's store; other status calls take one.
    diagnostic = "null"
    if receiver_lease and plan.receiver_access != "issued":
        diagnostic = receiver_lease + ".diagnostic_store"
    elif function.diagnostic:
        diagnostic = "diagnostic_store"
        declarations.append("diagnostic_store: ?*diagnostics.DiagnosticStore")
    close_commit = [f"{receiver_lease}.commit();"] if plan.consumes else []

    def parent_anchor(owner):
        return (
            leases[owner.parent_parameter] + ".anchor()"
            if owner.parent_parameter
            else "null"
        )

    def capture(value, local, owner=None):
        if owner:
            return f"try {values.public(value)}.adopt({local}, {parent_anchor(owner)}, {diagnostic})"
        return decode(values, value, local)

    if completion:
        if not function.diagnostic:
            raise failure(function, "completion start requires a diagnostic parameter")
        body.append("const native_arguments = .{ " + ", ".join(arguments) + " };")
        result = plan.result
        if plan.execution == "command":
            public, copier = "completion.CommandCompletion", "completion.command"
            submit = f"completion.submit({public}, {diagnostic}, {copier}, c.{plan.name}, native_arguments)"
        elif result is None:
            public = "void"
            submit = f"completion.submit(void, {diagnostic}, completion.unit, c.{plan.name}, native_arguments)"
        elif completion.result_owner:
            values.add(result)
            public = values.public(result)
            parent = parent_anchor(completion.result_owner)
            body.append(
                f"const result_context = OwnerCopyContext{{ .parent = if (@as(?owner.Anchor, {parent})) |anchor| anchor.retain() else null, .diagnostic_store = {diagnostic} }};"
            )
            copier = f"struct {{ fn copy(raw: *const c.mln_completion_result, context: *OwnerCopyContext) status.Error!{public} {{ return {public}.adopt(try completion.value(c.{result.native})(raw), context.parent, context.diagnostic_store); }} }}.copy"
            submit = f"completion.submitWithCopyContext({public}, OwnerCopyContext, {diagnostic}, {copier}, result_context, c.{plan.name}, native_arguments)"
        else:
            values.add(result)
            public = values.public(result)
            is_dynamic = dynamic(result)
            nullable = result.nullable
            empty_optional = result.optional == "empty"
            # Completion nullability describes the payload pointer, independent of an empty buffer.
            from dataclasses import replace

            content = replace(result, nullable=False, optional=None)
            public = values.public(content)
            if is_dynamic:
                needs_allocator = True
                public = f"OwnedValue({public})"
            if nullable or empty_optional:
                public = "?" + public
            expression = decode(values, content, "raw_value")
            if result.kind == "array":
                expression = decode(
                    values,
                    content,
                    "@as(?[*]const "
                    + values.native_type(result.element)
                    + ", @ptrCast(@alignCast(result.value)))",
                    "result",
                )
                raw_setup = ""
            else:
                raw_setup = f"const raw_value = try completion.value({values.native_type(content)})(result);"
            null_check = "if (result.value == null) return null;" if nullable else ""
            empty_check = (
                "if (raw_value.size == 0) return null;"
                if empty_optional and result.kind == "buffer"
                else "if (result.value_count == 0) return null;"
                if empty_optional
                else ""
            )
            if is_dynamic:
                copied = f"{raw_setup} {empty_check} var arena = std.heap.ArenaAllocator.init(target.*); errdefer arena.deinit(); const copy_allocator = arena.allocator(); const copied_value = {expression.replace('allocator', 'copy_allocator')}; return .{{ .arena = arena, .value = copied_value }};"
                copier = f"struct {{ fn copy(result: *const c.mln_completion_result, target: *std.mem.Allocator) status.Error!{public} {{ {null_check} {copied} }} }}.copy"
                submit = f"completion.submitWithCopyContext({public}, std.mem.Allocator, {diagnostic}, {copier}, allocator, c.{plan.name}, native_arguments)"
            else:
                copier = f"struct {{ fn copy(result: *const c.mln_completion_result) status.Error!{public} {{ {null_check} {raw_setup} return {expression}; }} }}.copy"
                submit = f"completion.submit({public}, {diagnostic}, {copier}, c.{plan.name}, native_arguments)"
        return_type = f"completion.Future({public})"
        body.append(
            f"var readiness = try {submit};"
            if output_values
            else f"const readiness = try {submit};"
        )
        body.append("roots.accept();")
        body.extend(close_commit)
        if output_values:
            body.append("errdefer readiness.deinit();")
            fields, copies = [], []
            for name, local, value in output_values:
                if name not in owned:
                    raise failure(
                        function, "completion immediate output needs ownership"
                    )
                label = identifier(name.removeprefix("out_"))
                fields.append(f"{label}: {values.public(value)}")
                copies.append(f".{label} = {capture(value, local, owned[name])}")
            return_type = "struct { " + ", ".join(fields) + f", ready: {return_type} }}"
            body.append("return .{ " + ", ".join(copies) + ", .ready = readiness }; ")
        else:
            body.append("return readiness;")
    else:
        call = f"c.{plan.name}({', '.join(arguments)})"
        if plan.result:
            values.add(plan.result)
            value = plan.result
            return_type = values.public(value)
            body.append(f"const raw_result = {call};")
            body.append("roots.accept();")
            expression = decode(values, value, "raw_result")
            if dynamic(value):
                needs_allocator = True
                return_type = f"OwnedValue({return_type})"
                body.extend(
                    [
                        "var arena = std.heap.ArenaAllocator.init(allocator);",
                        "errdefer arena.deinit();",
                        "const output_allocator = arena.allocator();",
                    ]
                )
                body.append(
                    "const copied_value = "
                    + expression.replace("allocator", "output_allocator")
                    + ";"
                )
                expression = ".{ .arena = arena, .value = copied_value }"
            body.append(f"return {expression};")
        else:
            native_call = (
                f"{call};"
                if function.return_type.kind == "void"
                else status_call(function, arguments, diagnostic)
            )
            body.append(
                f"if (!{receiver_lease}.deferred) {{ {native_call} }}"
                if plan.consumes
                else native_call
            )
            if decision:
                body.append(f"{receiver_lease}.finishComplete(true);")
            body.append("roots.accept();")
            body.extend(close_commit)
            fields, copies = [], []
            borrowed = any(dynamic(value) for _, _, value in output_values)
            if borrowed:
                needs_allocator = True
                body.extend(
                    [
                        "var arena = std.heap.ArenaAllocator.init(allocator);",
                        "errdefer arena.deinit();",
                        "const output_allocator = arena.allocator();",
                    ]
                )
            for name, local, value in output_values:
                fields.append(
                    (identifier(name.removeprefix("out_")), values.public(value))
                )
                copies.append(
                    capture(value, local, owned.get(name)).replace(
                        "allocator", "output_allocator"
                    )
                )
            return_type = (
                "void"
                if not fields
                else fields[0][1]
                if len(fields) == 1
                else "struct { "
                + ", ".join(f"{name}: {typ}" for name, typ in fields)
                + " }"
            )
            expression = (
                copies[0]
                if len(copies) == 1
                else ".{ "
                + ", ".join(
                    f".{field[0]} = {copied}" for field, copied in zip(fields, copies)
                )
                + " }"
            )
            if borrowed:
                return_type = f"OwnedValue({return_type})"
                body.append("const copied_value = " + expression + ";")
                expression = ".{ .arena = arena, .value = copied_value }"
            if fields:
                body.append(f"return {expression};")
    if needs_allocator:
        declarations.insert(0, "allocator: std.mem.Allocator")
        if any("input_allocator" in item for item in [*setup, *arguments]):
            setup[:0] = [
                "var input_arena = std.heap.ArenaAllocator.init(allocator);",
                "defer input_arena.deinit();",
                "const input_allocator = input_arena.allocator();",
            ]
    code = (
        f"pub fn {camel(plan.name.removeprefix('mln_'))}({', '.join(declarations)}) status.Error!{return_type} {{\n    "
        + "\n    ".join([*setup, *body])
        + "\n}\n"
    )
    return code, "global", "", None


def view_operation(plan, values):
    view = plan.view
    functions = values.bound.source.functions_by_name
    begin = status_call(
        functions[view.owner.view_begin],
        ["lease.native", "&token"],
        "lease.diagnostic_store",
    )
    get = status_call(plan.function, ["lease.native", "&raw"], "lease.diagnostic_store")
    value = plan.outputs[0].value.element
    values.add(value)
    handle_type = values.public(
        next(p.value for p in plan.inputs if p.name == plan.receiver)
    )
    typ = values.public(value)
    code = f'''pub fn {camel(plan.name.removeprefix("mln_"))}(comptime Result: type, handle: {handle_type}, context: anytype, comptime use: *const fn (@TypeOf(context), {typ}) anyerror!Result) anyerror!Result {{
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
