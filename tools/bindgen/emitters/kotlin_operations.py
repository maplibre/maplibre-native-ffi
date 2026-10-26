"""Lower each operation plan to one common Kotlin function over the runtime helpers.

A generated operation names its C function, maps its parameters, and decodes
its result. Keep-alive, arenas, completions, status checks, and callback
acceptance live in the handwritten `internal/call/NativeCall.kt`.
"""

from __future__ import annotations

from dataclasses import replace

from ..semantic import output_member
from .kotlin_values import Unsupported, identifier, is_text, name, owner_name

EXECUTIONS = {
    "command",
    "query",
    "snapshot",
    "immediate",
    "operation",
    "lifecycle",
    "event_batch",
}

# The locals that generated bodies declare, which a parameter must not shadow.
# A parameter can share a helper function's name, because Kotlin resolves a call
# to the function when the parameter's type has no `invoke`.
LOCALS = {
    "handle",
    "completion",
    "diagnostic",
    "out",
    "batch",
    "raw",
    "holder",
    "registrations",
    "binding",
    "bindingCallbacks",
    "scope",
    "block",
}


class Native:
    """Records the C functions that generated code calls."""

    def __init__(self, bound):
        self.bound = bound
        self.functions = {}

    def call(self, function, arguments):
        self.functions[function.name] = function
        return f"C.{function.name}({', '.join(arguments)})"

    def checked(self, function, arguments):
        """A call that throws on a failed status, or the bare call for one without a status."""
        if not function.diagnostic:
            return self.call(function, arguments)
        return f"check({self.call(function, [*arguments, 'diagnostic'])})"

    def present(self, plan, arguments):
        """A call whose absence status returns false and whose other failures throw.

        Kotlin has no C constants, so the call passes the status's value.
        """
        call = self.call(plan.function, [*arguments, "diagnostic"])
        return f"present({call}, absent = {plan.absence.value})"

    def status(self, function, arguments):
        """The status-returning call for a runtime helper that checks it."""
        if not function.diagnostic:
            raise Unsupported("helper-checked call requires a status and diagnostic")
        return self.call(function, [*arguments, "diagnostic"])


def parameter_name(native):
    local = identifier(native)
    return local + "Value" if local in LOCALS else local


def receiver_value(plan):
    value = next(p.value for p in plan.inputs if p.name == plan.receiver)
    return value.unwrapped


def method_name(plan):
    """Name an operation's method after its language-neutral member."""
    return identifier(plan.member)


def public_method(plan):
    """The method that a host calls for [plan]: a view's is the `with` function
    that lends its value to a block."""
    return identifier("with_" + plan.view.stem) if plan.view else method_name(plan)


def copies_borrowed(value):
    if not value:
        return False
    if value.kind in {"array", "buffer", "reference"}:
        return True
    return any(copies_borrowed(field.value) for field in value.fields)


def needs_read(plan):
    return bool(
        plan.receiver
        and not plan.completion
        and not plan.consumes
        and not plan.owned_outputs
        and not plan.view
        and any(
            copies_borrowed(output.value.unwrapped)
            for output in plan.outputs
            if not output.sized_by and not output.result_length
        )
    )


def receiver_arguments(plan):
    return "this, binding" if plan.receiver else "null, null"


def call_helper(plan):
    """The helper that runs [plan]'s call: a read holds off its receiver's release."""
    helper = "nativeRead" if needs_read(plan) else "nativeCall"
    return f"{helper}({receiver_arguments(plan)})"


def signature(plan, values):
    """The method name, public parameters, inputs, result plan, and result type."""
    if (
        plan.registrations
        and any(not registration.path for registration in plan.registrations)
        and not plan.completion
    ) or plan.scoped_receiver:
        raise Unsupported("operation requires an owner or callback transaction")
    receiver = next((p for p in plan.inputs if p.name == plan.receiver), None)
    if receiver and receiver_value(plan).native not in values.bound.public_handles:
        raise Unsupported("operation requires its generated owner")
    if plan.execution not in EXECUTIONS:
        raise Unsupported("execution requires another Kotlin runtime primitive")
    lengths = {
        p.value.length
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"} and p.value.length
    } | {p.value.length for p in plan.outputs if p.result_length}
    inputs = [
        p for p in plan.inputs if p.name != plan.receiver and p.name not in lengths
    ]
    params = []
    for parameter in inputs:
        values.check(parameter.value)
        params.append((parameter_name(parameter.name), values.public(parameter.value)))
    result = (
        plan.outputs[0].value
        if plan.completion and plan.completion.immediate_owners
        else plan.result
        if plan.completion
        else plan.outputs[0].value
        if plan.outputs
        else plan.result
    )
    if plan.caller_sized_text:
        result_type, result = "String", None
    elif plan.drain:
        result = plan.drain.view.value.unwrapped
        values.check(result)
        result_type = values.public(result)
    elif len(plan.outputs) > 1:
        result_type = outputs_signature(plan, values)
        result = None
    elif result:
        if (
            not plan.completion or plan.completion.immediate_owners
        ) and result.kind == "reference":
            result = result.element
        values.check(result)
        result_type = values.public(result)
    else:
        result_type = "CommandCompletion" if plan.execution == "command" else "Unit"
    return method_name(plan), params, inputs, result, result_type


