"""Lower each operation plan to one common Kotlin function over the runtime helpers.

A generated operation names its C function, maps its parameters, and decodes
its result. Admission, keep-alive, arenas, completions, status checks, and
callback acceptance live in the hand-written `internal/call/NativeCall.kt`.
"""

from __future__ import annotations

from dataclasses import replace

from ..managed_contracts import LOCALS
from ..names import camel
from .kotlin_values import Unsupported, identifier, name, owner_name

EXECUTIONS = {
    "command",
    "query",
    "snapshot",
    "immediate",
    "operation",
    "render_driver",
    "drain",
    "lifecycle",
    "event_batch",
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
            if "mln_status" in (
                function.return_type.declaration,
                function.return_type.spelling,
            ):
                raise Unsupported("status result requires a diagnostic parameter")
            return self.call(function, arguments)
        return f"check({self.call(function, [*arguments, 'diagnostic'])})"


def parameter_name(native):
    local = identifier(native)
    return local + "Value" if local in LOCALS else local


def receiver_value(plan):
    value = next(p.value for p in plan.inputs if p.name == plan.receiver)
    return value.element if value.kind == "reference" else value


def method_name(plan):
    """Name an operation's method after its receiver prefix, or its whole name."""
    prefix = receiver_value(plan).native + "_" if plan.receiver else "mln_"
    return identifier(
        plan.name.removeprefix(prefix if plan.name.startswith(prefix) else "mln_")
    )


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
            copies_borrowed(
                output.value.element
                if output.value.kind == "reference"
                else output.value
            )
            for output in plan.outputs
        )
    )


def receiver_arguments(plan):
    return "this, binding" if plan.receiver else "null, null"


def access(plan):
    if plan.receiver and plan.receiver_access == "issued":
        return ", Access.ISSUED"
    if needs_read(plan):
        return ", Access.READ"
    return ""


def signature(plan, values):
    """The method name, public parameters, inputs, result plan, and result type."""
    if (
        (
            plan.registrations
            and any(not registration.path for registration in plan.registrations)
        )
        or plan.direct_registrations
        or plan.scoped_receiver
    ):
        raise Unsupported("operation requires an owner or callback transaction")
    if plan.view:
        values.views.add(plan.outputs[0].value.element.native)
    receiver = next((p for p in plan.inputs if p.name == plan.receiver), None)
    if receiver and receiver_value(plan).native not in values.bound.public_handles:
        raise Unsupported("operation requires its generated owner")
    if plan.execution not in EXECUTIONS:
        raise Unsupported("execution requires another Kotlin runtime primitive")
    lengths = {
        p.value.length
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"} and p.value.length
    }
    inputs = [
        p for p in plan.inputs if p.name != plan.receiver and p.name not in lengths
    ]
    params = []
    for parameter in inputs:
        if parameter.value.kind != "handle" and parameter.value.lifetime not in {
            "call",
            "value",
        }:
            raise Unsupported("input requires retained storage")
        if (
            parameter.value.kind == "buffer"
            and parameter.value.nullable
            and parameter.value.buffer_form != "view"
        ):
            raise Unsupported(
                "nullable input buffer requires independent presence storage"
            )
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
    if len(plan.outputs) > 1:
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
        value = (
            parameter.value.element
            if parameter.value.kind == "reference"
            else parameter.value
        )
        values.check(value)
        if value.kind not in {"scalar", "enum", "record", "buffer"}:
            raise Unsupported("output parameter requires copied value storage")
        fields.append((identifier(parameter.name.removeprefix("out_")), value))
    values.multiple[result_name] = fields
    return result_name


def declaration(params, defaults=None):
    """Kotlin parameter declarations, defaulting each nullable parameter to null."""
    result = []
    for local, typ in params:
        default = (defaults or {}).get(local)
        if default is None and typ.endswith("?"):
            default = "null"
        result.append(f"{local}: {typ}" + (f" = {default}" if default else ""))
    return ", ".join(result)


def call_arguments(plan, values):
    """The native-call carrier for each C parameter of [plan], in C order."""
    lengths = {
        p.value.length: p
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"}
        and p.value.length not in {None, "nul", "1"}
    }
    result = []
    for parameter in plan.inputs:
        if parameter.name == plan.receiver:
            result.append("handle")
        elif parameter.name in lengths:
            source = lengths[parameter.name]
            result.append(values.length(source.value, parameter_name(source.name)))
        else:
            result.append(
                values.argument(parameter.value, parameter_name(parameter.name))
            )
    return result


