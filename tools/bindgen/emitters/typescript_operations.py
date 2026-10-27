"""Lower each TypeScript operation, and render the owner classes, top-level
functions, and addon interface.

Each operation lowers by its shape: the arguments convert alike, and the shape
decides how the call completes and what it returns.

Every generated method calls one runtime helper for its execution shape:
`run`, `command`, `query`, and `attach` for completions,
`closeSync` and `closeAsync` for a release, `answerSync` for an answer that
consumes its receiver, `adopt` for a handle that native
code transfers to a new owner, and `ViewScope` for a borrowed view. A call
passes the raw handle ID, and native code reports a released one.

The delivery table tells the realm how each drained delivery reaches the host
value that a registration rooted: which callback runs, with which copied
arguments, and which handle the host takes from the batch first.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import assert_never

from ..model import Parameter
from ..names import type_name
from ..semantic import (
    BoundApi,
    OperationPlan,
    OwnedOutputPlan,
    ValuePlan,
    output_member,
)
from .typescript import Lowered, attachment_name, owner_native, scoped_name
from .typescript_native_values import NativeValues
from .typescript_values import (
    OWNER_MEMBERS,
    Unsupported,
    Values,
    doc,
    identifier,
    is_counted,
    owner_class,
    parameter_name,
    public_name,
    registration,
)


@dataclass
class Call:
    """An operation whose arguments are converted, before its shape lowers
    the call itself."""

    lowered: Lowered
    # The C arguments, in order, without the diagnostic.
    args: list[str]
    # The position of the next JavaScript argument.
    index: int
    # The outputs that the call returns, with the value that each holds.
    returned: list[tuple[str, ValuePlan]]
    # The output whose true value reports that native kept no registration.
    declined: str | None


def simple_disposer(bound: BoundApi, native: str | None) -> bool:
    """Whether a handle's disposal is an immediate call on the handle alone,
    passing no completion when the dispose takes an optional one."""
    if not native:
        return False
    plan = bound.operations_by_name.get(native)
    optional = bool(plan and plan.completion and plan.completion.optional)
    return bool(
        plan
        and (not plan.completion or optional)
        and len(plan.function.parameters) == 1 + optional
        and plan.consumes
    )


def check_owner(bound: BoundApi, owned: OwnedOutputPlan) -> None:
    if owned.handle.native not in bound.public_handles:
        raise Unsupported("owned output requires a generated class")
    if not simple_disposer(bound, owned.handle.cleanup):
        raise Unsupported("owned output requires an immediate disposer")


def scalar_local(ctype: str) -> str:
    return ctype.removeprefix("const ")


def lower_arguments(plan: OperationPlan, values: Values, natives: NativeValues) -> Call:
    """Converts the receiver and each argument, and names the operation."""
    function = plan.function
    bound = values.bound
    if plan.scoped_receiver:
        raise Unsupported("a scoped response needs a callback on its native thread")
    if len(plan.registrations) and not plan.status:
        raise Unsupported("registration requires an admission status")
    receiver, receiver_reference = None, False
    if plan.receiver:
        first = function.parameters[0]
        if first.type.pointee is not None:
            receiver = type_name(first.type.pointee)
            receiver_reference = True
        else:
            receiver = type_name(first.type)
        if receiver not in bound.public_handles:
            receiver, receiver_reference = None, False
    if receiver_reference and not plan.consumes:
        raise Unsupported("mutable handle receiver requires consumption")
    if plan.factory and plan.factory not in bound.public_handles:
        raise Unsupported("factory requires a generated class")
    handle_plan = bound.handles.get(receiver) if receiver else None
    method = identifier(plan.member)
    if (
        plan.consumes
        and handle_plan
        and plan.name == handle_plan.release
        and not handle_plan.release_inputs
    ):
        method = "close"
    elif receiver and method in OWNER_MEMBERS:
        # The owner already has these members, and the release is close().
        raise Unsupported(f"method name {method} is reserved by the owner class")
    lowered = Lowered(plan, receiver, method)
    lowered.registrations = bool(plan.registrations)
    declined = next(
        (r.accepted_unless for r in plan.registrations if r.accepted_unless), None
    )
    call = Call(lowered, [], 0, [], declined)
    if receiver:
        lower_receiver(call, receiver_reference)
    for parameter in function.parameters[1 if receiver else 0 :]:
        lower_parameter(call, parameter, values, natives)
    # A parameter with a default must not precede one without.
    seen_required = False
    for position in range(len(lowered.parameters) - 1, -1, -1):
        name, ts, default = lowered.parameters[position]
        if default is None:
            seen_required = True
        elif seen_required and default == "{}":
            lowered.parameters[position] = (name, ts, None)
        elif seen_required:
            lowered.parameters[position] = (name, ts + " | undefined", None)
    return call


def lower_receiver(call: Call, reference: bool) -> None:
    """Reads the receiver's handle, and the owner token of a receiver that the
    call consumes."""
    lowered = call.lowered
    plan = lowered.plan
    receiver = lowered.owner
    locals_, body, args = lowered.c_locals, lowered.c_body, call.args
    name = plan.function.parameters[0].name
    local = f"arg_{name}"
    locals_.append(f"{receiver} {local} = 0;")
    body.append(
        f'{{ uint64_t number = 0; if (!mln_ts_read_u64(call, call->argv[{call.index}], &number, "{identifier(name)}")) goto done; {local} = ({receiver})number; }}'
    )
    args.append(f"&{local}" if reference else local)
    lowered.native_parameters.append((identifier(name), "bigint"))
    lowered.arguments.append("handle" if plan.consumes else "this.id")
    call.index += 1
    if plan.consumes:
        # The call that consumes the handle ends its owner's token, so no
        # realm teardown can finalize the consumed handle. A handle that has
        # no owner yet passes no token.
        body.append(
            f"if (!mln_ts_read_owner(call, call->argv[{call.index}])) goto done;"
        )
        lowered.native_parameters.append(("token", "object | undefined"))
        lowered.arguments.append("token")
        call.index += 1


def lower_parameter(
    call: Call, parameter: Parameter, values: Values, natives: NativeValues
) -> None:
    """Converts one argument after the receiver."""
    plan = call.lowered.plan
    locals_, body, args = call.lowered.c_locals, call.lowered.c_body, call.args
    # Counts that a sized input carries, by count parameter.
    counts = {
        p.value.length: p.name
        for p in plan.inputs
        if p.value.kind == "array" or is_counted(p.value)
    }
    # Caller-sized text starts in call memory of a guessed size, and the call
    # runs again with enough memory when the text did not fit.
    text = plan.caller_sized_text
    local = f"arg_{parameter.name}"
    if plan.completion and parameter.name == plan.completion.parameter:
        locals_.append(f"mln_completion {local};")
        locals_.append(f"const mln_completion* present_{parameter.name} = NULL;")
        args.append(f"present_{parameter.name}")
        return
    if parameter.name in counts:
        args.append(f"({parameter.type.spelling})count_{counts[parameter.name]}")
        return
    if text and parameter.name == text.name:
        locals_.append(f"char* {local} = NULL;")
        body.append(f"{local} = mln_ts_allocate(call, arg_{text.value.length});")
        body.append(f"if ({local} == NULL) goto done;")
        args.append(local)
        return
    if text and parameter.name == text.value.length:
        locals_.append(f"size_t {local} = 256;")
        args.append(local)
        return
    if text and parameter.name == text.result_length:
        locals_.append(f"size_t {local} = 0;")
        args.append(f"&{local}")
        return
    if parameter.name in {p.name for p in plan.outputs}:
        lower_output(call, parameter, values, natives)
    else:
        lower_input(call, parameter, values, natives)


def lower_output(
    call: Call, parameter: Parameter, values: Values, natives: NativeValues
) -> None:
    """Gives an output native storage that the call writes."""
    lowered = call.lowered
    plan = lowered.plan
    bound = values.bound
    outputs = {p.name: p for p in plan.outputs}
    locals_, body, args = lowered.c_locals, lowered.c_body, call.args
    local = f"arg_{parameter.name}"
    value = outputs[parameter.name].value
    if plan.drain and parameter.name == plan.drain.batch.name:
        # The call releases the batch once it copies the slice.
        handle = value.unwrapped.handle
        assert handle is not None
        release = bound.source.functions_by_name[handle.release]
        if release.diagnostic or release.return_type.kind != "void":
            raise Unsupported("a drained batch needs a void release")
        locals_.append(f"{handle.native} {local} = MLN_HANDLE_NULL;")
        args.append(f"&{local}")
        lowered.drained = f"{handle.release}({local});"
        return
    owned = next((o for o in plan.owned_outputs if o.parameter == parameter.name), None)
    if owned:
        check_owner(bound, owned)
        lowered.owned = owned
        locals_.append(f"{owned.handle.native} {local} = MLN_HANDLE_NULL;")
        args.append(f"&{local}")
        return
    if value.kind == "native_pointer":
        locals_.append(f"void* {local} = NULL;")
        args.append(f"&{local}")
        call.returned.append((parameter.name, value))
        return
    sized_by = outputs[parameter.name].sized_by
    if sized_by:
        # One output element per element of the input array, in call
        # memory that the input list's count bounds.
        element = value.element
        assert element is not None
        values.check(element, "out")
        count = f"count_{sized_by}"
        locals_.append(f"{element.native}* {local} = NULL;")
        body.append(f"{local} = mln_ts_allocate(call, sizeof *{local} * {count});")
        body.append(f"if ({local} == NULL) goto done;")
        init = natives.init(element, f"{local}[index]")
        if init[1:] or not init[0].startswith("memset("):
            body.append(f"for (size_t index = 0; index < {count}; index++) {{")
            body += [f"  {line}" for line in init]
            body.append("}")
        args.append(local)
        call.returned.append((parameter.name, value))
        return
    if value.kind != "reference" or value.element is None:
        raise Unsupported("output needs one initialized native storage value")
    element = value.element
    values.check(element, "out")
    if element.kind == "record":
        locals_.append(f"{element.native} {local};")
        body.extend(natives.init(element, local))
    else:
        assert parameter.type.pointee is not None
        locals_.append(f"{scalar_local(parameter.type.pointee.spelling)} {local};")
        body.append(f"memset(&{local}, 0, sizeof {local});")
    args.append(f"&{local}")
    if parameter.name == call.declined:
        lowered.declined = True
    call.returned.append((parameter.name, element))


def lower_input(
    call: Call, parameter: Parameter, values: Values, natives: NativeValues
) -> None:
    """Reads an input from its JavaScript argument."""
    lowered = call.lowered
    plan = lowered.plan
    function = plan.function
    bound = values.bound
    inputs = {p.name: p for p in plan.inputs}
    locals_, body, args = lowered.c_locals, lowered.c_body, call.args
    local = f"arg_{parameter.name}"
    public_name_ = parameter_name(parameter.name)
    if parameter.name not in inputs:
        raise Unsupported("parameter missing a resolved input contract")
    value = inputs[parameter.name].value
    js = f"call->argv[{call.index}]"
    path = f'"{public_name_}"'
    call.index += 1
    lowered.arguments.append(
        f"{public_name_}.id" if value.kind == "handle" else public_name_
    )
    if value.kind == "handle":
        values.check(value, "in")
        locals_.append(f"uint64_t {local} = 0;")
        body.append(f"if (!mln_ts_read_u64(call, {js}, &{local}, {path})) goto done;")
        args.append(local)
        lowered.parameters.append((public_name_, values.ts(value), None))
        lowered.native_parameters.append((public_name_, "bigint"))
        return
    if value.kind == "array" or is_counted(value):
        values.check(value, "in")
        count_parameter = next(p for p in function.parameters if p.name == value.length)
        count_value = inputs.get(count_parameter.name)
        count_value = (
            count_value.value
            if count_value
            else bound.values.get(type_name(count_parameter.type))
        )
        if count_value is None:
            raise Unsupported("sized input needs a count parameter plan")
        locals_.append(f"size_t count_{parameter.name} = 0;")
        if value.kind == "array":
            assert value.element is not None
            locals_.append(f"{value.element.native}* {local} = NULL;")
            read = natives.read_list(
                value,
                js,
                local,
                f"count_{parameter.name}",
                count_value,
                path,
                "goto done",
            )
            args.append(local)
        else:
            locals_.append(f"const void* {local} = NULL;")
            read = natives.read_sized(
                value,
                js,
                local,
                f"count_{parameter.name}",
                count_value,
                path,
                "goto done",
            )
            args.append(f"({parameter.type.spelling}){local}")
        ts = values.ts(value)
        if value.nullable:
            body.append(f"if (!mln_ts_is_absent(call, {js})) {{")
            body += [f"  {line}" for line in read]
            body.append("}")
            lowered.parameters.append((public_name_, ts, "undefined"))
            lowered.native_parameters.append((public_name_, ts + " | undefined"))
        else:
            body += read
            lowered.parameters.append((public_name_, ts, None))
            lowered.native_parameters.append((public_name_, ts))
        return
    if value.kind == "reference":
        element = value.unwrapped
        values.check(element, "in")
        if element.kind in {"record", "union"}:
            locals_.append(f"{element.native} {local};")
            read = natives.init(element, local) + natives.read(
                element, js, local, path, "goto done"
            )
        else:
            assert parameter.type.pointee is not None
            ctype = scalar_local(parameter.type.pointee.spelling)
            locals_.append(f"{ctype} {local};")
            read = [f"memset(&{local}, 0, sizeof {local});"] + natives.read(
                element, js, local, path, "goto done"
            )
        registration_value = element.kind == "record" and element.registration
        nullable = value.nullable or (registration_value and values.absent(element))
        ts = values.ts(element)
        if value.nullable:
            locals_.append(f"const void* present_{parameter.name} = NULL;")
            body.append(f"if (!mln_ts_is_absent(call, {js})) {{")
            body += [f"  {line}" for line in read]
            body.append(f"  present_{parameter.name} = &{local};")
            body.append("}")
            args.append(f"present_{parameter.name}")
        elif nullable:
            # An absent registration passes its zero default unset.
            body += read[: len(natives.init(element, local))]
            body.append(f"if (!mln_ts_is_absent(call, {js})) {{")
            body += [f"  {line}" for line in read[len(natives.init(element, local)) :]]
            body.append("}")
            args.append(f"&{local}")
        else:
            body += read
            args.append(f"&{local}")
        optional = (
            not registration_value
            and not value.nullable
            and values.all_omittable(element)
        )
        lowered.native_parameters.append(
            (public_name_, ts + (" | undefined" if nullable or optional else ""))
        )
        lowered.parameters.append(
            (
                public_name_,
                ts,
                "undefined" if nullable else ("{}" if optional else None),
            )
        )
        return
    values.check(value, "in")
    ctype = parameter.type.spelling
    if value.kind == "buffer" and value.buffer_form == "view":
        locals_.append(f"mln_buffer_view {local} = {{0}};")
    elif value.kind == "buffer":
        locals_.append(f"const char* {local} = NULL;")
    elif value.kind == "record":
        locals_.append(f"{value.native} {local};")
        body += natives.init(value, local)
    elif value.kind == "native_pointer":
        locals_.append(f"{ctype} {local} = NULL;")
    else:
        locals_.append(f"{scalar_local(ctype)} {local};")
        body.append(f"memset(&{local}, 0, sizeof {local});")
    read = natives.read(value, js, local, path, "goto done")
    absent = values.absent(value) or (
        value.kind == "record" and values.all_omittable(value)
    )
    if absent:
        body.append(f"if (!mln_ts_is_absent(call, {js})) {{")
        body += [f"  {line}" for line in read]
        body.append("}")
    else:
        body += read
    args.append(local)
    ts = values.ts(value)
    lowered.native_parameters.append(
        (public_name_, ts + (" | undefined" if absent else ""))
    )
    lowered.parameters.append(
        (
            public_name_,
            ts,
            "undefined" if values.absent(value) else ("{}" if absent else None),
        )
    )


def lower_function(
    plan: OperationPlan, values: Values, natives: NativeValues
) -> Lowered:
    call = lower_arguments(plan, values, natives)
    match plan.shape:
        case "discard" | "release" | "answer":
            return lower_consuming(call, values, natives)
        case "release_async":
            return lower_release_async(call, values, natives)
        case "thread_start":
            return lower_thread_start(call, values.bound)
        case "attach":
            return lower_attach(call, values, natives)
        case "command":
            return lower_command(call, values, natives)
        case "query":
            return lower_query(call, values, natives)
        case "run":
            return lower_run(call, values, natives)
        case "view":
            return lower_view(call, values, natives)
        case "drain":
            return lower_drain(call, values, natives)
        case "owned":
            return lower_owned(call, values, natives)
        case "call":
            # Any other immediate call returns its outputs or its result.
            publish(call, values, natives, invoke(call, values))
            return call.lowered
        case unreachable:
            assert_never(unreachable)


def lower_consuming(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A release, a discard, or an answer: an immediate call that consumes its
    receiver and returns nothing."""
    lowered = call.lowered
    if lowered.plan.blocks:
        # Only a call without a receiver waits on a thread of its own.
        raise Unsupported(
            f"blocks={lowered.plan.blocks} takes no receiver or registration and returns one owner"
        )
    publish(call, values, natives, invoke(call, values))
    # An answer that consumes its receiver is no release, so no disposer runs
    # it and a repeated one reaches native.
    shape = "answer" if lowered.plan.answers else "close"
    consume(lowered, None if lowered.owned or lowered.returns != "void" else shape)
    return lowered


