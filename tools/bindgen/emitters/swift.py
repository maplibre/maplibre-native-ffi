"""Emit Swift operations over the native completion runtime."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.names import camel
from tools.bindgen.semantic import (
    BoundApi,
    OperationPlan,
    output_member,
)

# The condition that selects each platform the package builds for.
PLATFORMS = {
    "ios": "os(iOS)",
    "linux": "os(Linux)",
    "macos": "os(macOS)",
    "tvos": "os(tvOS)",
}

SCALARS = {
    "double": "Double",
    "float": "Float",
    "bool": "Bool",
    "_Bool": "Bool",
    "uint32_t": "UInt32",
    "uint64_t": "UInt64",
    "int32_t": "Int32",
    "int64_t": "Int64",
    "uint8_t": "UInt8",
    "size_t": "Int",
    "ptrdiff_t": "Int",
    "intptr_t": "Int",
    "uintptr_t": "UInt",
    "uint16_t": "UInt16",
    "int16_t": "Int16",
    "int8_t": "Int8",
    "long": "CLong",
    "unsigned long": "CUnsignedLong",
}


def identifier(value: str) -> str:
    return "`" + value + "`"


def name(value: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in value.split("_"))


def words(text: str) -> list[str]:
    return [word.lower() for word in re.findall(r"[A-Z]?[a-z]+|[A-Z]+|\d+", text)]


def repeats(method: str, label: str) -> bool:
    """Whether a first argument label only repeats words of its method's name,
    as `setStyleUrl(url:)` and `deleteOfflineRegion(regionId:)` do. A trailing
    `Id` or `Name` names the key of what the method already names."""
    named = words(label)
    while len(named) > 1 and named[-1] in {"id", "name"}:
        named.pop()
    return set(named) <= set(words(method))


def native(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def native_call(function: Function, *arguments: str) -> str:
    """Calls a C function, passing the enclosing `diagnostic` when it takes one."""
    if function.diagnostic:
        arguments = (*arguments, "diagnostic")
    return f"{function.name}({', '.join(arguments)})"


def checked(statement: str) -> str:
    """Runs a status-returning statement under a fresh per-call diagnostic."""
    return f"try checkStatus {{ diagnostic in {statement} }}"


def unsupported(function: Function | Record, detail: str) -> ModelError:
    return ModelError([f"{function.location}: Swift: {function.name}: {detail}"])


def doc(bound: BoundApi, native: str, indent: str = "") -> str:
    """The DocC comment of a declaration, or empty when it has none."""
    return docs.line_comment(bound.doc(native), indent)


def member_doc(bound: BoundApi, native: str) -> str:
    """The DocC comment of an operation, which `render` places in an extension
    and so wraps at the member indent."""
    return doc(bound, native, "  ")


def operation(
    plan: OperationPlan, value_types, *, discard=False
) -> tuple[str, str | None]:
    from .swift_dynamic_values import dynamic
    from .swift_ownership import owner_name

    function = plan.function
    params = list(function.parameters)
    receiver = next(
        (
            (
                p.value.element.native
                if p.value.kind == "reference" and p.value.element
                else p.value.native
            )
            for p in plan.inputs
            if p.name == (plan.receiver or plan.scoped_receiver)
        ),
        None,
    )
    if receiver:
        params.pop(0)
    owner = (
        name(receiver.removeprefix("mln_"))
        if plan.scoped_receiver
        else owner_name(value_types.bound.handles[receiver])
        if receiver
        else "Maplibre"
    )
    if plan.view:
        from .swift_views import operation as view_operation

        code, view_owner = view_operation(plan, value_types)
        return member_doc(value_types.bound, function.name) + code, view_owner
    if plan.consumes:
        from .swift_ownership import consumed

        return consumed(plan, owner, value_types)
    completion = bool(
        plan.completion and params and params[-1].name == plan.completion.parameter
    )
    if completion:
        params.pop()
    execution = plan.execution
    if plan.scoped_receiver:
        value_types.add(value_types.bound.values[receiver])
    declarations, arguments, outputs = [], [], []
    input_plans = {parameter.name: parameter.value for parameter in plan.inputs}

    lengths = {
        value.length: (
            f"(bindingArg{index}?.count ?? 0)"
            if value.nullable or value.optional == "empty"
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
    method = camel(plan.member)
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
                f"try {local}.nativeValue(arena: arena)"
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
            from .swift_dynamic_values import encode

            value_types.add(value_plan)
            declarations.append(
                f"{label}: {value_types.public(value_plan)}"
                + (
                    " = nil"
                    if value_plan.nullable or value_plan.optional == "empty"
                    else ""
                )
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
    method = camel(plan.member)
    if method in {"close", "deinit"}:
        raise unsupported(function, "method name is reserved by the handle runtime")
    method = identifier(method)
    args = [
        *(["try nativePointer" if plan.scoped_receiver else "raw"] if receiver else []),
        *arguments,
        *(["completion"] if completion else []),
    ]
    call = native_call(function, *args)
    if "try " in call:
        call = "try " + call
    doc_comment = member_doc(value_types.bound, function.name)
    modifier = "" if receiver else "static "
    target = ""
    if plan.scoped_receiver:
        target = "owner: self"
    signature = f"{modifier}func {method}({', '.join(declarations)})"
    if discard:
        call = native_call(
            function, *["nil" if arg == "completion" else arg for arg in args]
        )
        if "try " in call:
            call = "try " + call
        return (
            f"{signature} throws {{\n  try owner.nativeInvoke({target}) {{ raw, arena, diagnostic in {call} }}\n}}\n",
            owner + "Discarding",
        )
    if outputs and completion:
        if len(outputs) != 1 or len(plan.completion.immediate_owners) != 1:
            raise unsupported(
                function, "multiple immediate completion owners require a result record"
            )
        owned = plan.completion.immediate_owners[0]
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
            f"""{doc_comment}{signature} throws -> {result} {{
  var value0: {owned.handle.native} = 0
  return try nativeAttach(as: {result}.init) {{ raw, arena, completion, diagnostic in {call} }} adopt: {{ try {public}(adopting: value0{parent}) }}
}}
""",
            owner,
        )
    if completion and execution in ("query", "command", "operation", "lifecycle"):
        returned = plan.result
        shape = (
            "none"
            if returned is None
            else "array"
            if returned.kind == "array"
            else "value"
        )
        result_type = returned.native if returned else "void"
        nullable = bool(returned and returned.nullable)
        empty_optional = bool(returned and returned.optional == "empty")
        encoding = (
            (returned.element.encoding if shape == "array" else returned.encoding)
            if returned
            else None
        )
        start = "Start"
        conversion = ""
        copying = None
        if execution == "command":
            if shape != "none" or result_type != "void":
                raise unsupported(
                    function, "typed command result needs a conversion rule"
                )
            result, start = "CommandCompletion", "Command"
        elif shape == "none" and result_type == "void":
            result, start = "Void", "Unit"
        else:
            if returned.ownership != "borrowed":
                raise unsupported(function, "payload needs a borrowed ownership rule")
            if shape == "value" and returned.kind == "scalar":
                result = value_types.public(returned)
                conversion = f"try NativeCompletion.value(result, as: {result}.self)"
            elif (
                shape == "value"
                and plan.result is not None
                and plan.result.buffer_form == "view"
                and encoding in ("utf8", "json", "bytes")
            ):
                text = encoding == "utf8"
                result = "String" if text else "Data"
                conversion = (
                    f"try NativeCompletion.{'string' if text else 'data'}(result)"
                )
            elif shape == "value" and plan.result and plan.result.kind == "record":
                value_types.add(plan.result)
                result = value_types.public(plan.result)
                conversion = f"{'try ' if dynamic(plan.result) else ''}{result}(raw: try NativeCompletion.value(result, as: {result_type}.self))"
                copying = (
                    f"{{ try {result}(raw: $0) }}"
                    if dynamic(plan.result)
                    else f"{result}.init(raw:)"
                )
            elif (
                shape == "array"
                and plan.result is not None
                and plan.result.element is not None
                and plan.result.element.buffer_form == "view"
                and encoding == "utf8"
            ):
                result = "[String]"
                conversion = "try NativeCompletion.values(result, as: mln_buffer_view.self).map { try NativeString.copyUTF8(data: $0.data, size: $0.size) }"
            elif shape == "array" and plan.result and plan.result.element:
                from .swift_dynamic_values import decode

                value_types.add(plan.result.element)
                result = f"[{value_types.public(plan.result.element)}]"
                conversion = f"try NativeCompletion.values(result, as: {result_type}.self).map {{ {decode(value_types, plan.result.element, '$0')} }}"
            else:
                raise unsupported(
                    function, "result needs a supported payload/ownership rule"
                )
            if empty_optional:
                if shape != "value" or not (
                    plan.result is not None and plan.result.buffer_form == "view"
                ):
                    raise unsupported(
                        function, "empty optional requires a view payload"
                    )
                result += "?"
                conversion = f"try NativeCompletion.value(result, as: mln_buffer_view.self).size == 0 ? nil : {conversion}"
            elif nullable:
                result += "?"
                conversion = f"if {'result.pointee.value == nil' if shape == 'array' else 'result.pointee.value_count == 0'} {{ return nil }}; return {conversion}"
            conversion = (
                f"copying: {copying}"
                if copying and not (empty_optional or nullable)
                else f"convert: {{ result in {conversion} }}"
            )
        attribute = "@discardableResult\n" if execution == "command" else ""
        returns = "" if result == "Void" else f" -> {result}"
        return (
            f"""{doc_comment}{attribute}{signature} async throws{returns} {{
  try await native{start}({target}{conversion}) {{ raw, arena, completion, diagnostic in {call} }}
}}
""",
            owner,
        )
    if execution in {"immediate", "snapshot", "event_batch"} and not completion:
        if plan.result:
            from .swift_dynamic_values import decode

            if function.diagnostic:
                raise unsupported(
                    function, "a returned value cannot carry a status diagnostic"
                )
            if receiver:
                raise unsupported(function, "a returned value needs a global call")
            value_types.add(plan.result)
            result = value_types.public(plan.result)
            copied = decode(value_types, plan.result, call)
            return (
                f"""{doc_comment}{signature} throws -> {result} {{
  try nativeDirect({target}) {{ arena in {copied} }}
}}
""",
                owner,
            )
        if plan.drain:
            return drain(
                plan, outputs, signature, doc_comment, target, call, value_types
            ), owner
        output_plans = {parameter.name: parameter.value for parameter in plan.outputs}
        capture, storage, types = [], [], []
        if text:
            storage.append("  var bindingText = Data()")
            types.append("Data")
            capture.append("bindingText")
            fit = f"NativeString.callerSized(&bindingText) {{ bindingTextStorage, bindingTextCapacity, bindingTextLength in {call} }}"
            call = "\n".join(
                [*hoisted, f"return {'try ' if 'try ' in call else ''}{fit}"]
            )
        for index, output in enumerate(outputs):
            value = output_plans[output.name]
            output_plan = next(p for p in plan.outputs if p.name == output.name)
            if output_plan.sized_by:
                value_types.add(value)
                element = value.element
                raw = SCALARS.get(
                    native(output.type.pointee), native(output.type.pointee)
                )
                initial = f"{element.default}()" if element.default else f"{raw}()"
                count = lengths[value.length]
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
            if value.kind == "reference":
                value = value.element
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
            raw = SCALARS.get(native(output.type.pointee), native(output.type.pointee))
            if value.kind in {"record", "buffer"}:
                initial = f"{value.default}()" if value.default else f"{raw}()"
            else:
                initial = "false" if raw == "Bool" else "0"
            storage.append(f"  var value{index}: {raw} = {initial}")
            if not value.default:
                for field in value.fields:
                    if field.role == "size":
                        storage.append(
                            f"  value{index}.{identifier(field.name)} = UInt32(MemoryLayout<{raw}>.size)"
                        )
            capture.append(value_types.copy(value, f"value{index}"))
        absent = (
            f"{', ' if target else ''}absentOn: {plan.absence.status}"
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
            call = f"let status = {call}; arena.decline(if: status == MLN_STATUS_OK && {declined}); return status"
        invoke = (
            f"nativeInvoke({target}{absent}) {{ raw, arena, diagnostic in {call} }}"
        )
        if not types:
            return (
                f"""{doc_comment}{signature} throws {{
  try {invoke}
}}
""",
                owner,
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
            f"""{doc_comment}{signature} throws -> {result} {{
{chr(10).join(storage)}
  return try {invoke} result: {{ {copied} }}
}}
""",
            owner,
        )
    raise unsupported(function, "execution/handle lifecycle needs a supported rule")


def drain(plan, outputs, signature, doc_comment, target, call, value_types):
    """A drain returns a copy of its slice, and releases the batch that holds
    it once the copy ends or fails. Nothing queued is an empty slice."""
    from .swift_ownership import disposer_arguments

    drain = plan.drain
    view = drain.view.value.unwrapped
    handle = drain.batch.value.unwrapped.handle
    value_types.add(view)
    index = {output.name: i for i, output in enumerate(outputs)}
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
    invoke = f"nativeInvoke({target}{', ' if target else ''}absentOn: {plan.absence.status}) {{ raw, arena, diagnostic in {call} }}"
    return f"""{doc_comment}{signature} throws -> {value_types.public(view)} {{
  var {batch}: {handle.native} = 0
  var {slice_view} = {view.native}(){size}
  return try {invoke} result: {{
    defer {{ {released} }}
    return {value_types.copy(view, slice_view)}
  }} ?? []
}}
"""


def gated(plan: OperationPlan, code: str) -> str:
    """An operation's declaration, compiled only for the platforms it serves."""
    platforms = served(plan.function, frozenset(PLATFORMS))
    if platforms is None:
        return code
    condition = " || ".join(PLATFORMS[name] for name in sorted(platforms))
    return f"#if {condition}\n{code}#endif\n"


