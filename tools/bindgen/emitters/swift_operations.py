"""Lower Swift operations over the native completion runtime.

A consuming call or a view lowers through its owner's runtime. Every other
operation converts its parameters the same way, and its shape decides how the
call runs and what the method returns: an immediate call returns its value or
outputs, a drain its slice, and a completion awaits its result.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import assert_never

from ..model import CType, Function, Parameter, Record, Unsupported
from ..names import camel
from ..semantic import OperationPlan, output_member
from .swift import (
    SCALARS,
    TEXT_ENCODINGS,
    doc,
    identifier,
    member_doc,
    method_name,
    name,
    native_call,
    owner_type,
    receiver_native,
    repeats,
)
from .swift_dynamic_values import decode, dynamic, encode
from .swift_ownership import consumed, disposer_arguments, owner_name
from .swift_views import operation as view_operation


def native(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def unsupported(function: Function | Record, detail: str) -> Unsupported:
    return Unsupported(f"{function.location}: Swift: {function.name}: {detail}")


@dataclass
class Call:
    """A method's signature and its native call, which each shape's ending
    completes."""

    owner: str | None
    receiver: str | None
    # The module's call helpers, which a factory's handle shadows.
    helpers: str
    completion: bool
    doc_comment: str
    signature: str
    # The receiver that a scoped call passes its runtime, or empty.
    target: str
    args: list[str]
    call: str
    outputs: list[Parameter]
    lengths: dict[str, str]
    # Inputs that convert once before a caller-sized call that may run again.
    hoisted: list[str]


def operation(
    plan: OperationPlan, value_types, *, discard=False
) -> tuple[str, str | None]:
    """An operation's declaration and the type whose extension holds it, or
    None when it is a global function or declares its own extension. With
    `discard`, a command passes no completion."""
    match plan.shape:
        case "discard" | "release" | "release_async" | "answer":
            return consumed(plan, owner_type(plan, value_types.bound), value_types)
        case "view":
            code, view_owner = view_operation(plan, value_types)
            return member_doc(value_types.bound, plan.function.name) + code, view_owner
        case "drain":
            return drained(plan, prepare(plan, value_types), value_types)
        case "thread_start" | "owned" | "call":
            call = prepare(plan, value_types)
            if plan.result:
                return returned(plan, call, value_types)
            return invoked(plan, call, value_types)
        case "attach" | "command" | "query" | "run":
            call = prepare(plan, value_types)
            if discard:
                return discarded(plan, call)
            if call.outputs:
                return attached(plan, call, value_types)
            return completed(plan, call, value_types)
        case _:
            assert_never(plan.shape)


def prepare(plan: OperationPlan, value_types) -> Call:
    """Converts an operation's parameters and declares its method."""
    function = plan.function
    params = list(function.parameters)
    receiver = receiver_native(plan)
    if receiver:
        params.pop(0)
    owner = owner_type(plan, value_types.bound)
    # A factory is a static member of a handle, whose call helpers shadow the
    # global ones that a call without a receiver uses.
    helpers = "MaplibreNativeFFI." if plan.factory else ""
    completion = bool(
        plan.completion and params and params[-1].name == plan.completion.parameter
    )
    if completion:
        params.pop()
    if plan.scoped_receiver:
        value_types.add(value_types.bound.values[receiver])
    declarations, arguments, outputs = [], [], []
    input_plans = {parameter.name: parameter.value for parameter in plan.inputs}

    lengths = {
        value.length: (
            f"(bindingArg{index}?.count ?? 0)"
            if value.nullable
            else f"bindingArg{index}.count"
        )
        for index, param in enumerate(params)
        if (value := input_plans.get(param.name))
        and value.kind in {"array", "buffer"}
        and value.length
        and value.length != "nul"
        and not value.length.isdigit()
    }
    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    text_arguments = (
        {
            text.name: "bindingTextStorage",
            text.value.length: "bindingTextCapacity",
            text.result_length: "bindingTextLength",
        }
        if text
        else {}
    )
    output_names = {parameter.name for parameter in plan.outputs}
    method = method_name(plan)
    for parameter_index, param in enumerate(params):
        local = f"bindingArg{parameter_index}"
        label = (
            (
                "_"
                if not declarations and repeats(method, camel(param.name))
                else identifier(camel(param.name))
            )
            + " "
            + local
        )
        value_plan = input_plans.get(param.name)
        if param.name in text_arguments:
            arguments.append(text_arguments[param.name])
        elif param.name in lengths:
            arguments.append(f"try NativeInputArena.count({lengths[param.name]})")
        elif param.name in output_names:
            outputs.append(param)
            arguments.append(f"&value{len(outputs) - 1}")
        elif value_plan and value_plan.kind in {"record", "enum"}:
            value_types.add(value_plan)
            declarations.append(f"{label}: {value_types.public(value_plan)}")
            arguments.append(
                value_types.native(value_plan, local)
                if value_plan.kind == "enum"
                else f"try {local}.nativeValue(arena: arena)"
                if dynamic(value_plan)
                else f"{local}.nativeValue()"
            )
        elif (
            value_plan
            and value_plan.kind == "reference"
            and value_plan.element
            and value_plan.element.kind == "record"
        ):
            value_types.add(value_plan.element)
            public = value_types.public(value_plan.element)
            converted = (
                "try $0.nativeValue(arena: arena)"
                if dynamic(value_plan.element)
                else "$0.nativeValue()"
            )
            if value_plan.nullable:
                declarations.append(f"{label}: {public}? = nil")
                arguments.append(
                    f"{'try ' if dynamic(value_plan.element) else ''}{local}.map {{ arena.store({converted}) }}"
                )
            else:
                declarations.append(f"{label}: {public}")
                arguments.append(f"arena.store({converted.replace('$0', local)})")
        elif value_plan and value_plan.kind == "handle":
            declarations.append(f"{label}: {value_types.public(value_plan)}")
            arguments.append(f"arena.use({local}.handle)")
        elif value_plan and value_plan.kind in {"scalar", "native_pointer"}:
            declarations.append(f"{label}: {value_types.public(value_plan)}")
            arguments.append(value_types.native(value_plan, local))
        elif value_plan and value_plan.kind in {"array", "reference", "buffer"}:
            value_types.add(value_plan)
            declarations.append(
                f"{label}: {value_types.public(value_plan)}"
                + (" = nil" if value_plan.nullable else "")
            )
            arguments.append(encode(value_types, value_plan, local))
        else:
            raise unsupported(
                function, f"parameter {param.name}: unsupported {native(param.type)}"
            )
    # Text that did not fit calls again, so its inputs convert once, before
    # the first call.
    hoisted = []
    if text:
        for index, argument in enumerate(arguments):
            if argument not in text_arguments.values():
                hoisted.append(f"let bindingInput{index} = {argument}")
                arguments[index] = f"bindingInput{index}"
    if method in {"close", "deinit"}:
        raise unsupported(function, "method name is reserved by the handle runtime")
    args = [
        *(["try nativePointer" if plan.scoped_receiver else "raw"] if receiver else []),
        *arguments,
        *(["completion"] if completion else []),
    ]
    call = native_call(function, *args)
    if "try " in call:
        call = "try " + call
    if owner is None:
        doc_comment = doc(value_types.bound, function.name)
        modifier = "public "
    else:
        doc_comment = member_doc(value_types.bound, function.name)
        modifier = "" if receiver else "static "
    return Call(
        owner=owner,
        receiver=receiver,
        helpers=helpers,
        completion=completion,
        doc_comment=doc_comment,
        signature=f"{modifier}func {identifier(method)}({', '.join(declarations)})",
        target="owner: self" if plan.scoped_receiver else "",
        args=args,
        call=call,
        outputs=outputs,
        lengths=lengths,
        hoisted=hoisted,
    )


