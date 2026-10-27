"""Lower each .NET operation plan to its method, by the shape of its call.

Every shape converts its receiver and checks its contract the same way. A shape
that calls native with its parameters converts them once, and its function
builds the body around that call.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import assert_never

from tools.bindgen.managed_contracts import KEYWORDS, LOCALS
from tools.bindgen.model import Function
from tools.bindgen.names import camel, pascal, type_name
from tools.bindgen.semantic import (
    BoundApi,
    HandlePlan,
    OperationPlan,
    OwnedOutputPlan,
    output_member,
)

from .dotnet import (
    PRIMITIVES,
    disposer_arguments,
    method,
    method_name,
    native_call,
    operation_owner,
    owner_name,
    public_type,
    receiver_native,
)
from .dotnet_values import (
    TEXT_ENCODINGS,
    Unsupported,
    Values,
    member,
    raw_handle,
)


def operation_contract(plan: OperationPlan) -> str | None:
    """Reject contracts that the .NET operation skeleton does not implement."""
    function = plan.function
    execution = plan.execution
    result = plan.result
    value = result.element if result and result.kind == "array" else result
    if execution == "query" and (result is None or result.ownership != "borrowed"):
        return "query requires borrowed result storage"
    if (
        value
        and value.lifetime != "call"
        and not (value.kind == "native_pointer" and value.lifetime == "process")
    ):
        return "result lifetime requires a retention rule"
    if (
        result
        and result.kind == "array"
        and result.nullable
        and value
        and value.buffer_form == "view"
    ):
        return "optional array result needs a presence rule"
    method = pascal(plan.member)
    if (
        not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", method)
        or method in KEYWORDS["dotnet"]
    ):
        return f"method {method!r} requires identifier escaping"
    seen = set()
    planned_parameters = {p.name: p for p in (*plan.inputs, *plan.outputs)}
    for parameter in function.parameters[1:]:
        if plan.completion and parameter.name == plan.completion.parameter:
            continue
        name = camel(parameter.name)
        if (
            not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name)
            or name in KEYWORDS["dotnet"]
            or name
            in (
                LOCALS | {"diagnostic"}
                if execution == "query"
                else {"completion", "arena", "cancellationToken", "diagnostic"}
            )
            or name in seen
        ):
            return f"parameter {parameter.name!r} collides with a reserved or generated identifier"
        seen.add(name)
        if name.startswith("native"):
            return f"parameter {parameter.name!r} collides with generated allocation identifiers"
        planned = planned_parameters[parameter.name]
        value = planned.value
        if (
            value.length not in (None, "1")
            and not (value.length == "nul" and value.encoding == "utf8")
            and planned.direction != "in"
            and not planned.sized_by
            and not planned.result_length
        ):
            return f"parameter {parameter.name} requires counted-buffer conversion"
    return None


def checked_call(function: Function, arguments: str) -> str:
    """Checks a synchronous call against the calling thread's diagnostic."""
    if not function.diagnostic:
        raise Unsupported(f"{function.name} returns a status without a diagnostic")
    return f"Check({native_call(function, arguments, 'Diagnostic')});"


def present_call(plan: OperationPlan, arguments: str, absent: str = "null") -> str:
    """Returns `absent` for the absence status, and checks every other status."""
    call = native_call(plan.function, arguments, "Diagnostic")
    assert plan.absence is not None
    return f"if (!Present({call}, mln_status.{plan.absence.status})) return {absent};"


def submitted(function: Function, args: list[str], start: str) -> list[str]:
    """Calls a completion-taking C function with the completion that `start`
    fills, leaving the call's task in `nativePending`.

    Arguments convert from left to right, so a conversion that throws does so
    before the completion starts.
    """
    index = args.index("completion")
    if any(not arg.startswith("&") for arg in args[index + 1 :]):
        raise Unsupported("an argument after the completion could throw once it starts")
    arguments = ", ".join([*args[:index], start, *args[index + 1 :]])
    return [
        "mln_completion nativeCompletion;",
        f"var nativeStatus = {native_call(function, arguments, 'Diagnostic')};",
    ]


def unit_start() -> str:
    return "NativeCompletion.StartUnit(&nativeCompletion, out var nativePending)"


