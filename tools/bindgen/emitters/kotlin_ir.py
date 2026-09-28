"""Lower Kotlin operations from verified parameter and result plans."""

from __future__ import annotations

from dataclasses import replace

from ..managed_contracts import LOCALS
from . import kotlin_callbacks
from .kotlin_values import Unsupported, identifier, name, owner_name


def parameter_name(native):
    local = identifier(native)
    return local + "Value" if local in LOCALS else local


def signature(plan, values):
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
    receiver_type = receiver_value(plan) if receiver else None
    if receiver and receiver_type.native not in values.bound.public_handles:
        raise Unsupported("operation requires its generated owner")
    if plan.execution not in {
        "command",
        "query",
        "snapshot",
        "immediate",
        "operation",
        "render_driver",
        "drain",
        "lifecycle",
        "event_batch",
    }:
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
    multiple = len(plan.outputs) > 1
    result = (
        plan.outputs[0].value
        if plan.completion and plan.completion.immediate_owners
        else plan.result
        if plan.completion
        else plan.outputs[0].value
        if plan.outputs
        else plan.result
    )
    if multiple:
        from .kotlin_outputs import signature as output_signature

        result_type = output_signature(plan, values)
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
    return (
        method_name(plan),
        params,
        inputs,
        result,
        result_type,
    )


def method_name(plan):
    """Name an operation's method after its receiver prefix, or its whole name."""
    prefix = receiver_value(plan).native + "_" if plan.receiver else "mln_"
    return identifier(
        plan.name.removeprefix(prefix if plan.name.startswith(prefix) else "mln_")
    )


def admission(plan):
    owner = receiver_handle(plan) + ".toLong()" if plan.receiver else "null"
    return f'org.maplibre.nativeffi.internal.callback.CallbackAdmission.check({owner}, "{plan.name}")'


def receiver_value(plan):
    value = next(p.value for p in plan.inputs if p.name == plan.receiver)
    return value.element if value.kind == "reference" else value


def receiver_handle(plan):
    return (
        ("bindingIssued" if plan.receiver_access == "issued" else "binding")
        + name(receiver_value(plan).native)
        + "Handle()"
    )


def call_arguments(plan, values, platform):
    lengths = {
        p.value.length: p
        for p in plan.inputs
        if p.value.kind in {"array", "buffer"}
        and p.value.length not in {None, "nul", "1"}
    }
    result = []
    for parameter in plan.inputs:
        if parameter.name == plan.receiver:
            result.append(receiver_handle(plan))
        elif parameter.name in lengths:
            source = lengths[parameter.name]
            expression = parameter_name(source.name)
            if source.value.kind == "buffer" and source.value.encoding == "utf8":
                expression += ".encodeToByteArray()"
            result.append(
                expression
                + ".size."
                + ("convert()" if platform == "nativeMain" else "toLong()")
            )
        else:
            result.append(
                native_input(
                    parameter.value, parameter_name(parameter.name), values, platform
                )
            )
    return result


def native_input(value, local, values, platform):
    if value.kind == "handle":
        return local + ".binding" + name(value.native) + "Handle()"
    if value.kind == "reference":
        child = value.element
        if child.kind not in {"record", "buffer"}:
            raise Unsupported("pointer input requires a record")
        expression = values.cast_native(
            child, local + ("!!" if value.nullable else ""), platform
        )
        null = "MemorySegment.NULL" if platform == "jvmMain" else "null"
        return (
            f"if ({local} == null) {null} else {expression}"
            if value.nullable
            else expression
        )
    expression = values.cast_native(value, local, platform)
    if platform == "nativeMain" and (
        value.kind == "record" or value.buffer_form == "view"
    ):
        expression += ".pointed.readValue()"
    return expression


