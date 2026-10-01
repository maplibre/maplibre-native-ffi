"""Generate Dart operations and value conversion from the shared binding plans."""

from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import replace

from tools.bindgen import native_ports
from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import KEYWORDS, LOCALS, conflicting_functions
from tools.bindgen.model import Api
from tools.bindgen.names import camel, type_name
from tools.bindgen.native_capture import copy_kind
from tools.bindgen.semantic import BoundApi, OperationPlan

from .dart_values import (
    Unsupported,
    Values,
    deferred_key,
    owner_names,
    public_name,
)


def native_call(function, arguments):
    """Call a C function, passing the isolate's diagnostic when it takes one."""
    if function.diagnostic:
        arguments = [*arguments, "nativeDiagnostic"]
    return f"raw.{function.name}({', '.join(arguments)})"


def adopt_owner(owned, expression, receiver, function, values):
    if owned.handle.native not in values.bound.public_handles:
        raise Unsupported("owned output requires a generated class")
    public, native = owner_names(owned.handle.native)
    parent = ""
    if owned.handle.parent in values.bound.public_handles:
        if not owned.parent_parameter:
            raise Unsupported("owned output requires its parent owner")
        parent = (
            "this"
            if receiver and function.parameters[0].name == owned.parent_parameter
            else camel(owned.parent_parameter)
        )
    arguments = (parent + ", " if parent else "") + f"{native}({expression})"
    result = f"{public}._({arguments})"
    if values.bound.operations_by_name[function.name].registrations:
        result = f"({result}.._state.retain(registrations))"
    if not owned.handle.dispose:
        raise Unsupported("owned adoption requires a native disposer")
    dispose = values.bound.source.functions_by_name[owned.handle.dispose]
    cleanup = native_call(dispose, ["handle"])
    if dispose.return_type.kind != "void":
        cleanup = f"_check({cleanup})"
    result = f"_adoptOwned({expression}, () => {result}, (handle) {{ {cleanup}; }})"
    return public, result


def lower_port_registration(plan, registration, callback, values):
    """Lower a receiver's direct registration that a native port delivers.

    The port stays rooted by the receiver until native release retires it. A
    registration that native code does not accept releases the port at once.
    """
    function = plan.function
    receiver = type_name(function.parameters[0].type) if plan.receiver else None
    if receiver not in values.bound.public_handles:
        raise Unsupported("port registration requires an owned receiver")
    if (
        plan.execution != "immediate"
        or plan.completion
        or type_name(function.return_type) != "mln_status"
    ):
        raise Unsupported("port registration requires immediate admission status")
    inputs = {p.name: p.value for p in plan.inputs}
    if inputs[registration.release_callback].native != "mln_runtime_callback_release":
        raise Unsupported("port registration requires the runtime release callback")
    controls = {
        function.parameters[0].name,
        registration.callback,
        registration.user_data,
        registration.release_callback,
        registration.accepted_unless,
    }
    if any(p.name not in controls for p in function.parameters):
        raise Unsupported("port registration requires no other parameters")
    outputs = {p.name: p.value for p in plan.outputs}
    declined = outputs.get(registration.accepted_unless)
    if registration.accepted_unless and (
        not declined or not declined.element or declined.element.native != "bool"
    ):
        raise Unsupported("port registration refusal requires a boolean output")
    value = inputs[registration.callback]
    values.check(value)
    key = (
        "(raw.mln_adapter_dart_port_callback."
        f"{native_ports.constant(plan.name, registration.callback)} & 0xffffffff)"
    )
    decoded, offset = [], 1
    for parameter in callback.parameters:
        if parameter.name != callback.context:
            expression, offset = values.port_copy(parameter.value, offset)
            decoded.append(expression)
    name = camel(function.name.removeprefix(receiver.removesuffix("_handle") + "_"))
    arguments = {
        function.parameters[0].name: "handle.raw",
        registration.callback: f"raw.mln_adapter_dart_port_function({key}).cast()",
        registration.user_data: "port.context",
        registration.release_callback: "Native.addressOf<NativeFunction<raw.mln_runtime_callback_releaseFunction>>(raw.mln_adapter_dart_port_release).cast()",
        registration.accepted_unless: "declined",
    }
    call = native_call(function, [arguments[p.name] for p in function.parameters])
    if declined:
        storage = "      final declined = arena<Bool>();\n"
        accept, result, public = (
            "accepted = !declined.value;",
            "\n        return declined.value;",
            "bool",
        )
    else:
        storage, accept, result, public = "", "accepted = true;", "", "void"
    # Delivery queued before the receiver closes is dropped.
    body = (
        "      final handle = _handle;\n"
        + storage
        + f"      final port = _callbackPorts.register({{\n        {key}: (message) {{ if (!isClosed) {{ callback({', '.join(decoded)}); }} }},\n      }});\n"
        + f"      var accepted = false;\n      try {{\n        _check({call});\n        {accept}{result}\n      }} finally {{ if (!accepted) {{ port.reject(); }} }}"
    )
    return (
        receiver,
        f"  {public} {name}({values.public(value)} callback) => withNativeArena((arena) {{\n{body}\n  }});\n",
    )


