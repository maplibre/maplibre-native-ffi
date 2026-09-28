"""Generate Dart operations and value conversion from the shared binding plans."""

from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import replace

from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import KEYWORDS, LOCALS, conflicting_functions
from tools.bindgen.model import Api
from tools.bindgen.names import camel, type_name
from tools.bindgen.native_capture import copy_kind
from tools.bindgen.semantic import BoundApi, OperationPlan

from .dart_values import (
    Unsupported,
    Values,
    generated_owners,
    owner_names,
    public_name,
)


def adopt_owner(owned, expression, receiver, function, values):
    if owned.handle.native not in values.bound.public_handles:
        raise Unsupported("owned output requires a generated class")
    _, public, native = owner_names(owned.handle.native)
    parent = ""
    if owned.parent_parameter:
        parent = (
            "this as " + owner_names(owned.handle.parent)[1]
            if receiver and function.parameters[0].name == owned.parent_parameter
            else camel(owned.parent_parameter)
        )
    arguments = (parent + ", " if parent else "") + f"{native}({expression})"
    result = f"{public}._({arguments})"
    if values.bound.operations_by_name[function.name].registrations:
        result = f"({result}.._state.retain(registrations))"
    if not owned.handle.dispose:
        raise Unsupported("owned adoption requires a native disposer")
    cleanup = f"raw.{owned.handle.dispose}(handle)"
    if (
        values.bound.operations_by_name[owned.handle.dispose].function.return_type.kind
        != "void"
    ):
        cleanup = f"_check({cleanup})"
    result = f"_adoptOwned({expression}, () => {result}, (handle) {{ {cleanup}; }})"
    return public, result


def registration_body(plan, values, body, status=None):
    roots = (
        "_callbackPorts"
        if plan.receiver and not plan.owned_outputs
        else "_NativeCallbackPorts()"
    )
    if status:
        body = body.replace(
            "      return status;",
            "      if (status == nativeStatusOk) { registrations.accept(); }\n      return status;",
        )
        body = body.replace(
            "if (status == nativeStatusOk) { created",
            "if (status == nativeStatusOk) { registrations.accept(); created",
        )
    else:
        marker = f"_check(raw.{plan.name}("
        lines = body.splitlines()
        for index, line in enumerate(lines):
            if marker in line:
                lines.insert(index + 1, "      registrations.accept();")
                break
        body = "\n".join(lines)
    return f"      final registrations = _NativeRegistrations({roots});\n      try {{\n{body}\n      }} finally {{ registrations.close(); }}"


