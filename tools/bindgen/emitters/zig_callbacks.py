"""Generate retained callback descriptors from their resolved registration plans."""

import re

from ..model import ModelError
from ..names import camel
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
    constructors = []
    root_type = f"callbacks.Registration({public})"
    for field in value.fields:
        if field.name not in registration.callbacks:
            continue
        plan = values.bound.callbacks[field.value.native]
        local = identifier(field.name)
        names = {p.name: f"native_arg_{i}" for i, p in enumerate(plan.parameters)}
        parameters = list(plan.arguments)
        result = (
            "void"
            if plan.result.ctype.kind == "void" or plan.status
            else values.public(plan.result)
        )
        argument_types = [values.public(p.value) for p in parameters]
        fields.append(
            f"    {local}: ?*const fn (?*anyopaque{''.join(', ' + t for t in argument_types)}) anyerror!{result} = null,"
        )
        constructors.append((local, argument_types, result))
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
        owned = []
        transfer = ""
        for parameter in parameters:
            raw = names[parameter.name]
            if plan.decision and parameter.name == plan.decision.parameter:
                converted.append("request")
            elif (
                parameter.value.kind == "handle"
                and parameter.value.ownership == "owned"
            ):
                # Keep the handle until the host accepts ownership.
                adopted = "owned_" + parameter.name
                transfer = "ownership_transferred = true; "
                native_release = parameter.value.handle.release
                release = f"{camel(native_release.removeprefix('mln_'))}(&{adopted})"
                if (
                    values.bound.operations_by_name[
                        native_release
                    ].function.return_type.kind
                    != "void"
                ):
                    release += " catch {}"
                owned.append(
                    f"var {adopted} = {values.public(parameter.value)}.adopt({raw}); "
                    f"defer if (!ownership_transferred) {{ {release}; }}; "
                )
                converted.append(adopted)
            elif (
                parameter.value.kind == "reference" and parameter.value.element.response
            ):
                converted.append(f".{'{'} .native = @ptrCast({raw}) {'}'}")
            elif parameter.value.kind == "buffer":
                converted.append(borrow(parameter.value, raw))
            else:
                expression = decode(values, parameter.value, raw)
                for name, source in names.items():
                    expression = expression.replace("raw." + identifier(name), source)
                converted.append(expression)
        response = next(
            (names[p.name] for p in parameters if p.value.unwrapped.response), None
        )
        scope = (
            f"var scope: callbacks.Scope = .{{}}; scope.enter(@intFromPtr({response})); defer scope.leave();"
            if response
            else ""
        )
        call = f"host(state.value.context{''.join(', ' + expression for expression in converted)})"
        report = f'callbacks.reportError("{plan.native}", err);'
        if plan.result.ctype.kind == "void":
            action = f"try {call}; {transfer}"
        elif plan.status:
            action = f"{call} catch |err| {{ {report} return status.rawStatus(err); }}; {transfer}return c.MLN_STATUS_OK;"
        else:
            action = f"const result = {call} catch |err| {{ {report} {fallback} }}; {transfer}return {'result' if plan.result.kind == 'scalar' else 'result.toNative()'};"
        decision_setup = ""
        if plan.decision:
            decision = plan.decision
            raw_request = names[decision.parameter]
            request_type = values.public(
                next(p.value for p in plan.parameters if p.name == decision.parameter)
            )
            decision_setup = f"const request = {request_type}.adopt({raw_request}); "
            action = f"const result = {call} catch |err| {{ {report} {fallback} }}; {transfer}return result.toNative();"
        ownership_setup = "var ownership_transferred = false; " if owned else ""
        body = f"fn invoke({', '.join(names[p.name] + ': marshal.CallbackArg(c.' + plan.native + ', ' + str(i) + ')' for i, p in enumerate(plan.parameters))}) anyerror!marshal.CallbackResult(c.{plan.native}) {{ const state = {root_type}.get({names[plan.context]}); const host = state.value.{local} orelse {{ {fallback} }}; {scope} {decision_setup} {ownership_setup}{''.join(owned)}"
        if any("allocator" in expression for expression in converted):
            body += "var arena = std.heap.ArenaAllocator.init(std.heap.smp_allocator); defer arena.deinit(); const allocator = arena.allocator(); "
        used = set(
            re.findall(
                r"\bnative_arg_\d+\b",
                " ".join([names[plan.context], scope, decision_setup, *owned, action]),
            )
        )
        body += "".join(
            f"_ = {names[p.name]}; "
            for p in plan.parameters
            if names[p.name] not in used
        )
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
    trampolines.append(constructor(public, constructors))
    return fields, writes, captures, trampolines


def borrow(value, source):
    """Text that native lends for the callback's duration, without a copy."""
    if value.length != "nul" or value.buffer_form == "view":
        raise ModelError([f"Zig: {value.native}: callback text must be NUL-terminated"])
    if value.nullable or value.optional == "empty":
        return f"if ({source}) |text| std.mem.span(text) else null"
    return f"std.mem.span({source} orelse return error.NativeError)"


def constructor(public, callbacks):
    """`init`, which erases a typed context pointer for each callback."""
    parameters = "".join(f"comptime {local}: anytype, " for local, _, _ in callbacks)
    members = []
    for local, argument_types, result in callbacks:
        arguments = [
            f"argument_{index}: {typ}" for index, typ in enumerate(argument_types)
        ]
        forwarded = "".join(
            f", argument_{index}" for index in range(len(argument_types))
        )
        members.append(
            f".{local} = if (@TypeOf({local}) == @TypeOf(null)) null else struct {{ fn call(erased: ?*anyopaque{''.join(', ' + a for a in arguments)}) anyerror!{result} {{ return {local}(@as(Context, @ptrCast(@alignCast(erased.?))){forwarded}); }} }}.call,"
        )
    return f"""    /// Builds a `{public}` whose callbacks receive `context`, a pointer, as
    /// its own type. Each callback takes the context and then the arguments
    /// of its field, and may return any error; pass null to leave one unset.
    /// Native runs `release`, when it is not null, once it releases the
    /// registration.
    pub fn init(context: anytype, {parameters}comptime release: ?fn (@TypeOf(context)) void) {public} {{
        const Context = @TypeOf(context);
        if (@typeInfo(Context) != .pointer) @compileError("a {public} context must be a pointer");
        return .{{
            .context = @ptrCast(@constCast(context)),
            .release_context = if (release == null) null else struct {{ fn call(erased: ?*anyopaque) void {{ release.?(@as(Context, @ptrCast(@alignCast(erased.?)))); }} }}.call,
            {" ".join(members)}
        }};
    }}"""