def result_decode(value, values, platform):
    bare = replace(value, nullable=False, optional=None)
    if value.kind == "handle":
        expression = owned_raw(platform)
    elif value.kind == "array":
        native = value.element.native
        pointer = (
            "mln_completion_result.value(result)"
            if platform == "jvmMain"
            else f"result.pointed.value?.reinterpret<{native}>()"
            if platform == "nativeMain"
            else f"MaplibreNativeC.{native}(result.value())"
        )
        count = (
            "mln_completion_result.value_count(result)"
            if platform == "jvmMain"
            else "result.pointed.value_count"
            if platform == "nativeMain"
            else "result.value_count()"
        )
        expression = values.read_array(value, pointer, count, platform)
    elif value.kind == "buffer":
        pointer = (
            "NativeAccess.completionValue(result, mln_buffer_view.sizeof())"
            if platform == "jvmMain"
            else "result.pointed.value!!.reinterpret<mln_buffer_view>().pointed"
            if platform == "nativeMain"
            else "MaplibreNativeC.mln_buffer_view(result.value())"
        )
        expression = values.cast_public(
            replace(value, optional=None), pointer, platform
        )
    elif value.kind == "record":
        pointer = (
            f"NativeAccess.completionValue(result, {value.native}.sizeof())"
            if platform == "jvmMain"
            else f"result.pointed.value!!.reinterpret<{value.native}>().pointed"
            if platform == "nativeMain"
            else f"MaplibreNativeC.{value.native}(result.value())"
        )
        expression = values.cast_public(bare, pointer, platform)
    else:
        typ, layout = values.scalar(value)
        if platform == "jvmMain":
            expression = f"NativeAccess.completionValue(result, ValueLayout.JAVA_{layout}.byteSize()).get(ValueLayout.JAVA_{layout}, 0)"
        elif platform == "nativeMain":
            var = {
                "Boolean": "BooleanVar",
                "Double": "DoubleVar",
                "Float": "FloatVar",
                "UByte": "UByteVar",
                "Byte": "ByteVar",
                "UShort": "UShortVar",
                "Short": "ShortVar",
                "UInt": "UIntVar",
                "Int": "IntVar",
                "ULong": "ULongVar",
                "Long": "LongVar",
            }[typ]
            expression = f"result.pointed.value!!.reinterpret<{var}>().pointed.value"
        else:
            pointer = {
                "BOOLEAN": "BoolPointer",
                "DOUBLE": "DoublePointer",
                "FLOAT": "FloatPointer",
                "BYTE": "BytePointer",
                "SHORT": "ShortPointer",
                "INT": "IntPointer",
                "LONG": "LongPointer",
            }[layout]
            expression = f"{pointer}(result.value()).get()"
        expression = values.cast_public(bare, expression, platform)
    if value.optional == "empty":
        expression += ".takeIf { it.isNotEmpty() }"
    elif value.nullable:
        count = (
            "mln_completion_result.value_count(result)"
            if platform == "jvmMain"
            else "result.pointed.value_count.toLong()"
            if platform == "nativeMain"
            else "result.value_count()"
        )
        if value.kind == "array":
            absent = (
                "mln_completion_result.value(result).address() == 0L"
                if platform == "jvmMain"
                else "result.pointed.value == null"
                if platform == "nativeMain"
                else "result.value() == null || result.value().isNull"
            )
            expression = f"if ({absent}) null else {expression}"
        else:
            expression = f"if ({count} == 0L) null else {expression}"
    return "{ result -> " + expression + " }"


def operation(plan, values, platform):
    source = _operation(plan, values, platform)
    if platform == "commonMain" or not plan.receiver:
        return source
    start = source.find(" {")
    expression = source.find(" = ")
    if expression >= 0 and (start < 0 or expression < start):
        signature, body = (
            source[:expression],
            "return " + source[expression + 3 :].strip(),
        )
    else:
        signature, body = source[:start], source[start + 2 : source.rfind("}")]
    return (
        signature
        + " { try { "
        + body
        + " } finally { org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this) } }\n"
    )


def _operation(plan, values, platform):
    callback = kotlin_callbacks.operation(plan, values, platform)
    if callback is not None:
        return callback
    method, parameters, _inputs, result, result_type = signature(plan, values)
    if plan.consumes:
        from .kotlin_lifecycle import operation as lifecycle_operation

        return lifecycle_operation(plan, values, platform)
    if len(plan.outputs) > 1:
        from .kotlin_outputs import operation as output_operation

        return output_operation(plan, values, platform)
    if plan.view:
        return view_operation(plan, values, platform)
    if (plan.result and plan.result.kind == "handle") or plan.owned_outputs:
        return owned_operation(plan, values, platform)
    if not plan.completion:
        return immediate(plan, values, platform)
    params = ", ".join(
        f"{n}: {t}"
        + (" = null" if platform == "commonMain" and t.endswith("?") else "")
        for n, t in parameters
    )
    if platform == "commonMain":
        return f"  public fun {method}({params}): Deferred<{result_type}>\n"
    arguments = call_arguments(plan, values, platform) + ["completion"]
    prefix = {
        "jvmMain": "MapLibreNativeC.",
        "androidMain": "MaplibreNativeC.",
        "nativeMain": "",
    }[platform]
    call = prefix + plan.name + "(" + ", ".join(arguments) + ")"
    call = (
        "Arena.ofConfined().use { arena -> " + call + " }"
        if platform == "jvmMain"
        else "memScoped { val arena = this; " + call + " }"
        if platform == "nativeMain"
        else "PointerScope().use { arena -> " + call + " }"
    )
    bridge = (
        "CompletionBridge.command"
        if plan.execution == "command"
        else "CompletionBridge.unit"
        if result is None
        else f"CompletionBridge.submit({result_decode(result, values, platform)},"
    )
    expression = (
        bridge
        + " { completion -> "
        + call
        + " }"
        + (")" if result is not None and plan.execution != "command" else "")
    )
    ensure = (
        ("    NativeAccess.ensureLoaded()\n" if platform != "nativeMain" else "")
        + "    "
        + admission(plan)
        + "\n"
    )
    return f"  public actual fun {method}({params}): Deferred<{result_type}> {{\n{ensure}    return {expression}\n  }}\n"


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