def discarded(plan: OperationPlan, call: Call) -> tuple[str, str | None]:
    """The form of a command that passes no completion."""
    # Only a method of a receiver has a discarding form.
    assert call.owner is not None
    native = native_call(
        plan.function, *["nil" if arg == "completion" else arg for arg in call.args]
    )
    if "try " in native:
        native = "try " + native
    return (
        f"{call.signature} throws {{\n  try owner.nativeInvoke({call.target}) {{ raw, arena, diagnostic in {native} }}\n}}\n",
        call.owner + "Discarding",
    )


def attached(plan: OperationPlan, call: Call, value_types) -> tuple[str, str | None]:
    """A completion that readies the owner it creates returns the owner and a
    task that finishes once the owner is ready."""
    completion = plan.completion
    assert completion is not None
    if len(call.outputs) != 1 or len(completion.immediate_owners) != 1:
        raise unsupported(
            plan.function,
            "multiple immediate completion owners require a result record",
        )
    owned = completion.immediate_owners[0]
    public = owner_name(owned.handle)
    result = "New" + name(owned.handle.stem)
    member = camel(output_member(owned.parameter))
    value_types.attachments[result] = (
        f"/// A new {member} and the task that reports it ready.\n"
        f"public struct {result}: Sendable {{\n"
        f"  /// The {member}, which the caller owns before it is ready.\n"
        f"  public let {member}: {public}\n"
        f"  /// Finishes once the {member} is ready, and throws if it fails.\n"
        "  public let ready: Task<Void, Error>\n"
        "}\n"
    )
    parent = ", parent: self" if owned.parent_parameter == plan.receiver else ""
    return (
        f"""{call.doc_comment}{call.signature} throws -> {result} {{
  var value0: {owned.handle.native} = 0
  return try nativeAttach(as: {result}.init) {{ raw, arena, completion, diagnostic in {call.call} }} adopt: {{ try {public}(adopting: value0{parent}) }}
}}
""",
        call.owner,
    )


