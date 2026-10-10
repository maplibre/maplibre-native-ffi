"""Emit Swift operations over the native completion runtime."""

from __future__ import annotations

from collections import defaultdict
from pathlib import Path

from tools.bindgen.compiler import compile_api
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.names import camel
from tools.bindgen.semantic import BoundApi, OperationPlan, output_member

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


def operation(plan: OperationPlan, value_types) -> tuple[str, str | None]:
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
    if plan.direct_registrations:
        from .swift_callbacks import direct_operation

        return direct_operation(plan, value_types)
    if plan.view:
        from .swift_views import operation as view_operation

        return view_operation(plan, value_types)
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
    output_names = {parameter.name for parameter in plan.outputs}
    for parameter_index, param in enumerate(params):
        local = f"bindingArg{parameter_index}"
        label = identifier(camel(param.name)) + " " + local
        value_plan = input_plans.get(param.name)
        if param.name in lengths:
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
            arguments.append(f"try arena.borrow({local}.handle)")
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
    method = camel(plan.member)
    if method in {"close", "requireLiveHandle", "deinit"}:
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
    doc = f"/// Calls `{function.name}`.\n"
    modifier = "" if receiver else "static "
    claim = any(
        callback.decision and callback.decision.complete == function.name
        for callback in value_types.bound.callbacks.values()
    )
    access = (
        ", .issued"
        if plan.receiver_access == "issued" and receiver
        else ", .claim"
        if claim
        else ""
    )
    # A handle operation calls its receiver's NativeReceiver helpers. A
    # callback-scoped response names itself as the owner its call acts on, and
    # a global call has neither.
    target = f'"{function.name}"{access}'
    if plan.scoped_receiver:
        target = f'owner: self, "{function.name}"'
    signature = f"{modifier}func {method}({', '.join(declarations)})"
    if outputs and completion:
        if len(outputs) != 1 or len(plan.completion.immediate_owners) != 1:
            raise unsupported(
                function, "multiple immediate completion owners require a result record"
            )
        owned = plan.completion.immediate_owners[0]
        public = owner_name(owned.handle)
        result = name(owned.handle.stem) + "Attachment"
        member = camel(output_member(owned.parameter))
        value_types.attachments[result] = (
            f"public struct {result}: Sendable {{ public let {member}: {public}; public let completion: Task<Void, Error> }}\n"
        )
        parent = ", parent: self" if owned.parent_parameter == plan.receiver else ""
        return (
            f"""{doc}{signature} throws -> {result} {{
  var value0: {owned.handle.native} = 0
  return try nativeAttach({target}, as: {result}.init) {{ raw, arena, completion, diagnostic in {call} }} adopt: {{ try {public}(adopting: value0{parent}) }}
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
        # An array's elements carry its text encoding.
        encoding = (
            (returned.element.encoding if shape == "array" else returned.encoding)
            if returned
            else None
        )
        start = "Start"
        conversion = ""
        # A result that one initializer copies from its single native value.
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
            if (
                plan.result
                and plan.result.kind == "handle"
                and plan.completion
                and plan.completion.result_owned
            ):
                value_types.add(plan.result)
                result = value_types.public(plan.result)
                parent = (
                    ", parent: self"
                    if plan.completion.result_owner.parent_parameter == plan.receiver
                    else ""
                )
                conversion = f"try {result}(adopting: NativeCompletion.value(result, as: {plan.result.native}.self){parent})"
                copying = f"{{ try {result}(adopting: $0{parent}) }}"
            elif returned.ownership != "borrowed":
                raise unsupported(
                    function,
                    "payload needs a borrowed ownership rule or an owned-result adapter",
                )
            if plan.result and plan.result.kind == "handle":
                pass
            elif shape == "value" and returned.kind == "scalar":
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
                # A copied record's initializer also takes the record's bytes.
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
                f", copying: {copying}"
                if copying and not (empty_optional or nullable)
                else f", convert: {{ result in {conversion} }}"
            )
        attribute = "@discardableResult\n" if execution == "command" else ""
        returns = "" if result == "Void" else f" -> {result}"
        return (
            f"""{doc}{attribute}{signature} async throws{returns} {{
  try await native{start}({target}{conversion}) {{ raw, arena, completion, diagnostic in {call} }}
}}
""",
            owner,
        )
    if (
        execution in {"immediate", "snapshot", "event_batch", "render_driver"}
        and not completion
    ):
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
                f"""{doc}{signature} throws -> {result} {{
  try nativeDirect({target}) {{ arena in {copied} }}
}}
""",
                owner,
            )
        output_plans = {parameter.name: parameter.value for parameter in plan.outputs}
        capture, storage, types = [], [], []
        for index, output in enumerate(outputs):
            value = output_plans[output.name]
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
                elif owned.parent_parameter:
                    # A parent passed as another argument is that handle.
                    position = next(
                        i
                        for i, p in enumerate(params)
                        if p.name == owned.parent_parameter
                    )
                    parent = f", parent: bindingArg{position}"
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
        invoke = f"nativeInvoke({target}) {{ raw, arena, diagnostic in {call} }}"
        if not outputs:
            return (
                f"""{doc}{signature} throws {{
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
        return (
            f"""{doc}{signature} throws -> {result} {{
{chr(10).join(storage)}
  return try {invoke} result: {{ {copied} }}
}}
""",
            owner,
        )
    raise unsupported(function, "execution/handle lifecycle needs a supported rule")


def lower(
    api: Api | BoundApi,
) -> tuple[dict[str, list[str]], list[str], dict[str, str]]:
    chunks, generated, failures = defaultdict(list), [], {}
    bound = compile_api(api)
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
            if extended:
                # Operations on one type share one extension per file.
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
            header = name(Path(function.location.path).stem)
            chunks[
                f"Generated/Operations/Generated{header}{owner}Operations.swift"
            ].append(chunk)
            generated.append(function.name)
        except ModelError as error:
            value_types.used = previous
            failures[function.name] = str(error)
    from .swift_ownership import owner_declarations

    chunks["Generated/GeneratedOwners.swift"].append(owner_declarations(bound))
    declarations = {
        item.name: item for item in (*api.typedefs, *api.records, *api.enums)
    }
    for native, value in sorted(value_types.used.items()):
        header = name(Path(declarations[native].location.path).stem)
        chunks[f"Generated/Values/Generated{header}Values.swift"].append(
            value_types.record(value)
        )
    if value_types.attachments:
        chunks["Generated/GeneratedAttachments.swift"].extend(
            value_types.attachments.values()
        )
    if value_types.views:
        chunks["Generated/GeneratedViews.swift"].extend(value_types.views.values())
    return dict(chunks), generated, failures


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
    chunks, _, _ = lower(api)
    prefix = "// Code generated by tools/bindgen; DO NOT EDIT.\n\ninternal import CMaplibreNativeC\nimport Foundation\n\n"
    return {path: prefix + render(bodies) for path, bodies in sorted(chunks.items())}


def coverage(api: Api | BoundApi) -> dict:
    _, generated, failures = lower(api)
    return {"generated": generated, "unsupported": failures}
