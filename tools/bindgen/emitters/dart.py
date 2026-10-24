"""Generate Dart operations and value conversion from the shared binding plans."""

from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import replace

from tools.bindgen import async_hosts
from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import KEYWORDS, LOCALS
from tools.bindgen.model import Api
from tools.bindgen.names import camel, pascal, type_name
from tools.bindgen.semantic import (
    BoundApi,
    OperationPlan,
    output_member,
)

from .dart_values import (
    Unsupported,
    Values,
    doc,
    owner_names,
)

# Functions that only host platform code can call. `mln_android_init` takes a
# `JNIEnv*` and a `Context`, which an isolate has no safe way to obtain, so a
# Flutter app's Android host code calls it before the isolate starts.
HOST_ONLY = {"mln_android_init"}


def native_call(function, arguments):
    """Call a C function, passing the isolate's diagnostic when it takes one."""
    if function.diagnostic:
        arguments = [*arguments, "nativeDiagnostic"]
    return f"raw.{function.name}({', '.join(arguments)})"


def disposer_arguments(handle, function, live):
    """The arguments of a handle's disposer: the handle, and a null completion
    when the dispose is also the release."""
    arguments = [live]
    for parameter in function.parameters[1:]:
        if parameter.name != handle.dispose_completion:
            raise Unsupported(f"{function.name} disposes with more than its handle")
        arguments.append("nullptr")
    return arguments


def adopt_owner(owned, expression, receiver, function, values):
    if owned.handle.native not in values.bound.public_handles:
        raise Unsupported("owned output requires a generated class")
    public, native = owner_names(owned.handle)
    parent = ""
    if owned.handle.parent in values.bound.public_handles:
        if not owned.parent_parameter:
            raise Unsupported("owned output requires its parent owner")
        parent = "this"
    arguments = (parent + ", " if parent else "") + f"{native}({expression})"
    result = f"{public}._({arguments})"
    if values.bound.operations_by_name[function.name].registrations:
        result = f"({result}.._state.retain(registrations))"
    if not owned.handle.cleanup:
        raise Unsupported("owned adoption requires a native disposer")
    dispose = values.bound.source.functions_by_name[owned.handle.cleanup]
    cleanup = native_call(dispose, disposer_arguments(owned.handle, dispose, "handle"))
    if dispose.return_type.kind != "void":
        cleanup = f"_check({cleanup})"
    result = f"_adoptOwned({expression}, () => {result}, (handle) {{ {cleanup}; }})"
    return public, result


