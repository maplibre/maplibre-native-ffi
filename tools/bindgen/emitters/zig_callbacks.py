"""Generate retained callback descriptors from their resolved registration plans."""

import re

from ..model import ModelError
from ..names import camel
from ..semantic import required_callbacks
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
    required = {f.name for f in required_callbacks(value)}
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
        summary = (
            "    /// An error that names a native status, such as\n"
            "    /// error.InvalidArgument, answers native that status. Any other\n"
            f"    /// error is logged and answers {plan.failure}.\n"
            if plan.status
            else ""
        )
        # Quoted identifiers cannot be extended into another identifier.
        trampoline = identifier(field.name + "Trampoline")
        typ = callback_type(values, field)
        if field.name in required:
            fields.append(f"{summary}    {local}: {typ},")
            captures.append(f"            .{local} = {local},")
            writes.append(f"        raw.{local} = {trampoline};")
        else:
            fields.append(f"{summary}    {local}: ?{typ} = null,")
            captures.append(f"            .{local} = null,")
            writes.append(
                f"        raw.{local} = if (self.{local} != null) {trampoline} else null;"
            )
        constructors.append((local, argument_types, result, field.name in required))
        fallback = (
            "return;"
            if plan.result.ctype.kind == "void"
            else f"return {failure_value(plan)};"
            if plan.failure not in {None, "contain"}
            else "return std.mem.zeroes(marshal.CallbackResult(c." + plan.native + "));"
        )
        converted = []
        owned = []
        transfer = ""
        for parameter in parameters:
            raw = names[parameter.name]
            if (
                parameter.value.kind == "handle"
                and parameter.value.ownership == "owned"
            ):
                # Keep the handle until the host accepts ownership.
                adopted = "owned_" + parameter.name
                transfer = "ownership_transferred = true; "
                native_release = parameter.value.handle.release
                release = f"{adopted}.{camel(values.bound.operations_by_name[native_release].member)}()"
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
            action = f"{call} catch |err| {{ if (status.rawStatus(err)) |deliberate| return deliberate; {report} return {failure_value(plan)}; }}; {transfer}return c.MLN_STATUS_OK;"
        else:
            action = f"const result = {call} catch |err| {{ {report} {fallback} }}; {transfer}return {'result' if plan.result.kind == 'scalar' else 'result.toNative()'};"
        ownership_setup = "var ownership_transferred = false; " if owned else ""
        body = f"fn invoke({', '.join(names[p.name] + ': marshal.CallbackArg(c.' + plan.native + ', ' + str(i) + ')' for i, p in enumerate(plan.parameters))}) anyerror!marshal.CallbackResult(c.{plan.native}) {{ const state = {root_type}.get({names[plan.context]}); const host = state.value.{local}{'' if field.name in required else f' orelse {{ {fallback} }}'}; {scope} {ownership_setup}{''.join(owned)}"
        if any("allocator" in expression for expression in converted):
            body += "var arena = std.heap.ArenaAllocator.init(std.heap.smp_allocator); defer arena.deinit(); const allocator = arena.allocator(); "
        used = set(
            re.findall(
                r"\bnative_arg_\d+\b",
                " ".join([names[plan.context], scope, *owned, action]),
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
    root = f"const retained = try roots.retain({public}, self); raw.{identifier(registration.user_data)} = retained; raw.{identifier(registration.release)} = {root_type}.releaseNative;"
    if required:
        writes.append(f"        {root}")
    else:
        empty = " and ".join(
            f"self.{identifier(name)} == null" for name in registration.callbacks
        )
        writes.append(f"        if (!({empty})) {{ {root} }}")
    trampolines.append(constructor(public, constructors))
    return fields, writes, captures, trampolines


# Words that a Zig identifier cannot spell without `@"..."`: keywords and
# primitive values and types.
RESERVED = frozenset(
    {
        "addrspace",
        "align",
        "allowzero",
        "and",
        "anyframe",
        "anytype",
        "asm",
        "break",
        "callconv",
        "catch",
        "comptime",
        "const",
        "continue",
        "defer",
        "else",
        "enum",
        "errdefer",
        "error",
        "export",
        "extern",
        "fn",
        "for",
        "if",
        "inline",
        "linksection",
        "noalias",
        "noinline",
        "nosuspend",
        "opaque",
        "or",
        "orelse",
        "packed",
        "pub",
        "resume",
        "return",
        "struct",
        "suspend",
        "switch",
        "test",
        "threadlocal",
        "try",
        "union",
        "unreachable",
        "var",
        "volatile",
        "while",
        "anyerror",
        "anyopaque",
        "bool",
        "comptime_float",
        "comptime_int",
        "false",
        "noreturn",
        "null",
        "true",
        "type",
        "undefined",
        "void",
        "isize",
        "usize",
        "c_char",
        "c_short",
        "c_ushort",
        "c_int",
        "c_uint",
        "c_long",
        "c_ulong",
        "c_longlong",
        "c_ulonglong",
        "c_longdouble",
        "f16",
        "f32",
        "f64",
        "f80",
        "f128",
    }
)


def callback_type(values, field):
    """The function pointer type of a registration's callback field."""
    plan = values.bound.callbacks[field.value.native]
    result = (
        "void"
        if plan.result.ctype.kind == "void" or plan.status
        else values.public(plan.result)
    )
    named = "".join(
        f", {parameter_name(p.name)}: {values.public(p.value)}" for p in plan.arguments
    )
    return f"*const fn (context: ?*anyopaque{named}) anyerror!{result}"


def parameter_name(name):
    """A callback argument's name in a function type: its C name, quoted only
    when Zig reserves it."""
    if name in RESERVED or re.fullmatch(r"[iu]\d+", name):
        return identifier(name)
    return name


def failure_value(plan):
    """The Zig value of a callback's `failure=`: a literal, such as a scalar
    result's integer or boolean, or else a C constant."""
    failure = plan.failure
    if plan.result.kind == "scalar" or failure.lstrip("-").isdigit():
        return failure
    return "c." + failure


def borrow(value, source):
    """Text that native lends for the callback's duration, without a copy."""
    if value.buffer_form == "view":
        return f"try marshal.borrowView({source})"
    if value.length != "nul":
        raise ModelError([f"Zig: {value.native}: callback text must be NUL-terminated"])
    if value.nullable:
        return f"if ({source}) |text| std.mem.span(text) else null"
    return f"std.mem.span({source} orelse return error.NativeError)"


def constructor(public, callbacks):
    """`init`, which erases a typed context pointer for each callback."""
    parameters = "".join(f"comptime {local}: anytype, " for local, *_ in callbacks)
    members = []
    for local, argument_types, result, required in callbacks:
        arguments = [
            f"argument_{index}: {typ}" for index, typ in enumerate(argument_types)
        ]
        forwarded = "".join(
            f", argument_{index}" for index in range(len(argument_types))
        )
        unset = "" if required else f"if (@TypeOf({local}) == @TypeOf(null)) null else "
        members.append(
            f".{local} = {unset}struct {{ fn call(erased: ?*anyopaque{''.join(', ' + a for a in arguments)}) anyerror!{result} {{ return {local}(@as(Context, @ptrCast(@alignCast(erased.?))){forwarded}); }} }}.call,"
        )
    unset = (
        "pass null to leave an optional one unset"
        if any(required for *_, required in callbacks)
        else "pass null to leave one unset"
    )
    return f"""    /// Builds a `{public}` whose callbacks receive `context`, a pointer, as
    /// its own type. Each callback takes the context and then the arguments
    /// of its field, and may return any error; {unset}.
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