def outputs_signature(plan, values):
    result_name = name(plan.name) + "Result"
    fields = []
    for parameter in plan.outputs:
        value = parameter.value.unwrapped
        values.check(value)
        if value.kind not in {"scalar", "enum", "record", "buffer", "array"}:
            raise Unsupported("output parameter requires copied value storage")
        fields.append((identifier(output_member(parameter.name)), value))
    values.multiple[result_name] = fields
    return result_name


def declaration(params):
    """Kotlin parameter declarations, defaulting each nullable parameter to null."""
    return ", ".join(
        f"{local}: {typ}" + (" = null" if typ.endswith("?") else "")
        for local, typ in params
    )


def counted_inputs(plan):
    """The array and buffer inputs that C takes with a separate length, by length parameter."""
    return {
        p.value.length: p
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"}
        and p.value.length not in {None, "nul", "1"}
    }


def encoded_text(plan):
    """The text and JSON inputs with a separate length, which a call encodes once into a local."""
    return [
        p
        for p in counted_inputs(plan).values()
        if p.value.kind == "buffer" and is_text(p.value)
    ]


def encodings(plan):
    """The statements that encode each of [plan]'s `encoded_text` inputs, to start a body."""
    statements = []
    for parameter in encoded_text(plan):
        local = parameter_name(parameter.name)
        nullable = "?" if parameter.value.nullable else ""
        statements.append(f"val {local}Utf8 = {local}{nullable}.encodeToByteArray(); ")
    return "".join(statements)


def call_arguments(plan, values):
    """The native-call carrier for each C parameter of [plan], in C order.

    An `encoded_text` input refers to the local that `encodings` declares.
    """
    lengths = counted_inputs(plan)
    encoded = {p.name for p in encoded_text(plan)}

    def source(parameter):
        local = parameter_name(parameter.name)
        if parameter.name in encoded:
            return replace(parameter.value, encoding="bytes"), local + "Utf8"
        return parameter.value, local

    result = []
    for parameter in plan.inputs:
        if parameter.name == plan.receiver:
            result.append("handle")
        elif parameter.name in lengths:
            result.append(values.length(*source(lengths[parameter.name])))
        else:
            result.append(values.argument(*source(parameter)))
    return result


def output_storage(value, values, count=None):
    """Allocate one output and decode it after the call: (allocation, decoded).

    A caller-sized output array takes [count] elements.
    """
    if value.kind == "array" and count is not None:
        element = value.element
        size = values.element_size(element)
        align = values.element_align(element)
        allocation = f"allocate({count}.toLong() * {size}, {align})"
        size_field = next((f for f in element.fields if f.role == "size"), None)
        if size_field:
            write = values.write_scalar(
                size_field.value,
                values.at("item", element, size_field.name),
                f"{size}.toUInt()",
            )
            allocation += f".also {{ base -> repeat({count}) {{ index -> val item = base + index.toLong() * {size}; {write} }} }}"
        item = values.decode_item(element, "item")
        return (
            allocation,
            f"readArray({{out}}, {count}.toULong(), {size}.toLong()) {{ item -> {item} }}",
        )
    if value.kind == "record" or (
        value.kind == "buffer" and value.buffer_form == "view"
    ):
        if value.kind == "buffer":
            allocation = (
                "allocate(2 * NativeMemory.addressSize, NativeMemory.addressSize)"
            )
        else:
            allocation = (
                f"allocate({values.size(value.native)}, {values.align(value.native)})"
            )
            size = next((f for f in value.fields if f.role == "size"), None)
            if size and values.sized(value, size):
                allocation = (
                    f"sized({values.size(value.native)}, {values.align(value.native)})"
                )
            elif size:
                allocation += (
                    ".also { "
                    + values.write_scalar(
                        size.value,
                        values.at("it", value, size.name),
                        f"{values.size(value.native)}.toUInt()",
                    )
                    + " }"
                )
        return allocation, values.decode(value, "{out}")
    if value.kind in {"scalar", "enum"}:
        return values.storage(value.ctype), values.decode(value, "{out}")
    raise Unsupported("immediate output requires scalar or copied record storage")


