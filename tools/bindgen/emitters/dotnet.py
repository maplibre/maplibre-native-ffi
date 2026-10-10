"""Emit .NET public operations from the common header model.

The handwritten runtime owns tasks, callback roots, status handling, and handle
state. This module owns per-declaration signatures, native calls, and borrowed
result copies. Unsupported declarations carry an explicit diagnostic.
"""

from __future__ import annotations

import os
import re
from collections import defaultdict
from dataclasses import dataclass

from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import KEYWORDS, LOCALS, conflicting_functions
from tools.bindgen.model import Api, Function
from tools.bindgen.names import camel, pascal, type_name
from tools.bindgen.semantic import (
    BoundApi,
    DefaultSupport,
    HandlePlan,
    OperationPlan,
    output_member,
    public_stem,
)

from . import dotnet_native
from .dotnet_values import Unsupported, Values, member, raw_handle, typed_mask


def operation_contract(plan: OperationPlan) -> str | None:
    """Reject contracts that the .NET operation skeleton does not implement."""
    function = plan.function
    execution = plan.execution
    result = plan.result
    # An array result's elements carry the value attributes.
    value = result.element if result and result.kind == "array" else result
    if execution == "query" and (result is None or result.ownership != "borrowed"):
        return "query requires borrowed result storage"
    if value and value.optional == "null":
        return "null optional result needs a presence rule"
    if (
        value
        and value.lifetime != "call"
        and not (value.kind == "native_pointer" and value.lifetime == "process")
    ):
        return "result lifetime requires a retention rule"
    if (
        result
        and result.kind == "array"
        and (result.nullable or value.optional)
        and value.buffer_form == "view"
    ):
        return "optional array result needs a presence rule"
    if value and value.optional == "empty" and value.encoding != "utf8":
        return "optional binary result needs an empty-value conversion"
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
        # A null pointer argument needs a pointer that is not a C string.
        if (value.nullable or value.optional == "null") and not (
            parameter.type.pointee and type_name(parameter.type.pointee) != "char"
        ):
            return f"parameter {parameter.name} needs a nullable input conversion"
        if (
            value.length not in (None, "1")
            and not (value.length == "nul" and value.encoding == "utf8")
            and planned.direction != "in"
        ):
            return f"parameter {parameter.name} requires counted-buffer conversion"
    return None


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
    "int16_t": "short",
    "uint16_t": "ushort",
    "size_t": "nuint",
    "int": "int",
    "unsigned int": "uint",
    "uint8_t": "byte",
    "int8_t": "sbyte",
}


def owner_name(handle: HandlePlan | str) -> str:
    """The public owner class of a handle plan, or of a receiver type that
    declares no handle contract."""
    stem = handle.stem if isinstance(handle, HandlePlan) else public_stem(handle)
    return pascal(stem) + "Handle"


# Every generated public type shares the binding's root namespace, as each
# other binding exposes one flat module. Files stay grouped by their header.
NAMESPACE = "Maplibre.NativeFfi"


def directory_for(path: str) -> str:
    """The source directory of a header's declarations, named after the header."""
    return pascal(path.rsplit("/", 1)[-1].removesuffix(".h"))


def public_type(name: str) -> str:
    if name in PRIMITIVES:
        return PRIMITIVES[name]
    return pascal(name.removeprefix("mln_"))


# Generated sources take their namespace imports from GlobalUsings.g.cs. The
# compiler treats a .g.cs file as generated code, which disables nullable
# annotations unless the file enables them.
HEADER = (
    "// Generated from the C headers by tools/bindgen. Do not edit.\n#nullable enable\n"
)

# Operations and converters import the members of the helpers they call
# themselves, which keeps short names such as Check out of handwritten files.
OPERATION_HELPERS = (
    "using static Maplibre.NativeFfi.Internal.NativeCall;\n"
    "using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;\n\n"
)
VALUE_HELPERS = "using static Maplibre.NativeFfi.Internal.Struct.NativeValues;\n"


def native_call(function: Function, arguments: str, diagnostic: str) -> str:
    """Calls a C function, passing diagnostic when it takes one."""
    if function.diagnostic:
        arguments = f"{arguments}, {diagnostic}" if arguments else diagnostic
    return f"NativeMethods.{function.name}({arguments})"