def lower_deferred_registration(plan, registration, callback, values):
    """Lower a direct registration of a deferred callback through a native port.

    The generated deferred adapter answers each call at once and posts a copy
    to the port, which delivers it on this isolate. The receiver, or the
    isolate for a global registration, roots the port until native release
    retires it. A registration that native code does not accept releases the
    port at once.
    """
    function = plan.function
    receiver = type_name(function.parameters[0].type) if plan.receiver else None
    if receiver and receiver not in values.bound.public_handles:
        raise Unsupported("deferred registration requires an owned receiver")
    if (
        plan.execution != "immediate"
        or plan.completion
        or type_name(function.return_type) != "mln_status"
    ):
        raise Unsupported("deferred registration requires immediate admission status")
    if not registration.release_callback or registration.accepted_unless:
        raise Unsupported("deferred registration requires an unconditional release")
    controls = {
        registration.callback,
        registration.user_data,
        registration.release_callback,
    } | ({function.parameters[0].name} if receiver else set())
    if any(p.name not in controls for p in function.parameters):
        raise Unsupported("deferred registration requires no other parameters")
    inputs = {p.name: p.value for p in plan.inputs}
    value = inputs[registration.callback]
    values.check(value)
    key = deferred_key(callback)
    release = inputs[registration.release_callback].native
    arguments = {
        registration.callback: f"raw.mln_adapter_deferred_callback_function({key}).cast()",
        registration.user_data: "port.context",
        registration.release_callback: f"Native.addressOf<NativeFunction<raw.{release}Function>>(raw.mln_adapter_deferred_callback_release).cast()",
    }
    if receiver:
        arguments[function.parameters[0].name] = "_handle.raw"
        name = camel(function.name.removeprefix(receiver.removesuffix("_handle") + "_"))
        roots = "_callbackPorts"
    else:
        name = camel(function.name.removeprefix("mln_"))
        roots = "_globalCallbackPorts"
    call = native_call(function, [arguments[p.name] for p in function.parameters])
    body = (
        f"    final port = {roots}.registerDeferred({key}, (message) => _deliver{public_name(callback.native)}(callback, message));\n"
        f"    var accepted = false;\n    try {{\n      _check({call});\n      accepted = true;\n    }} finally {{ if (!accepted) {{ port.reject(); }} }}"
    )
    return (
        receiver or "Globals",
        f"  void {name}({values.public(value)} callback) {{\n{body}\n  }}\n",
    )


def lower_direct_registration(plan, values):
    """Lower a direct registration through a native port."""
    if len(plan.direct_registrations) != 1:
        raise Unsupported("one direct registration required")
    registration = plan.direct_registrations[0]
    for port_plan, _, callback, _ in native_ports.direct_callbacks(values.bound):
        if port_plan.name == plan.name:
            return lower_port_registration(plan, registration, callback, values)
    value = next(p.value for p in plan.inputs if p.name == registration.callback)
    callback = values.bound.callbacks.get(value.native)
    if callback and callback.deferred:
        return lower_deferred_registration(plan, registration, callback, values)
    raise Unsupported("direct callback requires a native port or deferred adapter")