def output_count(plan, index):
    """The element count of [plan]'s output at [index] when the caller sizes it."""
    sized_by = plan.outputs[index].sized_by
    return f"{parameter_name(sized_by)}.size" if sized_by else None


def result_value(value, values):
    """Convert a C function's carrier result to its public type."""
    if value.kind == "native_pointer":
        return "NativePointer.ofAddress({raw})"
    if value.kind == "enum":
        _suffix, typ = values.accessor(value)
        return name(value.native) + "({raw}" + PUBLIC_FROM_CARRIER[typ] + ")"
    if value.kind == "scalar":
        typ = values.scalar(value)[0]
        return "{raw}" + PUBLIC_FROM_CARRIER[typ]
    raise Unsupported("immediate result requires a scalar or record")


PUBLIC_FROM_CARRIER = {
    "Boolean": "",
    "Double": "",
    "Float": "",
    "Byte": "",
    "Short": "",
    "Int": "",
    "Long": "",
    "UByte": ".toUByte()",
    "UShort": ".toUShort()",
    "UInt": ".toUInt()",
    "ULong": ".toULong()",
}


def completion_decode(value, values):
    """The decode lambda body for a completion result at `result`."""
    bare = replace(value, nullable=False)
    if value.kind == "array":
        element = value.element
        item = values.decode_item(element, "it")
        expression = (
            "readStrided(CompletionBridge.valuePointer(result), CompletionBridge.valueCount(result), "
            f"CompletionBridge.valueSize(result), {values.element_size(element)}) {{ {item} }}"
        )
    elif value.kind in {"buffer", "record", "scalar", "enum"}:
        expression = values.decode(bare, "CompletionBridge.value(result)")
    else:
        raise Unsupported("completion result requires a copied value")
    if value.nullable:
        absent = (
            "CompletionBridge.valuePointer(result) == 0L"
            if value.kind == "array"
            else "CompletionBridge.valueCount(result) == 0uL"
        )
        expression = f"if ({absent}) null else {expression}"
    return "{ result -> " + expression + " }"


def operation(plan, values, native, *, discard=False):
    """The common Kotlin method for [plan]."""
    from . import kotlin_callbacks

    callback = kotlin_callbacks.operation(plan, values, native)
    if callback is not None:
        return callback
    if plan.blocks and not (plan.owned_outputs and not plan.completion):
        raise Unsupported(f"blocks={plan.blocks} needs one owned output")
    if plan.consumes:
        return lifecycle(plan, values, native)
    text = plan.caller_sized_text
    if text:
        return caller_sized_text(plan, values, native, text)
    if plan.drain:
        return drain(plan, values, native)
    if len(plan.outputs) > 1:
        return multiple_outputs(plan, values, native)
    if plan.view:
        return view(plan, values, native)
    if (plan.result and plan.result.kind == "handle") or plan.owned_outputs:
        return owned(plan, values, native)
    if not plan.completion:
        return immediate(plan, values, native)
    method, params, _inputs, result, result_type = signature(plan, values)
    arguments = call_arguments(plan, values) + ["completion"]
    body = encodings(plan) + native.checked(plan.function, arguments)
    head = f"  public fun {method}({declaration(params)}): Deferred<{result_type}> ="
    owners = "owner, binding" if discard else receiver_arguments(plan)
    callbacks = ", bindingCallbacks" if plan.registrations else ""
    if discard:
        head = f"  public fun {method}({declaration(params)}): Unit ="
        helper = f"nativeDiscard({owners}{callbacks})"
    elif plan.execution == "command":
        helper = f"nativeCommand({owners}{callbacks})"
    elif result is None:
        helper = f"nativeUnit({owners}{callbacks})"
    else:
        helper = (
            f"nativeSubmit({owners}, {completion_decode(result, values)}{callbacks})"
        )
    return f"{head} {helper} {{ {body} }}\n"


