"""Lower Dart operations over the handle and completion runtimes.

Every operation converts its parameters the same way. Its shape decides what
the method does with the call: a consuming call closes or answers its owner,
an immediate call returns its outputs, a drain or view wraps them, and a
completion returns a future.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field, replace
from typing import assert_never

from ..managed_contracts import KEYWORDS, LOCALS
from ..model import Unsupported
from ..names import camel, pascal
from ..semantic import OperationPlan
from .dart import (
    attachment_name,
    method_name,
    native_call,
    receiver_handle,
    view_method,
)
from .dart_values import Values, owner_names


@dataclass
class Method:
    """A method under construction: what the parameter conversions add to its
    signature, setup, native arguments, and returned values."""

    owner: str
    name: str
    receiver: str | None
    signature: list[str] = field(default_factory=list)
    setup: list[str] = field(default_factory=list)
    args: list[str] = field(default_factory=list)
    returns: list[tuple[str, str]] = field(default_factory=list)
    call: str = ""
    # The registration transaction that an attachment opens before its call.
    transaction: list[str] = field(default_factory=list)
    # The output that receives the owner an attachment creates at once.
    attach_output: str | None = None
    # The batch that a drain releases once it copies the slice.
    drained: str | None = None

    @property
    def parameters(self) -> str:
        return ", ".join(self.signature)


def lower_function(
    plan: OperationPlan, values: Values, *, discard=False, dispose=False
) -> tuple[str, str]:
    """An operation's owner and method. With `discard`, the method passes no
    completion, and with `dispose`, it is the release's synchronous dispose."""
    function = plan.function
    # A dispose that is also the release passes no completion, so it consumes
    # synchronously, as a separate dispose does.
    disposed = None
    if dispose:
        assert plan.completion is not None
        disposed = plan.completion.parameter
        plan = replace(plan, completion=None, shape="release")
    if plan.consumes == "always" and (
        plan.completion or function.return_type.kind != "void"
    ):
        raise Unsupported("unconditional consumption requires a void immediate release")
    if plan.scoped_receiver:
        raise Unsupported(
            "operation requires its ownership or registration transaction"
        )
    if not plan.status and plan.completion:
        raise Unsupported("asynchronous operation requires admission status")
    receiver, receiver_reference = receiver_handle(plan, values.bound)
    method = Method(
        receiver or "Globals",
        method_name(plan, values.bound, dispose=dispose),
        receiver,
    )
    convert(plan, values, method, receiver_reference, disposed)
    register(plan, method)
    match plan.shape:
        case "discard" | "release" | "answer":
            code = consumed(plan, method)
        case "thread_start" | "owned" | "call":
            public, result = returned(plan, values, method)
            if plan.absence:
                public += "?"
            body = immediate(plan, method, result)
            code = (
                f"  {public} {method.name}({method.parameters}) {method_body(body)}\n"
            )
        case "drain":
            code = drained(plan, values, method)
        case "view":
            public, result = returned(plan, values, method)
            code = scoped_view(plan, method, public, immediate(plan, method, result))
        case "attach" | "command" | "query" | "run" | "release_async" if discard:
            code = f"  void {method.name}({method.parameters}) => _discard({start(method)});\n"
        case "attach":
            code = attached(plan, method)
        case "command":
            code = commanded(plan, method)
        case "query" | "run" | "release_async":
            code = completed(plan, values, method)
        case _:
            assert_never(plan.shape)
    return method.owner, code