def lower_function(plan: OperationPlan, values: Values) -> tuple[str, str]:
    function = plan.function
    if plan.consumes == "always" and (
        plan.completion or function.return_type.kind != "void"
    ):
        raise Unsupported("unconditional consumption requires a void immediate release")
    if plan.direct_registrations:
        return lower_direct_registration(plan, values)
    if plan.scoped_receiver or plan.direct_registrations:
        raise Unsupported(
            "operation requires its ownership or registration transaction"
        )
    receiver = type_name(function.parameters[0].type) if plan.receiver else None
    receiver_reference = False
    if (
        receiver
        and function.parameters
        and function.parameters[0].type.pointee
        and type_name(function.parameters[0].type.pointee)
        in values.bound.public_handles
    ):
        receiver = type_name(function.parameters[0].type.pointee)
        receiver_reference = True
    owner = receiver if receiver in values.bound.public_handles else "Globals"
    if owner == "Globals":
        receiver = None
    execution = plan.execution
    if execution not in {
        "command",
        "query",
        "operation",
        "immediate",
        "snapshot",
        "lifecycle",
        "drain",
        "event_batch",
        "render_driver",
    }:
        raise Unsupported("execution requires another runtime skeleton")
    status_return = type_name(function.return_type) == "mln_status"
    if not status_return and plan.completion:
        raise Unsupported("asynchronous operation requires admission status")
    name = camel(
        function.name.removeprefix(
            receiver.removesuffix("_handle") + "_"
            if receiver
            and function.name.startswith(receiver.removesuffix("_handle") + "_")
            else "mln_"
        )
    )
    handle_plan = values.bound.handles.get(receiver)
    if (
        plan.consumes
        and handle_plan
        and plan.name == handle_plan.release
        and not handle_plan.release_inputs
    ):
        name = "close"
    if name in KEYWORDS["dart"]:
        name += "Value"
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
    attach_output = None
    signature, optional, setup, args, returns = (
        [],
        [],
        [],
        [
            "handle.raw"
            if plan.consumes
            else "_state.handleId"
            if plan.receiver_access == "issued"
            else "_handle.raw"
        ]
        if receiver
        else [],
        [],
    )
    if receiver_reference:
        if not plan.consumes:
            raise Unsupported("mutable handle receiver requires consumption")
        setup.append("final receiverPointer = arena<Uint64>()..value = handle.raw;")
        args = ["receiverPointer"]
    for parameter in function.parameters[1 if receiver else 0 :]:
        if plan.completion and parameter.name == plan.completion.parameter:
            args.append("completion")
            continue
        local = camel(parameter.name)
        if (
            local in KEYWORDS["dart"]
            or local in LOCALS - {"value"}
            or local.startswith("native")
            # The generated library imports the C declarations as `raw`.
            or local == "raw"
        ):
            local += "Value"
        if parameter.name in counts:
            args.append(
                f"bytes{camel(counts[parameter.name])}.size"
                if inputs[counts[parameter.name]].value.kind == "buffer"
                else f"{camel(counts[parameter.name])}.length"
            )
            continue
        if parameter.name in outputs:
            output = outputs[parameter.name].value
            if output.kind != "reference" or not output.element:
                raise Unsupported("output needs one initialized native storage value")
            output = output.element
            owned = next(
                (o for o in plan.owned_outputs if o.parameter == parameter.name), None
            )
            if owned:
                if plan.completion:
                    attach_output = local
                    args.append(local)
                    returns.append(
                        adopt_owner(owned, "handle", receiver, function, values)
                    )
                    continue
                setup.append(f"final {local} = arena<Uint64>();")
                args.append(local)
                returns.append(
                    adopt_owner(owned, local + ".value", receiver, function, values)
                )
                continue
            values.check(output)
            setup.append(f"final {local} = arena<{values.ffi(output)}>();")
            if output.kind == "record":
                if output.default:
                    setup.append(f"{local}.ref = raw.{output.default}();")
                elif any(f.role == "size" for f in output.fields):
                    setup.append(f"{local}.ref.size = sizeOf<{values.ffi(output)}>();")
            args.append(local + ".cast()" if output.kind == "native_pointer" else local)
            expression = f"{local}." + (
                "ref"
                if output.kind == "record" or output.buffer_form == "view"
                else "value"
            )
            if plan.view:
                if (
                    output.kind != "record"
                    or not plan.view.owner.view_begin
                    or not plan.view.owner.view_end
                ):
                    raise Unsupported(
                        "borrowed view requires native scope operations and record output"
                    )
                returns.append(
                    (
                        "Scoped" + values.public(output),
                        "Scoped"
                        + values.public(output)
                        + "._(this, "
                        + values.copy(output, expression)
                        + ")",
                    )
                )
            else:
                returns.append((values.public(output), values.copy(output, expression)))
            continue
        if parameter.name not in inputs:
            raise Unsupported("parameter missing a resolved input contract")
        value = inputs[parameter.name].value
        if value.lifetime not in {"call", "value"}:
            raise Unsupported("input requires retained native storage")
        values.check(value)
        if value.kind == "buffer" and value.nullable and value.buffer_form != "view":
            raise Unsupported(
                "nullable counted pointer needs nullable count conversion"
            )
        typ = values.public(value)
        if typ.endswith("?"):
            optional.append(f"{typ} {local}")
        else:
            signature.append(f"{typ} {local}")
        bare = replace(value, nullable=False, optional=None)
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
                f"for (var index = 0; index < {local}.length; index++) {{ native{local}[index] = {values.native(child, local + '[index]')}; }}",
            ]
            args.append(f"native{local}")
        else:
            args.append(values.native(value, local))
    if optional:
        signature.append("{" + ", ".join(optional) + "}")
    call = native_call(function, args)
    roots = (
        "_callbackPorts"
        if plan.receiver and not plan.owned_outputs
        else "_NativeCallbackPorts()"
    )
    transaction = []
    if plan.registrations:
        if not status_return:
            raise Unsupported("registration requires an admission status")
        if plan.completion and plan.completion.result_owner:
            raise Unsupported("an adopted result cannot retain its registrations")
        # A transaction accepts its registrations once native code admits the call.
        transaction = [f"final registrations = _NativeRegistrations({roots});"]
        call = f"registrations.run(() => {call})"
        if not (plan.completion and returns):
            setup += transaction
            transaction = []
    if plan.consumes and not plan.completion:
        if not receiver or returns:
            raise Unsupported("consumption requires an owned receiver without outputs")
        body = setup + (
            [f"return {call};"]
            if status_return
            else [f"{call};", "return nativeStatusOk;"]
        )
        return (
            owner,
            f"  void {name}({', '.join(signature)}) => _state.close({closure('handle', body)});\n",
        )
    if not plan.completion:
        if not status_return and plan.result:
            values.check(plan.result)
            returns.insert(
                0,
                (values.public(plan.result), values.copy(plan.result, "nativeResult")),
            )
        public = (
            returns[0][0]
            if len(returns) == 1
            else "(" + ", ".join(t for t, _ in returns) + ")"
            if returns
            else "void"
        )
        result = (
            returns[0][1]
            if len(returns) == 1
            else "(" + ", ".join(v for _, v in returns) + ")"
            if returns
            else None
        )
        body = setup + [
            f"_check({call});"
            if status_return
            else f"final nativeResult = {call};"
            if plan.result
            else f"{call};"
        ]
        if result:
            body.append(f"return {result};")
        return owner, f"  {public} {name}({', '.join(signature)}) {method_body(body)}\n"
    start = f"(arena, completion) {block(setup + [f'return {call};'])}"
    if returns:
        if (
            len(returns) != 1
            or len(plan.completion.immediate_owners) != 1
            or plan.result
        ):
            raise Unsupported(
                "completion requires one immediate owner and void readiness"
            )
        public, adoption = returns[0]
        attachment = attachment_name(plan.completion.immediate_owners[0])
        output = attach_output
        start = f"(arena, completion, {output}) {block(setup + [f'return {call};'])}"
        operation = f"_attach({start}, (handle) => {adoption}, {attachment}.new)"
        if transaction:
            body = [*transaction, f"return {operation};"]
            return (
                owner,
                f"  {attachment} {name}({', '.join(signature)}) {{\n"
                + "".join(f"    {line}\n" for line in body)
                + "  }\n",
            )
        return owner, f"  {attachment} {name}({', '.join(signature)}) => {operation};\n"
    if execution == "command":
        if plan.result:
            raise Unsupported("command requires a resultless receipt")
        operation = f"_command({start})"
    elif plan.result is None:
        public, operation = "void", f"_run({start})"
    elif plan.completion.result_owner:
        public, decode = adopt_owner(
            plan.completion.result_owner, "handle", receiver, function, values
        )
        operation = f"_queryOwned(raw.mln_adapter_completion_copy_kind.{copy_kind(plan.result.native)}, {start}, (handle) => {decode})"
    else:
        result = plan.result
        values.check(result)
        public = values.public(result)
        element = result.element if result.kind == "array" else result
        if result.kind == "array" and result.optional:
            raise Unsupported("empty optional array needs cardinality interpretation")
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
        if public != "void" or not receiver:
            raise Unsupported(
                "asynchronous consumption requires an owned void receiver"
            )
        return (
            owner,
            f"  Future<void> {name}({', '.join(signature)}) => _state.closeAsync((handle) => {operation});\n",
        )
    if execution == "command":
        return (
            owner,
            f"  Future<CommandCompletion> {name}({', '.join(signature)}) => {operation};\n",
        )
    return owner, f"  Future<{public}> {name}({', '.join(signature)}) => {operation};\n"


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
    kind = (
        copy_kind(element.native)
        if element.kind in {"record", "handle"} or element.buffer_form == "view"
        else "MLN_ADAPTER_COMPLETION_COPY_FLAT"
    )
    suffix = (
        ".ref"
        if element.kind == "record" or element.buffer_form == "view"
        else ".value"
    )
    code = (
        f"_CompletionValue(raw.mln_adapter_completion_copy_kind.{kind}, sizeOf<{ffi}>(), "
        f"(element) => {values.copy(read, f'element.cast<{ffi}>(){suffix}')})"
    )
    stem = "_result" + re.sub(r"\W", "", values.public(read).replace("?", "OrNull"))
    name, index = stem, 2
    while values.results.get(name, code) != code:
        name, index = f"{stem}{index}", index + 1
    values.results[name] = code
    return name