def completion_query(
    plan: OperationPlan, values: Values, records: set[str]
) -> tuple[str, str]:
    """The result type of a completion's value, and the converter that copies
    it, or `Task` and no converter when it carries none.

    Each converter is a static lambda over static methods, which allocates no
    delegate per call.
    """
    result = plan.result
    if result is None:
        return "Task", ""
    shape = "array" if result.kind == "array" else "value"
    value = result.element if shape == "array" else result
    assert value is not None
    native = value.native
    nullable = result.nullable
    if value.buffer_form == "view" and value.encoding in {"utf8", "json", "bytes"}:
        encoding = value.encoding
        public = "string" if encoding in TEXT_ENCODINGS else "byte[]"
        copy = "CopyUtf8View" if encoding in TEXT_ENCODINGS else "CopyBufferView"
        copy = f"ValueStructs.{copy}"
        if shape == "array":
            return (
                f"{public}[]",
                f"static nativeResult => NativeCompletion.Array<mln_buffer_view, {public}>(nativeResult, {copy})",
            )
        if shape == "value":
            if nullable:
                return (
                    f"{public}?",
                    f"static nativeResult => NativeCompletion.Optional<mln_buffer_view, {public}>(nativeResult, {copy})",
                )
            return (
                public,
                f"static nativeResult => NativeCompletion.Single<mln_buffer_view, {public}>(nativeResult, {copy})",
            )
        raise Unsupported(f"buffer result shape {shape!r} needs a conversion rule")
    if value.kind == "scalar" and native in PRIMITIVES and shape == "value":
        if nullable:
            raise Unsupported("nullable scalar completion needs a presence conversion")
        scalar = PRIMITIVES[native]
        return (
            scalar,
            f"static nativeResult => NativeCompletion.Value<{scalar}>(nativeResult)",
        )
    if value.kind == "record":
        record = values.record(native)
        values.decoder(record)
        records.add(native)
        public = public_type(native)
        copy = (
            f"static value => Copy{public}(value)"
            if native in values.strided_records
            else f"Copy{public}"
        )
        if shape == "array":
            helper = "OptionalArray" if nullable else "Array"
            return (
                f"{public}[]{'?' if nullable else ''}",
                f"static nativeResult => NativeCompletion.{helper}<{native}, {public}>(nativeResult, {copy})",
            )
        if not nullable:
            return (
                public,
                f"static nativeResult => NativeCompletion.Single<{native}, {public}>(nativeResult, {copy})",
            )
        helper = "Optional" if values.declares_class(record) else "OptionalValue"
        return (
            f"{public}?",
            f"static nativeResult => NativeCompletion.{helper}<{native}, {public}>(nativeResult, {copy})",
        )
    raise Unsupported(f"result {native!r} with shape {shape!r} needs a conversion rule")


@dataclass(frozen=True)
class Operation:
    """What every shape of an operation's method shares."""

    plan: OperationPlan
    bound: BoundApi
    values: Values
    # The owner class of each handle, and of the receiver, by C type.
    owners: dict[str, str]
    owner_class: str
    declared: str | None
    # The C type of the receiver, or empty for a static method.
    receiver: str
    handle: HandlePlan | None
    discard: bool

    def emit(
        self, call: Call, result_type: str, body: list[str]
    ) -> tuple[str, str, set[str]]:
        modifiers = "public" if self.receiver else "public static"
        return (
            self.owner_class,
            method(
                f"{modifiers} {result_type} {self.declared}({', '.join(call.parameters)})",
                call.pinned(body),
            ),
            call.records | self.values.plans.keys(),
        )


@dataclass(frozen=True)
class Call:
    """An operation's converted parameters, and the native call it makes."""

    parameters: list[str]
    args: list[str]
    prologue: list[str]
    outputs: list[tuple[str, str]]
    # Byte spans that C borrows only for the call, pinned around it as
    # (pointer, span) pairs instead of copied.
    pins: list[tuple[str, str]]
    scoped: bool
    records: set[str]
    immediate_owners: list[tuple[OwnedOutputPlan, str, str]]
    # The storage, capacity, and length locals of caller-sized text.
    text: tuple[str, str, str] | None

    @property
    def arguments(self) -> str:
        return ", ".join(self.args)

    def pinned(self, body: list[str]) -> list[str]:
        """Pins the byte spans around the whole body, which returns inside."""
        if not self.pins:
            return body
        fixed = ", ".join(f"{pointer} = {span}" for pointer, span in self.pins)
        return [f"fixed (byte* {fixed})", "{", *(f"    {line}" for line in body), "}"]