def convert(
    plan: OperationPlan,
    values: Values,
    method: Method,
    receiver_reference: bool,
    disposed: str | None,
):
    """Converts each parameter after the receiver into the native call's
    arguments, and makes the call."""
    function = plan.function
    receiver = method.receiver
    inputs = {p.name: p for p in plan.inputs}
    outputs = {p.name: p for p in plan.outputs}
    counts = {
        p.value.length: p.name
        for p in plan.inputs
        if p.value.kind == "array"
        or p.value.kind == "buffer"
        and p.value.buffer_form != "view"
        and p.value.length not in {None, "1", "nul"}
    }
    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    text_arguments = (
        {
            text.name: "textStorage",
            text.value.length: "textCapacity",
            text.result_length: "textLength",
        }
        if text
        else {}
    )
    signature, optional, setup, returns = (
        method.signature,
        [],
        method.setup,
        method.returns,
    )
    args = ["handle.raw" if plan.consumes else "_handle.raw"] if receiver else []
    if receiver_reference:
        if not plan.consumes:
            raise Unsupported("mutable handle receiver requires consumption")
        setup.append("final receiverPointer = arena<Uint64>()..value = handle.raw;")
        args = ["receiverPointer"]
    for parameter in function.parameters[1 if receiver else 0 :]:
        if plan.completion and parameter.name == plan.completion.parameter:
            args.append("completion")
            continue
        if parameter.name == disposed:
            args.append("nullptr")
            continue
        local = camel(parameter.name)
        if (
            local in KEYWORDS["dart"]
            or local in LOCALS - {"value"}
            or local.startswith("native")
            or local == "raw"
        ):
            local += "Value"
        if parameter.name in text_arguments:
            assert text is not None
            if parameter.name == text.name:
                setup.append("final text = _CallerSizedText();")
                returns.append(
                    ("Object?", "_decodeJson(text.text)")
                    if text.value.encoding == "json"
                    else ("Uint8List", "text.text")
                )
            args.append(text_arguments[parameter.name])
            continue
        if parameter.name in counts:
            args.append(
                f"bytes{camel(counts[parameter.name])}.size"
                if inputs[counts[parameter.name]].value.kind == "buffer"
                else f"{camel(counts[parameter.name])}.length"
            )
            continue
        if parameter.name in outputs:
            output_plan = outputs[parameter.name]
            output = output_plan.value
            if output_plan.sized_by:
                values.check(output)
                source = camel(output_plan.sized_by)
                count = f"{source}.length"
                element = output.element
                assert element is not None
                setup.append(
                    f"final {local} = arena<{values.ffi(element)}>({source}.isEmpty ? 1 : {count});"
                )
                if element.kind == "record" and any(
                    f.role == "size" for f in element.fields
                ):
                    setup.append(
                        f"for (var index = 0; index < {count}; index++) {{ {local}[index].size = sizeOf<{values.ffi(element)}>(); }}"
                    )
                args.append(local)
                returns.append(
                    (values.public(output), values.copy(output, local, count))
                )
                continue
            if output.kind != "reference" or not output.element:
                raise Unsupported("output needs one initialized native storage value")
            output = output.element
            if plan.drain and parameter.name == plan.drain.batch.name:
                # The drain releases the batch once it copies the slice.
                setup.append(f"final {local} = arena<Uint64>();")
                args.append(local)
                method.drained = local
                continue
            owned = next(
                (o for o in plan.owned_outputs if o.parameter == parameter.name), None
            )
            if owned:
                if plan.completion:
                    method.attach_output = local
                    args.append(local)
                    returns.append(adopt_owner(owned, "handle", function, values))
                    continue
                setup.append(f"final {local} = arena<Uint64>();")
                args.append(local)
                returns.append(adopt_owner(owned, local + ".value", function, values))
                continue
            values.check(output)
            setup.append(f"final {local} = arena<{values.ffi(output)}>();")
            if output.kind == "record" and any(f.role == "size" for f in output.fields):
                setup.append(f"{local}.ref.size = sizeOf<{values.ffi(output)}>();")
            args.append(local + ".cast()" if output.kind == "native_pointer" else local)
            expression = f"{local}." + (
                "ref"
                if output.kind == "record" or output.buffer_form == "view"
                else "value"
            )
            if plan.view and (
                output.kind != "record"
                or not plan.view.owner.view_begin
                or not plan.view.owner.view_end
            ):
                raise Unsupported(
                    "borrowed view requires native scope operations and record output"
                )
            returns.append((values.public(output), values.copy(output, expression)))
            continue
        if parameter.name not in inputs:
            raise Unsupported("parameter missing a resolved input contract")
        value = inputs[parameter.name].value
        values.check(value)
        typ = values.public(value)
        # A JSON parameter is always a value, which null is too.
        json = value.kind == "buffer" and value.encoding == "json"
        if typ.endswith("?") and not json:
            optional.append(f"{typ} {local}")
        else:
            signature.append(f"{typ} {local}")
        if (
            value.kind == "buffer"
            and value.buffer_form != "view"
            and value.length not in {None, "1", "nul"}
        ):
            view = (
                f"nativeStringView({local}, arena).value"
                if value.encoding == "utf8"
                else f"nativeBufferView({local}, arena)"
            )
            setup.append(f"final bytes{local} = {view};")
            args.append(f"bytes{local}.data.cast()")
        elif value.kind == "reference":
            args.append(values.native(value, local))
        elif value.kind == "array":
            if value.nullable:
                raise Unsupported(
                    "nullable input array needs independent count presence"
                )
            child = value.element
            setup += [
                f"final native{local} = arena<{values.ffi(child)}>({local}.isEmpty ? 1 : {local}.length);",
                f"for (var index = 0; index < {local}.length; index++) {{ {values.write_element(child, local + '[index]', 'native' + local)} }}",
            ]
            args.append(f"native{local}")
        else:
            args.append(values.native(value, local))
    if optional:
        signature.append("{" + ", ".join(optional) + "}")
    if text:
        # Text that did not fit calls again, so its inputs convert once,
        # before the first call.
        for index, parameter in enumerate(function.parameters):
            if parameter.name in text_arguments or (receiver and index == 0):
                continue
            local = "native" + pascal(parameter.name)
            setup.append(f"final {local} = {args[index]};")
            args[index] = local
    method.args = args
    method.call = native_call(function, args)
    if text:
        method.call = f"text.call(arena, (textStorage, textCapacity, textLength) => {method.call})"