def attachment_name(owned):
    return owner_names(owned.handle.native)[0].removesuffix("Handle") + "Attachment"


def lower(api: Api | BoundApi):
    methods, generated, unsupported = defaultdict(list), [], {}
    bound = compile_api(api)
    values = Values(bound)
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    conflicts = conflicting_functions(bound.source, "dart")
    for plan in bound.operations:
        try:
            if plan.name in conflicts:
                raise Unsupported("public method name collides after conversion")
            # Failed attempts cannot leave unrenderable value fragments behind.
            local_values = Values(bound)
            owner, body = lower_function(plan, local_values)
            local_values.render()
            values.used.update(local_values.used)
            values.results.update(local_values.results)
            methods[owner].append(body)
            generated.append(plan.name)
        except Unsupported as error:
            unsupported[plan.name] = f"{plan.function.location}: {error}"
    return methods, generated, unsupported, values


def render_scoped_views(bound, generated, values):
    chunks = []
    emitted = set()
    diagnostic = set()
    for plan in bound.operations:
        if not plan.view or plan.name not in generated:
            continue
        diagnostic.add(
            bound.source.functions_by_name[plan.view.owner.view_begin].diagnostic
        )
        value = plan.outputs[0].value.element
        public = values.public(value)
        if public in emitted:
            continue
        emitted.add(public)
        getters = []
        for name, typ, children, group in values.members(value):
            native_pointer = (
                len(children) == 1 and children[0].value.kind == "native_pointer"
            )
            result = (
                f"ScopedNativePointer(_value.{name}.address, checkValid: _scope.checkActive, debugName: '{public}.{name}')"
                if native_pointer
                else f"_scope.active(_value).{name}"
            )
            getters.append(
                f"  {'ScopedNativePointer' if native_pointer else typ} get {name} => {result};"
            )
        chunks.append(
            f"final class Scoped{public} {{\n  Scoped{public}._({owner_names(plan.view.owner.native)[0]} owner, this._value) : _scope = _NativeViewScope(() => owner._handle, raw.{plan.view.owner.view_begin}, raw.{plan.view.owner.view_end});\n  final {public} _value;\n  final _NativeViewScope _scope;\n  T withView<T>(T Function(Scoped{public}) use) => _scope.use(() => use(this));\n"
            + "\n".join(getters)
            + "\n}\n"
        )
    if chunks and diagnostic != {True}:
        raise Unsupported("borrowed view scopes require a diagnostic begin")
    return "\n".join(chunks)