def lower_release_async(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A release that completes later, which closes its owner once its
    completion reports success. A completion that reports more rejects."""
    plan = call.lowered.plan
    assert plan.completion is not None
    if plan.completion.immediate_owners:
        return lower_attach(call, values, natives)
    if plan.execution == "command":
        return lower_command(call, values, natives)
    if plan.result:
        return lower_query(call, values, natives)
    return lower_run(call, values, natives)


def lower_attach(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A completion that readies the owner that the call creates at once, so
    the addon returns the owner's handle."""
    lowered = call.lowered
    plan = lowered.plan
    assert plan.completion is not None and lowered.owned is not None
    submit(call, values, natives)
    if len(plan.completion.immediate_owners) != 1 or plan.result or call.returned:
        raise Unsupported("attachment requires one immediate owner and void readiness")
    lowered.shape, lowered.returns = "attach", attachment_name(lowered.owned)
    publish(call, values, natives, invoke(call, values))
    consume(lowered, None)
    return lowered


def lower_command(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A completion that reports a disposition."""
    lowered = call.lowered
    submit(call, values, natives)
    if lowered.plan.result:
        raise Unsupported("command requires a resultless receipt")
    lowered.shape, lowered.returns = "command", "Promise<CommandCompletion>"
    settle(call, values, natives)
    consume(lowered, None)
    return lowered


def lower_query(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A completion that delivers a result, which the decoder that the addon
    records converts."""
    lowered = call.lowered
    result = lowered.plan.result
    assert result is not None
    submit(call, values, natives, result)
    ts = values.ts(result)
    if result.nullable and ts != "unknown":
        ts += " | undefined"
    lowered.shape, lowered.returns = "query", f"Promise<{ts}>"
    settle(call, values, natives)
    consume(lowered, None)
    return lowered


def lower_run(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """Any other completion, which reports only success."""
    lowered = call.lowered
    submit(call, values, natives)
    lowered.shape, lowered.returns = "run", "Promise<void>"
    settle(call, values, natives)
    consume(lowered, "close_async")
    return lowered


def lower_view(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A call inside its owner's borrowed view scope."""
    lowered = call.lowered
    view = lowered.plan.view
    assert view is not None
    publish(call, values, natives, invoke(call, values))
    [(_, value)] = call.returned
    if value.kind != "record":
        raise Unsupported("borrowed view requires a record output")
    # Read as with<Stem>: the call runs inside the view scope, and its value
    # is readable only while the callback runs.
    lowered.view = value.native
    lowered.shape = "view"
    lowered.method = identifier("with_" + view.stem)
    lowered.parameters = [("use", f"(view: {scoped_name(value.native)}) => T", None)]
    lowered.returns = "T"
    return lowered


def lower_drain(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """A call that publishes a batch and a slice view of it, which returns a
    copy of the slice."""
    lowered = call.lowered
    assert lowered.drained is not None
    publish(call, values, natives, invoke(call, values))
    # The batch is released whether or not its copy succeeded.
    lowered.c_body.append(lowered.drained)
    lowered.shape = "drain"
    return lowered


def lower_owned(call: Call, values: Values, natives: NativeValues) -> Lowered:
    """An immediate call that creates an owner, which adopts the handle."""
    lowered = call.lowered
    assert lowered.owned is not None
    lowered.shape = "owned"
    lowered.returns = owner_class(lowered.owned.handle)
    publish(call, values, natives, invoke(call, values))
    return lowered


def submit(
    call: Call, values: Values, natives: NativeValues, result: ValuePlan | None = None
) -> None:
    """Reads the completion token that settles the call's Promise, with the
    decoder that converts its `result`."""
    lowered = call.lowered
    plan = lowered.plan
    completion = plan.completion
    assert completion is not None
    if not plan.status:
        raise Unsupported("asynchronous operation requires admission status")
    if result is not None:
        values.check(result, "out")
        lowered.decoder = natives.decoder(result)
    local = f"arg_{completion.parameter}"
    token = f"call->argv[{call.index}]"
    nullable = completion.discardable or completion.optional
    lowered.native_parameters.append(
        ("completion", "number | undefined" if nullable else "number")
    )
    lowered.discardable = completion.discardable and lowered.owner is not None
    decoder = natives.decoder_id(lowered.decoder)
    lowered.c_body.append(
        f"if (!mln_ts_read_completion(call, {token}, {'true' if nullable else 'false'}, {decoder}, &{local}, &present_{completion.parameter})) goto done;"
    )


def settle(call: Call, values: Values, natives: NativeValues) -> None:
    """Calls a function whose completion settles its Promise, so the addon
    returns nothing."""
    publish(call, values, natives, invoke(call, values))
    call.lowered.native_result = "void"


def consume(lowered: Lowered, shape: str | None) -> None:
    """Lowers a call that consumes its receiver as `shape`, which None
    rejects."""
    if not lowered.plan.consumes:
        return
    if not lowered.owner:
        raise Unsupported("consumption requires an owned receiver")
    if shape is None:
        raise Unsupported("consumption requires an owned receiver without outputs")
    lowered.shape = shape


def invoke(call: Call, values: Values) -> ValuePlan | None:
    """Calls the C function, accepts the registrations, consumes the
    receiver, and checks the status. Returns the value that the function
    returns, if any."""
    lowered = call.lowered
    plan = lowered.plan
    function = plan.function
    status = plan.status
    text = plan.caller_sized_text
    locals_, body = lowered.c_locals, lowered.c_body
    call_arguments = ", ".join(
        call.args + (["&diagnostic"] if function.diagnostic else [])
    )
    if function.diagnostic:
        locals_.append("mln_diagnostic diagnostic;")
        body.append("diagnostic.size = sizeof diagnostic;")
        body.append("diagnostic.message[0] = '\\0';")
    result_value = None
    if status and text:
        storage, capacity, length = (
            f"arg_{text.name}",
            f"arg_{text.value.length}",
            f"arg_{text.result_length}",
        )
        locals_.append("mln_status status;")
        body += [
            "for (;;) {",
            f"  status = {function.name}({call_arguments});",
            f"  if (status != MLN_STATUS_OK || {length} < {capacity}) break;",
            f"  {capacity} = {length} + 1;",
            f"  {storage} = mln_ts_allocate(call, {capacity});",
            f"  if ({storage} == NULL) goto done;",
            "}",
        ]
    elif status:
        locals_.append("mln_status status;")
        body.append(f"status = {function.name}({call_arguments});")
    elif function.return_type.kind == "void":
        body.append(f"{function.name}({call_arguments});")
    else:
        if plan.result is None:
            raise Unsupported("immediate result needs a resolved value")
        values.check(plan.result, "out")
        locals_.append(f"{function.return_type.spelling} result;")
        body.append(f"result = {function.name}({call_arguments});")
        result_value = plan.result
    if status and plan.registrations:
        accepted = "status == MLN_STATUS_OK"
        if lowered.declined:
            accepted += f" && !arg_{call.declined}"
        body.append(f"if ({accepted}) mln_ts_accept(call);")
    if plan.consumes == "always":
        body.append("mln_ts_consume(call);")
    elif plan.consumes:
        if not status:
            raise Unsupported("consumption on success requires a status")
        body.append("if (status == MLN_STATUS_OK) mln_ts_consume(call);")
    if plan.absence:
        if not status:
            raise Unsupported("absence requires a status")
        body.append(f"if (status == {plan.absence.status}) goto done;")
        lowered.absent = True
    if status:
        body.append("if (!mln_ts_check(call, status, &diagnostic)) goto done;")
    return result_value


def publish(
    call: Call, values: Values, natives: NativeValues, result_value: ValuePlan | None
) -> None:
    """Sets the addon's result: the handle of the owner that the call
    creates, its outputs, or the value that the function returns. A status
    that reports the output absent returns undefined."""
    lowered = call.lowered
    text = lowered.plan.caller_sized_text
    returned = call.returned
    body = lowered.c_body
    sized_by = {p.name: p.sized_by for p in lowered.plan.outputs}

    def output_count(name):
        return f"count_{sized_by[name]}" if sized_by[name] else None

    if lowered.owned:
        if returned:
            raise Unsupported("an owned output requires no other output")
        body.append(
            f"call->result = mln_ts_u64(env, (uint64_t)arg_{lowered.owned.parameter});"
        )
        lowered.native_result = "bigint"
    elif text:
        json = text.value.encoding == "json"
        body.append(
            f"call->result = {'mln_ts_json' if json else 'mln_ts_text'}(env, arg_{text.name}, arg_{text.result_length});"
        )
        lowered.native_result = lowered.returns = "unknown" if json else "string"
    elif len(returned) > 1:
        members = []
        body.append("call->result = mln_ts_object(env);")
        for name, value in returned:
            key = identifier(output_member(name))
            body.append(
                f'if (!mln_ts_set(env, call->result, "{key}", {natives.to_js(value, f"arg_{name}", output_count(name))})) goto done;'
            )
            members.append(f"readonly {key}: {values.ts(value)}")
        lowered.native_result = lowered.returns = "{ " + "; ".join(members) + " }"
    elif returned:
        name, value = returned[0]
        body.append(
            f"call->result = {natives.to_js(value, f'arg_{name}', output_count(name))};"
        )
        lowered.native_result = lowered.returns = values.ts(value)
    elif result_value is not None:
        body.append(f"call->result = {natives.to_js(result_value, 'result')};")
        lowered.native_result = lowered.returns = values.ts(result_value)
    if lowered.absent:
        lowered.native_result += " | undefined"
        if not lowered.drained:
            lowered.returns += " | undefined"


def lower_thread_start(call: Call, bound: BoundApi) -> Lowered:
    """A call that waits for a thread to start, which runs on a thread of its own.

    A thread that a browser page starts runs only after the page yields, so
    the page cannot wait for one. The export copies the converted arguments,
    with the call memory that they point into, and returns at once; the call's
    thread creates the owner and settles the Promise through the realm.
    """
    lowered = call.lowered
    plan = lowered.plan
    function = plan.function
    owned = lowered.owned
    if (
        lowered.owner
        or plan.completion
        or plan.registrations
        or plan.absence
        or not plan.status
        or not function.diagnostic
        or not owned
        or len(plan.outputs) != 1
    ):
        raise Unsupported(
            f"blocks={plan.blocks} takes no receiver or registration and returns one owner"
        )
    types = {}
    for declaration in lowered.c_locals:
        match = re.fullmatch(r"(.+?) (\w+)(?: = .*)?;", declaration)
        if match:
            types[match.group(2)] = match.group(1)
    output = f"&arg_{owned.parameter}"
    members, run_arguments = [], []
    for argument in call.args:
        name = argument.removeprefix("&")
        if argument == output:
            run_arguments.append("&handle")
        elif re.fullmatch(r"arg_\w+", name) and name in types:
            members.append(name)
            run_arguments.append(argument.replace(name, f"arguments->{name}"))
        else:
            raise Unsupported(f"blocks={plan.blocks} copies only converted arguments")
    name = plan.name
    structure = f"mln_ts_{name}_arguments"
    native = owned.handle.native
    assert owned.handle.cleanup is not None
    cleanup = bound.source.functions_by_name[owned.handle.cleanup]
    # A dispose takes the handle, then nulls for an optional completion and a
    # diagnostic, which simple_disposer() allows.
    extra = len(cleanup.parameters) - 1 + bool(cleanup.diagnostic)
    dispose = ", ".join([f"({native})handle", *["NULL"] * extra])
    fields = "".join(f"  {types[member]} {member};\n" for member in members)
    lowered.c_helpers = (
        f"typedef struct {structure} {{\n{fields}}} {structure};\n\n"
        f"static mln_status mln_ts_run_{name}(\n"
        "  const void* data, uint64_t* out_handle, mln_diagnostic* diagnostic\n) {\n"
        f"  const {structure}* arguments = data;\n"
        f"  {native} handle = MLN_HANDLE_NULL;\n"
        f"  mln_status status = {function.name}({', '.join([*run_arguments, 'diagnostic'])});\n"
        "  *out_handle = (uint64_t)handle;\n"
        "  return status;\n}\n\n"
        f"static void mln_ts_dispose_{name}(uint64_t handle) {{\n"
        f"  {cleanup.name}({dispose});\n}}\n"
    )
    # The thread writes the handle, so the export keeps no output.
    lowered.c_locals = [
        declaration
        for declaration in lowered.c_locals
        if not declaration.startswith(f"{native} {output.removeprefix('&')} ")
    ]
    lowered.c_locals.append(f"{structure} arguments;")
    lowered.c_body += [
        *[f"arguments.{member} = {member};" for member in members],
        (
            f"mln_ts_start_thread(call, call->argv[{call.index}], mln_ts_run_{name}, "
            f"mln_ts_dispose_{name}, &arguments, sizeof arguments);"
        ),
    ]
    lowered.native_parameters.append(("start", "number"))
    lowered.shape = "thread_start"
    lowered.returns = f"Promise<{owner_class(owned.handle)}>"
    return lowered


HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"
# What a call that waits for a thread to start does in TypeScript, in place of
# the C notes on blocking.
THREAD_START_NOTE = (
    "Resolves once the call completes, so a browser page, which cannot block, "
    "can call it."
)


def signature(entry: Lowered) -> str:
    parts = []
    for name, ts, default in entry.parameters:
        if default == "undefined":
            parts.append(f"{name}?: {ts}")
        elif default:
            parts.append(f"{name}: {ts} = {default}")
        else:
            parts.append(f"{name}: {ts}")
    return ", ".join(parts)


def construct(bound, entry: Lowered, owned, handle_expression: str) -> str:
    """Constructs the owner of an adopted handle, or disposes the handle."""
    public = owner_class(owned.handle)
    arguments = ["ADOPT", handle_expression]
    if owned.handle.parent in bound.public_handles:
        if not owned.parent_parameter:
            raise Unsupported("owned output requires its parent owner")
        first = entry.plan.function.parameters[0].name
        arguments.append(
            "this"
            if entry.owner and first == owned.parent_parameter
            else parameter_name(owned.parent_parameter)
        )
    if entry.registrations:
        arguments.append("registrations")
    return f"adopt({handle_expression}, () => new {public}({', '.join(arguments)}), {disposer(owned.handle)})"


def disposer(handle) -> str:
    # The handle has no owner yet, so the disposal passes no owner token, and
    # it passes no completion to a dispose that takes one.
    completion = ", undefined" if handle.dispose_completion else ""
    return f"(disposed) => native.{handle.cleanup}(disposed, undefined{completion})"


def render_dispose(bound, handle, indent: str) -> str:
    """The synchronous dispose of a handle whose dispose is also its release:
    the release passes a completion, and this passes none."""
    text = doc(bound, handle.dispose, indent)
    text += f"{indent}dispose(): void {{\n"
    text += f"{indent}  closeSync(this, (handle, token) =>\n"
    text += f"{indent}    native.{handle.dispose}(handle, token, undefined),\n"
    text += f"{indent}  );\n"
    return text + f"{indent}}}\n"


def holder(entry: Lowered) -> str:
    """Where a call's accepted registrations live: its receiver, or the realm."""
    return "rootsOf(this)" if entry.owner else ""


def method_body(bound, values, entry: Lowered, discard=False) -> list[str]:
    call = f"native.{entry.plan.name}({', '.join(entry.arguments)})"
    if discard:
        call = f"native.{entry.plan.name}({', '.join([*entry.arguments, 'undefined'])})"
        lines = [f"{call};"]
        if entry.registrations:
            lines.append(f"registrations.commit({holder(entry)});")
            return [
                "withRegistrations((registrations) => {",
                *[f"  {line}" for line in lines],
                "});",
            ]
        return lines
    arguments = ", ".join([*entry.arguments, "completion"])
    start = f"(completion) => native.{entry.plan.name}({arguments})"
    if entry.registrations and entry.plan.completion and entry.shape != "attach":
        # The receiver roots the registrations once native code accepts them.
        start = (
            f"(completion) => {{ native.{entry.plan.name}({arguments}); "
            f"registrations.commit({holder(entry)}); }}"
        )
    shape = entry.shape
    if shape == "close":
        lines = [f"closeSync(this, (handle, token) => {call});"]
    elif shape == "answer":
        lines = [f"answerSync(this, (handle, token) => {call});"]
    elif shape == "close_async":
        lines = [f"return closeAsync(this, (handle, token) => submitRun({start}));"]
    elif shape == "run":
        lines = [f"return run({start});"]
    elif shape == "command":
        lines = [f"return command({start});"]
    elif shape == "query":
        result = entry.returns.removeprefix("Promise<").removesuffix(">")
        lines = [f"return query<{result}>({start});"]
    elif shape == "attach":
        assert entry.owned is not None
        member = identifier(output_member(entry.owned.parameter))
        lines = [
            f"const [{member}, ready] = attach({start}, (handle) => {construct(bound, entry, entry.owned, 'handle')});",
            f"return {{ {member}, ready }};",
        ]
    elif shape == "thread_start":
        arguments = ", ".join([*entry.arguments, "start"])
        adopted = construct(bound, entry, entry.owned, "handle")
        lines = [
            f"return threadStart((start) => native.{entry.plan.name}({arguments}), (handle) => {adopted});"
        ]
    elif shape == "owned":
        lines = [f"const handle = {call};"]
        adopted = construct(bound, entry, entry.owned, "handle")
        if entry.absent:
            lines.append(f"return handle === undefined ? undefined : {adopted};")
        else:
            lines.append(f"return {adopted};")
    elif shape == "drain":
        # Nothing queued reads as no elements.
        lines = [f"return {call} ?? [];"]
    elif shape == "view":
        view = entry.plan.view
        assert view is not None and entry.view is not None
        scope = (
            f"new ViewScope(ADOPT, this, (handle) => native.{view.begin}(handle), "
            f"(scope) => native.{view.end}(scope))"
        )
        lines = [
            f"return {scope}.use((scope) => use(new {scoped_name(entry.view)}(ADOPT, scope, {call})));"
        ]
    elif entry.registrations:
        result = "const result = " if entry.returns != "void" else ""
        commit = f"registrations.commit({holder(entry)});"
        if entry.declined:
            commit = f"if (!result) {commit}"
        lines = [f"{result}{call};", commit]
        if result:
            lines.append("return result;")
    else:
        lines = [f"return {call};" if entry.returns != "void" else f"{call};"]
    if entry.registrations:
        if len(lines) == 1 and lines[0].startswith("return "):
            body = lines[0].removeprefix("return ").removesuffix(";")
            return [f"return withRegistrations((registrations) => {body});"]
        return [
            "return withRegistrations((registrations) => {",
            *[f"  {line}" for line in lines],
            "});",
        ]
    return lines


def render_method(bound, values, entry: Lowered, indent: str) -> str:
    body = method_body(bound, values, entry)
    head = (
        "static " if entry.plan.factory else "" if entry.owner else "export function "
    )
    notes = (THREAD_START_NOTE,) if entry.shape == "thread_start" else ()
    text = doc(bound, entry.plan.name, indent, notes)
    generic = "<T>" if entry.shape == "view" else ""
    text += f"{indent}{head}{entry.method}{generic}({signature(entry)}): {entry.returns} {{\n"
    text += "".join(f"{indent}  {line}\n" for line in body)
    return text + f"{indent}}}\n"


def render_discarding(bound, values, public, entries) -> str:
    """The view of an owner whose commands discard their completions."""
    name = f"{public}Discarding"
    text = (
        f"/**\n * The commands of a {{@link {public}}} that discard their outcomes:\n"
        " * each returns once native code accepts it and allocates no completion.\n"
        " * A later completion on the same map fences them.\n */\n"
        f"export class {name} {{\n"
        f"  readonly #owner: {public};\n\n"
        "  /**\n   * Created by the API, which passes a key that only it holds.\n"
        "   *\n   * @internal\n   */\n"
        f"  constructor(key: AdoptKey, owner: {public}) {{\n"
        "    checkKey(key);\n"
        "    this.#owner = owner;\n"
        "  }\n"
    )
    for entry in sorted(entries, key=lambda e: e.method):
        body = method_body(bound, values, entry, discard=True)
        body = [
            line.replace("this.id", "this.#owner.id").replace(
                "rootsOf(this)", "rootsOf(this.#owner)"
            )
            for line in body
        ]
        text += "\n" + doc(bound, entry.plan.name, "  ")
        text += f"  {entry.method}({signature(entry)}): void {{\n"
        text += "".join(f"    {line}\n" for line in body)
        text += "  }\n"
    return text + "}\n"


def render_owner(bound, values, native: str, handle, methods) -> str:
    public = owner_class(handle)
    parameters = ["key: AdoptKey", "handle: bigint"]
    options = []
    if handle.parent in bound.public_handles:
        parameters.append(f"parent: {owner_class(bound.handles[handle.parent])}")
        options.append("parent")
    parameters.append("registrations?: Registrations")
    options.append("registrations")
    if handle.abandon:
        options.append("session: true")
    if handle.observable_root_release:
        options.append("root: true")
    summary = doc(bound, native)
    text = "/**\n * Owner of one native handle.\n */\n" if not summary else summary
    text += f"export class {public} extends Owner {{\n"
    text += (
        "  /**\n   * Created by the API, which passes a key that only it holds.\n"
        "   *\n   * @internal\n   */\n"
    )
    text += f"  constructor({', '.join(parameters)}) {{\n"
    text += f'    super(key, handle, "{public}", {{ {", ".join(options)} }});\n'
    text += "  }\n"
    if native in bound.event_sources:
        # Redeclares the inherited ID only to document it for this handle.
        text += (
            "\n  /**\n"
            "   * The native handle value, which runtime events report as their\n"
            "   * source. It stays readable after close, so events drained later\n"
            "   * still match this handle.\n"
            "   */\n"
            "  declare readonly id: bigint;\n"
        )
    discarding = [e for e in methods if e.discardable]
    if discarding:
        text += (
            "\n  /** The commands of this owner that discard their outcomes. */\n"
            f"  get discarding(): {public}Discarding {{\n"
            f"    return new {public}Discarding(ADOPT, this);\n  }}\n"
        )
    # A handle without scoped cleanup gets neither `using` nor `await using`.
    scoped = [] if handle.scoped_release is None else methods
    asynchronous = next((e.method for e in scoped if e.shape == "close_async"), None)
    synchronous = (
        ("dispose" if scoped and handle.dispose_completion else None)
        or next(
            (e.method for e in scoped if e.shape == "close" and e.method == "dispose"),
            None,
        )
        or next(
            (
                e.method
                for e in scoped
                if e.shape == "close"
                and all(d == "undefined" for _, _, d in e.parameters)
            ),
            None,
        )
    )
    # defineDisposers() installs these when the host defines their symbols.
    if asynchronous or synchronous:
        text += (
            f"  /**\n   * Calls `{asynchronous or synchronous}()`, for `await using`.\n"
            "   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.\n"
            "   */\n"
            "  declare [Symbol.asyncDispose]: () => Promise<void>;\n"
        )
    if synchronous:
        text += (
            f"  /**\n   * Calls `{synchronous}()`, for `using`.\n"
            "   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.\n"
            "   */\n"
            "  declare [Symbol.dispose]: () => void;\n"
        )
    rendered = [
        (entry.method, render_method(bound, values, entry, "  ")) for entry in methods
    ]
    if handle.dispose_completion:
        rendered.append(("dispose", render_dispose(bound, handle, "  ")))
    for _, method in sorted(rendered, key=lambda item: item[0]):
        text += "\n" + method
    text += "}\n"
    text += (
        f"defineDisposers({public}, "
        + (f'"{asynchronous}"' if asynchronous else "undefined")
        + ", "
        + (f'"{synchronous}"' if synchronous else "undefined")
        + ");\n"
    )
    if discarding:
        text += "\n" + render_discarding(bound, values, public, discarding)
    return text


def render_attachments(bound, lowered: list[Lowered]) -> list[str]:
    chunks = {}
    for entry in lowered:
        if entry.shape != "attach":
            continue
        assert entry.owned is not None
        name = attachment_name(entry.owned)
        member = identifier(output_member(entry.owned.parameter))
        chunks[name] = (
            f"/** A new {member} and the completion that reports it ready. */\n"
            f"export interface {name} {{\n"
            f"  /** The {member}, which the caller owns before it is ready. */\n"
            f"  readonly {member}: {owner_class(entry.owned.handle)};\n"
            "  /**\n"
            f"   * Settles once the {member} is ready. It rejects with a `MaplibreError`\n"
            "   * when it fails to become ready, so await it or attach a rejection\n"
            "   * handler: an unhandled rejection ends a Node process by default.\n"
            "   */\n"
            "  readonly ready: Promise<void>;\n}\n"
        )
    return [chunks[name] for name in sorted(chunks)]


def render_views(bound, values: Values, lowered: list[Lowered]) -> list[str]:
    chunks: dict[str, str] = {}
    for entry in lowered:
        if entry.shape != "view" or entry.view is None or entry.view in chunks:
            continue
        value = bound.values[entry.view]
        record = public_name(value.native)
        name = scoped_name(value.native)
        view = entry.plan.view
        assert view is not None
        # The scope guards only what the view's owner lends; a copy of any
        # other member stays readable after the scope ends.
        getters = "".join(
            f'\n  get {identifier(f.name)}(): {record}["{identifier(f.name)}"] {{\n'
            + (
                f"    return this.#scope.active(this.#value).{identifier(f.name)};\n  }}\n"
                if f.scoped
                else f"    return this.#value.{identifier(f.name)};\n  }}\n"
            )
            for f in values.fields(value)
        )
        method = f"{owner_class(view.owner)}.{entry.method}"
        chunks[entry.view] = (
            f"/**\n * A `{record}` whose native objects stay valid only while the\n"
            f" * callback of {{@link {method}}} runs. Its other members stay\n"
            " * readable after the callback returns.\n */\n"
            f"export class {name} {{\n"
            f"  readonly #value: {record};\n"
            "  readonly #scope: ViewScope;\n\n"
            "  /**\n   * Created by the API, which passes a key that only it holds.\n"
            "   *\n   * @internal\n   */\n"
            f"  constructor(key: AdoptKey, scope: ViewScope, value: {record}) {{\n"
            "    checkKey(key);\n"
            "    this.#value = value;\n"
            "    this.#scope = scope;\n"
            "  }\n" + getters + "}\n"
        )
    return [chunks[name] for name in sorted(chunks)]


def delivered(bound, values: Values):
    """Each queue-delivered callback that a generated registration names, with
    the registrations and fields that name it, by delivery constant."""
    result = {}
    for value in values.used.values():
        if value.kind != "record" or not value.registration:
            continue
        entry = registration(bound, value)
        if entry.form != "queue":
            continue
        for field in entry.callbacks:
            callback = bound.callbacks[field.value.native]
            result.setdefault(callback.delivery.constant, (callback, []))[1].append(
                (entry, field)
            )
    return result


def render_delivery(bound, values: Values, callback, uses) -> str:
    """One entry of the delivery table: the host callback of a rooted
    registration, run with the delivery's copied arguments."""
    forms = {(entry.function, field.name) for entry, field in uses}
    if len(forms) != 1:
        raise Unsupported(f"{callback.native}: registrations name it differently")
    entry, field = uses[0]
    public = public_name(entry.value.native)
    if entry.function:
        function_type = public
        target = f"target as {public}"
    else:
        member = identifier(field.name)
        function_type = f'NonNullable<{public}["{member}"]>'
        target = f"(target as {public}).{member}"
    names = [parameter_name(p.name) for p in callback.arguments]
    owned = [
        (p, name)
        for p, name in zip(callback.arguments, names)
        if values.delivered_owner(callback, p)
    ]
    lines = [f"const callback = {target};"]
    if field.value.nullable and not entry.function:
        # An unset callback drops its deliveries, and the batch releases what
        # they own.
        lines.append("if (callback === undefined) return;")
    decoded = []
    for parameter, name in zip(callback.arguments, names):
        key = identifier(parameter.name)
        decoded.append(f"payload.{key}")
    lines.append(
        f"const [{', '.join(names)}] = [{', '.join(decoded)}] as Parameters<{function_type}>;"
        if names
        else ""
    )
    if owned:
        if len(owned) != 1:
            raise Unsupported(f"{callback.native}: one delivered owner at most")
        parameter, name = owned[0]
        handle = values.delivered_handle(parameter)
        # The owner takes the handle from the batch before the callback runs,
        # so closing it after a failure releases an unanswered request, which
        # native code then settles.
        lines += [
            f"const owner = takeDelivered(index, {name} as unknown as bigint, (handle) => new {owner_class(handle)}(ADOPT, handle), {disposer(handle)});",
            "try {",
            f"  callback({', '.join('owner' if n == name else n for n in names)});",
            "} catch (error) {",
            "  owner.close();",
            "  throw error;",
            "}",
        ]
    else:
        lines.append(f"callback({', '.join(names)});")
    lines = [line for line in lines if line]
    uses_index = any("index" in line for line in lines)
    payload = "payload" if names else "_payload"
    head = f"[{bound.enum_number(callback.delivery.constant)}]: (target, {payload}{', index' if uses_index else ''}) => {{"
    return (
        f"  // {callback.delivery.constant}\n  {head}\n"
        + "".join(f"    {line}\n" for line in lines)
        + "  },\n"
    )


def render_deliveries(bound, values: Values) -> str:
    deliveries = delivered(bound, values)
    entries = "".join(
        render_delivery(bound, values, callback, uses)
        for _, (callback, uses) in sorted(
            deliveries.items(), key=lambda item: bound.enum_number(item[0])
        )
    )
    return (
        "// How each drained delivery reaches the host value that its registration\n"
        "// rooted, by delivery type.\n"
        "defineDeliveries({\n"
        + "".join(f"{line}\n" for line in entries.splitlines())
        + "});\n"
    )


def code(text: str) -> str:
    """TypeScript text without its comments, whose prose imports nothing."""
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.DOTALL)


def value_imports(values: Values, text: str) -> str:
    names = sorted({public_name(native) for native in values.used})
    used = [name for name in names if re.search(rf"\b{name}\b", text)]
    if not used:
        return ""
    return f'import type {{ {", ".join(used)} }} from "./generated_values.ts";\n'


def runtime_imports(text: str) -> str:
    modules = {
        "./internal/addon.ts": ["native"],
        "./internal/completions.ts": [
            "type CommandCompletion",
            "attach",
            "command",
            "query",
            "run",
            "submitRun",
            "threadStart",
        ],
        "./internal/owners.ts": [
            "ADOPT",
            "type AdoptKey",
            "Owner",
            "ViewScope",
            "adopt",
            "answerSync",
            "checkKey",
            "closeAsync",
            "closeSync",
            "defineDisposers",
            "rootsOf",
            "takeDelivered",
        ],
        "./internal/realm.ts": [
            "type Registrations",
            "defineDeliveries",
            "withRegistrations",
        ],
    }
    lines = []
    for module, names in modules.items():
        used = [
            name
            for name in names
            if re.search(rf"\b{name.removeprefix('type ')}\b", text)
        ]
        if used:
            lines.append(f'import {{ {", ".join(used)} }} from "{module}";\n')
    return "".join(lines)


def render_operations(bound, values: Values, lowered: list[Lowered]):
    by_owner: dict[str | None, list[Lowered]] = {}
    for entry in lowered:
        # A factory is a static method of the handle that it creates.
        by_owner.setdefault(owner_native(entry), []).append(entry)
    chunks = []
    for entry in sorted(by_owner.get(None, []), key=lambda e: e.method):
        chunks.append(render_method(bound, values, entry, ""))
    for native, handle in sorted(bound.public_handles.items()):
        chunks.append(
            render_owner(bound, values, native, handle, by_owner.get(native, []))
        )
    chunks += render_attachments(bound, lowered)
    chunks += render_views(bound, values, lowered)
    chunks.append(render_deliveries(bound, values))
    body = "\n".join(chunks)
    operations = (
        HEADER
        + "\n"
        + runtime_imports(code(body))
        + value_imports(values, code(body))
        + "\n"
        + body
    )
    interface = []
    for entry in sorted(lowered, key=lambda e: e.plan.name):
        parameters = ", ".join(f"{name}: {ts}" for name, ts in entry.native_parameters)
        interface.append(f"  {entry.plan.name}({parameters}): {entry.native_result};\n")
    for begin, end in sorted(view_scopes(lowered)):
        interface.append(f"  {begin}(handle: bigint): bigint;\n")
        interface.append(f"  {end}(scope: bigint): void;\n")
    interface_text = "".join(interface)
    native = (
        HEADER
        + "\n"
        + value_imports(values, interface_text)
        + "\n"
        + "/** The addon function of each generated operation, by C function name. */\n"
        + "export interface NativeOperations {\n"
        + interface_text
        + "}\n"
    )
    if (version := bound.source.c_version) is not None:
        native += (
            "\n/** The C ABI contract version of the headers this binding was generated from. */\n"
            f"export const MLN_C_VERSION = {version};\n"
        )
    return {
        "bindings/typescript/src/generated_operations.ts": operations,
        "bindings/typescript/src/generated_native.ts": native,
    }


def view_scopes(lowered: list[Lowered]) -> set[tuple[str, str]]:
    """The begin and end functions of each borrowed view scope that a
    generated view uses."""
    return {
        (entry.plan.view.begin, entry.plan.view.end)
        for entry in lowered
        if entry.shape == "view" and entry.plan.view
    }