def register(plan: OperationPlan, method: Method):
    """Runs the call in a transaction that accepts its registrations once
    native code admits the call, unless native reports that it kept nothing
    from them."""
    if not plan.registrations:
        return
    if not plan.status:
        raise Unsupported("registration requires an admission status")
    # The receiver roots the callbacks it registers, an adopted owner roots
    # those of its call, and the isolate roots a global call's.
    roots = (
        "_CallbackRoots()"
        if plan.owned_outputs
        else "_callbackRoots"
        if method.receiver
        else "_globalCallbackRoots"
    )
    transaction = [f"final registrations = _NativeRegistrations({roots});"]
    declined = next(
        (r.accepted_unless for r in plan.registrations if r.accepted_unless),
        None,
    )
    declined = f", declined: () => {camel(declined)}.value" if declined else ""
    method.call = f"registrations.run(() => {method.call}{declined})"
    # An attachment opens the transaction before the closure that makes its
    # call, and every other call opens it in its setup.
    if plan.shape == "attach":
        method.transaction = transaction
    else:
        method.setup += transaction


def consumed(plan: OperationPlan, method: Method) -> str:
    """A call that consumes its receiver at once: a release closes the owner,
    and another consuming call answers it."""
    if not method.receiver or method.returns:
        raise Unsupported("consumption requires an owned receiver without outputs")
    call = method.call
    body = method.setup + (
        [f"return {call};"] if plan.status else [f"{call};", "return nativeStatusOk;"]
    )
    action = "answer" if plan.shape == "answer" else "close"
    return f"  void {method.name}({method.parameters}) => _state.{action}({closure('handle', body)});\n"


def returned(
    plan: OperationPlan, values: Values, method: Method
) -> tuple[str, str | None]:
    """The public type of what an immediate call returns, and the expression
    that copies it."""
    returns = method.returns
    if not plan.status and plan.result:
        values.check(plan.result)
        returns.insert(
            0,
            (values.public(plan.result), values.copy(plan.result, "nativeResult")),
        )
    if not returns:
        return "void", None
    if len(returns) == 1:
        return returns[0]
    return (
        "(" + ", ".join(t for t, _ in returns) + ")",
        "(" + ", ".join(v for _, v in returns) + ")",
    )