def lower_direct_registration(plan, values):
    if len(plan.direct_registrations) != 1:
        raise Unsupported("one direct registration required")
    registration = plan.direct_registrations[0]
    callback = next(p.value for p in plan.inputs if p.name == registration.callback)
    if registration.owner_release and registration.accepted_unless:
        if not any(
            c.decision and c.decision.cancel_registration == plan.name
            for c in values.bound.callbacks.values()
        ):
            raise Unsupported(
                "owner cancellation requires verified provider decision protocol"
            )
        return (
            owner_names(
                next(p.value.native for p in plan.inputs if p.name == plan.receiver)
            )[0],
            "  bool setCancelCallback(void Function() callback) => _registerResourceCancellation(this as ResourceRequestHandle, callback);\n",
        )
    adapters = [
        a for a in values.bound.callback_adapters if a.callback == callback.native
    ]
    if (
        not registration.release_callback
        or len(adapters) != 1
        or adapters[0].context != "mln_adapter_log_callback_state"
    ):
        raise Unsupported("direct callback requires a verified native queue adapter")
    adapter = adapters[0]
    return (
        "Globals",
        f"""  void logSetCallback(LogCallback callback, {{bool consume = false}}) {{
    final state = _LogCallbackState(callback, consume: consume);
    _callbackReleases.register(state.pointer.cast(), state.close, arena: state.arena);
    try {{
      _check(raw.{plan.name}(Native.addressOf<NativeFunction<raw.{callback.native}Function>>(raw.{adapter.function}), state.pointer.cast(), Native.addressOf<NativeFunction<raw.mln_log_callback_releaseFunction>>(raw.mln_adapter_dart_release)));
      _logCallbackState = state;
    }} catch (_) {{ _callbackReleases.reject(state.pointer.cast()); rethrow; }}
  }}
""",
    )


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
    owner = (
        owner_names(receiver)[0]
        if receiver in values.bound.public_handles
        else "Globals"
    )
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
        setup.append(
            "      final receiverPointer = arena<Uint64>()..value = handle.raw;"
        )
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
                setup.append(f"      final {local} = arena<Uint64>();")
                args.append(local)
                returns.append(
                    adopt_owner(owned, local + ".value", receiver, function, values)
                )
                continue
            values.check(output)
            setup.append(f"      final {local} = arena<{values.ffi(output)}>();")
            if output.kind == "record":
                if output.default:
                    setup.append(f"      {local}.ref = raw.{output.default}();")
                elif any(f.role == "size" for f in output.fields):
                    setup.append(
                        f"      {local}.ref.size = sizeOf<{values.ffi(output)}>();"
                    )
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
                        + "._(this as AcquiredFrame, "
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
            setup.append(f"      final bytes{local} = {view};")
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
                f"      final native{local} = arena<{values.ffi(child)}>({local}.isEmpty ? 1 : {local}.length);",
                f"      for (var index = 0; index < {local}.length; index++) {{ native{local}[index] = {values.native(child, local + '[index]')}; }}",
            ]
            args.append(f"native{local}")
        else:
            args.append(values.native(value, local))
    if optional:
        signature.append("{" + ", ".join(optional) + "}")
    call = f"raw.{function.name}({', '.join(args)})"
    if plan.consumes and not plan.completion:
        if not receiver or returns:
            raise Unsupported("consumption requires an owned receiver without outputs")
        body = "\n".join(setup) + (
            f"\n      return {call};"
            if status_return
            else f"\n      {call};\n      return nativeStatusOk;"
        )
        return (
            owner,
            f"  void {name}({', '.join(signature)}) => _state.close((handle) => withNativeArena((arena) {{\n{body}\n  }}), threadLastErrorMessage);\n",
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
        body = (
            "\n".join(setup)
            + (
                f"\n      _check({call});"
                if status_return
                else f"\n      final nativeResult = {call};"
                if plan.result
                else f"\n      {call};"
            )
            + (f"\n      return {result};" if result else "")
        )
        if plan.registrations:
            body = registration_body(plan, values, body)
        return (
            owner,
            f"  {public} {name}({', '.join(signature)}) => withNativeArena((arena) {{\n{body}\n  }});\n",
        )
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
        owned = plan.completion.immediate_owners[0]
        accept = "registrations.accept();" if plan.registrations else ""
        body = (
            "\n".join(setup)
            + f"\n      final status = {call};\n      if (status == nativeStatusOk) {{ {accept} try {{ created = {adoption}; }} catch (error, stack) {{ adoptionError = error; adoptionStack = stack; }} }}\n      return status;"
        )
        if plan.registrations:
            body = registration_body(plan, values, body, "status")
        return (
            owner,
            f"  {public.removesuffix('Handle')}Attachment {name}({', '.join(signature)}) {{\n    {public}? created;\n    Object? adoptionError; StackTrace? adoptionStack;\n    final completed = startNativeCompletion<void>(copyKind: raw.mln_adapter_completion_copy_kind.MLN_ADAPTER_COMPLETION_COPY_FLAT, elementSize: 0, start: (completion) => withNativeArena((arena) {{\n{body}\n    }}), decode: (_) {{}});\n    if (adoptionError != null) {{ completed.ignore(); Error.throwWithStackTrace(adoptionError!, adoptionStack!); }}\n    final session = created!;\n    return {public.removesuffix('Handle')}Attachment(session, completed.whenComplete(() {{ session.isClosed; }}));\n  }}\n",
        )

    if plan.registrations:
        setup = [
            registration_body(
                plan,
                values,
                "\n".join(setup)
                + f"\n      final status = {call};\n      return status;",
                "status",
            )
        ]
        call = None
    start = (
        "(completion) => withNativeArena((arena) {\n"
        + "\n".join(setup)
        + (f"\n      return {call};" if call else "")
        + "\n    })"
    )
    if execution == "command":
        if plan.result:
            raise Unsupported("command requires a resultless receipt")
        return (
            owner,
            f"  Future<CommandCompletion> {name}({', '.join(signature)}) => _startCommand({start});\n",
        )
    result = plan.result
    if result is None:
        public, kind, size, decode = (
            "void",
            "MLN_ADAPTER_COMPLETION_COPY_FLAT",
            "0",
            "null",
        )
    elif plan.completion.result_owner:
        public, decode = adopt_owner(
            plan.completion.result_owner,
            "result.value.cast<Uint64>().value",
            receiver,
            function,
            values,
        )
        kind, size = copy_kind(result.native), "sizeOf<Uint64>()"
    else:
        values.check(result)
        public = values.public(result)
        element = result.element if result.kind == "array" else result
        bare = replace(element, nullable=False)
        kind = (
            copy_kind(element.native)
            if element.kind in {"record", "handle"} or element.buffer_form == "view"
            else "MLN_ADAPTER_COMPLETION_COPY_FLAT"
        )
        size = f"sizeOf<{values.ffi(element)}>()"
        pointer = f"result.value.cast<{values.ffi(element)}>()"
        if result.kind == "array":
            decode = values.copy(
                replace(result, nullable=False), pointer, "result.value_count"
            )
            if result.optional:
                raise Unsupported(
                    "empty optional array needs cardinality interpretation"
                )
            if result.nullable:
                decode = f"result.value == nullptr ? null : {decode}"
        else:
            expression = pointer + (
                ".ref"
                if element.kind == "record" or element.buffer_form == "view"
                else ".value"
            )
            decode = values.copy(
                replace(bare, nullable=result.nullable)
                if bare.kind == "buffer"
                else bare,
                expression,
            )
            if result.nullable:
                decode = f"result.value_count == 0 ? null : {decode}"
    if plan.consumes:
        if public != "void" or not receiver:
            raise Unsupported(
                "asynchronous consumption requires an owned void receiver"
            )
        return (
            owner,
            f"  Future<void> {name}({', '.join(signature)}) => _state.closeAsync((handle) => startNativeCompletion(copyKind: raw.mln_adapter_completion_copy_kind.{kind}, elementSize: {size}, start: {start}, decode: (result) {{}}));\n",
        )
    return (
        owner,
        f"  Future<{public}> {name}({', '.join(signature)}) => startNativeCompletion(\n    copyKind: raw.mln_adapter_completion_copy_kind.{kind},\n    elementSize: {size},\n    start: {start},\n    decode: {'(result) {}' if public == 'void' else '(result) => ' + decode},\n    claimBeforeDecode: {str(bool(plan.completion.result_owner)).lower()},\n  );\n",
    )


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
            methods[owner].append(body)
            generated.append(plan.name)
        except Unsupported as error:
            unsupported[plan.name] = f"{plan.function.location}: {error}"
    return methods, generated, unsupported, values


def render_scoped_views(bound, generated, values):
    chunks = []
    emitted = set()
    for plan in bound.operations:
        if not plan.view or plan.name not in generated:
            continue
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
                else f"_value.{name}"
            )
            getters.append(
                f"  {'ScopedNativePointer' if native_pointer else typ} get {name} {{ _scope.checkActive(); return {result}; }}"
            )
        chunks.append(
            f"final class Scoped{public} {{\n  Scoped{public}._(AcquiredFrame owner, this._value) : _scope = _GeneratedNativeViewScope(owner, raw.{plan.view.owner.view_begin}, raw.{plan.view.owner.view_end});\n  final {public} _value;\n  final _GeneratedNativeViewScope _scope;\n  T withView<T>(T Function(Scoped{public}) use) => _scope.use(() => use(this));\n"
            + "\n".join(getters)
            + "\n}\n"
        )
    if chunks:
        chunks.append("""final class _GeneratedNativeViewScope {
  _GeneratedNativeViewScope(this.owner, this.begin, this.end);
  final AcquiredFrame owner;
  final int Function(int, Pointer<Pointer<Void>>) begin;
  final void Function(Pointer<Void>) end;
  int _active = 0;
  void checkActive() { if (_active == 0) { throwInvalidState('borrowed native value requires an active withView callback'); } }
  T use<T>(T Function() callback) => withNativeArena((arena) {
    final token = arena<Pointer<Void>>();
    _check(begin(owner._handle.raw, token));
    _active++;
    try {
      final result = callback();
      if (result is Future) { throwInvalidArgument('withView callback must complete synchronously'); }
      return result;
    } finally { _active--; end(token.value); }
  });
}
""")
    return "\n".join(chunks)