def render_owner(native, handle, bodies, bound):
    public, native_type = owner_names(native)
    parameters, fields = [], []
    if handle.parent in bound.public_handles:
        parameters.append("this._parent")
        fields.append(
            "  // Keeps the parent owner reachable while this owner lives.\n"
            f"  // ignore: unused_field\n  final {owner_names(handle.parent)[0]} _parent;"
        )
    if any("_callbackPorts" in body for body in bodies):
        fields.append(
            "  // Roots this owner's port registrations for as long as it lives.\n"
            "  final _callbackPorts = _NativeCallbackPorts();"
        )
    fields.append(f"  final NativeHandleState<{native_type}> _state;")
    fields.append(f"  {native_type} get _handle => _state.handle;")
    parameters.append(f"{native_type} handle")
    return (
        f"/// Issued `{native}` handle id.\n"
        f"extension type const {native_type}(int raw) implements NativeHandle {{}}\n\n"
        f"/// Owner of one native `{native}` handle.\n"
        f"final class {public} implements Finalizable {{\n"
        f"  {public}._({', '.join(parameters)}) : _state = NativeHandleState(handle, '{public}');\n"
        + "\n".join(fields)
        + "\n\n  /// Whether this binding object has released its native handle.\n"
        "  bool get isClosed => _state.isClosed;\n\n"
        "  /// The issued native handle id.\n"
        "  BigInt get identity => uint64FromNative(_state.handleId);\n\n"
        + "".join(bodies)
        + "}\n"
    )