def checked_call(function: Function, arguments: str) -> str:
    """Checks a synchronous call against the calling thread's diagnostic."""
    if not function.diagnostic:
        raise Unsupported(f"{function.name} returns a status without a diagnostic")
    return f"Check({native_call(function, arguments, 'Diagnostic')});"


def submission(function: Function, arguments: str) -> str:
    """A completion-taking C call as the submission that NativeCallScope runs."""
    return (
        f"(completion, diagnostic) => {native_call(function, arguments, 'diagnostic')}"
    )


def method(signature: str, body: list[str]) -> str:
    return (
        f"    {signature}\n    {{\n"
        + "".join(f"        {line}\n" for line in body)
        + "    }\n"
    )


def completion_query(
    plan: OperationPlan, values: Values, records: set[str], api
) -> tuple[str, str, str]:
    """The result type, submission helper, and copy of a completion's value."""
    result = plan.result
    if result is None:
        return "Task", "Run", ""
    shape = "array" if result.kind == "array" else "value"
    # An array result's elements carry the value attributes.
    value = result.element if shape == "array" else result
    native = value.native
    nullable = result.nullable
    if value.buffer_form == "view" and value.encoding in {"utf8", "json", "bytes"}:
        encoding = value.encoding
        public = "string" if encoding == "utf8" else "byte[]"
        optional = value.optional == "empty"
        copy = (
            ("CopyOptionalUtf8View" if optional else "CopyUtf8View")
            if encoding == "utf8"
            else "CopyBufferView"
        )
        copy = f"ValueStructs.{copy}"
        if shape == "array":
            return f"{public}[]", f"QueryArray<mln_buffer_view, {public}>", copy
        if shape == "value":
            if nullable or optional:
                return f"{public}?", f"QueryOptional<mln_buffer_view, {public}>", copy
            return public, f"Query<mln_buffer_view, {public}>", copy
        raise Unsupported(f"buffer result shape {shape!r} needs a conversion rule")
    if value.kind == "scalar" and native in PRIMITIVES and shape == "value":
        if nullable:
            raise Unsupported("nullable scalar completion needs a presence conversion")
        scalar = PRIMITIVES[native]
        return scalar, f"Query<{scalar}, {scalar}>", "static value => value"
    if value.kind == "record":
        # Validate the whole record before emitting the operation.
        record = values.record(native)
        values.decoder(record)
        records.add(native)
        public = public_type(native)
        copy = (
            f"static value => Copy{public}(value)"
            if native in values.item_buffers
            else f"Copy{public}"
        )
        if shape == "array":
            helper = "QueryOptionalArray" if nullable else "QueryArray"
            return (
                f"{public}[]{'?' if nullable else ''}",
                f"{helper}<{native}, {public}>",
                copy,
            )
        if not nullable:
            return public, f"Query<{native}, {public}>", copy
        helper = (
            "QueryOptional" if values.declares_class(record) else "QueryOptionalValue"
        )
        return f"{public}?", f"{helper}<{native}, {public}>", copy
    raise Unsupported(f"result {native!r} with shape {shape!r} needs a conversion rule")