def registration_runtime(values):
    descriptors = [v for v in values.used.values() if v.registration]
    if not descriptors:
        return ""
    methods = []
    for value in descriptors:
        public = public_name(value.native)
        roots = "ports" if values.port_callbacks(value) else "_callbackReleases"
        methods.append(
            f"  Pointer<raw.{value.native}> prepare{public}({public} value) {{ final registration = _prepare{public}(value, {roots}); _pending.add(registration); return registration.pointer; }}"
        )
    return (
        """final class _NativeRegistrations {
  _NativeRegistrations(this.ports);
  final _NativeCallbackPorts ports;
  final _pending = <_NativeRegistration>[];
  bool _accepted = false;
  void accept() { _accepted = true; }
  void close() {
    for (final registration in _pending.reversed) {
      if (!_accepted) { registration.reject(); }
      registration.releaseMemory?.call();
    }
    _pending.clear();
  }
"""
        + "\n".join(methods)
        + "\n}\n"
    )


def generate(api: Api | BoundApi) -> str:
    methods, generated, _, values = lower(api)
    _, conversions = values.render()
    parts = re.split(
        r"(?m)(?=^(?:Pointer<raw\.\w+>|_NativeRegistration<raw\.\w+>|\w+) _(?:write|read|prepare)\w+\()",
        conversions,
    )
    conversion_map = {}
    for part in parts:
        match = re.search(r"(_(?:write|read|prepare)\w+)\(", part)
        if match:
            conversion_map[match[1]] = part
    needed = set(
        re.findall(
            r"_(?:write|read|prepare)\w+",
            "".join(body for bodies in methods.values() for body in bodies)
            + registration_runtime(values),
        )
    )
    needed.update("_read" + public_name(value.native) for value in values.projections)
    while True:
        expanded = needed | set(
            re.findall(
                r"_(?:write|read|prepare)\w+",
                "".join(conversion_map.get(name, "") for name in needed),
            )
        )
        if expanded == needed:
            break
        needed = expanded
    conversions = "".join(
        part for name, part in conversion_map.items() if name in needed
    )
    if "_generatedArenaUtf8" in conversions:
        conversions += """String _generatedArenaUtf8(Pointer<Uint8> data, int size, int offset, int length) {
  if (offset < 0 || length < 0 || offset > size || length > size - offset) { throwInvalidState('native message slice exceeds its arena'); }
  return length == 0 ? '' : utf8.decode((data + offset).asTypedList(length));
}
"""
    chunks = [
        "// Generated from the C headers by tools/bindgen. Do not edit.\npart of 'runtime.dart';\n",
        "final class _NativeRegistration<T extends Struct> {\n  const _NativeRegistration(this.pointer, this.reject, [this.releaseMemory]);\n  final Pointer<T> pointer;\n  final void Function() reject;\n  final void Function()? releaseMemory;\n}\n"
        if "_prepare" in conversions
        else "",
        registration_runtime(values),
        conversions,
        "",
        "int _generatedInteger(int value, int minimum, int maximum) {\n  if (value < minimum || value > maximum) { throwInvalidArgument('integer is outside its native range'); }\n  return value;\n}\n"
        if "_generatedInteger(" in conversions
        or any(
            "_generatedInteger(" in body
            for bodies in methods.values()
            for body in bodies
        )
        else "",
    ]
    native_types = {
        owner_names(native)[0]: owner_names(native)[2]
        for native in values.bound.public_handles
    }
    generated_mixins = {
        owner_names(native)[0] for native in generated_owners(values.bound)
    }
    for owner, bodies in sorted(methods.items()):
        if owner == "Globals":
            chunks.extend(bodies)
            continue
        handle = native_types[owner]
        chunks.append(
            f"mixin _Generated{owner}Operations implements Finalizable {{\n"
            + (
                f"  {handle} get _handle;\n"
                if any("_handle" in body for body in bodies)
                else ""
            )
        )
        if any("_state." in body for body in bodies):
            chunks.append(f"  NativeHandleState<{handle}> get _state;\n")
        if any("_callbackPorts" in body for body in bodies):
            chunks.append("  _NativeCallbackPorts get _callbackPorts;\n")
        chunks.extend(bodies)
        chunks.append("}\n")
        if owner in generated_mixins:
            public = owner + "Handle"
            chunks.append(
                f"/// {owner} handle id.\nextension type const {handle}(int raw) implements NativeHandle {{}}\n\n"
                f"final class {public} with _Generated{owner}Operations {{\n  {public}._({handle} handle) : _state = NativeHandleState(handle, '{public}');\n  @override final NativeHandleState<{handle}> _state;\n  @override {handle} get _handle => _state.handle;\n  bool get isClosed => _state.isClosed;\n}}\n"
            )
    chunks.append(render_scoped_views(compile_api(api), generated, values))
    return "\n".join(chunks)


def coverage(api: Api | BoundApi):
    _, generated, unsupported, values = lower(api)
    support = {}
    bound = compile_api(api)
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
    return (
        "// Generated from the C headers by tools/bindgen. Do not edit.\nimport 'dart:typed_data';\nimport 'render/native_pointer.dart';\n\n"
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