def output_storage(value, values):
    """Allocate one output and decode it after the call: (allocation, decoded)."""
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
            if size:
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
        return "allocate(8)", values.decode(value, "{out}")
    raise Unsupported("immediate output requires scalar or copied record storage")


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
    bare = replace(value, nullable=False, optional=None)
    if value.kind == "array":
        element = value.element
        item = values.decode_item(element, "it", None, None)
        expression = (
            "readArray(CompletionBridge.valuePointer(result), CompletionBridge.valueCount(result), "
            f"{values.element_size(element)}.toLong()) {{ {item} }}"
        )
    elif value.kind in {"buffer", "record", "scalar", "enum"}:
        expression = values.decode(
            replace(bare, optional=None), "CompletionBridge.value(result)"
        )
    else:
        raise Unsupported("completion result requires a copied value")
    if value.optional == "empty":
        expression += ".takeIf { it.isNotEmpty() }"
    elif value.nullable:
        absent = (
            "CompletionBridge.valuePointer(result) == 0L"
            if value.kind == "array"
            else "CompletionBridge.valueCount(result) == 0uL"
        )
        expression = f"if ({absent}) null else {expression}"
    return "{ result -> " + expression + " }"


def operation(plan, values, native):
    """The common Kotlin method for [plan]."""
    from . import kotlin_callbacks

    callback = kotlin_callbacks.operation(plan, values, native)
    if callback is not None:
        return callback
    method, params, _inputs, result, result_type = signature(plan, values)
    if plan.consumes:
        return lifecycle(plan, values, native)
    if len(plan.outputs) > 1:
        return multiple_outputs(plan, values, native)
    if plan.view:
        return view(plan, values, native)
    if (plan.result and plan.result.kind == "handle") or plan.owned_outputs:
        return owned(plan, values, native)
    if not plan.completion:
        return immediate(plan, values, native)
    arguments = call_arguments(plan, values) + ["completion"]
    body = native.checked(plan.function, arguments)
    head = f"  public fun {method}({declaration(params)}): Deferred<{result_type}> ="
    owners = receiver_arguments(plan)
    callbacks = ", bindingCallbacks" if plan.registrations else ""
    if plan.execution == "command":
        helper = f'nativeCommand({owners}, "{plan.name}"{callbacks})'
    elif result is None:
        helper = f'nativeUnit({owners}, "{plan.name}"{callbacks})'
    else:
        helper = f'nativeSubmit({owners}, "{plan.name}", {completion_decode(result, values)}{callbacks})'
    return f"{head} {helper} {{ {body} }}\n"


def immediate(plan, values, native):
    method, params, _inputs, result, result_type = signature(plan, values)
    arguments = call_arguments(plan, values)
    setup, decoded = [], None
    if plan.outputs:
        allocation, decoded = output_storage(result, values)
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
    else:
        setup.append(native.checked(plan.function, arguments))
    if decoded:
        setup.append(decoded)
    helper = f'nativeCall({receiver_arguments(plan)}, "{plan.name}"{access(plan)})'
    body = "; ".join(setup)
    return f"  public fun {method}({declaration(params)}): {result_type} = {helper} {{ {body} }}\n"


def multiple_outputs(plan, values, native):
    method, params, _inputs, _result, result_type = signature(plan, values)
    arguments = call_arguments(plan, values)
    setup, copied = [], []
    for index, (field, value) in enumerate(values.multiple[result_type]):
        allocation, decoded = output_storage(value, values)
        setup.append(f"val out{index} = {allocation}")
        arguments.append(f"out{index}")
        copied.append(f"{field} = " + decoded.replace("{out}", f"out{index}"))
    setup.append(native.checked(plan.function, arguments))
    setup.append(f"{result_type}({', '.join(copied)})")
    helper = f'nativeCall({receiver_arguments(plan)}, "{plan.name}"{access(plan)})'
    return f"  public fun {method}({declaration(params)}): {result_type} = {helper} {{ {'; '.join(setup)} }}\n"