def emit_operation(
    plan: OperationPlan, bound: BoundApi, *, discard=False
) -> tuple[str, str, set[str]]:
    """The class that declares an operation's method, the method, and the
    records and values it copies. A discarding command passes no completion."""
    op = operation(plan, bound, discard)
    match plan.shape:
        case "discard":
            return retire(op)
        case "release":
            return release(op)
        case "release_async":
            return release_async(op)
        case "answer":
            return released(op)
        case "thread_start":
            # A .NET thread may wait for a native thread to start, so the call
            # lowers as it would if it did not wait.
            return called(op, converted(op))
        case "attach":
            return attach(op, converted(op))
        case "command":
            return command(op, converted(op))
        case "query":
            return query(op, converted(op))
        case "run":
            return submission(op, converted(op), "Task", unit_start())
        case "view":
            return view(op, converted(op))
        case "drain":
            return drain(op, converted(op))
        case "owned":
            return owned(op, converted(op))
        case "call":
            return immediate(op, converted(op))
        case _:
            assert_never(plan.shape)


def called(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    """An immediate call of another shape, by what it outputs."""
    plan = op.plan
    if plan.view:
        return view(op, call)
    if plan.drain:
        return drain(op, call)
    return owned(op, call) if plan.owned_outputs else immediate(op, call)


def operation(plan: OperationPlan, bound: BoundApi, discard: bool) -> Operation:
    owners = {name: owner_name(handle) for name, handle in bound.handles.items()}
    if plan.receiver:
        parameter = next(
            item for item in plan.function.parameters if item.name == plan.receiver
        )
        native = type_name(parameter.type.pointee or parameter.type)
        owners.setdefault(native, owner_name(native))
    function = plan.function
    if plan.consumes == "always" and (
        plan.completion or function.return_type.kind != "void"
    ):
        raise Unsupported("unconditional consumption requires a void immediate release")
    if reason := operation_contract(plan):
        raise Unsupported(reason)
    receiver = receiver_native(plan)
    if receiver and receiver not in owners and not plan.scoped_receiver:
        raise Unsupported(f"receiver {receiver} requires its generated owner")
    if plan.factory and plan.factory not in owners:
        raise Unsupported("factory output requires its generated owner")
    return Operation(
        plan,
        bound,
        Values(bound),
        owners,
        operation_owner(plan, bound),
        method_name(plan, bound, discard=discard),
        receiver,
        bound.handles.get(receiver),
        discard,
    )


def retire(op: Operation) -> tuple[str, str, set[str]]:
    # Retirement runs the discard, since it is the handle's cleanup.
    return (
        op.owner_class,
        method(f"public void {op.declared}()", ["state.Retire();"]),
        set(),
    )


def release(op: Operation) -> tuple[str, str, set[str]]:
    handle = op.handle
    assert handle is not None
    if handle.release_inputs:
        return released(op)
    if len(op.plan.function.parameters) != 1:
        return called(op, converted(op))
    # Dispose already runs the release when no separate dispose exists.
    if handle.dispose in {None, handle.release}:
        return op.owners[op.receiver], "", set()
    return (
        op.owners[op.receiver],
        f"    public void {op.declared}() {{ state.Close(); }}\n",
        set(),
    )


def release_async(op: Operation) -> tuple[str, str, set[str]]:
    assert op.handle is not None
    if op.handle.release_inputs:
        return released(op)
    native_type = raw_handle(op.handle)
    return (
        op.owners[op.receiver],
        (
            method(
                f"public Task {op.declared}()",
                ["state.Close(idempotent: false);", "return teardown;"],
            )
            # A scope that exits after an explicit close or dispose does
            # nothing, as .NET requires of a repeated dispose.
            + "\n    public ValueTask DisposeAsync() => state.Close() ? new(teardown) : default;\n\n"
            + method(
                f"private mln_status StartRelease({native_type} handle, mln_diagnostic* _)",
                [
                    *submitted(
                        op.plan.function,
                        ["handle", "completion"],
                        unit_start(),
                    ),
                    "teardown = nativePending.Submitted(nativeStatus);",
                    "return mln_status.MLN_STATUS_OK;",
                ],
            )
        ),
        set(),
    )


def released(op: Operation) -> tuple[str, str, set[str]]:
    """A release with inputs, or an answer that consumes its receiver, runs
    through the owner's state, which marks the handle closed once native
    accepts it. An answer keeps its name, and a repeated one reaches native."""
    plan, values = op.plan, op.values
    declarations, arguments = [], ["live"]
    # The release runs synchronously, so its scope outlives the call. The
    # release delegate cannot capture the scope, so the arguments convert
    # before it.
    body = ["using var scope = new NativeCallScope();"]
    for parameter in plan.inputs:
        if parameter.name == plan.receiver:
            continue
        value = parameter.value
        values.supported(value)
        name = camel(parameter.name)
        declarations.append(f"{values.public_type(value)} {name}")
        local = f"native{pascal(name)}"
        if value.kind == "reference" and value.element:
            source = name + (
                ".Value"
                if value.nullable and not values.is_reference_type(value.element)
                else ""
            )
            encoded = f"scope.Value({values.encode(value.element, source)})"
            body.append(
                f"var {local} = "
                + (f"{name} is null ? null : {encoded}" if value.nullable else encoded)
                + ";"
            )
        else:
            body.append(f"var {local} = {values.encode(value, name)};")
        arguments.append(local)
    call = native_call(plan.function, ", ".join(arguments), "diagnostic")
    return (
        op.owner_class,
        method(
            f"public void {op.declared}({', '.join(declarations)})",
            [
                *body,
                f"state.Release((live, diagnostic) => {call}{', idempotent: false' if plan.answers else ''});",
            ],
        ),
        set(values.plans),
    )


def view(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    function = op.plan.function
    handle = op.handle
    assert handle and handle.view_begin and handle.view_end
    if len(call.outputs) != 1:
        raise Unsupported("borrowed view requires one output descriptor")
    output_type, copied = call.outputs[0]
    parameters = [*call.parameters, f"Action<{output_type}> callback"]
    body = call.prologue + [
        "ArgumentNullException.ThrowIfNull(callback);",
        "var viewScope = new NativeViewScope();",
        "void* token = null;",
        checked_call(
            op.bound.source.functions_by_name[handle.view_begin], "Handle, &token"
        ),
        "try",
        "{",
        f"    {checked_call(function, call.arguments)}",
        f"    callback({copied});",
        "}",
        "finally",
        "{",
        "    viewScope.Expire();",
        f"    NativeMethods.{handle.view_end}(token);",
        "}",
    ]
    return (
        op.owner_class,
        method(f"public void {op.declared}({', '.join(parameters)})", body),
        call.records | op.values.plans.keys(),
    )


def drain(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    # The drain copies its slice, and releases the batch that holds it once
    # the copy ends or fails. Nothing queued is an empty slice.
    plan = op.plan
    assert plan.drain is not None
    if call.scoped or len(call.outputs) != 1:
        raise Unsupported("a drain requires one slice and no native scope")
    result_type, copied = call.outputs[0]
    batch = plan.drain.batch.value.unwrapped.handle
    assert batch is not None
    release = op.bound.source.functions_by_name[batch.release]
    body = call.prologue + [
        present_call(plan, call.arguments, "[]"),
        "try",
        "{",
        f"    return {copied};",
        "}",
        "finally",
        "{",
        f"    {native_call(release, disposer_arguments(batch, release, camel(plan.drain.batch.name)), 'null')};",
        "}",
    ]
    return op.emit(call, result_type, body)


def adopted(op: Operation, call: Call) -> tuple[str, str, str]:
    """The owner class of the one owner that a call creates, its local, and
    the parent argument that its adoption takes."""
    if len(call.immediate_owners) != 1:
        raise Unsupported("multiple owned outputs require a generated aggregate")
    owner, local, _ = call.immediate_owners[0]
    parent = (
        "this, "
        if owner.parent_parameter and owner.parent_parameter == op.plan.receiver
        else ""
    )
    return op.owners[owner.handle.native], local, parent


def attach(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    result_type, local, parent = adopted(op, call)
    body = call.prologue + [
        *submitted(op.plan.function, call.args, unit_start()),
        "var nativeReady = nativePending.Submitted(nativeStatus);",
        "scope.Accept();",
        f"return {result_type}.Adopt({parent}{local}, nativeReady);",
    ]
    return op.emit(call, result_type, body)


def owned(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    plan = op.plan
    result_type, local, parent = adopted(op, call)
    if plan.absence:
        # The scope accepts its inputs only after a call that publishes.
        if call.scoped:
            raise Unsupported("absence requires a call without a native scope")
        body = call.prologue + [
            present_call(plan, call.arguments),
            f"return {result_type}.Adopt({parent}{local});",
        ]
        return op.emit(call, result_type + "?", body)
    body = call.prologue + [checked_call(plan.function, call.arguments)]
    constructor = f"{result_type}.Adopt({parent}{local})"
    if plan.registrations:
        body += [
            "scope.Accept();",
            f"var owner = {constructor};",
            "return owner;",
        ]
    else:
        if call.scoped:
            body.append("scope.Accept();")
        body.append(f"return {constructor};")
    return op.emit(call, result_type, body)


def immediate(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    plan, values = op.plan, op.values
    function = plan.function
    prologue, outputs = call.prologue, call.outputs
    if function.return_type.kind != "void" and not plan.status:
        if plan.result is None:
            raise Unsupported("direct return needs a resolved result")
        if function.diagnostic:
            raise Unsupported("direct return cannot report a diagnostic")
        values.supported(plan.result)
        body = prologue + [
            f"var returned = NativeMethods.{function.name}({call.arguments});",
            f"return {values.copy(plan.result, 'returned')};",
        ]
        return op.emit(call, values.public_type(plan.result), body)
    if outputs:
        result_type = (
            outputs[0][0]
            if len(outputs) == 1
            else "("
            + ", ".join(
                f"{type_} {pascal(output_member(parameter.name))}"
                for (type_, _), parameter in zip(outputs, plan.outputs, strict=True)
            )
            + ")"
        )
        result = (
            outputs[0][1]
            if len(outputs) == 1
            else "(" + ", ".join(value for _, value in outputs) + ")"
        )
    else:
        result_type, result = "void", None
    if plan.absence:
        raise Unsupported("absence requires an owned handle output")
    body = prologue + [checked_call(function, call.arguments)]
    if call.text:
        storage, capacity, length = call.text
        body = prologue + [
            "while (true)",
            "{",
            f"    {checked_call(function, call.arguments)}",
            f"    if ({length} < {capacity}) break;",
            f"    {capacity} = {length} + 1;",
            f"    {storage} = scope.OutputArray<sbyte>(checked((int){capacity}));",
            "}",
        ]
    if call.scoped:
        accept = "scope.Accept();"
        # A rejected registration stores nothing, so the scope releases it.
        rejected = next(
            (
                registration.accepted_unless
                for registration in plan.registrations
                if registration.accepted_unless
            ),
            None,
        )
        if rejected:
            accept = f"if (!{camel(rejected)}) {accept}"
        body.append(accept)
    if result is not None:
        body.append(f"return {result};")
    return op.emit(call, result_type, body)


def command(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    if op.discard:
        args = call.args
        index = args.index("completion")
        arguments = ", ".join([*args[:index], "null", *args[index + 1 :]])
        body = call.prologue + [
            checked_call(op.plan.function, arguments),
            "scope.Accept();",
        ]
        return op.emit(call, "void", body)
    if op.plan.result is not None:
        raise Unsupported(
            "command carries a typed payload that needs its own conversion"
        )
    return submission(
        op,
        call,
        "Task<CommandCompletion>",
        "NativeCompletion.StartCommand(&nativeCompletion, out var nativePending)",
    )


def query(op: Operation, call: Call) -> tuple[str, str, set[str]]:
    value_type, converter = completion_query(op.plan, op.values, call.records)
    return submission(
        op,
        call,
        f"Task<{value_type}>",
        f"NativeCompletion.Start<{value_type}>(&nativeCompletion, {converter}, out var nativePending)",
    )


def submission(
    op: Operation, call: Call, result_type: str, start: str
) -> tuple[str, str, set[str]]:
    """Submits the call with the completion that `start` fills, and returns
    its task once the call scope accepts its inputs."""
    body = call.prologue + [
        *submitted(op.plan.function, call.args, start),
        "return scope.Accepted(nativePending.Submitted(nativeStatus));",
    ]
    return op.emit(call, result_type, body)


def converted(op: Operation) -> Call:
    """Converts each parameter that the receiver leaves, and enters the call."""
    plan, values, owners, receiver = op.plan, op.values, op.owners, op.receiver
    function = plan.function
    input_plans = {parameter.name: parameter.value for parameter in plan.inputs}
    asynchronous = plan.completion is not None
    records: set[str] = {receiver} if plan.scoped_receiver else set()
    args = ["Pointer" if plan.scoped_receiver else "Handle"] if receiver else []
    parameters = []
    # Parameters that may be absent, which default to null when trailing.
    omittable: set[str] = set()
    # Byte spans that C borrows only for the call, pinned around it as
    # (pointer, span) pairs instead of copied.
    pins: list[tuple[str, str]] = []
    prologue: list[str] = []
    outputs = []
    scoped = False
    immediate_owners = []
    counts = {
        parameter.value.length: parameter.name
        for parameter in plan.inputs
        if parameter.value.kind in {"array", "buffer"}
        and parameter.value.length not in {None, "nul"}
    }
    outputs_by_name = {parameter.name: parameter for parameter in plan.outputs}
    # Caller-sized text starts in scope memory of a guessed size, and the call
    # runs again with enough memory when the text did not fit.
    text = plan.caller_sized_text
    text_storage, text_capacity, text_length = None, None, None
    if text:
        assert text.value.length is not None and text.result_length is not None
        text_storage, text_capacity, text_length = (
            camel(text.name),
            camel(text.value.length),
            camel(text.result_length),
        )
    for parameter in function.parameters:
        if parameter.name == (plan.receiver or plan.scoped_receiver):
            continue
        name = camel(parameter.name)
        if text and parameter.name == text.name:
            scoped = True
            prologue += [
                f"nuint {text_capacity} = 256;",
                f"nuint {text_length} = 0;",
                f"var {name} = scope.OutputArray<sbyte>((int){text_capacity});",
            ]
            args.append(name)
            outputs.append(
                (
                    "string",
                    f"RuntimeStructs.CopyUtf8({name}, {text_length})",
                )
            )
            continue
        if text and parameter.name == text.value.length:
            args.append(name)
            continue
        if text and parameter.name == text.result_length:
            args.append(f"&{name}")
            continue
        if parameter.name in counts:
            counted = input_plans[counts[parameter.name]]
            size = (
                f"buffer{pascal(counts[parameter.name])}.size"
                if counted.encoding in TEXT_ENCODINGS
                and (
                    counted.kind == "buffer"
                    or counted.element
                    and counted.element.kind == "scalar"
                )
                else f"{camel(counts[parameter.name])}.Length"
            )
            args.append(f"checked(({PRIMITIVES[type_name(parameter.type)]}){size})")
            continue
        ctype = type_name(parameter.type)
        value_plan = input_plans.get(parameter.name)
        output = outputs_by_name.get(parameter.name)
        output_plan = output.value.element if output else None
        is_output = output is not None
        if output and output.sized_by and not asynchronous:
            value = output.value
            element = value.element
            assert element is not None
            values.supported(value)
            scoped = True
            count = f"{camel(output.sized_by)}.Length"
            raw = values.raw_type(element)
            prologue.append(f"var {name} = scope.OutputArray<{raw}>({count});")
            if element.kind == "record" and any(
                f.role == "size" for f in element.fields
            ):
                prologue.append(
                    f"for (var index = 0; index < {count}; index++) {name}[index].{member('size')} = (uint)sizeof({raw});"
                )
            args.append(name)
            outputs.append((values.public_type(value), values.copy(value, name, count)))
            continue
        if (
            asynchronous
            and is_output
            and parameter.name not in {owner.parameter for owner in plan.owned_outputs}
        ):
            raise Unsupported("asynchronous output requires a completion value")
        if plan.completion and parameter.name == plan.completion.parameter:
            args.append("completion")
        elif plan.drain and parameter.name == plan.drain.batch.name:
            prologue.append(
                f"{raw_handle(plan.drain.batch.value.unwrapped.handle)} {name} = default;"
            )
            args.append(f"&{name}")
        elif parameter.name in {owner.parameter for owner in plan.owned_outputs}:
            owner = next(
                owner
                for owner in plan.owned_outputs
                if owner.parameter == parameter.name
            )
            if owner.handle.native not in owners:
                raise Unsupported("owned output requires its generated owner")
            native = raw_handle(owner.handle)
            prologue.append(f"{native} {name} = default;")
            args.append(f"&{name}")
            immediate_owners.append((owner, name, native))
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind in {"array", "buffer"}
            and input_plans[parameter.name].length not in {None, "nul"}
            and input_plans[parameter.name].encoding in {"utf8", "bytes", "json"}
            and (
                input_plans[parameter.name].kind == "buffer"
                or (element := input_plans[parameter.name].element) is not None
                and element.kind == "scalar"
            )
        ):
            value_plan = input_plans[parameter.name]
            raw = (
                values.raw_type(value_plan.element) + "*"
                if value_plan.element
                else values.raw_type(value_plan)
            )
            if value_plan.encoding == "bytes" and not value_plan.nullable:
                parameters.append(f"ReadOnlySpan<byte> {name}")
                pins.append((f"pinned{pascal(name)}", name))
                args.append(f"({raw})pinned{pascal(name)}")
                continue
            public = "string" if value_plan.encoding in TEXT_ENCODINGS else "byte[]"
            parameters.append(f"{public} {name}")
            scoped = True
            encoder = "Utf8" if value_plan.encoding in TEXT_ENCODINGS else "Buffer"
            prologue.append(f"var buffer{pascal(name)} = scope.{encoder}({name});")
            args.append(f"({raw})buffer{pascal(name)}.data")
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind == "native_pointer"
        ):
            parameters.append(f"NativePointer {name}")
            args.append(f"(void*){name}.Address")
        elif value_plan and value_plan.kind == "handle" and value_plan.native in owners:
            parameters.append(f"{owners[value_plan.native]} {name}")
            scoped = True
            args.append(f"scope.Use({name})")
        elif value_plan and value_plan.kind == "enum":
            values.supported(value_plan)
            parameters.append(f"{public_type(value_plan.native)} {name}")
            args.append(values.encode(value_plan, name))
        elif value_plan and value_plan.kind == "scalar" and ctype in PRIMITIVES:
            parameters.append(
                f"{'ulong' if ctype == 'size_t' else PRIMITIVES[ctype]} {name}"
            )
            args.append(
                f"(byte)({name} ? 1 : 0)"
                if ctype in {"bool", "_Bool"}
                else f"checked((nuint){name})"
                if ctype == "size_t"
                else name
            )
        elif value_plan and value_plan.kind == "scalar":
            values.supported(value_plan)
            parameters.append(f"{values.public_type(value_plan)} {name}")
            args.append(values.encode(value_plan, name))
        elif value_plan and value_plan.kind == "record":
            record_plan = values.record(value_plan.native)
            values.encoder(record_plan)
            scoped |= values.needs_scope(record_plan)
            parameters.append(f"{public_type(value_plan.native)} {name}")
            args.append(values.encode(record_plan, name))
            records.add(value_plan.native)
        elif (
            value_plan
            and value_plan.buffer_form == "view"
            and value_plan.encoding in {"bytes", "json", "utf8"}
        ):
            if value_plan.encoding == "bytes" and not value_plan.nullable:
                parameters.append(f"ReadOnlySpan<byte> {name}")
                pins.append((f"pinned{pascal(name)}", name))
                args.append(
                    f"new mln_buffer_view {{ data = pinned{pascal(name)}, size = (nuint){name}.Length }}"
                )
                continue
            parameters.append(f"{values.public_type(value_plan)} {name}")
            if value_plan.nullable:
                omittable.add(name)
            scoped = True
            args.append(values.encode(value_plan, name))
        elif (
            value_plan
            and value_plan.buffer_form == "pointer"
            and value_plan.length == "nul"
            and value_plan.encoding == "utf8"
        ):
            parameters.append(f"string {name}")
            scoped = True
            args.append(f"scope.CString({name})")
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind == "array"
        ):
            value_plan = input_plans[parameter.name]
            values.supported(value_plan)
            if not values.can_encode(value_plan):
                raise Unsupported("array element requires an input converter")
            parameters.append(f"{values.public_type(value_plan)} {name}")
            scoped = True
            if value_plan.length and value_plan.length.isdecimal():
                prologue.append(f"ArgumentNullException.ThrowIfNull({name});")
                prologue.append(
                    f'if ({name}.Length != {value_plan.length}) throw new ArgumentException("Expected {value_plan.length} elements.", nameof({name}));'
                )
            args.append(values.encode(value_plan, name))
        elif value_plan and value_plan.kind == "reference":
            values.supported(value_plan)
            if not values.can_encode(value_plan):
                raise Unsupported("reference element requires an input converter")
            parameters.append(f"{values.public_type(value_plan)} {name}")
            if value_plan.nullable:
                omittable.add(name)
            element = value_plan.element
            assert element is not None
            scoped |= values.needs_scope(element)
            source = name + (
                ".Value"
                if value_plan.nullable and not values.is_reference_type(element)
                else ""
            )
            expression = values.encode(element, source)
            if value_plan.nullable:
                expression = f"{name} is null ? default({values.raw_type(element)}) : {expression}"
            local = f"native{pascal(name)}"
            prologue.append(f"var {local} = {expression};")
            args.append(
                f"{name} is null ? null : &{local}"
                if value_plan.nullable
                else f"&{local}"
            )
        elif output_plan and output_plan.kind == "enum":
            prologue.append(f"{values.native_type(output_plan)} {name} = default;")
            args.append(f"&{name}")
            outputs.append(
                (public_type(output_plan.native), values.copy(output_plan, name))
            )
        elif (
            output_plan
            and output_plan.kind == "scalar"
            and parameter.type.pointee
            and type_name(parameter.type.pointee) in PRIMITIVES
        ):
            native = PRIMITIVES[type_name(parameter.type.pointee)]
            prologue.append(f"{native} {name} = default;")
            args.append(f"&{name}")
            outputs.append(
                ("ulong", f"(ulong){name}") if native == "nuint" else (native, name)
            )
        elif output_plan and (
            output_plan.kind in {"record", "union"} or output_plan.buffer_form == "view"
        ):
            record = output_plan.native
            values.supported(output_plan)
            if output_plan.kind == "record":
                values.decoder(output_plan)
            size_field = next(
                (field for field in output_plan.fields if field.role == "size"), None
            )
            initial = (
                f"new {record} {{ {member(size_field.name)} = (uint)sizeof({record}) }}"
                if size_field
                else f"default({record})"
            )
            prologue.append(f"var {name} = {initial};")
            args.append(f"&{name}")
            copied = values.copy(output_plan, name)
            output_type = values.public_type(output_plan)
            if plan.view:
                output_type += "View"
                copied = f"new {output_type}({copied}, viewScope)"
            outputs.append((output_type, copied))
            if output_plan.kind == "record":
                records.add(record)
        else:
            raise Unsupported(
                f"parameter {parameter.name}: {parameter.type.spelling} needs a conversion rule"
            )
    for index in reversed(range(len(parameters))):
        if parameters[index].rsplit(" ", 1)[1] not in omittable:
            break
        parameters[index] += " = null"
    owner_expression = "null" if not receiver else "owner" if op.discard else "this"
    if (scoped or asynchronous) and not plan.scoped_receiver:
        entry = [f"using var scope = new NativeCallScope({owner_expression});"]
    elif plan.unchecked:
        # The ABI check calls this function, so reporting the version must not
        # run the check.
        entry = ["EnterUnchecked();"]
    else:
        entry = [f"using var call = Enter({owner_expression});"]
        if scoped:
            entry.append("using var scope = new NativeCallScope();")
    prologue = entry + prologue
    if op.discard:
        prologue.insert(0, "var receiverHandle = owner.Handle;")
        args[0] = "receiverHandle"
    if pins and (plan.view or text):
        raise Unsupported("a pinned span requires a single native call")
    return Call(
        parameters,
        args,
        prologue,
        outputs,
        pins,
        scoped,
        records,
        immediate_owners,
        (text_storage, text_capacity, text_length)
        if text_storage and text_capacity and text_length
        else None,
    )
