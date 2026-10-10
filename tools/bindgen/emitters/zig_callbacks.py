"""Generate retained callback descriptors from their resolved registration plans."""

from .zig import identifier
from .zig_dynamic_values import decode


def add(values, value):
    values.used[value.native] = value
    for field in value.fields:
        if field.name in value.registration.callbacks:
            callback = values.bound.callbacks[field.value.native]
            for parameter in callback.parameters:
                if parameter.name != callback.context:
                    values.add(parameter.value)
            if callback.result.ctype.kind != "void" and not callback.status:
                values.add(callback.result)
        elif (
            field.name not in {value.registration.user_data, value.registration.release}
            and field.role == "value"
        ):
            values.add(field.value)


def parts(values, value):
    fields = [
        "    context: ?*anyopaque = null,",
        "    release_context: ?*const fn (?*anyopaque) void = null,",
    ]
    writes, captures, trampolines = (
        [],
        ["            .context = null,", "            .release_context = null,"],
        [],
    )
    registration = value.registration
    public = values.public(value)
    root_type = f"callback.Registration({public})"
    for field in value.fields:
        if field.name not in registration.callbacks:
            continue
        plan = values.bound.callbacks[field.value.native]
        local = identifier(field.name)
        names = {p.name: f"native_arg_{i}" for i, p in enumerate(plan.parameters)}
        parameters = [p for p in plan.parameters if p.name != plan.context]
        result = (
            "void"
            if plan.result.ctype.kind == "void" or plan.status
            else values.public(plan.result)
        )
        fields.append(
            f"    {local}: ?*const fn (?*anyopaque{''.join(', ' + values.public(p.value) for p in parameters)}) status.Error!{result} = null,"
        )
        captures.append(f"            .{local} = null,")
        writes.append(
            f"        raw.{local} = if (self.{local} != null) {local}Trampoline else null;"
        )
        # Quoted identifiers cannot be extended into another identifier.
        trampoline = identifier(field.name + "Trampoline")
        writes[-1] = (
            f"        raw.{local} = if (self.{local} != null) {trampoline} else null;"
        )
        fallback = (
            "return;"
            if plan.result.ctype.kind == "void"
            else f"return {plan.failure if plan.failure.isdigit() else 'c.' + plan.failure};"
            if plan.failure not in {None, "contain"}
            else "return std.mem.zeroes(marshal.CallbackResult(c." + plan.native + "));"
        )
        converted = []
        for parameter in parameters:
            raw = names[parameter.name]
            if plan.decision and parameter.name == plan.decision.parameter:
                converted.append("request")
            elif (
                parameter.value.kind == "reference" and parameter.value.element.response
            ):
                converted.append(f".{'{'} .native = @ptrCast({raw}) {'}'}")
            else:
                expression = decode(values, parameter.value, raw)
                for name, source in names.items():
                    expression = expression.replace("raw." + identifier(name), source)
                converted.append(expression)
        policy = plan.reentry_policy
        owner = "0"
        if policy and policy.owner_parameter is None:
            # The receiver that registered the callback, which the roots record.
            owner = "state.owner"
        elif policy:
            parameter = next(
                p for p in plan.parameters if p.name == policy.owner_parameter
            )
            owner = (
                names[parameter.name]
                if parameter.value.kind == "handle"
                else f"@intFromPtr({names[parameter.name]})"
            )
        operations = (
            ", ".join('"' + name + '"' for name in policy.operations) if policy else ""
        )
        scope = (
            f"var scope: callback.Scope = .{{}}; scope.enter(&.{{ {operations} }}, {owner}); defer scope.leave();"
            if plan.reentry != "allow"
            else ""
        )
        call = f"host(state.value.context{''.join(', ' + expression for expression in converted)})"
        report = f'callback.reportError("{plan.native}", err);'
        if plan.result.ctype.kind == "void":
            action = f"{call} catch |err| {{ {report} return; }};"
        elif plan.status:
            action = f"{call} catch |err| {{ {report} return status.rawStatus(err); }}; return c.MLN_STATUS_OK;"
        else:
            action = f"const result = {call} catch |err| {{ {report} {fallback} }}; return {'result' if plan.result.kind == 'scalar' else 'result.toNative()'};"
        decision_setup = ""
        if plan.decision:
            decision = plan.decision
            raw_request = names[decision.parameter]
            request_type = values.public(
                next(p.value for p in plan.parameters if p.name == decision.parameter)
            )
            decision_setup = f"const request = try {request_type}.beginDecision({raw_request}); errdefer _ = request.finishDecision(false); "
            action = f"const result = {call} catch |err| {{ {report} return if (request.finishDecision(false)) c.{decision.accept} else c.{decision.pass_through}; }}; return if (request.finishDecision(result.toNative() == c.{decision.accept})) c.{decision.accept} else c.{decision.pass_through};"
        # Decode errors are contained at the C callback boundary.
        body = f"fn invoke({', '.join(names[p.name] + ': marshal.CallbackArg(c.' + plan.native + ', ' + str(i) + ')' for i, p in enumerate(plan.parameters))}) status.Error!marshal.CallbackResult(c.{plan.native}) {{ const state = {root_type}.get({names[plan.context]}); const host = state.value.{local} orelse {{ {fallback} }}; {scope} {decision_setup} "
        if any("allocator" in expression for expression in converted):
            body += "var arena = std.heap.ArenaAllocator.init(std.heap.smp_allocator); defer arena.deinit(); const allocator = arena.allocator(); "
        body += action + " }"
        body = body.replace("native_arg_", "callback_arg_")
        args = ", ".join(names[p.name] for p in plan.parameters)
        trampoline_signature = ", ".join(
            names[p.name]
            + ": marshal.CallbackArg(c."
            + plan.native
            + ", "
            + str(i)
            + ")"
            for i, p in enumerate(plan.parameters)
        )
        trampolines.append(
            f"    fn {trampoline}({trampoline_signature}) callconv(.c) marshal.CallbackResult(c.{plan.native}) {{ return struct {{ {body} }}.invoke({args}) catch |err| {{ {report} {fallback} }}; }}"
        )
    empty = " and ".join(
        f"self.{identifier(name)} == null" for name in registration.callbacks
    )
    writes.append(
        f"        if (!({empty})) {{ const retained = try roots.retain({public}, self); raw.{identifier(registration.user_data)} = retained; raw.{identifier(registration.release)} = {root_type}.releaseNative; }}"
    )
    return fields, writes, captures, trampolines