def immediate(plan: OperationPlan, method: Method, result: str | None) -> list[str]:
    """The body of a call that returns at once: it checks the call, then
    returns `result`."""
    call = method.call
    body = method.setup + [
        f"if (!_present({call}, raw.{plan.absence.status})) {{ return {'const []' if plan.drain else 'null'}; }}"
        if plan.absence
        else f"_check({call});"
        if plan.status
        else f"final nativeResult = {call};"
        if plan.result
        else f"{call};"
    ]
    if result:
        body.append(f"return {result};")
    return body


def drained(plan: OperationPlan, values: Values, method: Method) -> str:
    """A drain, which returns the slice that it copies and releases the batch
    that holds it."""
    assert plan.drain is not None
    public, result = returned(plan, values, method)
    batch = plan.drain.batch.value.unwrapped.handle
    assert batch is not None
    release = values.bound.source.functions_by_name[batch.release]
    if release.diagnostic or release.return_type.kind != "void":
        raise Unsupported("a drained batch needs a void release")
    result = (
        f"_drain({method.drained}.value, (batch) {{ {native_call(release, ['batch'])}; }}, "
        f"() => {result})"
    )
    body = immediate(plan, method, result)
    return f"  {public} {method.name}({method.parameters}) {method_body(body)}\n"


def scoped_view(plan: OperationPlan, method: Method, public: str, body: list[str]):
    """A `with` method that holds the owner's view scope open while its
    callback reads the copied record, whose pointers stay valid only then."""
    if method.signature or plan.absence:
        raise Unsupported("a borrowed view takes no inputs and is always present")
    assert plan.view is not None
    name = method.name
    reader = "_" + name
    owner = plan.view.owner
    return (
        f"  T {view_method(name)}<T>(T Function(Scoped{public} view) use) => "
        f"_NativeViewScope(_state, raw.{owner.view_begin}, raw.{owner.view_end})"
        f".use((scope) => use(Scoped{public}._(scope, {reader}())));\n\n"
        f"  {public} {reader}() {method_body(body)}\n"
    )


def start(method: Method, *outputs: str) -> str:
    """The closure that makes the call with the completion that the runtime
    allocates, and any outputs that it reads once the call returns."""
    parameters = ", ".join(["arena", "completion", *outputs])
    return f"({parameters}) {block(method.setup + [f'return {method.call};'])}"


def attached(plan: OperationPlan, method: Method) -> str:
    """A call that creates its owner at once and returns it with the
    completion that reports it ready."""
    assert plan.completion is not None
    if (
        len(method.returns) != 1
        or len(plan.completion.immediate_owners) != 1
        or plan.result
    ):
        raise Unsupported("completion requires one immediate owner and void readiness")
    _, adoption = method.returns[0]
    attachment = attachment_name(plan.completion.immediate_owners[0])
    assert method.attach_output is not None
    operation = f"_attach({start(method, method.attach_output)}, (handle) => {adoption}, {attachment}.new)"
    if method.transaction:
        body = [*method.transaction, f"return {operation};"]
        return (
            f"  {attachment} {method.name}({method.parameters}) {{\n"
            + "".join(f"    {line}\n" for line in body)
            + "  }\n"
        )
    return f"  {attachment} {method.name}({method.parameters}) => {operation};\n"


def submitted(method: Method) -> str:
    """The closure of a completion that returns no output, since only an
    attachment returns one."""
    if method.returns:
        raise Unsupported("completion requires one immediate owner and void readiness")
    return start(method)


def commanded(plan: OperationPlan, method: Method) -> str:
    """A command, whose completion reports its disposition."""
    start = submitted(method)
    if plan.result:
        raise Unsupported("command requires a resultless receipt")
    if plan.consumes:
        raise Unsupported("asynchronous consumption requires an owned void receiver")
    return f"  Future<CommandCompletion> {method.name}({method.parameters}) => _command({start});\n"