def immediate(plan, values, native):
    method, params, _inputs, result, result_type = signature(plan, values)
    arguments = call_arguments(plan, values)
    setup, decoded = [], None
    if plan.outputs:
        allocation, decoded = output_storage(result, values, output_count(plan, 0))
        setup.append(f"val out = {allocation}")
        arguments.append("out")
        decoded = decoded.replace("{out}", "out")
    if result and not plan.outputs and result.kind == "record":
        # The shim writes a record that C returns by value to a trailing address.
        allocation, decoded = output_storage(result, values)
        setup.append(f"val out = {allocation}")
        setup.append(native.call(plan.function, [*arguments, "out"]))
        decoded = decoded.replace("{out}", "out")
    elif result and not plan.outputs:
        setup.append("val raw = " + native.call(plan.function, arguments))
        decoded = result_value(result, values).replace("{raw}", "raw")
    elif plan.absence:
        raise Unsupported("absence requires an owned handle output")
    else:
        setup.append(native.checked(plan.function, arguments))
    if decoded:
        setup.append(decoded)
    # The ABI version check reads the C version, so reading it skips the check.
    helper = (
        "nativeUncheckedCall" if plan.name == "mln_c_version" else call_helper(plan)
    )
    body = encodings(plan) + "; ".join(setup)
    return f"  public fun {method}({declaration(params)}): {result_type} = {helper} {{ {body} }}\n"


def caller_sized_text(plan, values, native, text):
    """Text that native writes into storage the call sizes.

    The call starts with storage of a guessed size and calls again with
    enough storage while the reported length does not fit. Its inputs convert
    once, before the first call.
    """
    method, params, _inputs, _result, result_type = signature(plan, values)
    setup, inputs = [], {}
    for parameter, argument in zip(
        plan.inputs, call_arguments(plan, values), strict=True
    ):
        if parameter.name in {plan.receiver, text.value.length}:
            inputs[parameter.name] = argument
            continue
        local = "native" + name(parameter.name)
        setup.append(f"val {local} = {argument}")
        inputs[parameter.name] = local
    length = next(p for p in plan.outputs if p.name == text.result_length)
    carriers = {
        text.name: "textStorage",
        text.value.length: "textCapacity",
        text.result_length: "textLength",
    }
    arguments = [
        carriers.get(p.name) or inputs[p.name] for p in plan.function.parameters
    ]
    call = native.checked(plan.function, arguments)
    written = values.decode(length.value.unwrapped, "textLength")
    # The capacity stays a signed Long: comparing two ULongs needs
    # Long.compareUnsigned, which Android adds above the binding's API floor.
    setup += [
        f"val textLength = {values.storage(length.value.unwrapped.ctype)}",
        "var textCapacity = 256L",
        "var textStorage = allocate(textCapacity, 1)",
        "while (true) { "
        + "; ".join(
            [
                call,
                f"val written = {written}.toLong()",
                "if (written < textCapacity) break",
                "textCapacity = written + 1",
                "textStorage = allocate(textCapacity, 1)",
            ]
        )
        + " }",
        f"readBytes(textStorage, {written}).decodeToString()",
    ]
    # The ABI version check reads the C version, so reading it skips the check.
    helper = (
        "nativeUncheckedCall" if plan.name == "mln_c_version" else call_helper(plan)
    )
    body = encodings(plan) + "; ".join(setup)
    return f"  public fun {method}({declaration(params)}): {result_type} = {helper} {{ {body} }}\n"


def multiple_outputs(plan, values, native):
    method, params, _inputs, _result, result_type = signature(plan, values)
    arguments = call_arguments(plan, values)
    setup, copied = [], []
    for index, (field, value) in enumerate(values.multiple[result_type]):
        allocation, decoded = output_storage(value, values, output_count(plan, index))
        setup.append(f"val out{index} = {allocation}")
        arguments.append(f"out{index}")
        copied.append(f"{field} = " + decoded.replace("{out}", f"out{index}"))
    setup.append(native.checked(plan.function, arguments))
    setup.append(f"{result_type}({', '.join(copied)})")
    # The ABI version check reads the C version, so reading it skips the check.
    helper = (
        "nativeUncheckedCall" if plan.name == "mln_c_version" else call_helper(plan)
    )
    body = encodings(plan) + "; ".join(setup)
    return f"  public fun {method}({declaration(params)}): {result_type} = {helper} {{ {body} }}\n"