def immediate(plan, values, platform):
    method, parameters, _inputs, result, result_type = signature(plan, values)
    params = ", ".join(
        f"{n}: {t}"
        + (" = null" if platform == "commonMain" and t.endswith("?") else "")
        for n, t in parameters
    )
    if platform == "commonMain":
        return f"  public fun {method}({params}): {result_type}\n"
    args = call_arguments(plan, values, platform)
    setup = []
    decoded = "Unit"
    if plan.outputs:
        output = result
        if output.kind == "record" or (
            output.kind == "buffer" and output.buffer_form == "view"
        ):
            native = output.native
            allocate = (
                f"{native}.allocate(arena)"
                if platform == "jvmMain"
                else f"arena.alloc<{native}>()"
                if platform == "nativeMain"
                else f"MaplibreNativeC.{native}()"
            )
            setup.append("val output = " + allocate)
            if any(f.role == "size" for f in output.fields):
                setup.append(
                    f"{native}.size(output, {native}.sizeof().toInt())"
                    if platform == "jvmMain"
                    else f"output.size = sizeOf<{native}>().toUInt()"
                    if platform == "nativeMain"
                    else "output.size(output.sizeof())"
                )
            args.append("output.ptr" if platform == "nativeMain" else "output")
            decoded = values.cast_public(output, "output", platform)
        elif output.kind in {"scalar", "enum"}:
            typ, layout = values.scalar(output)
            var = {
                "Boolean": "BooleanVar",
                "Double": "DoubleVar",
                "Float": "FloatVar",
                "UByte": "UByteVar",
                "Byte": "ByteVar",
                "UShort": "UShortVar",
                "Short": "ShortVar",
                "UInt": "UIntVar",
                "Int": "IntVar",
                "ULong": "ULongVar",
                "Long": "LongVar",
            }[typ]
            pointer = {
                "BOOLEAN": "BoolPointer",
                "DOUBLE": "DoublePointer",
                "FLOAT": "FloatPointer",
                "BYTE": "BytePointer",
                "SHORT": "ShortPointer",
                "INT": "IntPointer",
                "LONG": "LongPointer",
            }[layout]
            if (output.scalar_carrier or output.ctype.spelling) == "size_t":
                pointer = "SizeTPointer"
                var = "size_tVar"
            allocate = (
                f"arena.allocate(ValueLayout.JAVA_{layout})"
                if platform == "jvmMain"
                else f"arena.alloc<{var}>()"
                if platform == "nativeMain"
                else f"{pointer}(1L)"
            )
            setup.append("val output = " + allocate)
            args.append("output.ptr" if platform == "nativeMain" else "output")
            read = (
                f"output.get(ValueLayout.JAVA_{layout}, 0)"
                if platform == "jvmMain"
                else "output.value"
                if platform == "nativeMain"
                else "output.get()"
            )
            decoded = values.cast_public(output, read, platform)
        else:
            raise Unsupported(
                "immediate output requires scalar or copied record storage"
            )
    prefix = {
        "jvmMain": "MapLibreNativeC.",
        "androidMain": "MaplibreNativeC.",
        "nativeMain": "",
    }[platform]
    if (
        not plan.outputs
        and result
        and result.kind == "record"
        and platform == "jvmMain"
    ):
        args.insert(0, "arena")
    call = prefix + plan.name + "(" + ", ".join(args) + ")"
    status = (
        plan.function.return_type.declaration == "mln_status"
        or plan.function.return_type.spelling == "mln_status"
    )
    if status:
        setup.append("BindingStatus.check(" + call + ")")
    elif result:
        setup.append("val nativeResult = " + call)
        decoded = (
            "nativeResult.useContents { "
            + values.cast_public(result, "this", platform)
            + " }"
            if platform == "nativeMain" and result.kind == "record"
            else values.cast_public(result, "nativeResult", platform)
        )
    else:
        setup.append(call)
    setup.append(decoded)
    arena = (
        "Arena.ofConfined().use { arena -> "
        if platform == "jvmMain"
        else "memScoped { val arena = this; "
        if platform == "nativeMain"
        else "PointerScope().use { arena -> "
    )
    ensure = (
        ("    NativeAccess.ensureLoaded()\n" if platform != "nativeMain" else "")
        + "    "
        + admission(plan)
        + "\n"
    )
    expression = arena + "\n      " + "\n      ".join(setup) + "\n    }"
    if needs_read(plan):
        expression = (
            "bindingRead"
            + name(receiver_value(plan).native)
            + " { "
            + expression
            + " }"
        )
    return f"  public actual fun {method}({params}): {result_type} {{\n{ensure}    return {expression}\n  }}\n"