def lower_function(
    plan: OperationPlan, values: Values, *, discard=False, dispose=False
) -> tuple[str, str]:
    function = plan.function
    # A dispose that is also the release passes no completion, so it consumes
    # synchronously, as a separate dispose does.
    disposed = None
    if dispose:
        disposed = plan.completion.parameter
        plan = replace(plan, completion=None)
    if plan.consumes == "always" and (
        plan.completion or function.return_type.kind != "void"
    ):
        raise Unsupported("unconditional consumption requires a void immediate release")
    if plan.scoped_receiver:
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
        "event_batch",
    }:
        raise Unsupported("execution requires another runtime skeleton")
    status_return = plan.status
    if not status_return and plan.completion:
        raise Unsupported("asynchronous operation requires admission status")
    name = camel(plan.member)
    handle_plan = values.bound.handles.get(receiver)
    if (
        plan.consumes
        and handle_plan
        and plan.name == handle_plan.release
        and not handle_plan.release_inputs
    ):
        name = "close"
    if dispose:
        name = "dispose"
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
    # Caller-sized text passes its storage, capacity, and length to a closure
    # that the runtime calls again when the text did not fit.
    text = plan.caller_sized_text
    text_arguments = (
        {
            text.name: "textStorage",
            text.value.length: "textCapacity",
            text.result_length: "textLength",
        }
        if text
        else {}
    )
    attach_output = None
    signature, optional, setup, args, returns = (
        [],
        [],
        [],
        ["handle.raw" if plan.consumes else "_handle.raw"] if receiver else [],
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
        if parameter.name == disposed:
            args.append("nullptr")
            continue
        local = camel(parameter.name)
        if (
            local in KEYWORDS["dart"]
            or local in LOCALS - {"value"}
            or local.startswith("native")
            or local == "raw"
        ):
            local += "Value"
        if parameter.name in text_arguments:
            if parameter.name == text.name:
                setup.append("final text = _CallerSizedText();")
                returns.append(("Uint8List", "text.text"))
            args.append(text_arguments[parameter.name])
            continue
        if parameter.name in counts:
            args.append(
                f"bytes{camel(counts[parameter.name])}.size"
                if inputs[counts[parameter.name]].value.kind == "buffer"
                else f"{camel(counts[parameter.name])}.length"
            )
            continue
        if parameter.name in outputs:
            output_plan = outputs[parameter.name]
            output = output_plan.value
            if output_plan.sized_by:
                values.check(output)
                source = camel(output_plan.sized_by)
                count = f"{source}.length"
                element = output.element
                setup.append(
                    f"final {local} = arena<{values.ffi(element)}>({source}.isEmpty ? 1 : {count});"
                )
                if element.kind == "record":
                    if element.default:
                        setup.append(
                            f"for (var index = 0; index < {count}; index++) {{ {local}[index] = raw.{element.default}(); }}"
                        )
                    elif any(f.role == "size" for f in element.fields):
                        setup.append(
                            f"for (var index = 0; index < {count}; index++) {{ {local}[index].size = sizeOf<{values.ffi(element)}>(); }}"
                        )
                args.append(local)
                returns.append(
                    (values.public(output), values.copy(output, local, count))
                )
                continue
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
        values.check(value)
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
                f"for (var index = 0; index < {local}.length; index++) {{ {values.write_element(child, local + '[index]', 'native' + local)} }}",
            ]
            args.append(f"native{local}")
        else:
            args.append(values.native(value, local))
    if optional:
        signature.append("{" + ", ".join(optional) + "}")
    if text:
        # Text that did not fit calls again, so its inputs convert once,
        # before the first call.
        for index, parameter in enumerate(function.parameters):
            if parameter.name in text_arguments or (receiver and index == 0):
                continue
            local = "native" + pascal(parameter.name)
            setup.append(f"final {local} = {args[index]};")
            args[index] = local
    call = native_call(function, args)
    if text:
        call = f"text.call(arena, (textStorage, textCapacity, textLength) => {call})"
    # The receiver roots the callbacks it registers, an adopted owner roots
    # those of its call, and the isolate roots a global call's.
    roots = (
        "_CallbackRoots()"
        if plan.owned_outputs
        else "_callbackRoots"
        if receiver
        else "_globalCallbackRoots"
    )
    transaction = []
    if plan.registrations:
        if not status_return:
            raise Unsupported("registration requires an admission status")
        if plan.completion and plan.completion.result_owner:
            raise Unsupported("an adopted result cannot retain its registrations")
        # A transaction accepts its registrations once native code admits the
        # call, unless native reports that it kept nothing from them.
        transaction = [f"final registrations = _NativeRegistrations({roots});"]
        declined = next(
            (r.accepted_unless for r in plan.registrations if r.accepted_unless),
            None,
        )
        declined = f", declined: () => {camel(declined)}.value" if declined else ""
        call = f"registrations.run(() => {call}{declined})"
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
        if plan.absence:
            public += "?"
        body = setup + [
            f"if (!_present({call}, raw.{plan.absence.status})) {{ return null; }}"
            if plan.absence
            else f"_check({call});"
            if status_return
            else f"final nativeResult = {call};"
            if plan.result
            else f"{call};"
        ]
        if result:
            body.append(f"return {result};")
        return owner, f"  {public} {name}({', '.join(signature)}) {method_body(body)}\n"
    start = f"(arena, completion) {block(setup + [f'return {call};'])}"
    if discard:
        return owner, f"  void {name}({', '.join(signature)}) => _discard({start});\n"
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
        operation = f"_queryOwned({start}, (handle) => {decode})"
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
    suffix = (
        ".ref"
        if element.kind == "record" or element.buffer_form == "view"
        else ".value"
    )
    code = (
        f"_CompletionValue(sizeOf<{ffi}>(), "
        f"(element) => {values.copy(read, f'element.cast<{ffi}>(){suffix}')})"
    )
    stem = "_result" + re.sub(r"\W", "", values.public(read).replace("?", "OrNull"))
    name, index = stem, 2
    while values.results.get(name, code) != code:
        name, index = f"{stem}{index}", index + 1
    values.results[name] = code
    return name


def attachment_name(owned):
    return pascal(owned.handle.stem) + "Attachment"


def lower(api: Api | BoundApi):
    methods, generated, unsupported = defaultdict(list), [], {}
    bound = compile_api(api)
    values = Values(bound)
    root_owners = values.root_owners
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    for plan in bound.operations:
        if plan.name in HOST_ONLY:
            continue
        try:
            # Failed attempts cannot leave unrenderable value fragments behind.
            local_values = Values(bound)
            owner, body = lower_function(plan, local_values)
            if owner == "Globals":
                body = abi_checked(body)
            body = doc(bound, plan.name, "  ") + body
            local_values.render()
            values.used.update(local_values.used)
            values.results.update(local_values.results)
            methods[owner].append(body)
            if plan.receiver and plan.completion and plan.completion.discardable:
                _, discarded = lower_function(plan, local_values, discard=True)
                methods[owner + "Discarding"].append(discarded)
            if plan.receiver and plan.completion and plan.completion.optional:
                _, disposed = lower_function(plan, local_values, dispose=True)
                methods[owner].append(doc(bound, plan.name, "  ") + disposed)
            generated.append(plan.name)
            if owner != "Globals" and plan.registrations and not plan.owned_outputs:
                root_owners.add(owner)
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
        for name, typ, field in values.members(value):
            native_pointer = field.value.kind == "native_pointer"
            result = (
                f"ScopedNativePointer(_value.{name}.address, checkValid: _scope.checkActive, debugName: '{public}.{name}')"
                if native_pointer
                else f"_scope.active(_value).{name}"
            )
            getters.append(
                f"  {'ScopedNativePointer' if native_pointer else typ} get {name} => {result};"
            )
        chunks.append(
            f"final class Scoped{public} {{\n  Scoped{public}._({owner_names(plan.view.owner)[0]} owner, this._value) : _scope = _NativeViewScope(owner._state, raw.{plan.view.owner.view_begin}, raw.{plan.view.owner.view_end});\n  final {public} _value;\n  final _NativeViewScope _scope;\n  T withView<T>(T Function(Scoped{public}) use) => _scope.use(() => use(this));\n"
            + "\n".join(getters)
            + "\n}\n"
        )
    if chunks and diagnostic != {True}:
        raise Unsupported("borrowed view scopes require a diagnostic begin")
    return "\n".join(chunks)