def drain(plan, values, native):
    """A drain that copies its batch's slice and releases the batch, or returns
    an empty list when nothing is queued."""
    method, params, _inputs, result, result_type = signature(plan, values)
    batch, view = plan.drain.batch, plan.drain.view
    handle = batch.value.unwrapped.handle
    slot = next(
        p.type.pointee for p in plan.function.parameters if p.name == batch.name
    )
    allocation, decoded = output_storage(result, values)
    storage = {batch.name: "batch", view.name: "out"}
    arguments = call_arguments(plan, values) + [storage[p.name] for p in plan.outputs]
    function = values.bound.source.functions_by_name[handle.release]
    if function.diagnostic or function.return_type.kind != "void":
        raise Unsupported("a drained batch needs a void release")
    release = native.call(function, ["it"])
    body = (
        f"val batch = {values.storage(slot)}; val out = {allocation}; "
        f"if ({native.present(plan, arguments)}) "
        f"drain(batch, {{ {release} }}) {{ {decoded.replace('{out}', 'out')} }} "
        "else emptyList()"
    )
    head = f"  public fun {method}({declaration(params)}): {result_type} ="
    return f"{head} {call_helper(plan)} {{ {encodings(plan)}{body} }}\n"


def disposal_method(handle):
    return identifier(handle.cleanup.removeprefix(handle.native + "_"))


def disposer_arguments(handle, function, receiver, null):
    """The arguments before the diagnostic of [function] when it disposes [handle].

    A disposer reports nothing, so it passes [null] for a dispose's optional
    completion.
    """
    if handle.dispose_completion and function.name == handle.dispose:
        return [receiver, null]
    return [receiver]


def disposal(handle):
    family = name(handle.native)
    return "GeneratedOwnerDisposal::" + family[0].lower() + family[1:]


def owned(plan, values, native):
    method, params, inputs, result, result_type = signature(plan, values)
    immediate_owner = bool(plan.owned_outputs)
    handle = plan.owned_outputs[0].handle if immediate_owner else result.handle
    if handle.native not in values.bound.public_handles:
        raise Unsupported("owned result needs an adoption boundary")
    parent = ""
    if handle.parent:
        parent_param = next(
            (
                p
                for p in plan.inputs
                if p.value.kind == "handle" and p.value.native == handle.parent
            ),
            None,
        )
        if not parent_param:
            raise Unsupported("owned result requires parent")
        parent = ", " + (
            f"this@Generated{name(receiver_value(plan).native)}Operations as {values.public(parent_param.value)}"
            if parent_param.name == plan.receiver
            else parameter_name(parent_param.name)
        )
    create = f"{{ {owner_name(handle)}(it{parent}) }}"
    attachment = immediate_owner and bool(plan.completion)
    if attachment:
        returns = "New" + name(handle.native)
        values.attachments[returns] = (
            identifier(output_member(plan.owned_outputs[0].parameter)),
            result,
        )
    else:
        returns = f"Deferred<{result_type}>" if plan.completion else result_type
    head = f"  public fun {method}({declaration(params)}): {returns} ="
    arguments = call_arguments(plan, values)
    owners = receiver_arguments(plan)
    arguments.append("out")
    registered = any(values.needs_registration(p.value) for p in inputs)
    if plan.blocks:
        return thread_start(plan, values, native, head, arguments, create, handle)
    callbacks = "{ it.bindingCallbacks }" if registered else "null"
    drop = f"{{ it.{disposal_method(handle)}() }}"
    if registered:
        adopted = (
            f"adoptRegistered(out, {disposal(handle)}, {create}, {callbacks}, {drop})"
        )
    else:
        adopted = f"adopt(out, {disposal(handle)}) {create}"
    if attachment:
        arguments.append("completion")
        call = native.status(plan.function, arguments)
        body = (
            f"attach({{ out, completion -> {call} }}, {disposal(handle)}, {create}, "
            f"{callbacks}, {drop}, ::{returns})"
        )
    else:
        slot = next(
            p.type.pointee
            for p in plan.function.parameters
            if p.name == plan.owned_outputs[0].parameter
        )
        if plan.absence:
            body = f"val out = {values.storage(slot)}; if ({native.present(plan, arguments)}) {adopted} else null"
            head = head.removesuffix(" =") + "? ="
        else:
            body = f"val out = {values.storage(slot)}; {native.checked(plan.function, arguments)}; {adopted}"
    return f"{head} nativeCall({owners}) {{ {encodings(plan)}{body} }}\n"