def completed(plan: OperationPlan, call: Call, value_types) -> tuple[str, str | None]:
    """A command returns its disposition, a query its copied result, and any
    other completion nothing."""
    function = plan.function
    returned = plan.result
    shape = (
        "none" if returned is None else "array" if returned.kind == "array" else "value"
    )
    result_type = returned.native if returned else "void"
    nullable = bool(returned and returned.nullable)
    encoding = (
        returned.element.encoding
        if returned and returned.kind == "array" and returned.element
        else returned.encoding
        if returned
        else None
    )
    start = "Start"
    conversion = ""
    copying = None
    if plan.shape == "command":
        if shape != "none" or result_type != "void":
            raise unsupported(function, "typed command result needs a conversion rule")
        result, start = "CommandCompletion", "Command"
    elif shape == "none" and result_type == "void":
        result, start = "Void", "Unit"
    else:
        assert returned is not None
        if returned.ownership != "borrowed":
            raise unsupported(function, "payload needs a borrowed ownership rule")
        if shape == "value" and returned.kind == "scalar":
            result = value_types.public(returned)
            conversion = f"try NativeCompletion.value(result, as: {result}.self)"
        elif (
            shape == "value"
            and returned.buffer_form == "view"
            and encoding in ("utf8", "json", "bytes")
        ):
            text = encoding in TEXT_ENCODINGS
            result = "String" if text else "Data"
            conversion = f"try NativeCompletion.{'string' if text else 'data'}(result)"
        elif shape == "value" and returned.kind == "record":
            value_types.add(returned)
            result = value_types.public(returned)
            conversion = f"{'try ' if dynamic(returned) else ''}{result}(raw: try NativeCompletion.value(result, as: {result_type}.self))"
            copying = (
                f"{{ try {result}(raw: $0) }}"
                if dynamic(returned)
                else f"{result}.init(raw:)"
            )
        elif (
            shape == "array"
            and returned.element is not None
            and returned.element.buffer_form == "view"
            and encoding == "utf8"
        ):
            result = "[String]"
            conversion = "try NativeCompletion.values(result, as: mln_buffer_view.self).map { try NativeString.copyUTF8(data: $0.data, size: $0.size) }"
        elif shape == "array" and returned.element:
            value_types.add(returned.element)
            result = f"[{value_types.public(returned.element)}]"
            conversion = f"try NativeCompletion.values(result, as: {result_type}.self).map {{ {decode(value_types, returned.element, '$0')} }}"
        else:
            raise unsupported(
                function, "result needs a supported payload/ownership rule"
            )
        if nullable:
            result += "?"
            conversion = f"if {'result.pointee.value == nil' if shape == 'array' else 'result.pointee.value_count == 0'} {{ return nil }}; return {conversion}"
        conversion = (
            f"copying: {copying}"
            if copying and not nullable
            else f"convert: {{ result in {conversion} }}"
        )
    attribute = "@discardableResult\n" if plan.shape == "command" else ""
    returns = "" if result == "Void" else f" -> {result}"
    return (
        f"""{call.doc_comment}{attribute}{call.signature} async throws{returns} {{
  try await {call.helpers}native{start}({call.target}{conversion}) {{ raw, arena, completion, diagnostic in {call.call} }}
}}
""",
        call.owner,
    )


