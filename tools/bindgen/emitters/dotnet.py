"""Emit .NET public operations from the common header model.

The handwritten runtime owns tasks, callback roots, status handling, and handle
state. This module owns per-declaration signatures, native calls, and borrowed
result copies. Unsupported declarations carry an explicit diagnostic.
"""

from __future__ import annotations

import json
import os
import re
from collections import defaultdict
from dataclasses import dataclass

from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import KEYWORDS, LOCALS, conflicting_functions
from tools.bindgen.model import Api, Function
from tools.bindgen.names import camel, pascal, type_name
from tools.bindgen.semantic import BoundApi, OperationPlan

from .dotnet_values import Unsupported, Values


def operation_contract(plan: OperationPlan) -> str | None:
    """Reject contracts that the .NET operation skeleton does not implement."""
    function = plan.function
    metadata = function.metadata
    execution = metadata.get("execution")
    if metadata.get("name"):
        return "explicit name metadata needs a naming rule"
    if metadata.get("receiver") and (
        not function.parameters or metadata["receiver"] != function.parameters[0].name
    ):
        return "explicit receiver requires a different owner skeleton"
    if execution == "query" and metadata.get("ownership") != "borrowed":
        return "query requires borrowed result storage"
    if metadata.get("optional") == "null":
        return "null optional result needs a presence rule"
    if metadata.get("lifetime") not in {None, "call"} and not (
        metadata.get("kind") == "native_pointer"
        and metadata.get("lifetime") == "process"
    ):
        return "result lifetime requires a retention rule"
    if (
        metadata.get("shape") == "array"
        and (metadata.get("nullable") == "true" or "optional" in metadata)
        and plan.result is not None
        and plan.result.element is not None
        and plan.result.element.buffer_form == "view"
    ):
        return "optional array result needs a presence rule"
    if metadata.get("optional") == "empty" and metadata.get("encoding") != "utf8":
        return "optional binary result needs an empty-value conversion"
    receiver = type_name(function.parameters[0].type) if function.parameters else ""
    method = pascal(function.name.removeprefix(receiver + "_"))
    if (
        not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", method)
        or method in KEYWORDS["dotnet"]
    ):
        return f"method {method!r} requires identifier escaping"
    seen = set()
    for parameter in function.parameters[1:]:
        if plan.completion and parameter.name == plan.completion.parameter:
            continue
        name = camel(parameter.name)
        if (
            not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name)
            or name in KEYWORDS["dotnet"]
            or name
            in (
                LOCALS
                if execution == "query"
                else {"completion", "arena", "cancellationToken"}
            )
            or name in seen
        ):
            return f"parameter {parameter.name!r} collides with a reserved or generated identifier"
        seen.add(name)
        if name.startswith("native"):
            return f"parameter {parameter.name!r} collides with generated allocation identifiers"
        if (
            parameter.metadata.get("nullable") == "true"
            or parameter.metadata.get("optional") == "null"
        ) and not (
            parameter.type.pointee and type_name(parameter.type.pointee) != "char"
        ):
            return f"parameter {parameter.name} needs a nullable input conversion"
        if (
            parameter.metadata.get("length") not in (None, "1")
            and not (
                parameter.metadata["length"] == "nul"
                and parameter.metadata.get("encoding") == "utf8"
                and parameter.type.pointee
                and type_name(parameter.type.pointee) == "char"
            )
            and parameter.metadata.get("direction", "in") != "in"
        ):
            return f"parameter {parameter.name} requires counted-buffer conversion"
    return None


def native_release(registration) -> bool:
    # TEMPORARY: owner_release still selects the owner-rooted path while the
    # bindings move to native release.
    return bool(registration.release_callback) and not registration.owner_release


@dataclass(frozen=True)
class Emission:
    files: dict[str, str]
    functions: tuple[str, ...]
    unsupported: dict[str, str]


PRIMITIVES = {
    "double": "double",
    "float": "float",
    "bool": "bool",
    "_Bool": "bool",
    "int32_t": "int",
    "uint32_t": "uint",
    "int64_t": "long",
    "uint64_t": "ulong",
    "size_t": "nuint",
    "int": "int",
    "unsigned int": "uint",
    "uint8_t": "byte",
    "int8_t": "sbyte",
}


def owner_name(native: str) -> str:
    return (
        pascal(native.removeprefix("mln_").removesuffix("_handle")).replace(
            "Geojson", "GeoJson"
        )
        + "Handle"
    )


def raw_handle(native: str) -> str:
    return "Mln" + owner_name(native).removesuffix("Handle")


def namespace_for(path: str) -> str:
    domain = path.rsplit("/", 1)[-1].removesuffix(".h")
    return (
        "Render"
        if domain in {"render_session", "render_target", "texture", "surface"}
        else "Runtime"
        if domain in {"completion", "wake"}
        else "Map"
        if domain == "projection"
        else pascal(domain)
    )


def public_type(name: str) -> str:
    if name in PRIMITIVES:
        return PRIMITIVES[name]
    return pascal(name.removeprefix("mln_"))


def string_copy(value: str, optional: bool) -> str:
    copied = f"RuntimeStructs.CopyUtf8((sbyte*){value}.data, {value}.size)"
    return f"{value}.size == 0 ? null : {copied}" if optional else copied


def operation_name(function: Function, receiver: str) -> str:
    suffix = function.name.removeprefix(receiver.removesuffix("_handle") + "_")
    # A collection-returning .NET query names the collection directly.
    if suffix.startswith("list_"):
        suffix = suffix.removeprefix("list_")
    return pascal(suffix.removeprefix("mln_"))