def emit_operation(plan: OperationPlan, bound: BoundApi) -> tuple[str, str, set[str]]:
    api = bound.source
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
    owner_expression = "null" if static else "this"
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
    guard = f'NativeCallbackGuard.EnsureAllowed(this, "{function.name}");'
    handle_plan = bound.handles.get(receiver)
    if (
        plan.consumes
        and handle_plan
        and function.name == handle_plan.release
        and handle_plan.release_inputs
    ):
        declarations, arguments = [], ["&live"]
        for parameter in plan.inputs:
            if parameter.name == plan.receiver:
                continue
            value = parameter.value
            values.supported(value)
            name = camel(parameter.name)
            declarations.append(f"{values.public_type(value)} {name}")
            if value.kind == "reference" and value.element:
                # The release runs synchronously, so its scope outlives the call.
                arguments.append(f"scope.Value({values.encode(value.element, name)})")
            else:
                arguments.append(values.encode(value, name))
        call = native_call(function, ", ".join(arguments), "diagnostic")
        return (
            owner_class,
            method(
                f"public void Release({', '.join(declarations)})",
                [
                    guard,
                    "using var scope = new NativeCallScope();",
                    f"state.Release((live, diagnostic) => {call});",
                ],
            ),
            set(values.plans),
        )
    if (
        handle_plan is not None
        and function.name == handle_plan.release
        and plan.completion
        and not handle_plan.release_inputs
    ):
        native_type = raw_handle(bound.handles[receiver])
        return (
            owners[receiver],
            (
                method(
                    "public Task CloseAsync()",
                    [guard, "state.Close();", "return teardown;"],
                )
                + "\n    public ValueTask DisposeAsync() => new(CloseAsync());\n\n"
                + method(
                    f"private mln_status StartRelease({native_type} handle, mln_diagnostic* _)",
                    [
                        f"teardown = NativeCompletion.SubmitUnit({submission(function, 'handle, completion')});",
                        "return mln_status.MLN_STATUS_OK;",
                    ],
                )
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
    asynchronous = plan.completion is not None
    records: set[str] = {receiver} if plan.scoped_receiver else set()
    args = ["Pointer" if plan.scoped_receiver else "Handle"] if receiver else []
    if plan.receiver_access == "issued":
        args[0] = "state.IssuedHandle"
    parameters = []
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
    direct = {
        registration.callback: registration
        for registration in plan.direct_registrations
    }
    direct_contexts = {
        registration.user_data: registration
        for registration in plan.direct_registrations
    }
    for registration in plan.direct_registrations:
        if not registration.release_callback:
            raise Unsupported("direct callback requires a native release")
    direct_releases = {
        registration.release_callback for registration in plan.direct_registrations
    }
    outputs_by_name = {parameter.name: parameter for parameter in plan.outputs}
    for parameter in function.parameters:
        if parameter.name == (plan.receiver or plan.scoped_receiver):
            continue
        if parameter.name in direct_contexts:
            registration = direct_contexts[parameter.name]
            callback = camel(registration.callback)
            callback_value = input_plans[registration.callback]
            # A callback restricted to its registration owner carries that owner.
            descriptor = (
                f"new NativeOwnedCallback({callback}, this)"
                if values.owned_direct_callback(callback_value)
                else callback
            )
            args.append(f"{callback} is null ? null : scope.Register({descriptor})")
            continue
        if parameter.name in direct_releases:
            args.append("&NativeCallbackRoot.Release")
            continue
        if parameter.name in direct:
            callback_value = input_plans[parameter.name]
            values.supported(callback_value)
            name = camel(parameter.name)
            parameters.append(f"{values.public_type(callback_value)} {name}")
            scoped = True
            args.append(
                f"{name} is null ? null : &Invoke{public_type(callback_value.native)}"
            )
            continue
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
        # The raw C type maps to its C# ABI carrier; the plan decides the rest.
        ctype = type_name(parameter.type)
        value_plan = input_plans.get(parameter.name)
        output = outputs_by_name.get(parameter.name)
        output_plan = output.value.element if output else None
        is_output = output is not None
        if (
            asynchronous
            and is_output
            and parameter.name not in {owner.parameter for owner in plan.owned_outputs}
        ):
            raise Unsupported("asynchronous output requires a completion value")
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
            native = raw_handle(owner.handle)
            if asynchronous:
                args.append("output")
            else:
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
                or input_plans[parameter.name].element
                and input_plans[parameter.name].element.kind == "scalar"
            )
        ):
            value_plan = input_plans[parameter.name]
            public = "string" if value_plan.encoding == "utf8" else "byte[]"
            parameters.append(f"{public} {name}")
            scoped = True
            encoder = "Utf8" if value_plan.encoding == "utf8" else "Buffer"
            prologue.append(f"var buffer{pascal(name)} = scope.{encoder}({name});")
            args.append(
                f"({values.raw_type(value_plan.element) + '*' if value_plan.element else values.raw_type(value_plan)})buffer{pascal(name)}.data"
            )
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
        elif value_plan and value_plan.kind == "enum" and ctype in PRIMITIVES:
            values.supported(value_plan)
            parameters.append(f"{public_type(value_plan.native)} {name}")
            args.append(f"({PRIMITIVES[ctype]}){name}")
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
            utf8 = value_plan.encoding == "utf8"
            parameters.append(f"{'string' if utf8 else 'byte[]'} {name}")
            scoped = True
            args.append(f"scope.{'Utf8' if utf8 else 'Buffer'}({name})")
        elif (
            value_plan
            and value_plan.buffer_form == "pointer"
            and value_plan.length == "nul"
            and value_plan.encoding == "utf8"
        ):
            parameters.append(f"string {name}")
            scoped = True
            args.append(f"scope.CStringArgument({name})")
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
            element = value_plan.element
            assert element is not None
            if asynchronous:
                # A submission closure cannot address a stack local.
                scoped = True
                args.append(values.encode(value_plan, name))
                continue
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
        elif (
            output_plan
            and output_plan.kind in {"scalar", "enum"}
            and type_name(parameter.type.pointee) in PRIMITIVES
        ):
            native = PRIMITIVES[type_name(parameter.type.pointee)]
            prologue.append(f"{native} {name} = default;")
            args.append(f"&{name}")
            if output_plan.kind == "enum":
                output_type = public_type(output_plan.native)
                outputs.append((output_type, f"({output_type}){name}"))
            else:
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
    decision_completion = any(
        decision.complete == function.name for decision in bound.decisions.values()
    )
    reads = bool(
        handle_plan
        and not asynchronous
        and outputs
        and function.name != handle_plan.release
    )
    # Entering checks the callback guard, keeps the receiver reachable, and, for
    # an owner-less operation, loads the native library. A scope that enters
    # for its receiver roots registrations in it, so only an owner can.
    if reads:
        entry = [f'using var read = state.Read(this, "{function.name}");']
        if scoped:
            entry.append("using var scope = new NativeCallScope();")
        args[0] = "read.Handle"
    elif (scoped or asynchronous) and not plan.scoped_receiver:
        entry = [
            f'using var scope = new NativeCallScope({owner_expression}, "{function.name}");'
        ]
    else:
        entry = [f'using var call = Enter({owner_expression}, "{function.name}");']
        if scoped:
            entry.append("using var scope = new NativeCallScope();")
    # The claim follows argument conversion, so an argument error leaves the
    # request open.
    claim = ["using var claim = state.BeginClaim();"] if decision_completion else []
    prologue = entry + prologue + claim
    arguments = ", ".join(args)
    # A free function that creates one owner is that owner's static factory,
    # named without the owner's prefix.
    prefix = bound.handles[factory].prefix + "_" if factory else None
    name = pascal(
        function.name.removeprefix(prefix)
        if prefix and function.name.startswith(prefix)
        else plan.member
    )
    modifiers = "public static" if static else "public"
    if plan.view:
        assert handle_plan and handle_plan.view_begin and handle_plan.view_end
        if len(outputs) != 1:
            raise Unsupported("borrowed view requires one output descriptor")
        output_type, copied = outputs[0]
        parameters.append(f"Action<{output_type}> callback")
        body = prologue + [
            "ArgumentNullException.ThrowIfNull(callback);",
            "var viewScope = new NativeViewScope();",
            "void* token = null;",
            checked_call(
                api.functions_by_name[handle_plan.view_begin], "read.Handle, &token"
            ),
            "try",
            "{",
            f"    {checked_call(function, arguments)}",
            f"    callback({copied});",
            "}",
            "finally",
            "{",
            "    viewScope.Expire();",
            f"    NativeMethods.{handle_plan.view_end}(token);",
            "}",
        ]
        return (
            owner_class,
            method(
                f"public void With{name.removeprefix('Get')}({', '.join(parameters)})",
                body,
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
        if asynchronous:
            body = prologue + [
                f"return scope.Attach<{native}, {result_type}>(",
                f"    (output, completion, diagnostic) => {native_call(function, arguments, 'diagnostic')},",
                f"    (handle, attachment) => {result_type}.Adopt({parent}handle, attachment)",
                ");",
            ]
        else:
            body = prologue + [checked_call(function, arguments)]
            constructor = f"{result_type}.Adopt({parent}{local})"
            if plan.registrations:
                body += [
                    f"var owner = {constructor};",
                    "scope.Accept(owner.CallbackOwner);",
                    "return owner;",
                ]
            else:
                if scoped:
                    body.append("scope.Accept();")
                body.append(f"return {constructor};")
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
        parameters.append("CancellationToken cancellationToken = default")
        # A handle that arrives after cancellation is disposed, not leaked.
        body = prologue + [
            f"return scope.Query<{raw_handle(bound.handles[result])}, {owners[result]}>(",
            f"    {submission(function, arguments)},",
            f"    handle => {owners[result]}.Adopt({parent}handle),",
            "    cancellationToken",
            ");",
        ]
    elif not asynchronous:
        if (
            handle_plan
            and handle_plan.release == function.name
            and len(function.parameters) == 1
        ):
            return (
                owners[receiver],
                f"    public void Close() {{ {guard} state.Close(); }}\n",
                records | values.plans.keys(),
            )
        if function.return_type.kind != "void" and not plan.status:
            if plan.result is None:
                raise Unsupported("direct return needs a resolved result")
            if function.diagnostic:
                raise Unsupported("direct return cannot report a diagnostic")
            values.supported(plan.result)
            result_type = values.public_type(plan.result)
            body = prologue + [
                f"var returned = NativeMethods.{function.name}({arguments});",
                f"return {values.copy(plan.result, 'returned')};",
            ]
            return (
                owner_class,
                method(
                    f"{modifiers} {result_type} {name}({', '.join(parameters)})", body
                ),
                records | values.plans.keys(),
            )
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
        body = prologue + [checked_call(function, arguments)]
        if scoped:
            accept = (
                "scope.Accept(CallbackOwner);"
                if handle_plan and (plan.registrations or plan.direct_registrations)
                else "scope.Accept();"
            )
            # A rejected registration stores nothing, so the scope releases it.
            rejected = next(
                (
                    registration.accepted_unless
                    for registration in plan.direct_registrations
                    if registration.accepted_unless
                ),
                None,
            )
            if rejected:
                accept = f"if (!{camel(rejected)}) {accept}"
            body.append(accept)
        if decision_completion:
            body.append("claim.Accept();")
        if result is not None:
            body.append(f"return {result};")
    else:
        parameters.append("CancellationToken cancellationToken = default")
        name += "Async"
        if execution == "command":
            if plan.result is not None:
                raise Unsupported(
                    "command carries a typed payload that needs its own conversion"
                )
            result_type = "Task<CommandCompletion>"
            call = (
                f"scope.Command({submission(function, arguments)}, cancellationToken);"
            )
        else:
            value_type, helper, copy = completion_query(plan, values, records, api)
            result_type = value_type if value_type == "Task" else f"Task<{value_type}>"
            copy = f", {copy}" if copy else ""
            call = f"scope.{helper}({submission(function, arguments)}{copy}, cancellationToken);"
        body = prologue + [f"return {call}"]
    return (
        owner_class,
        method(f"{modifiers} {result_type} {name}({', '.join(parameters)})", body),
        records | values.plans.keys(),
    )


def emit(api: Api | BoundApi) -> Emission:
    methods: dict[str, list[str]] = defaultdict(list)
    records: dict[str, set[str]] = defaultdict(set)
    supported = []
    bound = compile_api(api)
    api = bound.source
    owners = {name: owner_name(handle) for name, handle in bound.handles.items()}
    unsupported = {
        name: "\n".join(reasons) for name, reasons in bound.unsupported.items()
    }
    default_records: set[str] = set()
    owner_headers: dict[str, str] = {}
    conflicts = conflicting_functions(bound, "dotnet")
    for plan in bound.operations:
        function = plan.function
        try:
            if plan.support:
                if isinstance(plan.support, DefaultSupport):
                    value_name = plan.support.value
                    values = Values(bound)
                    values.decoder(values.record(value_name))
                    default_records.update(values.plans)
                    supported.append(function.name)
                    continue
                raise Unsupported("support operation requires its generated owner")
            if function.name in conflicts:
                raise Unsupported("public method name collides after conversion")
            owner, emitted, used_records = emit_operation(plan, bound)
        except Unsupported as error:
            unsupported[function.name] = f"{function.location}: {error}"
            continue
        methods[owner].append(emitted)
        records[owner].update(used_records)
        supported.append(function.name)
        # An owner without a declaration of its own lives with its first operation.
        owner_headers.setdefault(owner, function.location.path)
    files = {}
    files.update(dotnet_native.generate(bound))
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
    decision_handles = set(bound.decisions)
    created_handles.update(native for native in owners if owners[native] in methods)
    async_disposable: set[str] = set()
    for native in sorted(created_handles):
        handle = bound.handles[native]
        owner = owners[native]
        native_type = raw_handle(handle)
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
        # Adopt takes no pendingDecision, because a pending decision is
        # borrowed through BorrowDecision.
        adopt_extra = ", Task completion" if immediate_completion else ""
        cleanup = handle.dispose or handle.release
        cleanup_function = api.functions_by_name[cleanup]
        # Abandon disposes a handle that no owner can close any more.
        abandon = (
            f"=> {native_call(cleanup_function, 'live', 'diagnostic')};"
            if cleanup_function.diagnostic
            else f"{{ NativeMethods.{cleanup}(live); return mln_status.MLN_STATUS_OK; }}"
        )
        destroy = (
            "StartRelease"
            if asynchronous_release
            else f"static (live, diagnostic) => {native_call(release.function, 'live', 'diagnostic')}"
        )
        if handle.release_inputs or (
            not asynchronous_release
            and type_name(release.function.return_type) == "void"
        ):
            destroy = "Abandon"
        fields = [f"private readonly NativeHandleState<{native_type}> state;"]
        if asynchronous_release:
            fields.append("private volatile Task teardown = Task.CompletedTask;")
        if immediate_completion:
            fields.append("public Task Completion { get; }")
        constructor = []
        if immediate_completion:
            constructor.append(
                "Completion = completion.ContinueWith(static (finished, retained) => { GC.KeepAlive(retained); return finished; }, this, CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default).Unwrap();"
            )
        constructor.append(
            f"state = new(handle, {destroy}, nameof({owner}), Abandon{', pendingDecision' if decision_owner else ''}{', retainedParent: parent' if handle.parent else ''});"
        )
        declarations = (
            "".join(f"    {field}\n" for field in fields)
            + "\n"
            + method(
                f"internal {owner}({parent}{native_type} handle{extra})", constructor
            )
            + f"\n    internal static {owner} Adopt({parent}{native_type} handle{adopt_extra}) =>\n"
            f"        NativeHandleState<{native_type}>.Adopt(handle, () => new {owner}({argument}handle{extra_argument}), Abandon);\n"
            f"\n    private static mln_status Abandon({native_type} live, mln_diagnostic* diagnostic) {abandon}\n"
            f"\n    NativeHandleState<{native_type}> INativeOwner<{native_type}>.State => state;\n"
            f"    internal {native_type} Handle => state.Handle;\n"
            "    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;\n"
            "    // Runtime events report their source by this identity.\n"
            "    public ulong Id => state.IssuedHandle.Value;\n"
            "    public bool IsClosed => state.IsClosed;\n"
            f'    public void Dispose() {{ NativeCallbackGuard.EnsureAllowed(this, "{cleanup}"); state.Retire(); }}\n'
        )
        if decision_owner:
            declarations += f"    internal static {owner} BorrowDecision({native_type} handle) => new(handle, true);\n    internal bool FinishDecision(bool accepted) => state.FinishDecision(accepted);\n"
        methods[owner].insert(0, declarations)
    used_records = default_records | (
        set().union(*records.values()) if records else set()
    )
    values = Values(bound)
    # Every public enum is a public type, as in the other bindings, whether or
    # not a generated signature names it.
    enum_names = set()
    for value in bound.public_values.values():
        if value.kind == "enum":
            try:
                values.supported(value)
            except Unsupported:
                # The operations that use the enum report it unsupported.
                continue
            enum_names.add(value.native)
    # A handle owner's file sits with its release, a callback response scope's
    # with its record, and an owner without a declaration with its first
    # operation.
    declared_owners = {
        owner_name(handle): api.functions_by_name[handle.release].location.path
        for handle in bound.handles.values()
    } | {
        public_type(value.native): api.records_by_name[value.native].location.path
        for value in bound.public_values.values()
        if value.response
    }
    owner_directories = {
        owner: directory_for(declared_owners.get(owner, owner_headers[owner]))
        for owner in methods
    }
    # Every generated file shares one set of imports.
    files["GlobalUsings.g.cs"] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n"
        + "global using Maplibre.NativeFfi.Internal;\n"
        + "".join(
            f"global using Maplibre.NativeFfi.Internal.{namespace};\n"
            for namespace in ("C", "Callback", "Memory", "Pointer", "Struct")
        )
        # Generated values name presence bits without their enum.
        + "".join(
            f"global using static Maplibre.NativeFfi.Internal.C.{enum};\n"
            for enum in sorted(
                {
                    typed_mask(field)
                    for value in bound.values.values()
                    for field in value.fields
                }
                - {None}
            )
        )
    )
    values = Values(bound)
    for owner, body in sorted(methods.items()):
        directory = owner_directories[owner]
        handle_types = {
            owners[native]: raw_handle(bound.handles[native])
            for native in created_handles
        }
        interfaces = [
            *(["IDisposable"] if owner in handle_types else []),
            *(["IAsyncDisposable"] if owner in async_disposable else []),
            *(
                [f"INativeOwner<{handle_types[owner]}>"]
                if owner in handle_types
                else []
            ),
        ]
        bases = f" : {', '.join(interfaces)}" if interfaces else ""
        files[f"{directory}/{owner}.Operations.g.cs"] = (
            HEADER + OPERATION_HELPERS + f"namespace {NAMESPACE};\n\n"
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
    converters.extend(
        values.direct_callback_method(value)
        for _, value in sorted(direct_callbacks.items())
    )
    files["Internal/Struct/GeneratedValues.g.cs"] = (
        HEADER
        + VALUE_HELPERS
        + "using System.Runtime.CompilerServices;\nusing System.Runtime.InteropServices;\n\n"
        + "namespace Maplibre.NativeFfi.Internal.Struct;\n\ninternal static unsafe class GeneratedValues\n{\n"
        + "\n".join(converters)
        + "}\n"
    )
    for name in sorted(used_records):
        record = api.records_by_name[name]
        directory = directory_for(record.location.path)
        plan = values.record(name)
        declaration = values.declaration(plan)
        if plan.default:
            default = (
                f"    public static {public_type(name)} Default\n    {{\n"
                "        get\n        {\n"
                f'            using var call = NativeCall.Enter(null, "{plan.default}");\n'
                f"            return GeneratedValues.Copy{public_type(name)}(NativeMethods.{plan.default}());\n"
                "        }\n    }\n"
            )
            if declaration.rstrip().endswith(";"):
                declaration = declaration.rstrip()[:-1] + "\n{\n" + default + "}\n"
            else:
                declaration = declaration.rstrip()[:-1] + default + "}\n"
        files[f"{directory}/{public_type(name)}.g.cs"] = (
            HEADER + f"namespace {NAMESPACE};\n\n" + declaration
        )
    for enum in api.enums:
        if enum.name not in enum_names:
            continue
        directory = directory_for(enum.location.path)
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
        files[f"{directory}/{public_type(enum.name)}.g.cs"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            f"namespace {NAMESPACE};\n\n{flags}"
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
        directory = directory_for(api.records_by_name[native].location.path)
        properties = "".join(
            f"    public {values.member_type(value, member, fields)} {member} => scope.Active(value).{member};\n"
            for member, fields in values.members(value)
        )
        files[f"{directory}/{name}View.g.cs"] = (
            HEADER + f"namespace {NAMESPACE};\n\npublic sealed class {name}View\n{{\n"
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