def returned(plan: OperationPlan, call: Call, value_types) -> tuple[str, str | None]:
    """A call that returns a value copies it and cannot fail."""
    function = plan.function
    assert plan.result is not None
    if function.diagnostic:
        raise unsupported(function, "a returned value cannot carry a status diagnostic")
    if call.receiver:
        raise unsupported(function, "a returned value needs a global call")
    if plan.inputs or "try " in call.call:
        raise unsupported(function, "a returned value takes no input")
    value_types.add(plan.result)
    result = value_types.public(plan.result)
    copied = decode(value_types, plan.result, call.call)
    if "try " in copied:
        raise unsupported(function, "a returned value needs a plain copy")
    # A call that returns a value cannot fail, so it does not throw.
    # The ABI version check reads the C version, so reading it skips
    # the check.
    body = copied if plan.unchecked else f"nativeDirect {{ {copied} }}"
    return (
        f"""{call.doc_comment}{call.signature} -> {result} {{
  {body}
}}
""",
        call.owner,
    )


def invoked(plan: OperationPlan, call: Call, value_types) -> tuple[str, str | None]:
    """A status call returns a copy of what it writes to its outputs, and
    nothing when an absent output is absent."""
    outputs = call.outputs
    native_text = call.call
    output_plans = {parameter.name: parameter.value for parameter in plan.outputs}
    capture, storage, types = [], [], []
    if plan.caller_sized_text:
        storage.append('  var bindingText = ""')
        types.append("String")
        capture.append("bindingText")
        fit = f"NativeString.callerSized(&bindingText) {{ bindingTextStorage, bindingTextCapacity, bindingTextLength in {native_text} }}"
        native_text = "\n".join([*call.hoisted, f"return try {fit}"])
    for index, output in enumerate(outputs):
        value = output_plans[output.name]
        output_plan = next(p for p in plan.outputs if p.name == output.name)
        # An output parameter is a pointer to what native writes.
        pointee = output.type.pointee
        assert pointee is not None
        if output_plan.sized_by:
            value_types.add(value)
            element = value.element
            assert element is not None and value.length is not None
            raw = SCALARS.get(native(pointee), native(pointee))
            initial = f"{raw}()"
            count = call.lengths[value.length]
            storage.append(
                f"  var value{index}: [{raw}] = Array(repeating: {initial}, count: {count})"
            )
            for field in element.fields:
                if field.role == "size":
                    storage.append(
                        f"  for item in value{index}.indices {{ value{index}[item].size = UInt32(MemoryLayout<{raw}>.size) }}"
                    )
            types.append(value_types.public(value))
            copied = value_types.copy(element, "item")
            capture.append(
                f"{'try ' if 'try ' in copied else ''}value{index}.map {{ item in {copied} }}"
            )
            continue
        value = value.unwrapped
        value_types.add(value)
        if value.kind == "handle":
            owned = next(
                item for item in plan.owned_outputs if item.parameter == output.name
            )
            parent = ""
            if owned.parent_parameter and owned.parent_parameter == plan.receiver:
                parent = ", parent: self"
            storage.append(f"  var value{index}: {value.native} = 0")
            types.append(value_types.public(value))
            capture.append(
                f"try {value_types.public(value)}(adopting: value{index}{parent})"
            )
            continue
        public = value_types.public(value)
        types.append(public)
        raw = SCALARS.get(native(pointee), native(pointee))
        if value.kind in {"record", "buffer"}:
            initial = f"{raw}()"
        elif value.kind == "enum" and value.ctype.canonical.startswith("enum "):
            initial = f"{raw}(rawValue: 0)"
        else:
            initial = "false" if raw == "Bool" else "0"
        storage.append(f"  var value{index}: {raw} = {initial}")
        for field in value.fields:
            if field.role == "size":
                storage.append(
                    f"  value{index}.{identifier(field.name)} = UInt32(MemoryLayout<{raw}>.size)"
                )
        capture.append(value_types.copy(value, f"value{index}"))
    absent = (
        f"{', ' if call.target else ''}absentOn: {plan.absence.status}"
        if plan.absence
        else ""
    )
    declined = next(
        (
            f"value{index}"
            for registration in plan.registrations
            if registration.accepted_unless
            for index, output in enumerate(outputs)
            if output.name == registration.accepted_unless
        ),
        None,
    )
    if declined:
        # Native kept nothing from a declined registration, so its
        # callbacks release with the arena.
        native_text = f"let status = {native_text}; arena.decline(if: status == MLN_STATUS_OK && {declined}); return status"
    invoke = f"{call.helpers}nativeInvoke({call.target}{absent}) {{ raw, arena, diagnostic in {native_text} }}"
    if not types:
        return (
            f"""{call.doc_comment}{call.signature} throws {{
  try {invoke}
}}
""",
            call.owner,
        )
    result = (
        types[0]
        if len(types) == 1
        else "("
        + ", ".join(
            f"{identifier(camel(output_member(output.name)))}: {typ}"
            for output, typ in zip(outputs, types)
        )
        + ")"
    )
    copied = capture[0] if len(capture) == 1 else "(" + ", ".join(capture) + ")"
    if plan.absence:
        result += "?"
    return (
        f"""{call.doc_comment}{call.signature} throws -> {result} {{
{chr(10).join(storage)}
  return try {invoke} result: {{ {copied} }}
}}
""",
        call.owner,
    )


