"""Generate the TypeScript binding: its public API and its Node-API addon.

Each operation lowers once into both halves. The addon half converts the
JavaScript arguments into C storage, calls the C function directly, and
returns a JavaScript value. The TypeScript half names the operation on its
owner class or as a top-level function and calls the runtime helper for its
execution shape.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

from .. import async_hosts, docs, model
from ..compiler import compile_api
from ..model import Api
from ..names import type_name
from ..protocol import COMPLETION_RESULT
from ..semantic import (
    BoundApi,
    OperationPlan,
    OwnedOutputPlan,
    output_member,
)
from .typescript_native_values import NativeValues
from .typescript_values import (
    OWNER_MEMBERS,
    Unsupported,
    Values,
    enum_members,
    identifier,
    is_counted,
    native_fields,
    owner_class,
    parameter_name,
    public_name,
    registration,
    union_kinds,
)

TS_DIRECTORY = "bindings/typescript/src"
NATIVE_DIRECTORY = "bindings/typescript/native"


@dataclass
class Lowered:
    """One operation, as both halves of the binding call it."""

    plan: OperationPlan
    owner: str | None
    method: str
    # (name, type, default) of each public parameter.
    parameters: list[tuple[str, str, str | None]] = field(default_factory=list)
    # The TypeScript expressions that the addon function receives, before
    # the completion token.
    arguments: list[str] = field(default_factory=list)
    # (name, type) of each addon parameter, for the addon interface.
    native_parameters: list[tuple[str, str]] = field(default_factory=list)
    native_result: str = "void"
    shape: str = "call"
    returns: str = "void"
    owned: OwnedOutputPlan | None = None
    absent: bool = False
    registrations: bool = False
    # The output whose true value reports that native kept no registration.
    declined: bool = False
    # The record a borrowed view scopes, for a scoped view result.
    view: str | None = None
    # The completion decoder that the addon records for the result, if any.
    decoder: str | None = None
    # Whether a discarding view of the receiver also calls the command.
    discardable: bool = False
    # The C statement that releases a drained batch after its slice is copied.
    drained: str | None = None
    c_locals: list[str] = field(default_factory=list)
    c_body: list[str] = field(default_factory=list)
    # File-scope C that the export calls, such as a thread start's function.
    c_helpers: str = ""


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


def lower_function(
    plan: OperationPlan, values: Values, natives: NativeValues
) -> Lowered:
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
    inputs = {p.name: p for p in plan.inputs}
    outputs = {p.name: p for p in plan.outputs}
    # Counts that a sized input carries, by count parameter.
    counts = {
        p.value.length: p.name
        for p in plan.inputs
        if p.value.kind == "array" or is_counted(p.value)
    }
    locals_, body, args = lowered.c_locals, lowered.c_body, []
    index = 0
    if receiver:
        name = function.parameters[0].name
        local = f"arg_{name}"
        locals_.append(f"{receiver} {local} = 0;")
        body.append(
            f'{{ uint64_t number = 0; if (!mln_ts_read_u64(call, call->argv[{index}], &number, "{identifier(name)}")) goto done; {local} = ({receiver})number; }}'
        )
        args.append(f"&{local}" if receiver_reference else local)
        lowered.native_parameters.append((identifier(name), "bigint"))
        lowered.arguments.append("handle" if plan.consumes else "this.id")
        index += 1
        if plan.consumes:
            # The call that consumes the handle ends its owner's token, so no
            # realm teardown can finalize the consumed handle. A handle that has
            # no owner yet passes no token.
            body.append(
                f"if (!mln_ts_read_owner(call, call->argv[{index}])) goto done;"
            )
            lowered.native_parameters.append(("token", "object | undefined"))
            lowered.arguments.append("token")
            index += 1
    returned: list[tuple[str, object]] = []
    # Caller-sized text starts in call memory of a guessed size, and the call
    # runs again with enough memory when the text did not fit.
    text = plan.caller_sized_text

    def output_count(name):
        sized_by = outputs[name].sized_by
        return f"count_{sized_by}" if sized_by else None

    for parameter in function.parameters[1 if receiver else 0 :]:
        local = f"arg_{parameter.name}"
        public_name_ = parameter_name(parameter.name)
        if plan.completion and parameter.name == plan.completion.parameter:
            locals_.append(f"mln_completion {local};")
            locals_.append(f"const mln_completion* present_{parameter.name} = NULL;")
            args.append(f"present_{parameter.name}")
            continue
        if parameter.name in counts:
            args.append(f"({parameter.type.spelling})count_{counts[parameter.name]}")
            continue
        if text and parameter.name == text.name:
            locals_.append(f"char* {local} = NULL;")
            body.append(f"{local} = mln_ts_allocate(call, arg_{text.value.length});")
            body.append(f"if ({local} == NULL) goto done;")
            args.append(local)
            continue
        if text and parameter.name == text.value.length:
            locals_.append(f"size_t {local} = 256;")
            args.append(local)
            continue
        if text and parameter.name == text.result_length:
            locals_.append(f"size_t {local} = 0;")
            args.append(f"&{local}")
            continue
        if parameter.name in outputs:
            value = outputs[parameter.name].value
            if plan.drain and parameter.name == plan.drain.batch.name:
                # The call releases the batch once it copies the slice.
                handle = value.element.handle
                release = bound.source.functions_by_name[handle.release]
                if release.diagnostic or release.return_type.kind != "void":
                    raise Unsupported("a drained batch needs a void release")
                locals_.append(f"{handle.native} {local} = MLN_HANDLE_NULL;")
                args.append(f"&{local}")
                lowered.drained = f"{handle.release}({local});"
                continue
            owned = next(
                (o for o in plan.owned_outputs if o.parameter == parameter.name), None
            )
            if owned:
                check_owner(bound, owned)
                lowered.owned = owned
                locals_.append(f"{owned.handle.native} {local} = MLN_HANDLE_NULL;")
                args.append(f"&{local}")
                continue
            if value.kind == "native_pointer":
                locals_.append(f"void* {local} = NULL;")
                args.append(f"&{local}")
                returned.append((parameter.name, value))
                continue
            sized_by = outputs[parameter.name].sized_by
            if sized_by:
                # One output element per element of the input array, in call
                # memory that the input list's count bounds.
                element = value.element
                values.check(element, "out")
                count = f"count_{sized_by}"
                locals_.append(f"{element.native}* {local} = NULL;")
                body.append(
                    f"{local} = mln_ts_allocate(call, sizeof *{local} * {count});"
                )
                body.append(f"if ({local} == NULL) goto done;")
                init = natives.init(element, f"{local}[index]")
                if init[1:] or not init[0].startswith("memset("):
                    body.append(f"for (size_t index = 0; index < {count}; index++) {{")
                    body += [f"  {line}" for line in init]
                    body.append("}")
                args.append(local)
                returned.append((parameter.name, value))
                continue
            if value.kind != "reference" or value.element is None:
                raise Unsupported("output needs one initialized native storage value")
            element = value.element
            values.check(element, "out")
            if element.kind == "record":
                locals_.append(f"{element.native} {local};")
                body.extend(natives.init(element, local))
            else:
                locals_.append(
                    f"{scalar_local(parameter.type.pointee.spelling)} {local};"
                )
                body.append(f"memset(&{local}, 0, sizeof {local});")
            args.append(f"&{local}")
            if parameter.name == declined:
                lowered.declined = True
            returned.append((parameter.name, element))
            continue
        if parameter.name not in inputs:
            raise Unsupported("parameter missing a resolved input contract")
        value = inputs[parameter.name].value
        js = f"call->argv[{index}]"
        path = f'"{public_name_}"'
        index += 1
        lowered.arguments.append(
            f"{public_name_}.id" if value.kind == "handle" else public_name_
        )
        if value.kind == "handle":
            values.check(value, "in")
            locals_.append(f"uint64_t {local} = 0;")
            body.append(
                f"if (!mln_ts_read_u64(call, {js}, &{local}, {path})) goto done;"
            )
            args.append(local)
            lowered.parameters.append((public_name_, values.ts(value), None))
            lowered.native_parameters.append((public_name_, "bigint"))
            continue
        if value.kind == "array" or is_counted(value):
            values.check(value, "in")
            count_parameter = next(
                p for p in function.parameters if p.name == value.length
            )
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
            continue
        if value.kind == "reference":
            element = value.element
            values.check(element, "in")
            if element.kind in {"record", "union"}:
                locals_.append(f"{element.native} {local};")
                read = natives.init(element, local) + natives.read(
                    element, js, local, path, "goto done"
                )
            else:
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
                body += [
                    f"  {line}" for line in read[len(natives.init(element, local)) :]
                ]
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
            continue
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
    completion = plan.completion
    status = plan.status
    if completion:
        if not status:
            raise Unsupported("asynchronous operation requires admission status")
        local = f"arg_{completion.parameter}"
        token = f"call->argv[{index}]"
        nullable = completion.discardable or completion.optional
        lowered.native_parameters.append(
            ("completion", "number | undefined" if nullable else "number")
        )
        lowered.discardable = completion.discardable and receiver is not None
        if completion.immediate_owners:
            if (
                len(completion.immediate_owners) != 1
                or plan.result
                or not lowered.owned
                or returned
            ):
                raise Unsupported(
                    "attachment requires one immediate owner and void readiness"
                )
            lowered.shape = "attach"
            lowered.returns = attachment_name(lowered.owned)
        elif plan.execution == "command":
            if plan.result:
                raise Unsupported("command requires a resultless receipt")
            lowered.shape, lowered.returns = "command", "Promise<CommandCompletion>"
        elif plan.result is None:
            lowered.shape, lowered.returns = "run", "Promise<void>"
        else:
            result = plan.result
            values.check(result, "out")
            lowered.decoder = natives.decoder(result)
            ts = values.ts(result)
            if result.nullable and ts != "unknown":
                ts += " | undefined"
            lowered.shape, lowered.returns = "query", f"Promise<{ts}>"
        decoder = natives.decoder_id(lowered.decoder)
        body.append(
            f"if (!mln_ts_read_completion(call, {token}, {'true' if nullable else 'false'}, {decoder}, &{local}, &present_{completion.parameter})) goto done;"
        )
    if plan.blocks:
        return lower_thread_start(bound, lowered, args, index)
    call_arguments = ", ".join(args + (["&diagnostic"] if function.diagnostic else []))
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
            accepted += f" && !arg_{declined}"
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
    if lowered.owned and (not completion or lowered.shape == "attach"):
        if returned:
            raise Unsupported("an owned output requires no other output")
        body.append(
            f"call->result = mln_ts_u64(env, (uint64_t)arg_{lowered.owned.parameter});"
        )
        lowered.native_result = "bigint"
        if lowered.shape != "attach":
            lowered.shape = "owned"
            lowered.returns = owner_class(lowered.owned.handle)
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
        if lowered.drained:
            # The batch is released whether or not its copy succeeded.
            body.append(lowered.drained)
            lowered.shape = "drain"
        if plan.view:
            if value.kind != "record":
                raise Unsupported("borrowed view requires a record output")
            # Read as with<Stem>: the call runs inside the view scope, and its
            # value is readable only while the callback runs.
            lowered.view = value.native
            lowered.shape = "view"
            lowered.method = identifier("with_" + plan.view.stem)
            lowered.parameters = [
                ("use", f"(view: {scoped_name(value.native)}) => T", None)
            ]
            lowered.returns = "T"
    elif result_value is not None:
        body.append(f"call->result = {natives.to_js(result_value, 'result')};")
        lowered.native_result = lowered.returns = values.ts(result_value)
    if lowered.absent:
        lowered.native_result += " | undefined"
        if not lowered.drained:
            lowered.returns += " | undefined"
    if plan.consumes:
        if not receiver:
            raise Unsupported("consumption requires an owned receiver")
        if lowered.shape == "run":
            lowered.shape = "close_async"
        elif lowered.shape == "call" and lowered.returns == "void":
            # An answer that consumes its receiver is no release, so no
            # disposer runs it and a repeated one reaches native.
            lowered.shape = "close" if plan.release else "answer"
        else:
            raise Unsupported("consumption requires an owned receiver without outputs")
    if completion and lowered.shape != "attach":
        lowered.native_result = "void"
    return lowered


def lower_thread_start(
    bound: BoundApi, lowered: Lowered, args: list[str], index: int
) -> Lowered:
    """A call that waits for a thread to start, which runs on a thread of its own.

    A thread that a browser page starts runs only after the page yields, so
    the page cannot wait for one. The export copies the converted arguments,
    with the call memory that they point into, and returns at once; the call's
    thread creates the owner and settles the Promise through the realm.
    """
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
    for argument in args:
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
            f"mln_ts_start_thread(call, call->argv[{index}], mln_ts_run_{name}, "
            f"mln_ts_dispose_{name}, &arguments, sizeof arguments);"
        ),
    ]
    lowered.native_parameters.append(("start", "number"))
    lowered.shape = "thread_start"
    lowered.returns = f"Promise<{owner_class(owned.handle)}>"
    return lowered


def owner_native(entry: Lowered) -> str | None:
    """The handle whose class declares a method: its receiver, or the handle
    that a factory creates as a static method."""
    return entry.owner or entry.plan.factory


def attachment_name(owned: OwnedOutputPlan) -> str:
    return "New" + owner_class(owned.handle).removesuffix("Handle")


def scoped_name(native: str) -> str:
    return "Scoped" + public_name(native)


# The platforms that the Node-API addon builds for, and the browser module's.
# Every entry point shares one operations module, so it leaves out only what
# none of them can use.
TARGETS = frozenset({"linux", "macos", "ohos", "web"})


def lower_operations(api: Api | BoundApi):
    bound = compile_api(api, TARGETS)
    values = Values(bound)
    natives = NativeValues(values)
    lowered: list[Lowered] = []
    unsupported = {
        name: "\n".join(reasons) for name, reasons in bound.unsupported.items()
    }
    for plan in bound.operations:
        try:
            # A failed attempt leaves no converter or declaration behind.
            local_values = Values(bound)
            local_natives = NativeValues(local_values)
            result = lower_function(plan, local_values, local_natives)
            values.merge(local_values)
            natives.merge(local_natives)
            lowered.append(result)
        except Unsupported as error:
            unsupported[plan.name] = f"{plan.function.location}: {error}"
    # Native rules answer in place of a callback that TypeScript cannot run, so
    # the operations that only such a callback calls have no caller.
    registrations = [v for v in values.used.values() if v.registration]
    for name in async_hosts.rule_answered_operations(bound, registrations):
        unsupported.pop(name, None)
    return bound, values, natives, lowered, unsupported


def symbols(bound: BoundApi) -> docs.Symbol:
    """How TypeScript spells the C declarations that its summaries name.

    `lower` records them before it renders a summary.
    """
    return bound.symbols("typescript", _unlowered)


def _unlowered(bound: BoundApi) -> docs.Symbol:
    raise AssertionError("TypeScript names its symbols only while it lowers")


def _symbols(bound: BoundApi, values: Values, lowered: list[Lowered]) -> docs.Symbol:
    # The names come from the same lowering that declares them, so a summary
    # names only what this binding generates.
    methods = {entry.plan.name: entry for entry in lowered}
    # A record that holds only a union presents the union under its own name.
    holders = {
        member.value.native: value
        for value in values.used.values()
        if (member := value.union_member)
    }

    def type_(native: str) -> str | None:
        if native in bound.public_handles:
            return owner_class(bound.public_handles[native])
        value = holders.get(native) or values.used.get(native)
        return public_name(value.native) if value else None

    def operation(plan: OperationPlan) -> str | None:
        entry = methods.get(plan.name)
        if entry is None:
            return None
        owner = owner_native(entry)
        if owner is None:
            return entry.method
        return f"{owner_class(bound.handles[owner])}.{entry.method}"

    def constant(value, member: str) -> str | None:
        typ = type_(value.native)
        name = next((m for m, c, _ in enum_members(value) if c == member), None)
        return f"{typ}.{name}" if typ and name else None

    def field(value, member) -> str | None:
        typ = type_(value.native)
        if typ is None or value.union_member:
            return None
        if value.kind == "union":
            # A union's members are its variants, which `kind` tells apart.
            kind = next(k for _, k, arm in union_kinds(value) if arm == member.name)
            return f'{typ} & {{ kind: "{kind}" }}'
        if value.registration:
            entry = registration(bound, value)
            if entry.native and member in entry.native:
                return f"{native_fields(typ)}.{identifier(member.name)}"
        if member not in values.fields(value):
            return None
        return f"{typ}.{identifier(member.name)}"

    def flag(value, mask_flag) -> str | None:
        typ = type_(value.native)
        if typ is None or value.registration:
            return None
        return f"{typ}.{identifier(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{name}": f"CommandCompletion.{identifier(name)}"
            for name in ("generation", "render_update_generation")
        },
        # Native queues each call, and the realm drains the queue on its event
        # loop after the native callback has returned.
        threads=(
            "The callback runs asynchronously on the JavaScript thread that "
            "registered it, after native queues the call."
        ),
    )


def lower(api: Api | BoundApi) -> model.Lowered:
    from .typescript_native import render_native
    from .typescript_operations import delivered, render_operations, view_scopes

    bound, values, natives, lowered, unsupported = lower_operations(api)
    bound.symbols("typescript", lambda bound: _symbols(bound, values, lowered))
    # The operations name every value, so the value table follows them.
    outputs = render_operations(bound, values, lowered)
    natives.payload_reader(
        [callback for callback, _ in delivered(bound, values).values()]
    )
    outputs[f"{TS_DIRECTORY}/generated_values.ts"] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n\n"
        + values.render()
    )
    outputs.update(render_native(bound, natives, lowered, view_scopes(lowered)))
    return model.Lowered(outputs, unsupported, [entry.plan.name for entry in lowered])