def owned_raw(platform):
    return (
        "NativeAccess.completionValue(result, ValueLayout.JAVA_LONG.byteSize()).get(ValueLayout.JAVA_LONG, 0)"
        if platform == "jvmMain"
        else "result.pointed.value!!.reinterpret<ULongVar>().pointed.value"
        if platform == "nativeMain"
        else "LongPointer(result.value()).get()"
    )


def owned_operation(plan, values, platform):
    method, parameters, inputs, result, result_type = signature(plan, values)
    immediate_owner = bool(plan.owned_outputs)
    handle = plan.owned_outputs[0].handle if immediate_owner else result.handle
    if handle.native not in values.bound.public_handles:
        raise Unsupported("owned result needs an adoption boundary")
    family = name(handle.native)
    factory = family[0].lower() + family[1:]
    parent = None
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
        parent = (
            "this@Generated"
            + name(receiver_value(plan).native)
            + "Operations as "
            + values.public(parent_param.value)
            if parent_param.name == plan.receiver
            else parameter_name(parent_param.name)
        )
    params = ", ".join(
        f"{n}: {t}"
        + (" = null" if platform == "commonMain" and t.endswith("?") else "")
        for n, t in parameters
    )
    attachment = immediate_owner and bool(plan.completion)
    if attachment:
        returns = family + "Attachment"
        values.attachments[returns] = (
            identifier(plan.owned_outputs[0].parameter.removeprefix("out_")),
            result,
        )
    else:
        returns = f"Deferred<{result_type}>" if plan.completion else result_type
    if platform == "commonMain":
        return f"  public fun {method}({params}): {returns}\n"
    prefix = (
        "MapLibreNativeC."
        if platform == "jvmMain"
        else "MaplibreNativeC."
        if platform == "androidMain"
        else ""
    )

    def adopt(raw):
        return (
            owner_name(handle.native)
            + "("
            + raw
            + (", " + parent if parent else "")
            + ")"
        )

    def dispose(raw):
        return (
            "GeneratedOwnerDisposal."
            + factory
            + "("
            + raw
            + (".toLong()" if platform == "nativeMain" else "")
            + ")"
        )

    args = call_arguments(plan, values, platform)
    arena = (
        "Arena.ofConfined().use { arena -> "
        if platform == "jvmMain"
        else "memScoped { val arena = this; "
        if platform == "nativeMain"
        else "PointerScope().use { arena -> "
    )
    registered = any(values.needs_registration(parameter.value) for parameter in inputs)
    if plan.completion and not immediate_owner:
        args.append("completion")
        raw = owned_raw(platform)
        # A late owner the host never saw takes the any-thread disposal path.
        drop = "it." + disposal_method(handle) + "()"
        expression = (
            "CompletionBridge.submitOwned({ result -> "
            + adopt(raw)
            + " }, { "
            + drop
            + " }, { result -> "
            + dispose(raw)
            + " }, { completion -> "
            + arena
            + prefix
            + plan.name
            + "("
            + ", ".join(args)
            + ") } })"
        )
    else:
        allocate = (
            "arena.allocate(ValueLayout.JAVA_LONG)"
            if platform == "jvmMain"
            else "arena.alloc<ULongVar>().also { it.value = 0uL }"
            if platform == "nativeMain"
            else "LongPointer(1L).put(0L)"
        )
        raw = (
            "output.get(ValueLayout.JAVA_LONG, 0)"
            if platform == "jvmMain"
            else "output.value"
            if platform == "nativeMain"
            else "output.get()"
        )
        args.append("output.ptr" if platform == "nativeMain" else "output")
        if attachment:
            args.append("completion")
        native = prefix + plan.name + "(" + ", ".join(args) + ")"
        call = (
            "val ready = CompletionBridge.unitChecked { completion -> "
            + native
            + " }; "
            if attachment
            else "BindingStatus.check(" + native + "); "
        )
        wrapped = (
            "adoptOwned("
            + raw
            + ", { "
            + dispose("it")
            + " }, { "
            + adopt("it")
            + " })"
        )
        if registered:
            final = returns + "(owner, ready)" if attachment else "owner"
            disposer = disposal_method(handle)
            wrapped = (
                "run { val owner = "
                + wrapped
                + "; try { registrations.accept(owner.bindingCallbacks); "
                + final
                + " } catch (failure: Throwable) { try { owner."
                + disposer
                + "() } catch (cleanup: Throwable) { failure.addSuppressed(cleanup) }; throw failure } }"
            )
        elif attachment:
            wrapped = returns + "(" + wrapped + ", ready)"
        expression = arena + "val output = " + allocate + "; " + call + wrapped + " }"
    if registered:
        expression = (
            "run { val registrations = CallbackRegistrationScope(); try { "
            + expression
            + " } finally { registrations.close() } }"
        )
    ensure = (
        ("NativeAccess.ensureLoaded(); " if platform != "nativeMain" else "")
        + admission(plan)
        + "; "
    )
    if platform == "nativeMain" and not plan.receiver:
        ensure = "org.maplibre.nativeffi.Maplibre.loadNativeLibrary(); " + ensure
    return f"  public actual fun {method}({params}): {returns} {{ {ensure}return {expression} }}\n"