def drained(plan: OperationPlan, call: Call, value_types) -> tuple[str, str | None]:
    """A drain returns a copy of its slice, and releases the batch that holds
    it once the copy ends or fails. Nothing queued is an empty slice."""
    drain = plan.drain
    assert drain is not None and plan.absence is not None
    view = drain.view.value.unwrapped
    handle = drain.batch.value.unwrapped.handle
    assert handle is not None
    value_types.add(view)
    index = {output.name: i for i, output in enumerate(call.outputs)}
    batch, slice_view = (
        f"value{index[drain.batch.name]}",
        f"value{index[drain.view.name]}",
    )
    release = value_types.bound.source.functions_by_name[handle.release]
    arguments = disposer_arguments(handle, release, batch)
    if release.diagnostic:
        arguments.append("nil")
    released = f"{handle.release}({', '.join(arguments)})"
    if release.return_type.kind != "void":
        released = f"_ = {released}"
    size = next(
        (
            f"\n  {slice_view}.{identifier(field.name)} = UInt32(MemoryLayout<{view.native}>.size)"
            for field in view.fields
            if field.role == "size"
        ),
        "",
    )
    invoke = f"nativeInvoke({call.target}{', ' if call.target else ''}absentOn: {plan.absence.status}) {{ raw, arena, diagnostic in {call.call} }}"
    return (
        f"""{call.doc_comment}{call.signature} throws -> {value_types.public(view)} {{
  var {batch}: {handle.native} = 0
  var {slice_view} = {view.native}(){size}
  return try {invoke} result: {{
    defer {{ {released} }}
    return {value_types.copy(view, slice_view)}
  }} ?? []
}}
""",
        call.owner,
    )