def render_attachments(bound, generated):
    """Pair each owner an attachment creates at once with its completion."""
    chunks = {}
    for plan in bound.operations:
        if plan.name not in generated or not plan.completion:
            continue
        for owned in plan.completion.immediate_owners:
            name = attachment_name(owned)
            field = camel(owned.parameter.removeprefix("out_"))
            chunks[name] = (
                f"/// A new {field} and the completion of the attachment that created it.\n"
                f"final class {name} {{\n"
                f"  const {name}(this.{field}, this.completed);\n\n"
                f"  /// The {field}, usable at once while attachment completes.\n"
                f"  final {owner_names(owned.handle.native)[0]} {field};\n\n"
                "  /// Completes after native attachment finishes.\n"
                "  final Future<void> completed;\n}\n"
            )
    return [chunks[name] for name in sorted(chunks)]


def generate(api: Api | BoundApi) -> str:
    methods, generated, _, values = lower(api)
    bound = values.bound
    _, conversions = values.render()
    parts = re.split(
        r"(?m)(?=^(?:Pointer<raw\.\w+>|_NativeRegistration<raw\.\w+>|\w+) _(?:write|read|prepare|deliver)\w+\()",
        conversions,
    )
    conversion_map = {}
    for part in parts:
        match = re.search(r"(_(?:write|read|prepare|deliver)\w+)\(", part)
        if match:
            conversion_map[match[1]] = part
    descriptors = "".join(
        f"final {name} = {code};\n" for name, code in sorted(values.results.items())
    )
    needed = set(
        re.findall(
            r"_(?:write|read|prepare|deliver)\w+",
            "".join(body for bodies in methods.values() for body in bodies)
            + descriptors,
        )
    )
    needed.update("_read" + public_name(value.native) for value in values.projections)
    while True:
        expanded = needed | set(
            re.findall(
                r"_(?:write|read|prepare|deliver)\w+",
                "".join(conversion_map.get(name, "") for name in needed),
            )
        )
        if expanded == needed:
            break
        needed = expanded
    conversions = "".join(
        part for name, part in conversion_map.items() if name in needed
    )
    chunks = [
        "// Generated from the C headers by tools/bindgen. Do not edit.\npart of 'runtime.dart';\n",
        conversions,
        descriptors,
    ]
    chunks.extend(abi_checked(body) for body in methods.get("Globals", []))
    for native, handle in sorted(bound.public_handles.items()):
        chunks.append(render_owner(native, handle, methods.get(native, []), bound))
    chunks.extend(render_attachments(bound, generated))
    chunks.append(render_scoped_views(bound, generated, values))
    return "\n".join(chunks)