def completed(plan: OperationPlan, values: Values, method: Method) -> str:
    """A call whose completion delivers its result or only success. A
    consuming one closes its owner once native code accepts it."""
    start = submitted(method)
    if plan.result is None:
        # A release reports native rejection at the call, so its owner can
        # tell whether native accepted it.
        public = "void"
        operation = f"{'_submit' if plan.consumes else '_run'}({start})"
    else:
        result = plan.result
        values.check(result)
        public = values.public(result)
        element = result.element if result.kind == "array" else result
        assert element is not None
        bare = replace(element, nullable=False)
        if result.kind != "array" and bare.kind == "buffer":
            bare = replace(bare, nullable=result.nullable)
        helper = (
            ("_queryOptionalList" if result.nullable else "_queryList")
            if result.kind == "array"
            else "_queryOptional"
            if result.nullable
            else "_query"
        )
        operation = f"{helper}({completion_value(values, element, bare)}, {start})"
    if plan.consumes:
        if public != "void" or not method.receiver:
            raise Unsupported(
                "asynchronous consumption requires an owned void receiver"
            )
        return f"  Future<void> {method.name}({method.parameters}) => _state.closeAsync((handle) => {operation});\n"
    return f"  Future<{public}> {method.name}({method.parameters}) => {operation};\n"


def disposer_arguments(handle, function, live):
    """The arguments of a handle's disposer: the handle, and a null completion
    when the dispose is also the release."""
    arguments = [live]
    for parameter in function.parameters[1:]:
        if parameter.name != handle.dispose_completion:
            raise Unsupported(f"{function.name} disposes with more than its handle")
        arguments.append("nullptr")
    return arguments


def adopt_owner(owned, expression, function, values):
    if owned.handle.native not in values.bound.public_handles:
        raise Unsupported("owned output requires a generated class")
    public, native = owner_names(owned.handle)
    parent = ""
    if owned.handle.parent in values.bound.public_handles:
        if not owned.parent_parameter:
            raise Unsupported("owned output requires its parent owner")
        parent = "this"
    arguments = (parent + ", " if parent else "") + f"{native}({expression})"
    result = f"{public}._({arguments})"
    if values.bound.operations_by_name[function.name].registrations:
        result = f"({result}.._state.retain(registrations))"
    if not owned.handle.cleanup:
        raise Unsupported("owned adoption requires a native disposer")
    dispose = values.bound.source.functions_by_name[owned.handle.cleanup]
    cleanup = native_call(dispose, disposer_arguments(owned.handle, dispose, "handle"))
    if dispose.return_type.kind != "void":
        cleanup = f"_check({cleanup})"
    result = f"_adoptOwned({expression}, () => {result}, (handle) {{ {cleanup}; }})"
    return public, result


def block(body: list[str]) -> str:
    """A closure body: an expression for one return, a block otherwise."""
    if len(body) == 1 and body[0].startswith("return "):
        return "=> " + body[0].removeprefix("return ").removesuffix(";")
    return "{\n" + "\n".join(f"      {line}" for line in body) + "\n    }"


def closure(parameter: str, body: list[str]) -> str:
    """A closure over one parameter, with a scratch arena when it allocates."""
    if any(re.search(r"\barena\b", line) for line in body):
        return f"({parameter}) => withNativeArena((arena) {block(body)})"
    return f"({parameter}) {block(body)}"


def method_body(body: list[str]) -> str:
    """A method body, which takes a scratch arena only when it allocates."""
    uses_arena = any(re.search(r"\barena\b", line) for line in body)
    if not uses_arena:
        if len(body) == 1:
            line = body[0].removeprefix("return ")
            return f"=> {line}"
        return "{\n" + "\n".join(f"    {line}" for line in body) + "\n  }"
    return f"=> withNativeArena((arena) {block(body)});"


def completion_value(values: Values, element, read) -> str:
    """The descriptor of how a completion copies and reads one value type."""
    ffi = values.ffi(element)
    suffix = (
        ".ref"
        if element.kind == "record" or element.buffer_form == "view"
        else ".value"
    )
    code = (
        f"_CompletionValue(sizeOf<{ffi}>(), "
        f"(element) => {values.copy(read, f'element.cast<{ffi}>(){suffix}')})"
    )
    stem = "_result" + re.sub(r"\W", "", values.public(read).replace("?", "OrNull"))
    name, index = stem, 2
    while values.results.get(name, code) != code:
        name, index = f"{stem}{index}", index + 1
    values.results[name] = code
    return name