def disposal_method(handle):
    return identifier(handle.dispose.removeprefix(handle.native + "_"))


def view_operation(plan, values, platform):
    method, _parameters, _inputs, result, result_type = signature(plan, values)
    if not plan.view.owner.view_begin or not plan.view.owner.view_end:
        raise Unsupported("borrowed view requires a native scope gate")
    method = "with" + method[0].upper() + method[1:]
    if platform == "commonMain":
        return f"  public fun <T> {method}(block: ({result_type}) -> T): T\n"
    native = result.native
    prefix = (
        "MapLibreNativeC."
        if platform == "jvmMain"
        else "MaplibreNativeC."
        if platform == "androidMain"
        else ""
    )
    allocate = (
        f"{native}.allocate(arena)"
        if platform == "jvmMain"
        else f"arena.alloc<{native}>()"
        if platform == "nativeMain"
        else f"MaplibreNativeC.{native}()"
    )
    pointer = "output.ptr" if platform == "nativeMain" else "output"
    size = (
        f"{native}.size(output, {native}.sizeof().toInt())"
        if platform == "jvmMain"
        else f"output.size = sizeOf<{native}>().toUInt()"
        if platform == "nativeMain"
        else "output.size(output.sizeof())"
    )
    token = (
        "arena.allocate(ValueLayout.ADDRESS)"
        if platform == "jvmMain"
        else "arena.alloc<COpaquePointerVar>()"
        if platform == "nativeMain"
        else "PointerPointer<Pointer>(1L)"
    )
    token_pointer = "token.ptr" if platform == "nativeMain" else "token"
    token_value = (
        "token.get(ValueLayout.ADDRESS, 0)"
        if platform == "jvmMain"
        else "token.value"
        if platform == "nativeMain"
        else "token.get(Pointer::class.java, 0)"
    )
    arena = (
        "Arena.ofConfined().use { arena -> "
        if platform == "jvmMain"
        else "memScoped { val arena = this; "
        if platform == "nativeMain"
        else "PointerScope().use { arena -> "
    )
    begin = (
        prefix
        + plan.view.owner.view_begin
        + "("
        + receiver_handle(plan)
        + ", "
        + token_pointer
        + ")"
    )
    end = prefix + plan.view.owner.view_end + "(" + token_value + ")"
    return f"  public actual fun <T> {method}(block: ({result_type}) -> T): T = {arena}\n    {admission(plan)}\n    val scope = org.maplibre.nativeffi.internal.lifecycle.ViewScope()\n    val token = {token}\n    BindingStatus.check({begin})\n    try {{\n      val output = {allocate}\n      {size}\n      BindingStatus.check({prefix}{plan.name}({receiver_handle(plan)}, {pointer}))\n      block(GeneratedValues.read{name(native)}(output, scope))\n    }} finally {{ scope.close(); {end} }}\n  }}\n"