def abi_checked(body: str) -> str:
    """Validate the C ABI first in an entry point that needs no existing handle.

    Receiver operations are reachable only through a handle that such an entry
    point produced, so only globals carry the check.
    """
    head, newline, rest = body.partition("\n")
    if head.rstrip().endswith("{"):
        return f"{head}\n    ensureAbiVersion();{newline}{rest}"
    signature, arrow, expression = body.partition(" => ")
    if not arrow or not expression.rstrip().endswith(";"):
        raise Unsupported("global entry point needs an ABI check insertion point")
    return (
        f"{signature} {{\n    ensureAbiVersion();\n"
        f"    return {expression.rstrip()[:-1]};\n  }}\n"
    )


def coverage(api: Api | BoundApi):
    _, generated, unsupported, values = lower(api)
    support = {}
    bound = values.bound
    for value in values.used.values():
        if value.registration:
            for adapter in values.registration_adapters(value):
                for name in adapter.invokes:
                    if bound.operations_by_name[name].scoped_receiver:
                        support[name] = (
                            f"native callback adapter {adapter.function} for {value.native}"
                        )
                        unsupported.pop(name, None)
    return {"generated": generated, "support": support, "unsupported": unsupported}


def generate_values(api: Api | BoundApi) -> str:
    _, _, _, values = lower(api)
    declarations, _ = values.render()
    # Callback types name the handle owners that the runtime library defines.
    owners = sorted(
        name
        for name in (owner_names(native)[0] for native in values.bound.public_handles)
        if re.search(rf"\b{name}\b", declarations)
    )
    runtime = (
        f"import 'runtime/runtime.dart' show {', '.join(owners)};\n" if owners else ""
    )
    return (
        "// Generated from the C headers by tools/bindgen. Do not edit.\nimport 'dart:typed_data';\nimport 'render/native_pointer.dart';\n"
        + runtime
        + "\n"
        + declarations
        + """
bool _generatedValueEquals(Object? left, Object? right) {
  if (left is List && right is List) {
    if (left.length != right.length) { return false; }
    for (var index = 0; index < left.length; index++) {
      if (!_generatedValueEquals(left[index], right[index])) { return false; }
    }
    return true;
  }
  return left == right;
}
int _generatedValueHash(Object? value) => value is List
    ? Object.hashAll(value.map(_generatedValueHash)) : value.hashCode;
"""
    )