# Headers outside the umbrella header import as explicit submodules, as they do
# from MaplibreNativeC.framework.
SUBMODULES = {"plugin": "MaplibreNativeC.Plugin"}


def lower(
    api: Api | BoundApi,
) -> tuple[dict[str, list[str]], dict[str, set[str]], list[str], dict[str, str]]:
    chunks, imports, generated, failures = defaultdict(list), defaultdict(set), [], {}
    bound = compile_api(api, frozenset(PLATFORMS))
    api = bound.source
    from .swift_values import Values

    value_types = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            value_types.add(value)
    failures.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        function = plan.function
        previous = dict(value_types.used)
        try:
            chunk, extended = operation(plan, value_types)
            chunk = gated(plan, chunk)
            if extended:
                chunk = (extended, chunk)
            receiver = next(
                (item.value for item in plan.inputs if item.name == plan.receiver), None
            )
            if receiver and receiver.kind == "reference":
                receiver = receiver.element
            owner = (
                name(receiver.native.removeprefix("mln_"))
                if receiver and receiver.kind == "handle"
                else "Globals"
            )
            stem = Path(function.location.path).stem
            path = f"Generated/Operations/Generated{name(stem)}{owner}Operations.swift"
            chunks[path].append(chunk)
            if plan.receiver and plan.completion and plan.completion.discardable:
                discarded, view = operation(plan, value_types, discard=True)
                chunks[path].append((view, gated(plan, discarded)))
            if stem in SUBMODULES:
                imports[path].add(SUBMODULES[stem])
            generated.append(function.name)
        except ModelError as error:
            value_types.used = previous
            failures[function.name] = str(error)
    from .swift_ownership import owner_declarations

    chunks["Generated/GeneratedOwners.swift"].append(owner_declarations(bound))
    from .swift_ownership import owner_name

    command_owners = {
        owner_name(bound.handles[p.value.native])
        for plan in bound.operations
        if plan.name in generated and plan.completion and plan.completion.discardable
        for p in plan.inputs
        if p.name == plan.receiver
    }
    for owner in sorted(command_owners):
        chunks["Generated/GeneratedOwners.swift"].append(
            f"public struct {owner}Discarding {{ let owner: {owner} }}\n"
            f"public extension {owner} {{ var discarding: {owner}Discarding {{ {owner}Discarding(owner: self) }} }}\n"
        )
    declarations = {
        item.name: item for item in (*api.typedefs, *api.records, *api.enums)
    }
    for native, value in sorted(value_types.used.items()):
        stem = Path(declarations[native].location.path).stem
        path = f"Generated/Values/Generated{name(stem)}Values.swift"
        chunks[path].append(value_types.record(value))
        if stem in SUBMODULES:
            imports[path].add(SUBMODULES[stem])
    if value_types.attachments:
        chunks["Generated/GeneratedAttachments.swift"].extend(
            value_types.attachments.values()
        )
    if value_types.views:
        chunks["Generated/GeneratedViews.swift"].extend(value_types.views.values())
    return dict(chunks), dict(imports), generated, failures


def render(bodies: list) -> str:
    """Joins a file's declarations, gathering members by the type they extend."""
    members: dict[str, list[str]] = {}
    parts: list = []
    for body in bodies:
        if isinstance(body, str):
            parts.append(body)
            continue
        extended, member = body
        if extended not in members:
            members[extended] = []
            parts.append(extended)
        members[extended].append(member)
    return "\n".join(
        f"public extension {part} {{\n" + "\n".join(members[part]) + "}\n"
        if part in members
        else part
        for part in parts
    )


def generate(api: Api | BoundApi) -> dict[str, str]:
    chunks, imports, _, _ = lower(api)
    return {
        path: "// Code generated by tools/bindgen; DO NOT EDIT.\n\n"
        + "".join(
            f"internal import {module}\n"
            for module in ("MaplibreNativeC", *sorted(imports.get(path, ())))
        )
        + "import Foundation\n\n"
        + render(bodies)
        for path, bodies in sorted(chunks.items())
    }


def coverage(api: Api | BoundApi) -> dict:
    bound = compile_api(api, frozenset(PLATFORMS))
    _, _, generated, failures = lower(bound)
    return {
        "generated": generated,
        "unsupported": failures,
    }