def disposal_method(handle):
    return identifier(handle.dispose.removeprefix(handle.native + "_"))


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
    create = f"{{ {owner_name(handle.native)}(it{parent}) }}"
    attachment = immediate_owner and bool(plan.completion)
    if attachment:
        returns = name(handle.native) + "Attachment"
        values.attachments[returns] = (
            identifier(plan.owned_outputs[0].parameter.removeprefix("out_")),
            result,
        )
    else:
        returns = f"Deferred<{result_type}>" if plan.completion else result_type
    head = f"  public fun {method}({declaration(params)}): {returns} ="
    arguments = call_arguments(plan, values)
    owners = receiver_arguments(plan)
    if plan.completion and not immediate_owner:
        arguments.append("completion")
        body = native.checked(plan.function, arguments)
        drop = "it." + disposal_method(handle) + "()"
        return (
            f'{head} nativeSubmitOwned({owners}, "{plan.name}", {create}, {disposal(handle)}, '
            f"{{ {drop} }}) {{ {body} }}\n"
        )
    arguments.append("out")
    registered = any(values.needs_registration(p.value) for p in inputs)
    adopted = f"adopt(readI64(out), {disposal(handle)}) {create}"
    if registered:
        adopted += f".let {{ accept(it, it.bindingCallbacks) {{ it.{disposal_method(handle)}() }} }}"
    setup = ["val out = allocate(8)"]
    if attachment:
        arguments.append("completion")
        setup.append(
            "val ready = CompletionBridge.unitChecked { completion -> "
            + native.checked(plan.function, arguments)
            + " }"
        )
        setup.append(f"{returns}({adopted}, ready)")
    else:
        setup.append(native.checked(plan.function, arguments))
        setup.append(adopted)
    return f'{head} nativeCall({owners}, "{plan.name}") {{ {"; ".join(setup)} }}\n'


def lifecycle(plan, values, native):
    method, params, inputs, _result, _result_type = signature(plan, values)
    defaults = {}
    for (local, _typ), parameter in zip(params, inputs, strict=True):
        value = (
            parameter.value.element
            if parameter.value.kind == "reference"
            else parameter.value
        )
        if value.default and not _typ.endswith("?"):
            defaults[local] = (
                "GeneratedApi." + camel(value.default.removeprefix("mln_")) + "()"
            )
    arguments = call_arguments(plan, values)
    receiver_index = next(
        i for i, p in enumerate(plan.inputs) if p.name == plan.receiver
    )
    setup = []
    if plan.inputs[receiver_index].value.kind == "reference":
        setup.append("val holder = allocate(8).also { writeI64(it, handle) }")
        arguments[receiver_index] = "holder"
    if plan.completion:
        arguments.append("completion")
    setup.append(native.checked(plan.function, arguments))
    body = "; ".join(setup)
    if plan.completion:
        return (
            f"  public fun {method}({declaration(params, defaults)}): Deferred<Unit> = "
            f'nativeRetire(this, binding, "{plan.name}") {{ {body} }}\n'
        )
    return (
        f"  public fun {method}({declaration(params, defaults)}): Unit = "
        f'nativeClose(this, binding, "{plan.name}") {{ {body} }}\n'
    )


def view(plan, values, native):
    method, _params, _inputs, result, result_type = signature(plan, values)
    owner = plan.view.owner
    if not owner.view_begin or not owner.view_end:
        raise Unsupported("borrowed view requires a native scope gate")
    method = "with" + method[0].upper() + method[1:]
    functions = values.bound.source.functions_by_name
    allocation, _decoded = output_storage(result, values)
    begin = native.checked(functions[owner.view_begin], ["handle", "token"])
    end = native.checked(functions[owner.view_end], ["readAddress(token)"])
    read = native.checked(plan.function, ["handle", "out"])
    decoded = values.decode(result, "out", "scope")
    return (
        f"  public fun <T> {method}(block: ({result_type}) -> T): T = "
        f'nativeCall({receiver_arguments(plan)}, "{plan.name}") {{ '
        f"val token = allocate(8); {begin}; "
        "val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope(); "
        f"try {{ val out = {allocation}; {read}; block({decoded}) }} "
        f"finally {{ scope.close(); {end} }} }}\n"
    )


def owner_disposal(bound, native):
    """The any-thread disposal of every public handle, for leak cleanup."""
    lines = []
    for handle in bound.handles.values():
        if not handle.dispose or handle.dispose in bound.source.runtime_exports:
            continue
        method = name(handle.native)
        method = method[0].lower() + method[1:]
        function = bound.source.functions_by_name[handle.dispose]
        call = native.checked(function, ["handle"])
        if function.diagnostic:
            call = call.removeprefix("check(").removesuffix(")")
            call = "NativeDiagnostics.check { diagnostic -> " + call + " }"
        lines.append(
            f'  fun {method}(handle: Long) {{ CallbackAdmission.check(handle, "{handle.dispose}"); {call} }}'
        )
    return "internal object GeneratedOwnerDisposal {\n" + "\n".join(lines) + "\n}\n"