def thread_start(plan, values, native, head, arguments, create, handle):
    """A call that waits for a thread to start, as a suspend function.

    A thread that a browser page starts runs only after the page yields, so
    the call suspends where the page cannot wait. The call writes its own
    diagnostic, because other calls run while it waits. A caller cancelled
    meanwhile drops the owner that the call created.
    """
    if (
        plan.receiver
        or plan.absence
        or any(values.needs_registration(p.value) for p in plan.inputs)
    ):
        raise Unsupported(f"blocks={plan.blocks} takes no receiver or registration")
    slot = next(
        p.type.pointee
        for p in plan.function.parameters
        if p.name == plan.owned_outputs[0].parameter
    )
    call = native.call(plan.function, [*arguments, "diagnostic"])
    adopted = f"adopt(out, {disposal(handle)}) {create}"
    body = f"val out = {values.storage(slot)}; threadStart {{ diagnostic -> {call} }}; {adopted}"
    head = head.replace("  public fun ", "  public suspend fun ", 1)
    drop = f"{{ it.{disposal_method(handle)}() }}"
    return f"{head} nativeThreadStart({drop}) {{ {encodings(plan)}{body} }}\n"


def lifecycle(plan, values, native):
    method, params, _inputs, _result, _result_type = signature(plan, values)
    arguments = call_arguments(plan, values)
    receiver_index = next(
        i for i, p in enumerate(plan.inputs) if p.name == plan.receiver
    )
    setup = []
    if plan.inputs[receiver_index].value.kind == "reference":
        slot = next(
            p.type.pointee for p in plan.function.parameters if p.name == plan.receiver
        )
        setup.append(
            f"val holder = {values.storage(slot)}.also {{ writeI64(it, handle) }}"
        )
        arguments[receiver_index] = "holder"
    parameters = declaration(params)

    def body(*completion):
        call = native.checked(plan.function, [*arguments, *completion])
        return encodings(plan) + "; ".join([*setup, call])

    if not plan.completion:
        close = "nativeClose" if plan.release else "nativeAnswer"
        return f"  public fun {method}({parameters}): Unit = {close}(this, binding) {{ {body()} }}\n"
    retire = (
        f"  public fun {method}({parameters}): Deferred<Unit> = "
        f"nativeRetire(this, binding) {{ {body('completion')} }}\n"
    )
    if not plan.completion.optional:
        return retire
    # The release is also the handle's dispose, which passes no completion.
    from .kotlin import operation_doc

    dispose = disposal_method(receiver_value(plan).handle)
    return (
        retire
        + "\n"
        + operation_doc(values.bound, plan, "  ")
        + f"  public fun {dispose}({parameters}): Unit = nativeClose(this, binding) {{ {body('0L')} }}\n"
    )


def view(plan, values, native):
    values.views.add(plan.outputs[0].value.element.native)
    method, _params, _inputs, result, result_type = signature(plan, values)
    method = public_method(plan)
    functions = values.bound.source.functions_by_name
    allocation, _decoded = output_storage(result, values)
    begin = native.status(functions[plan.view.begin], ["handle", "it"])
    end = native.checked(functions[plan.view.end], ["it"])
    read = native.checked(plan.function, ["handle", "out"])
    decoded = values.decode(result, "out", "scope")
    # Native refuses to release the owner while the view is open.
    return (
        f"  public fun <T> {method}(block: ({result_type}) -> T): T = "
        f"nativeCall({receiver_arguments(plan)}) {{ "
        f"borrowView({{ {begin} }}, {{ {end} }}) {{ scope -> "
        f"val out = {allocation}; {read}; block({decoded}) }} }}\n"
    )


def owner_disposal(bound, native):
    """The any-thread disposal of every public handle, for leak cleanup."""
    lines = []
    for handle in bound.public_handles.values():
        if not handle.cleanup or handle.cleanup in bound.source.runtime_exports:
            continue
        method = name(handle.native)
        method = method[0].lower() + method[1:]
        function = bound.source.functions_by_name[handle.cleanup]
        arguments = disposer_arguments(handle, function, "handle", "0L")
        call = (
            native.call(function, arguments)
            if not function.diagnostic
            else native.status(function, arguments)
        )
        if function.diagnostic:
            call = "NativeDiagnostics.check { diagnostic -> " + call + " }"
        lines.append(f"  fun {method}(handle: Long) {{ {call} }}")
    return "internal object GeneratedOwnerDisposal {\n" + "\n".join(lines) + "\n}\n"