def render_owner(native, handle, bodies, bound, roots):
    public, native_type = owner_names(handle)
    parameters, fields = [], []
    if handle.parent in bound.public_handles:
        parameters.append("this._parent")
        fields.append(
            "  // Keeps the parent owner reachable while this owner lives.\n"
            f"  // ignore: unused_field\n  final {owner_names(bound.handles[handle.parent])[0]} _parent;"
        )
    if roots:
        fields.append(
            "  // Roots this owner's callback registrations for as long as it lives.\n"
            "  @override\n  final _callbackRoots = _CallbackRoots();"
        )
    fields.append(f"  final NativeHandleState<{native_type}> _state;")
    fields.append(f"  {native_type} get _handle => _state.handle;")
    commands = [
        p
        for p in bound.operations
        if p.completion
        and p.completion.discardable
        and any(i.name == p.receiver and i.value.native == native for i in p.inputs)
    ]
    if commands:
        fields.append(
            f"  {public}Discarding get discarding => {public}Discarding._(this);"
        )
    parameters.append(f"{native_type} handle")
    return (
        f"/// Issued `{native}` handle id.\n"
        f"extension type const {native_type}(int raw) implements NativeHandle {{}}\n\n"
        f"/// Owner of one native `{native}` handle.\n"
        + (f"///\n{summary}" if (summary := doc(bound, native)) else "")
        + f"final class {public} implements Finalizable{', _CallbackRootOwner' if roots else ''} {{\n"
        f"  {public}._({', '.join(parameters)}) : _state = NativeHandleState(handle, '{public}');\n"
        + "\n".join(fields)
        + "\n\n  /// The native handle id, which runtime events report as their source.\n"
        "  ///\n"
        "  /// It stays readable after close, so events drained later still match\n"
        "  /// this handle.\n"
        "  int get id => _state.handleId;\n\n" + "".join(bodies) + "}\n"
    )


def render_attachments(bound, generated):
    """Pair each owner an attachment creates at once with its completion."""
    chunks = {}
    for plan in bound.operations:
        if plan.name not in generated or not plan.completion:
            continue
        for owned in plan.completion.immediate_owners:
            name = attachment_name(owned)
            field = camel(output_member(owned.parameter))
            chunks[name] = (
                f"/// A new {field} and the completion of the attachment that created it.\n"
                f"final class {name} {{\n"
                f"  const {name}(this.{field}, this.completed);\n\n"
                f"  /// The {field}, usable at once while attachment completes.\n"
                f"  final {owner_names(owned.handle)[0]} {field};\n\n"
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
    chunks.extend(methods.get("Globals", []))
    for native, handle in sorted(bound.public_handles.items()):
        if discarded := methods.get(native + "Discarding"):
            public, native_type = owner_names(handle)
            chunks.append(
                f"extension type {public}Discarding._({public} _owner) {{\n"
                f"  {native_type} get _handle => _owner._handle;\n"
                + (
                    "  _CallbackRoots get _callbackRoots => _owner._callbackRoots;\n"
                    if native in values.root_owners
                    else ""
                )
                + "".join(discarded)
                + "}\n"
            )
        chunks.append(
            render_owner(
                native,
                handle,
                methods.get(native, []),
                bound,
                native in values.root_owners,
            )
        )
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
    bound = values.bound
    # A callback that must answer on its native thread has no Dart form, so its
    # registration carries native rules instead, and the operations that only
    # such a callback calls have no caller.
    registrations = [v for v in values.used.values() if v.registration]
    for name in async_hosts.rule_answered_operations(bound, registrations):
        unsupported.pop(name, None)
    return {
        "generated": generated,
        "unsupported": unsupported,
    }


def generate_values(api: Api | BoundApi) -> str:
    """The public value types, as a part of the handwritten values.dart."""
    _, _, _, values = lower(api)
    declarations, _ = values.render()
    return (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n"
        "part of 'values.dart';\n\n" + declarations
    )