def emit_operation(plan: OperationPlan, bound: BoundApi) -> tuple[str, str, set[str]]:
    api = bound.source
    owners = {name: owner_name(name) for name in bound.handles}
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
    input_plans = {parameter.name: parameter.value for parameter in plan.inputs}
    values = Values(bound)
    if reason := operation_contract(plan):
        raise Unsupported(reason)
    receiver_parameter = next(
        (
            parameter
            for parameter in function.parameters
            if parameter.name == (plan.receiver or plan.scoped_receiver)
        ),
        None,
    )
    receiver = type_name(receiver_parameter.type) if receiver_parameter else ""
    if receiver_parameter and receiver_parameter.type.pointee:
        receiver = type_name(receiver_parameter.type.pointee)
    if receiver and receiver not in owners and not plan.scoped_receiver:
        raise Unsupported(f"receiver {receiver} requires its generated owner")
    static = receiver_parameter is None
    factory = (
        plan.owned_outputs[0].handle.native
        if static and len(plan.owned_outputs) == 1
        else None
    )
    if factory and factory not in owners:
        raise Unsupported("factory output requires its generated owner")
    owner_class = (
        public_type(receiver)
        if plan.scoped_receiver
        else owners[receiver]
        if receiver
        else owners[factory]
        if factory
        else "Maplibre"
    )
    handle_plan = bound.handles.get(receiver)
    if (
        plan.consumes
        and handle_plan
        and function.name == handle_plan.release
        and handle_plan.release_inputs
    ):
        declarations, locals_, arguments = [], [], ["&live"]
        for parameter in plan.inputs:
            if parameter.name == plan.receiver:
                continue
            value = parameter.value
            values.supported(value)
            name = camel(parameter.name)
            declarations.append(f"{values.public_type(value)} {name}")
            if value.kind == "reference" and value.element:
                locals_.append(
                    f"var native{pascal(name)} = {values.encode(value.element, name)};"
                )
                arguments.append(f"&native{pascal(name)}")
            else:
                arguments.append(values.encode(value, name))
        return (
            owner_class,
            (
                f"    public void Release({', '.join(declarations)})\n    {{\n"
                f'        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(this, "{function.name}");\n'
                f"        state.Release(live => {{ {' '.join(locals_)} return NativeMethods.{function.name}({', '.join(arguments)}); }});\n"
                "    }\n"
            ),
            set(values.plans),
        )
    if (
        handle_plan is not None
        and function.name == handle_plan.release
        and plan.completion
        and not handle_plan.release_inputs
    ):
        native_type = raw_handle(receiver)
        return (
            owners[receiver],
            (
                "    public void Close() => CloseAsync().GetAwaiter().GetResult();\n\n"
                f'    public Task CloseAsync()\n    {{\n        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(this, "{function.name}");\n        state.Close();\n        return teardown;\n    }}\n\n'
                "    public ValueTask DisposeAsync() => new(CloseAsync());\n\n"
                f"    private mln_status StartRelease({native_type} handle)\n    {{\n"
                f"        teardown = NativeCompletion.SubmitUnit(completion => NativeMethods.{function.name}(handle, completion));\n"
                "        return mln_status.MLN_STATUS_OK;\n    }\n"
            ),
            set(),
        )
    execution = plan.execution
    if execution not in {
        "command",
        "query",
        "operation",
        "immediate",
        "snapshot",
        "lifecycle",
        "event_batch",
        "render_driver",
    }:
        raise Unsupported(f"execution {execution!r} requires another runtime skeleton")
    records: set[str] = {receiver} if plan.scoped_receiver else set()
    args = ["Pointer" if plan.scoped_receiver else "Handle"] if receiver else []
    if plan.receiver_access == "issued":
        args[0] = "state.IssuedHandle"
    parameters = []
    prologue = (
        [
            "        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();"
        ]
        if static
        else []
    )
    outputs = []
    call_locals = []
    scoped = False
    immediate_owners = []
    counts = {
        parameter.value.length: parameter.name
        for parameter in plan.inputs
        if parameter.value.kind in {"array", "buffer"}
        and parameter.value.length not in {None, "nul"}
    }
    direct = {
        registration.callback: registration
        for registration in plan.direct_registrations
    }
    direct_contexts = {
        registration.user_data: registration
        for registration in plan.direct_registrations
    }
    direct_releases = {
        registration.release_callback: registration
        for registration in plan.direct_registrations
        if registration.release_callback
    }
    for parameter in function.parameters:
        if parameter.name == (plan.receiver or plan.scoped_receiver):
            continue
        if parameter.name in direct_contexts:
            registration = direct_contexts[parameter.name]
            root = "root" + pascal(registration.callback)
            args.append(
                root
                if native_release(registration)
                else f"{root} is null ? null : {root}.Pointer"
            )
            continue
        if parameter.name in direct_releases:
            args.append(
                "&global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackRoot.Release"
                if native_release(direct_releases[parameter.name])
                else "null"
            )
            continue
        if parameter.name in direct:
            registration = direct[parameter.name]
            callback_value = input_plans[parameter.name]
            values.supported(callback_value)
            name = camel(parameter.name)
            parameters.append(f"{values.public_type(callback_value)} {name}")
            if native_release(registration):
                scoped = True
                prologue.append(
                    f"        var root{pascal(parameter.name)} = {name} is null ? null : scope.Register({name});"
                )
            else:
                prologue.append(
                    f"        using var root{pascal(parameter.name)} = {name} is null ? null : state.PrepareCallback({name}, this);"
                )
            args.append(
                f"{name} is null ? null : &Invoke{public_type(callback_value.native)}"
            )
            continue
        if parameter.metadata.get("lifetime") in {"owner", "completion", "process"}:
            raise Unsupported(f"parameter {parameter.name} requires retained storage")
        name = camel(parameter.name)
        if parameter.name in counts:
            counted = input_plans[counts[parameter.name]]
            size = (
                f"buffer{pascal(counts[parameter.name])}.size"
                if counted.encoding in {"utf8", "bytes", "json"}
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
        pointee = parameter.type.pointee
        if plan.completion and parameter.name == plan.completion.parameter:
            args.append("completion")
        elif parameter.name in {owner.parameter for owner in plan.owned_outputs}:
            owner = next(
                owner
                for owner in plan.owned_outputs
                if owner.parameter == parameter.name
            )
            if owner.handle.native not in owners:
                raise Unsupported("owned output requires its generated owner")
            native = raw_handle(owner.handle.native)
            prologue.append(f"        {native} {name} = default;")
            args.append(f"&native{pascal(name)}" if plan.completion else f"&{name}")
            immediate_owners.append((owner, name, native))
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind in {"array", "buffer"}
            and input_plans[parameter.name].length not in {None, "nul"}
            and input_plans[parameter.name].encoding in {"utf8", "bytes", "json"}
            and (
                input_plans[parameter.name].kind == "buffer"
                or input_plans[parameter.name].element
                and input_plans[parameter.name].element.kind == "scalar"
            )
        ):
            value_plan = input_plans[parameter.name]
            public = "string" if value_plan.encoding == "utf8" else "byte[]"
            parameters.append(f"{public} {name}")
            scoped = True
            encoder = "Utf8" if value_plan.encoding == "utf8" else "Buffer"
            prologue.append(
                f"        var buffer{pascal(name)} = scope.{encoder}({name});"
            )
            args.append(
                f"({values.raw_type(value_plan.element) + '*' if value_plan.element else values.raw_type(value_plan)})buffer{pascal(name)}.data"
            )
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind == "native_pointer"
        ):
            parameters.append(f"NativePointer {name}")
            args.append(f"(void*){name}.Address")
        elif ctype in owners:
            parameters.append(f"{owners[ctype]} {name}")
            prologue.append(f"        ArgumentNullException.ThrowIfNull({name});")
            prologue.append(f"        using var use{pascal(name)} = {name}.Borrow();")
            prologue.append(
                f"        var handle{pascal(name)} = use{pascal(name)}.Handle;"
            )
            args.append(f"handle{pascal(name)}")
        elif ctype in PRIMITIVES:
            enum = parameter.metadata.get("enum")
            if enum:
                if enum not in {item.name for item in api.enums}:
                    raise Unsupported(f"enum {enum} is not declared")
                definition = next(item for item in api.enums if item.name == enum)
                prefix = (
                    os.path.commonprefix(
                        [value.name for value in definition.values]
                    ).rsplit("_", 1)[0]
                    + "_"
                )
                names = [
                    pascal(value.name.removeprefix(prefix).lower())
                    for value in definition.values
                ]
                if len(names) != len(set(names)) or any(
                    not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item) for item in names
                ):
                    raise Unsupported(
                        "enum members require an identifier conversion rule"
                    )
                parameters.append(f"{public_type(enum)} {name}")
                args.append(f"({PRIMITIVES[ctype]}){name}")
            else:
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
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind == "scalar"
        ):
            value_plan = input_plans[parameter.name]
            values.supported(value_plan)
            parameters.append(f"{values.public_type(value_plan)} {name}")
            args.append(values.encode(value_plan, name))
        elif (
            ctype in api.records_by_name
            and getattr(input_plans.get(parameter.name), "buffer_form", None) != "view"
        ):
            value_plan = values.record(ctype)
            values.encoder(value_plan)
            scoped |= values.needs_scope(value_plan)
            parameters.append(f"{public_type(ctype)} {name}")
            args.append(values.encode(value_plan, name))
            records.add(ctype)
        elif (
            input_plans.get(parameter.name) is not None
            and input_plans[parameter.name].buffer_form == "view"
        ) and parameter.metadata.get("encoding") in {
            "bytes",
            "json",
            "utf8",
        }:
            public = (
                "byte[]"
                if parameter.metadata["encoding"] in {"bytes", "json"}
                else "string"
            )
            parameters.append(f"{public} {name}")
            prologue.append(
                f"        using var native{pascal(name)} = NativeStringView.From({name}, nameof({name}));"
            )
            args.append(f"native{pascal(name)}.Value")
        elif (
            pointee
            and type_name(pointee) == "char"
            and parameter.metadata.get("encoding") == "utf8"
        ):
            parameters.append(f"string {name}")
            prologue.append(f"        ArgumentNullException.ThrowIfNull({name});")
            prologue.append(
                f"        using var native{pascal(name)} = NativeUtf8String.FromNullableString({name}, nameof({name}));"
            )
            args.append(f"native{pascal(name)}.Pointer")
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
                prologue.append(f"        ArgumentNullException.ThrowIfNull({name});")
                prologue.append(
                    f'        if ({name}.Length != {value_plan.length}) throw new ArgumentException("Expected {value_plan.length} elements.", nameof({name}));'
                )
            args.append(values.encode(value_plan, name))
        elif (
            parameter.name in input_plans
            and input_plans[parameter.name].kind == "reference"
            and parameter.metadata.get("direction", "in") == "in"
        ):
            value_plan = input_plans[parameter.name]
            values.supported(value_plan)
            if not values.can_encode(value_plan):
                raise Unsupported("reference element requires an input converter")
            parameters.append(f"{values.public_type(value_plan)} {name}")
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
            declaration = f"var {local} = {expression};"
            if execution in {"immediate", "snapshot", "event_batch", "render_driver"}:
                prologue.append(f"        {declaration}")
            else:
                call_locals.append(declaration)
            args.append(
                f"{name} is null ? null : &{local}"
                if value_plan.nullable
                else f"&{local}"
            )
        elif (
            pointee
            and parameter.metadata.get("direction") == "out"
            and type_name(pointee) in PRIMITIVES
        ):
            native = PRIMITIVES[type_name(pointee)]
            prologue.append(f"        {native} {name} = default;")
            args.append(f"&{name}")
            output_plan = next(
                item.value.element
                for item in plan.outputs
                if item.name == parameter.name
            )
            if output_plan and output_plan.kind == "enum":
                output_type = public_type(output_plan.native)
                outputs.append((output_type, f"({output_type}){name}"))
            else:
                outputs.append(
                    ("ulong", f"(ulong){name}") if native == "nuint" else (native, name)
                )
        elif (
            pointee
            and parameter.metadata.get("direction") == "out"
            and type_name(pointee) in api.records_by_name
        ):
            record = type_name(pointee)
            output_plan = next(
                item.value.element
                for item in plan.outputs
                if item.name == parameter.name
            )
            assert output_plan
            values.supported(output_plan)
            if output_plan.kind == "record":
                values.decoder(output_plan)
            if any(
                field.name == "size" for field in api.records_by_name[record].fields
            ):
                initial = f"new {record} {{ size = (uint)sizeof({record}) }}"
            else:
                initial = f"default({record})"
            prologue.append(f"        var {name} = {initial};")
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
    prologue.insert(
        0,
        f'        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed({"null" if static else "this"}, "{function.name}");',
    )
    if handle_plan:
        prologue.insert(0, "        using var retained = this.state.Retain();")
    if scoped:
        prologue.insert(0, "        using var scope = new NativeCallScope();")
    if (
        handle_plan
        and execution in {"immediate", "snapshot", "event_batch", "render_driver"}
        and outputs
        and function.name != handle_plan.release
    ):
        prologue.insert(0, "        using var read = state.Borrow();")
        args[0] = "read.Handle"
    decision_completion = any(
        callback.decision and callback.decision.complete == function.name
        for callback in bound.callbacks.values()
    )
    if decision_completion:
        prologue.append("        using var claim = state.BeginClaim();")
    call = f"NativeMethods.{function.name}({', '.join(args)})"
    callback = (
        f"completion => {{ {' '.join(call_locals)} return {call}; }}"
        if call_locals
        else f"completion => {call}"
    )
    name = operation_name(function, factory or receiver)
    if plan.view:
        assert handle_plan and handle_plan.view_begin and handle_plan.view_end
        if len(outputs) != 1:
            raise Unsupported("borrowed view requires one output descriptor")
        output_type, copied = outputs[0]
        parameters.append(f"Action<{output_type}> callback")
        prologue.append("        ArgumentNullException.ThrowIfNull(callback);")
        prologue.append("        var viewScope = new NativeViewScope();")
        prologue.append("        void* token = null;")
        prologue.append(
            f"        NativeStatus.Check(NativeMethods.{handle_plan.view_begin}(read.Handle, &token));"
        )
        body = prologue + [
            "        try",
            "        {",
            f"            NativeStatus.Check({call});",
            f"            callback({copied});",
            "        }",
            "        finally",
            "        {",
            "            viewScope.Expire();",
            f"            NativeMethods.{handle_plan.view_end}(token);",
            "        }",
        ]
        return (
            owner_class,
            (
                f"    public void With{name.removeprefix('Get')}({', '.join(parameters)})\n    {{\n"
                + "\n".join(body)
                + "\n    }\n"
            ),
            records | values.plans.keys(),
        )
    if immediate_owners:
        if len(immediate_owners) != 1:
            raise Unsupported("multiple owned outputs require a generated aggregate")
        owner, local, native = immediate_owners[0]
        result_type = owners[owner.handle.native]
        parent = (
            "this, "
            if owner.parent_parameter and owner.parent_parameter == plan.receiver
            else ""
        )
        if owner.parent_parameter and not parent:
            raise Unsupported("owned output parent requires another input owner")
        if plan.completion:
            pre = f"{native} native{pascal(local)} = default; "
            callback = f"completion => {{ {pre}{' '.join(call_locals)} var status = {call}; if (status == mln_status.MLN_STATUS_OK) {local} = native{pascal(local)}; return status; }}"
            body = prologue + [
                f"        var attachment = NativeCompletion.SubmitUnit({callback});"
            ]
            constructor = f"{result_type}.Adopt({parent}{local}, attachment)"
        else:
            body = prologue + [f"        NativeStatus.Check({call});"]
            constructor = f"{result_type}.Adopt({parent}{local})"
        if scoped:
            body += [
                f"        var owner = {constructor};",
                "        scope.Accept(owner.CallbackOwner);"
                if plan.registrations
                else "        scope.Accept();",
                "        return owner;",
            ]
        else:
            body.append(f"        return {constructor};")
    elif execution == "lifecycle" and plan.result and plan.result.ownership == "owned":
        owner = plan.completion.result_owner if plan.completion else None
        if owner is None or owner.handle.native not in owners:
            raise Unsupported("owned completion requires its generated owner")
        result = owner.handle.native
        parent = (
            "this, "
            if owner.parent_parameter and owner.parent_parameter == plan.receiver
            else ""
        )
        if owner.parent_parameter and not parent:
            raise Unsupported("owned completion parent requires another input owner")
        result_type = f"Task<{owners[result]}>"
        name += "Async"
        native_type = raw_handle(result)
        expression = f"NativeCompletion.Submit({callback}, result => {owners[result]}.Adopt({parent}NativeCompletion.Value<{native_type}>(result)))"
        body = prologue + (
            [
                f"        var operation = {expression};",
                "        scope.Accept(this.CallbackOwner);"
                if handle_plan and plan.registrations
                else "        scope.Accept();",
                "        return operation;",
            ]
            if scoped
            else [f"        return {expression};"]
        )
    elif execution in {"immediate", "snapshot", "event_batch", "render_driver"}:
        definition = api.typedefs_by_name.get(receiver)
        if (
            definition
            and definition.metadata.get("release") == function.name
            and len(function.parameters) == 1
        ):
            return (
                owners[receiver],
                f'    public void Close() {{ global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(this, "{function.name}"); state.Close(); }}\n',
                records | values.plans.keys(),
            )
        if type_name(function.return_type) not in {"mln_status", "void"}:
            if plan.result is None:
                raise Unsupported("direct return needs a resolved result")
            values.supported(plan.result)
            result_type = values.public_type(plan.result)
            copied = values.copy(plan.result, "returned")
            body = prologue + [
                f"        var returned = {call};",
                f"        return {copied};",
            ]
            code = (
                f"    public {'static ' if static else ''}{result_type} {name}({', '.join(parameters)})\n    {{\n"
                + "\n".join(body)
                + "\n    }\n"
            )
            return owner_class, code, records | values.plans.keys()
        if outputs:
            result_type = (
                outputs[0][0]
                if len(outputs) == 1
                else "("
                + ", ".join(
                    f"{type_} {pascal(parameter.name.removeprefix('out_'))}"
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
        body = prologue + [f"        NativeStatus.Check({call});"]
        if scoped:
            body.append(
                "        scope.Accept(this.CallbackOwner);"
                if handle_plan and plan.registrations
                else "        scope.Accept();"
            )
        for registration in plan.direct_registrations:
            if not native_release(registration):
                condition = (
                    f"!{camel(registration.accepted_unless)}"
                    if registration.accepted_unless
                    else "true"
                )
                body.append(
                    f"        if ({condition}) root{pascal(registration.callback)}?.Accept();"
                )
        if decision_completion:
            body.append("        claim.Accept();")
        if result is not None:
            body.append(f"        return {result};")
    else:
        parameters.append("CancellationToken cancellationToken = default")
        name += "Async"
        if execution == "command":
            if (
                function.metadata.get("result") != "void"
                or function.metadata.get("shape") != "none"
            ):
                raise Unsupported(
                    "command carries a typed payload that needs its own conversion"
                )
            result_type = "Task<CommandCompletion>"
            expression = f"NativeCompletion.SubmitCommand({callback})"
        else:
            native = function.metadata.get("result")
            shape = function.metadata.get("shape", "value")
            if native == "void" and shape == "none":
                result_type = "Task"
                converter = "result => true"
            elif (
                plan.result is not None
                and "view"
                in {
                    plan.result.buffer_form,
                    plan.result.element and plan.result.element.buffer_form,
                }
            ) and function.metadata.get("encoding") in {
                "utf8",
                "json",
                "bytes",
            }:
                encoding = function.metadata["encoding"]
                value_type = "string" if encoding == "utf8" else "byte[]"
                value = "NativeCompletion.Value<mln_buffer_view>(result)"
                optional = function.metadata.get("optional") == "empty"
                nullable = function.metadata.get("nullable") == "true" or optional
                copy = (
                    string_copy("value", optional)
                    if encoding == "utf8"
                    else "ValueStructs.CopyBufferView(value)"
                )
                if shape == "array":
                    result_type = f"Task<{value_type}[]>"
                    converter = (
                        "result =>\n        {\n"
                        "            var values = NativeCompletion.Values<mln_buffer_view>(result);\n"
                        f"            var copied = new {value_type}[values.Length];\n"
                        "            for (var index = 0; index < values.Length; index++)\n"
                        "            {\n                var value = values[index];\n"
                        f"                copied[index] = {copy};\n            }}\n"
                        "            return copied;\n        }"
                    )
                elif shape == "value":
                    result_type = f"Task<{value_type}{'?' if nullable else ''}>"
                    missing = (
                        "if (result->value_count == 0) return null; "
                        if nullable
                        else ""
                    )
                    converter = (
                        f"result => {{ {missing}var value = {value}; return {copy}; }}"
                    )
                else:
                    raise Unsupported(
                        f"buffer result shape {shape!r} needs a conversion rule"
                    )
            elif native in PRIMITIVES and shape == "value":
                if function.metadata.get("nullable") == "true":
                    raise Unsupported(
                        "nullable scalar completion needs a presence conversion"
                    )
                result_type = f"Task<{PRIMITIVES[native]}>"
                converter = (
                    f"result => NativeCompletion.Value<{PRIMITIVES[native]}>(result)"
                )
            elif native in api.records_by_name and shape == "array":
                nullable = function.metadata.get("nullable") == "true"
                # Validate the whole record before emitting the operation.
                values.decoder(values.record(native))
                records.add(native)
                result_type = f"Task<{public_type(native)}[]{'?' if nullable else ''}>"
                converter = (
                    "result =>\n        {\n"
                    + (
                        "            if (result->value == null) return null;\n"
                        if nullable
                        else ""
                    )
                    + f"            var values = NativeCompletion.Values<{native}>(result);\n"
                    f"            var copied = new {public_type(native)}[values.Length];\n"
                    "            for (var index = 0; index < values.Length; index++)\n"
                    f"                copied[index] = Copy{public_type(native)}(values[index]);\n"
                    "            return copied;\n        }"
                )
            elif native in api.records_by_name and shape == "value":
                nullable = function.metadata.get("nullable") == "true"
                values.decoder(values.record(native))
                records.add(native)
                result_type = f"Task<{public_type(native)}{'?' if nullable else ''}>"
                copied = f"Copy{public_type(native)}(NativeCompletion.Value<{native}>(result))"
                if nullable:
                    copied = f"result->value_count == 0 ? ({public_type(native)}?)null : {copied}"
                converter = f"result => {copied}"
            else:
                raise Unsupported(
                    f"result {native!r} with shape {shape!r} needs a conversion rule"
                )
            expression = f"NativeCompletion.Submit({callback}, {converter})"
        body = prologue + (
            [
                f"        var operation = {expression};",
                "        scope.Accept(this.CallbackOwner);"
                if handle_plan and plan.registrations
                else "        scope.Accept();",
                "        return operation.WaitAsync(cancellationToken);",
            ]
            if scoped
            else [f"        return {expression}.WaitAsync(cancellationToken);"]
        )
    code = (
        f"    public {'static ' if static else ''}{result_type} {name}({', '.join(parameters)})\n"
        "    {\n" + "\n".join(body) + "\n    }\n"
    )
    return owner_class, code, records | values.plans.keys()


def emit(api: Api | BoundApi) -> Emission:
    methods: dict[str, list[str]] = defaultdict(list)
    records: dict[str, set[str]] = defaultdict(set)
    supported = []
    bound = compile_api(api)
    api = bound.source
    owners = {name: owner_name(name) for name in bound.handles}
    unsupported = {
        name: "\n".join(reasons) for name, reasons in bound.unsupported.items()
    }
    default_records: set[str] = set()
    conflicts = conflicting_functions(api, "dotnet")
    for plan in bound.operations:
        function = plan.function
        try:
            if plan.role == "support":
                if plan.support_for and plan.support_for.startswith("default:"):
                    value_name = plan.support_for.removeprefix("default:")
                    values = Values(bound)
                    values.decoder(values.record(value_name))
                    default_records.update(values.plans)
                    supported.append(function.name)
                    continue
                raise Unsupported("support operation requires its generated owner")
            if function.name in conflicts:
                raise Unsupported("public method name collides after conversion")
            owner, method, used_records = emit_operation(plan, bound)
        except Unsupported as error:
            unsupported[function.name] = f"{function.location}: {error}"
            continue
        methods[owner].append(method)
        records[owner].update(used_records)
        supported.append(function.name)
    files = {}
    files["Internal/C/Handles.g.cs"] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n"
        "namespace Maplibre.NativeFfi.Internal.C;\n\n"
        "internal interface IMlnHandle { ulong Value { get; } }\n\n"
        + "\n".join(
            f"internal readonly struct {raw_handle(native)}(ulong value) : IMlnHandle\n{{\n    public ulong Value {{ get; }} = value;\n    public bool IsNull => Value == 0;\n}}\n"
            for native in sorted(bound.handles)
        )
    )
    files["Internal/C/handle-remaps.json"] = (
        json.dumps(
            {native: raw_handle(native) for native in sorted(bound.handles)}, indent=2
        )
        + "\n"
    )
    created_handles = {
        handle.native
        for plan in bound.operations
        if plan.function.name in supported
        for handle in (
            [owner.handle for owner in plan.owned_outputs]
            + (
                [plan.result.handle]
                if plan.result
                and plan.result.kind == "handle"
                and plan.result.ownership == "owned"
                and plan.result.handle
                else []
            )
        )
    }
    decision_handles = {
        callback.decision.handle.native
        for callback in bound.callbacks.values()
        if callback.decision
    }
    created_handles.update(native for native in owners if owners[native] in methods)
    async_disposable: set[str] = set()
    for native in sorted(created_handles):
        handle = bound.handles[native]
        owner = owners[native]
        native_type = raw_handle(native)
        parent = f"{owners[handle.parent]} parent, " if handle.parent else ""
        argument = "parent, " if handle.parent else ""
        release = next(
            plan for plan in bound.operations if plan.function.name == handle.release
        )
        asynchronous_release = bool(release.completion)
        if (
            asynchronous_release
            and not handle.release_inputs
            and handle.release in supported
        ):
            async_disposable.add(owner)
        immediate_completion = any(
            plan.completion
            and any(output.handle.native == native for output in plan.owned_outputs)
            for plan in bound.operations
        )
        decision_owner = native in decision_handles
        extra = (
            ", Task completion"
            if immediate_completion
            else ", bool pendingDecision = false"
            if decision_owner
            else ""
        )
        extra_argument = ", completion" if immediate_completion else ""
        # A pending decision is only ever borrowed through BorrowDecision.
        adopt_extra = ", Task completion" if immediate_completion else ""
        destroy = (
            "StartRelease"
            if asynchronous_release
            else f"static live => NativeMethods.{handle.release}(live)"
        )
        cleanup = handle.dispose or handle.release
        cleanup_function = api.functions_by_name[cleanup]
        dispose = (
            f"static live => NativeMethods.{cleanup}(live)"
            if type_name(cleanup_function.return_type) == "mln_status"
            else f"static live => {{ NativeMethods.{cleanup}(live); return mln_status.MLN_STATUS_OK; }}"
        )
        if handle.release_inputs or (
            not asynchronous_release
            and type_name(release.function.return_type) == "void"
        ):
            destroy = dispose
        declarations = f"    private readonly NativeHandleState<{native_type}> state;\n"
        if asynchronous_release:
            declarations += "    private volatile Task teardown = Task.CompletedTask;\n"
        # Runtime events report their source by this identity.
        declarations += (
            "    private readonly ulong nativeId;\n    public ulong Id => nativeId;\n"
        )
        if immediate_completion:
            declarations += "    public Task Completion { get; }\n"
        declarations += (
            f"\n    internal {owner}({parent}{native_type} handle{extra})\n    {{\n"
        )
        declarations += "        nativeId = handle.Value;\n"
        if immediate_completion:
            declarations += "        Completion = completion.ContinueWith(static (finished, retained) => { GC.KeepAlive(retained); return finished; }, this, CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default).Unwrap();\n"
        declarations += f"        state = new NativeHandleState<{native_type}>(handle, {destroy}, nameof({owner}), {dispose}{', pendingDecision' if decision_owner else ''}{', retainedParent: parent' if handle.parent else ''});\n    }}\n"
        declarations += f"\n    internal static {owner} Adopt({parent}{native_type} handle{adopt_extra})\n    {{\n        {owner}? owner = null;\n        try\n        {{\n            owner = new {owner}({argument}handle{extra_argument});\n            return owner;\n        }}\n        catch\n        {{\n            if (owner is null) NativeMethods.{cleanup}(handle);\n            else owner.state.Retire();\n            throw;\n        }}\n    }}\n"
        declarations += f"\n    internal {native_type} Handle => state.Handle;\n    internal NativeHandleState<{native_type}>.ReadScope Borrow() => state.Borrow();\n    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner => state.CallbackOwner;\n    public bool IsClosed => state.IsClosed;\n"
        declarations += f'    public void Dispose() {{ global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(this, "{cleanup}"); state.Retire(); }}\n'
        if decision_owner:
            declarations += f"    internal static {owner} BorrowDecision({native_type} handle) => new(handle, true);\n    internal bool FinishDecision(bool accepted) => state.FinishDecision(accepted);\n"
        methods[owner].insert(0, declarations)
    used_records = default_records | (
        set().union(*records.values()) if records else set()
    )
    values = Values(bound)
    enum_names = {
        parameter.metadata["enum"]
        for function in api.functions
        if function.name in supported
        for parameter in function.parameters
        if "enum" in parameter.metadata
    }
    enum_names.update(
        plan.result.native
        for plan in bound.operations
        if plan.function.name in supported
        and plan.result
        and plan.result.kind == "enum"
    )
    for record_name in used_records:
        enum_names.update(
            field.value.native
            for field in values.record(record_name).fields
            if field.role == "value" and field.value.kind == "enum"
        )
    for plan in bound.operations:
        if plan.function.name in supported:
            for parameter in plan.inputs:
                if any(
                    parameter.name == item.callback
                    for item in plan.direct_registrations
                ):
                    values.supported(parameter.value)
    enum_names.update(values.enum_names)
    namespaces = {
        namespace_for(item.location.path)
        for item in (*api.records, *api.enums)
        if item.name in used_records or item.name in enum_names
    }
    extra_imports = "".join(
        f"using Maplibre.NativeFfi.{namespace};\n"
        for namespace in sorted(namespaces - {"Map", "Runtime", "Style"})
    )
    values = Values(bound)
    for owner, body in sorted(methods.items()):
        namespace = (
            "Runtime"
            if owner
            in {
                public_type(value.native)
                for value in bound.public_values.values()
                if value.response
            }
            else {
                **{
                    owner_name(handle.native): namespace_for(
                        api.functions_by_name[handle.release].location.path
                    )
                    for handle in bound.handles.values()
                },
                "Maplibre": "Base",
            }.get(owner, "Map")
        )
        interfaces = [
            *(["IDisposable"] if owner in {owners[n] for n in created_handles} else []),
            *(["IAsyncDisposable"] if owner in async_disposable else []),
        ]
        bases = f" : {', '.join(interfaces)}" if interfaces else ""
        files[f"{namespace}/{owner}.Operations.g.cs"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "#nullable enable\n"
            "using Maplibre.NativeFfi.Internal.C;\n"
            "using Maplibre.NativeFfi.Internal.Memory;\n"
            "using Maplibre.NativeFfi.Internal.Pointer;\n"
            "using Maplibre.NativeFfi.Internal.Status;\n"
            "using Maplibre.NativeFfi.Internal.Struct;\n"
            "using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;\n"
            "using Maplibre.NativeFfi.Runtime;\n"
            "using Maplibre.NativeFfi.Map;\n"
            "using Maplibre.NativeFfi.Style;\n" + extra_imports + "\n"
            f"namespace Maplibre.NativeFfi{'.' + namespace if owner != 'Maplibre' else ''};\n\n"
            f"public {'static' if owner == 'Maplibre' else 'sealed'} unsafe partial class {owner}{bases}\n{{\n"
            + "\n".join(body)
            + "}\n"
        )
    used_records = default_records | (
        set().union(*records.values()) if records else set()
    )
    converters = []
    for record_name in sorted(used_records):
        plan = values.record(record_name)
        converters.append(
            values.decoder(plan).replace("private static", "internal static")
        )
        converters.append(values.callback_methods(plan))
        if values.can_encode(plan):
            converters.append(
                values.encoder(plan).replace("private static", "internal static")
            )
    direct_callbacks = {
        parameter.value.native: parameter.value
        for plan in bound.operations
        if plan.function.name in supported
        for parameter in plan.inputs
        if any(
            parameter.name == registration.callback
            for registration in plan.direct_registrations
        )
    }
    owner_callbacks = {
        parameter.value.native
        for plan in bound.operations
        if plan.function.name in supported
        for parameter in plan.inputs
        if any(
            parameter.name == registration.callback
            and not native_release(registration)
            for registration in plan.direct_registrations
        )
    }
    converters.extend(
        values.direct_callback_method(value, name in owner_callbacks)
        for name, value in sorted(direct_callbacks.items())
    )
    files["Internal/Struct/GeneratedValues.g.cs"] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n#nullable enable\n"
        "using Maplibre.NativeFfi.Internal.C;\nusing Maplibre.NativeFfi.Internal.Memory;\nusing Maplibre.NativeFfi.Internal.Callback;\nusing System.Runtime.CompilerServices;\nusing System.Runtime.InteropServices;\n"
        + "".join(
            f"using Maplibre.NativeFfi.{namespace};\n"
            for namespace in sorted(namespaces)
        )
        + "namespace Maplibre.NativeFfi.Internal.Struct;\n\ninternal static unsafe class GeneratedValues\n{\n"
        + "\n".join(converters)
        + "}\n"
    )
    for name in sorted(used_records):
        record = api.records_by_name[name]
        namespace = namespace_for(record.location.path)
        plan = values.record(name)
        declaration = values.declaration(plan)
        if plan.default:
            default = (
                f"    public static {public_type(name)} Default\n    {{\n"
                "        get\n        {\n"
                f'            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(null, "{plan.default}");\n'
                "            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();\n"
                f"            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.Copy{public_type(name)}(global::Maplibre.NativeFfi.Internal.C.NativeMethods.{plan.default}());\n"
                "        }\n    }\n"
            )
            if declaration.rstrip().endswith(";"):
                declaration = declaration.rstrip()[:-1] + "\n{\n" + default + "}\n"
            else:
                declaration = declaration.rstrip()[:-1] + default + "}\n"
        # A nested value can be declared in a different domain header.
        imports = "".join(
            f"using Maplibre.NativeFfi.{item};\n"
            for item in sorted(namespaces - {namespace})
        )
        files[f"{namespace}/{public_type(name)}.g.cs"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "#nullable enable\nusing Maplibre.NativeFfi.Internal.C;\n" + imports + "\n"
            f"namespace Maplibre.NativeFfi.{namespace};\n\n" + declaration
        )
    # The completion runtime reads results through the completion record's
    # callbacks, so the enums those results carry, such as a command
    # disposition, are public without appearing in an operation signature.
    completion_records = {
        parameter.type.pointee.declaration
        for plan in bound.operations
        if plan.function.name in supported and plan.completion
        for parameter in plan.function.parameters
        if parameter.name == plan.completion.parameter and parameter.type.pointee
    }
    completion_values = Values(bound)
    for record_name in sorted(completion_records & bound.values.keys()):
        for field in bound.values[record_name].fields:
            if field.value.kind == "callback":
                completion_values.supported(field.value)
    enum_names.update(completion_values.enum_names)
    for enum in api.enums:
        if enum.name not in enum_names:
            continue
        namespace = namespace_for(enum.location.path)
        prefix = (
            os.path.commonprefix([value.name for value in enum.values]).rsplit("_", 1)[
                0
            ]
            + "_"
        )
        underlying = PRIMITIVES.get(enum.underlying_type.spelling)
        if underlying is None:
            underlying = {
                "unsigned long long": "ulong",
                "unsigned long": "ulong",
                "unsigned int": "uint",
                "int": "int",
            }[enum.underlying_type.canonical]
        flags = "[Flags]\n" if enum.metadata.get("kind") == "bitmask" else ""
        fields = [
            f"    {pascal(value.name.removeprefix(prefix).lower())} = {value.value},"
            for value in enum.values
        ]
        files[f"{namespace}/{public_type(enum.name)}.g.cs"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            f"namespace Maplibre.NativeFfi.{namespace};\n\n{flags}"
            f"public enum {public_type(enum.name)} : {underlying}\n{{\n"
            + "\n".join(fields)
            + "\n}\n"
        )
    for native in created_handles:
        dispose = bound.handles[native].dispose
        if dispose and dispose not in supported:
            supported.append(dispose)
            unsupported.pop(dispose, None)
    views = {
        output.value.element.native
        for operation in bound.operations
        if operation.function.name in supported and operation.view
        for output in operation.outputs
        if output.value.element
    }
    for native in sorted(views):
        value = values.record(native)
        name = public_type(native)
        namespace = namespace_for(api.records_by_name[native].location.path)
        properties = "".join(
            f"    public {values.member_type(value, member, fields)} {member} {{ get {{ scope.EnsureActive(); return value.{member}; }} }}\n"
            for member, fields in values.members(value)
        )
        files[f"{namespace}/{name}View.g.cs"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n#nullable enable\n"
            "using Maplibre.NativeFfi.Internal.Pointer;\n"
            + "".join(
                f"using Maplibre.NativeFfi.{item};\n"
                for item in sorted(namespaces - {namespace})
            )
            + f"namespace Maplibre.NativeFfi.{namespace};\n\npublic sealed class {name}View\n{{\n"
            f"    private readonly {name} value;\n    private readonly NativeViewScope scope;\n"
            f"    internal {name}View({name} value, NativeViewScope scope) {{ this.value = value; this.scope = scope; }}\n"
            + properties
            + "}\n"
        )
    return Emission(files, tuple(sorted(supported)), unsupported)


def generate(api: Api | BoundApi) -> dict[str, str]:
    return emit(api).files


def coverage(api: Api | BoundApi) -> dict:
    result = emit(api)
    return {"generated": list(result.functions), "unsupported": result.unsupported}
