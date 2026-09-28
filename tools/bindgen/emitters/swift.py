"""Emit Swift operations directly over the native completion runtime."""

from __future__ import annotations

from collections import defaultdict
from pathlib import Path

from tools.bindgen.compiler import compile_api
from tools.bindgen.model import Api, CType, Function, ModelError, Record
from tools.bindgen.semantic import BoundApi, OperationPlan

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


def camel(value: str) -> str:
    value = name(value)
    return value[:1].lower() + value[1:]


def native(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def unsupported(function: Function | Record, detail: str) -> ModelError:
    return ModelError([f"{function.location}: Swift: {function.name}: {detail}"])


def operation(plan: OperationPlan, api: Api, value_types) -> tuple[str, str | None]:
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
        else owner_name(receiver)
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
        params
        and params[-1].type.pointee
        and native(params[-1].type.pointee) == "mln_completion"
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
    for parameter_index, param in enumerate(params):
        type_ = native(param.type)
        if param.metadata.get("lifetime", "call") != "call":
            raise unsupported(
                function,
                f"parameter {param.name}: retained storage requires a lifetime adapter",
            )
        local = f"bindingArg{parameter_index}"
        label = identifier(camel(param.name)) + " " + local
        value_plan = input_plans.get(param.name)
        if param.name in lengths:
            arguments.append(f"try NativeInputArena.count({lengths[param.name]})")
        elif param.metadata.get("direction") == "out":
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
        elif type_ in SCALARS:
            declarations.append(f"{label}: {SCALARS[type_]}")
            arguments.append(local)
        elif (
            value_plan is not None and value_plan.buffer_form == "view"
        ) and param.metadata.get("encoding") in (
            "utf8",
            "bytes",
            "json",
        ):
            public = "String" if param.metadata["encoding"] == "utf8" else "Data"
            if param.metadata.get("optional") == "empty":
                declarations.append(f"{label}: {public}? = nil")
                arguments.append(
                    f"arena.view({local} ?? {chr(34) + chr(34) if public == 'String' else 'Data()'})"
                )
            else:
                declarations.append(f"{label}: {public}")
                arguments.append(f"arena.view({local})")
        else:
            raise unsupported(function, f"parameter {param.name}: unsupported {type_}")
    method = camel(
        function.name.removeprefix(
            receiver.removesuffix("_handle") + "_" if receiver else "mln_"
        ).removeprefix("mln_")
    )
    if method in {"close", "requireLiveHandle", "deinit"}:
        raise unsupported(function, "method name is reserved by the handle runtime")
    method = identifier(method)
    args = ", ".join(
        [
            *(
                ["try nativePointer" if plan.scoped_receiver else "handle.raw"]
                if receiver
                else []
            ),
            *arguments,
            *(["completion"] if completion else []),
        ]
    )
    call = f"{function.name}({args})"
    submit_try = "try " if "try " in args else ""
    doc = f"  /// Calls `{function.name}`.\n"
    modifier = "" if receiver else "static "
    receiver_setup = (
        "      let access = try self.handle.borrow()\n      defer { access.end(); withExtendedLifetime(self) {} }\n      let handle = access.handle"
        if receiver and not plan.scoped_receiver
        else ""
    )
    if plan.receiver_access == "issued" and receiver:
        receiver_setup = "      let handle = self.handle.issued\n      defer { withExtendedLifetime(self) {} }"
    claim = any(
        callback.decision and callback.decision.complete == function.name
        for callback in value_types.bound.callbacks.values()
    )
    if claim:
        receiver_setup += "\n      let claim = try self.handle.beginClaim()\n      defer { claim.end() }"
    receiver_setup = (
        f'      try NativeCallbackGuard.check(owner: {"self" if receiver else "nil"}, operation: "{function.name}")\n'
        + receiver_setup
    )
    record = None
    if outputs and completion:
        if len(outputs) != 1 or len(plan.completion.immediate_owners) != 1:
            raise unsupported(
                function, "multiple immediate completion owners require a result record"
            )
        owned = plan.completion.immediate_owners[0]
        public = name(owned.handle.native.removeprefix("mln_")) + "Handle"
        result = public.removesuffix("Handle") + "Attachment"
        member = camel(owned.parameter.removeprefix("out_"))
        value_types.attachments[result] = (
            f"public struct {result}: Sendable {{ public let {member}: {public}; public let completion: Task<Void, Error> }}\n"
        )
        parent = ", parent: self" if owned.parent_parameter == plan.receiver else ""
        return (
            f"""public extension {owner} {{
{doc}  {modifier}func {method}({", ".join(declarations)}) throws -> {result} {{
    try mapNativeFailure {{
{receiver_setup}
      let arena = NativeInputArena()
      defer {{ withExtendedLifetime(arena) {{}} }}
      var value0: {owned.handle.native} = 0
      let future = try NativeCompletion.startUnit {{ completion in {submit_try}arena.submit {{ {call} }} }}
      let owner = try {public}(adopting: value0{parent})
      return {result}({member}: owner, completion: Task {{ [owner] in
        defer {{ withExtendedLifetime(owner) {{}} }}
        try await mapNativeFailure {{ try await future.value() }}
      }})
    }}
  }}
}}
""",
            None,
        )
    if completion and execution in ("query", "command", "operation", "lifecycle"):
        shape = function.metadata.get("shape")
        result_type = function.metadata.get("result")
        nullable = function.metadata.get("nullable") == "true"
        empty_optional = function.metadata.get("optional") == "empty"
        if execution == "command":
            if shape != "none" or result_type != "void":
                raise unsupported(
                    function, "typed command result needs a conversion rule"
                )
            result, start, conversion = "CommandCompletion", "startCommand", ""
        elif shape == "none" and result_type == "void":
            result, start, conversion = "Void", "startUnit", ""
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
                start = "start"
            elif function.metadata.get("ownership") != "borrowed":
                raise unsupported(
                    function,
                    "payload needs a borrowed ownership rule or an owned-result adapter",
                )
            start = "start"
            if plan.result and plan.result.kind == "handle":
                pass
            elif shape == "value" and result_type in SCALARS:
                result = SCALARS[result_type]
                conversion = f"try NativeCompletion.value(result, as: {result}.self)"
            elif (
                shape == "value"
                and plan.result is not None
                and plan.result.buffer_form == "view"
                and function.metadata.get("encoding") in ("utf8", "json", "bytes")
            ):
                text = function.metadata["encoding"] == "utf8"
                result = "String" if text else "Data"
                conversion = (
                    f"try NativeCompletion.{'string' if text else 'data'}(result)"
                )
            elif shape == "value" and plan.result and plan.result.kind == "record":
                value_types.add(plan.result)
                result = value_types.public(plan.result)
                conversion = f"{'try ' if dynamic(plan.result) else ''}{result}(raw: try NativeCompletion.value(result, as: {result_type}.self))"
            elif (
                shape == "array"
                and plan.result is not None
                and plan.result.element is not None
                and plan.result.element.buffer_form == "view"
                and function.metadata.get("encoding") == "utf8"
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
            conversion = f" {{ result in {conversion} }}"
        attribute = "  @discardableResult\n" if execution == "command" else ""
        return (
            f"""public extension {owner} {{
{doc}{attribute}
  {modifier}func {method}({", ".join(declarations)}) async throws -> {result} {{
    try await awaitNative {{
{receiver_setup}
      let arena = NativeInputArena()
      defer {{ withExtendedLifetime(arena) {{}} }}
      return try NativeCompletion.{start}({{ completion in {submit_try}arena.submit {{ {call} }} }}){conversion}
    }}
  }}
}}
""",
            record,
        )
    if (
        execution in {"immediate", "snapshot", "event_batch", "render_driver"}
        and not completion
    ):
        if plan.result:
            from .swift_dynamic_values import decode

            value_types.add(plan.result)
            result = value_types.public(plan.result)
            raw = "value"
            copied = decode(value_types, plan.result, raw)
            return (
                f"""public extension {owner} {{
{doc}  {modifier}func {method}({", ".join(declarations)}) throws -> {result} {{
{receiver_setup}
    let arena = NativeInputArena()
    defer {{ withExtendedLifetime(arena) {{}} }}
    let value = {call}
    return {copied}
  }}
}}
""",
                None,
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
                parent = (
                    ", parent: self"
                    if owned.parent_parameter == plan.receiver and plan.receiver
                    else ""
                )
                storage.append(f"      var value{index}: {value.native} = 0")
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
            storage.append(f"      var value{index}: {raw} = {initial}")
            if not value.default:
                for field in value.fields:
                    if field.role == "size":
                        storage.append(
                            f"      value{index}.{identifier(field.name)} = UInt32(MemoryLayout<{raw}>.size)"
                        )
            capture.append(value_types.copy(value, f"value{index}"))
        hook = "      claim.accept()\n" if claim else ""
        result = (
            types[0]
            if len(types) == 1
            else "("
            + ", ".join(
                f"{identifier(camel(output.name.removeprefix('out_')))}: {typ}"
                for output, typ in zip(outputs, types)
            )
            + ")"
        )
        copied = capture[0] if len(capture) == 1 else "(" + ", ".join(capture) + ")"
        return (
            f"""public extension {owner} {{
{doc}  {modifier}func {method}({", ".join(declarations)}) throws -> {result} {{
    try mapNativeFailure {{
{receiver_setup}
      let arena = NativeInputArena()
      defer {{ withExtendedLifetime(arena) {{}} }}
{chr(10).join(storage)}
      try checkStatus(arena.submit {{ {call} }})
{hook}      return {copied}
    }}
  }}
}}
""",
            None,
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
            chunk, _ = operation(plan, api, value_types)
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


def generate(api: Api | BoundApi) -> dict[str, str]:
    chunks, _, _ = lower(api)
    prefix = "// Code generated by tools/bindgen; DO NOT EDIT.\n\ninternal import CMaplibreNativeC\nimport Foundation\n\n"
    return {path: prefix + "\n".join(bodies) for path, bodies in sorted(chunks.items())}


def coverage(api: Api | BoundApi) -> dict:
    _, generated, failures = lower(api)
    return {"generated": generated, "unsupported": failures}
