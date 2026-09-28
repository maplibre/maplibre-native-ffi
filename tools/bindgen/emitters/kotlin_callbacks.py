"""Lower retained callbacks and scoped responses for Kotlin native boundaries."""

from types import SimpleNamespace

from .kotlin_values import Unsupported, identifier, name

ROOTS = "org.maplibre.nativeffi.internal.callback.CallbackRoots"
SCOPE = "org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope"
ADMISSION = "org.maplibre.nativeffi.internal.callback.CallbackAdmission"


def status_callback(callback):
    return callback.result.native == "mln_status"


def check(value, values):
    if not (value.kind == "callback" or value.registration or value.response):
        return False
    if value.native in values.used:
        return True
    values.used[value.native] = value
    if value.kind == "callback":
        callback = values.bound.callbacks[value.native]
        if not callback.context:
            raise Unsupported("callback needs its context parameter")
        for parameter in callback.parameters:
            if parameter.name != callback.context:
                values.check(parameter.value)
        if callback.result.native != "void" and not status_callback(callback):
            values.check(callback.result)
    elif value.registration:
        for field in values.fields(value):
            values.check(field.value)
    return True


def public(value, values):
    if not check(value, values):
        return None
    return name(value.native) + ("?" if value.nullable or value.optional else "")


def common(values):
    result = []
    for value in list(values.used.values()):
        public = name(value.native)
        if value.kind == "callback":
            callback = values.bound.callbacks[value.native]
            parameters = ", ".join(
                f"{identifier(p.name)}: {values.public(p.value)}"
                for p in callback.parameters
                if p.name != callback.context
            )
            returns = (
                "Unit"
                if callback.result.native == "void" or status_callback(callback)
                else values.public(callback.result)
            )
            result.append(f"public typealias {public} = ({parameters}) -> {returns}")
        elif value.response:
            result.append(
                f"public class {public} internal constructor(internal val bindingAddress: Long, internal val bindingScope: org.maplibre.nativeffi.internal.callback.CallbackScope)"
            )
        elif value.registration:
            parameters = []
            for member, typ, children, group in values.members(value):
                default = (
                    "null" if typ.endswith("?") else values.default(children[0].value)
                )
                parameters.append(
                    f"  public val {member}: {typ}"
                    + (f" = {default}" if default else "")
                )
            result.append(
                f"public data class {public}(\n" + ",\n".join(parameters) + "\n)"
            )
    for callback, release in getattr(values, "direct_callbacks", {}).values():
        if release:
            result.append(
                f"internal class Generated{name(callback.native)}Registration(val callback: {values.public(callback)})"
            )
    return "\n".join(result) + "\n"


def cast_native(value, expression, values, platform):
    if value.registration:
        return f"GeneratedCallbacks.prepare{name(value.native)}(arena, {expression}, registrations)"
    if value.response:
        address = f"{expression}.bindingAddress.also {{ {expression}.bindingScope.ensureActive() }}"
        if platform == "jvmMain":
            return f"MemorySegment.ofAddress({address}).reinterpret({value.native}.sizeof())"
        if platform == "androidMain":
            return f"MaplibreNativeC.{value.native}(org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer({address}))"
        return f"({address}).toCPointer<{value.native}>()!!"
    return None


def cast_public(value, expression, values, platform, scope=None):
    if value.registration:
        if any(
            not field.value.nullable
            for field in value.fields
            if field.name in value.registration.callbacks
        ):
            raise Unsupported(
                "callback descriptor output has no host callback identity"
            )
        return f"GeneratedCallbacks.read{name(value.native)}({expression})"
    if value.response:
        address = (
            f"{expression}.address()"
            if platform == "jvmMain"
            else f"{expression}.address()"
            if platform == "androidMain"
            else f"{expression}.ptr.rawValue.toLong()"
        )
        return f"{name(value.native)}({address}, callbackScope)"
    return None


def native_type(value, values, platform):
    if value.kind == "reference":
        child = value.element
        return (
            "MemorySegment"
            if platform == "jvmMain"
            else f"MaplibreNativeC.{child.native}?"
            if platform == "androidMain"
            else f"CPointer<{child.native}>?"
        )
    if value.kind == "record":
        return (
            "MemorySegment"
            if platform == "jvmMain"
            else f"MaplibreNativeC.{value.native}?"
            if platform == "androidMain"
            else f"CValue<{value.native}>"
        )
    if value.kind in {"buffer", "native_pointer"}:
        return (
            "MemorySegment"
            if platform == "jvmMain"
            else "BytePointer?"
            if value.kind == "buffer" and platform == "androidMain"
            else "Pointer?"
            if platform == "androidMain"
            else "CPointer<ByteVar>?"
            if value.kind == "buffer"
            else "COpaquePointer?"
        )
    if value.kind == "handle":
        return "ULong" if platform == "nativeMain" else "Long"
    if value.native == "void":
        return "Unit"
    typ = values.scalar(value)[0]
    return typ if platform == "nativeMain" else typ.removeprefix("U")


def token_expression(parameter, platform):
    local = identifier(parameter)
    return (
        f"{local}.address()"
        if platform == "jvmMain"
        else f"{local}?.address() ?: 0L"
        if platform == "androidMain"
        else f"{local}?.rawValue?.toLong() ?: 0L"
    )


def literal(symbol, callback, values, platform):
    if callback.result.native == "void":
        return "Unit"
    if symbol is None:
        return (
            "Unit"
            if callback.result.native == "void"
            else "0u"
            if native_type(callback.result, values, platform) == "UInt"
            else "0"
        )
    number = next(
        (
            number
            for value in values.bound.values.values()
            for key, number in value.enum_values
            if key == symbol
        ),
        None,
    )
    if number is None:
        try:
            number = int(symbol, 0)
        except ValueError as error:
            raise Unsupported(
                f"callback failure {symbol} needs a numeric constant"
            ) from error
    typ = native_type(callback.result, values, platform)
    if typ == "UInt":
        return f"{number & 0xFFFFFFFF}u"
    return str(number if number <= 0x7FFFFFFF else number - (1 << 32))


def stub(native, function, values, platform):
    callback = values.bound.callbacks[native]
    names = ", ".join(identifier(p.name) for p in callback.parameters)
    if platform == "jvmMain":
        return f"{native}.allocate({native}.Function {{ {names} -> {function}({names}) }}, Arena.global())"
    if platform == "nativeMain":
        return f"staticCFunction(::{function})"
    parameters = ", ".join(
        f"{identifier(p.name)}: {native_type(p.value, values, platform)}"
        for p in callback.parameters
    )
    typ = native_type(callback.result, values, platform)
    return f"object : MaplibreNativeC.{native}() {{ override fun call({parameters}): {typ} = {function}({names}) }}.apply {{ retainReference<Pointer>() }}"


def callback_thunk(value, field, values, platform):
    callback = values.bound.callbacks[field.value.native]
    function = "generated" + name(value.native) + name(field.name)
    parameters = ", ".join(
        f"{identifier(p.name)}: {native_type(p.value, values, platform)}"
        for p in callback.parameters
    )
    returns = native_type(callback.result, values, platform)
    failure = literal(callback.failure, callback, values, platform)
    token = token_expression(callback.context, platform)
    owner = "null"
    if callback.reentry_policy and callback.reentry_policy.owner_parameter:
        parameter = next(
            p
            for p in callback.parameters
            if p.name == callback.reentry_policy.owner_parameter
        )
        owner = (
            identifier(parameter.name) + ".toLong()"
            if parameter.value.kind == "handle"
            else token_expression(parameter.name, platform)
        )
    allowed = (
        "null"
        if callback.reentry == "allow"
        else "setOf("
        + ", ".join(
            '"' + operation + '"'
            for operation in (
                callback.reentry_policy.operations if callback.reentry_policy else ()
            )
        )
        + ")"
    )
    lines = [
        f"private fun {function}({parameters}): {returns} {{",
        "  try {",
        f"    val root = {ROOTS}.get({token}) ?: return {failure}",
        f"    val value = root.value as {name(value.native)}",
        f"    val invoke = value.{identifier(field.name)}"
        + (f" ?: return {failure}" if field.value.nullable else ""),
        f"    val callbackScope = {ADMISSION}.scope({owner}, {allowed})",
        "    try {",
    ]
    arguments = []
    decision = callback.decision
    for parameter in callback.parameters:
        if parameter.name == callback.context:
            continue
        local = identifier(parameter.name)
        if decision and parameter.name == decision.parameter:
            carrier = (
                local
                if platform == "androidMain"
                else f"org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest({local})"
                if platform == "jvmMain"
                else f"org.maplibre.nativeffi.internal.lifecycle.resourceRequestHandle({local})"
            )
            lines.append(
                f"      val requestOwner = org.maplibre.nativeffi.resource.ResourceRequestHandle({carrier})"
            )
            arguments.append("requestOwner")
        elif parameter.value.kind == "record" and platform == "nativeMain":
            arguments.append(
                f"{local}.useContents {{ {values.cast_public(parameter.value, 'this', platform)} }}"
            )
        else:
            expression = local + (
                "!!"
                if platform == "androidMain"
                and parameter.value.kind in {"record", "reference", "buffer"}
                else ""
            )
            arguments.append(values.cast_public(parameter.value, expression, platform))
    invocation = f"invoke({', '.join(arguments)})"
    if decision:
        lines += [
            f"      return try {{ requestOwner.finishBindingDecision({invocation}.rawValue.toUInt()) }} catch (_: Throwable) {{ requestOwner.finishBindingException() }} finally {{ org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(requestOwner) }}"
            + (".toInt()" if platform != "nativeMain" else "")
        ]
    elif callback.result.native == "void":
        lines += [f"      {invocation}", "      return Unit"]
    elif status_callback(callback):
        lines += [f"      {invocation}", "      return 0"]
    else:
        lines += [
            "      return " + values.cast_native(callback.result, invocation, platform)
        ]
    lines += [
        "    } finally { callbackScope.close() }",
        f"  }} catch (_: Throwable) {{ return {failure} }}",
        "}",
    ]
    return function, "\n".join(lines)


def conversions(values, platform):
    descriptors = [v for v in list(values.used.values()) if v.registration]
    if not descriptors:
        return direct_conversions(values, platform)
    methods, thunks, stubs = [], [], []
    arena_type = (
        "Arena"
        if platform == "jvmMain"
        else "PointerScope"
        if platform == "androidMain"
        else "MemScope"
    )
    for value in descriptors:
        public = name(value.native)
        native = value.native
        typ = (
            "MemorySegment"
            if platform == "jvmMain"
            else f"MaplibreNativeC.{native}"
            if platform == "androidMain"
            else f"CPointer<{native}>"
        )
        source_typ = typ if platform != "nativeMain" else native
        initialize = (
            f"MapLibreNativeC.{value.default}(arena)"
            if value.default and platform == "jvmMain"
            else f"{native}.allocate(arena)"
            if platform == "jvmMain"
            else f"MaplibreNativeC.{value.default}()"
            if value.default and platform == "androidMain"
            else f"MaplibreNativeC.{native}()"
            if platform == "androidMain"
            else f"arena.alloc<{native}>().ptr"
        )
        body = [
            f"  fun prepare{public}(arena: {arena_type}, value: {public}, registrations: {SCOPE}): {typ} {{",
            f"    val result = {initialize}",
        ]
        base = "result.pointed" if platform == "nativeMain" else "result"
        if platform == "nativeMain":
            body.append(
                f"    result.reinterpret<ByteVar>().let {{ bytes -> repeat(sizeOf<{native}>().toInt()) {{ bytes[it] = 0 }} }}"
            )
            if value.default:
                body.append(f"    {value.default}().place(result)")
        size = (
            f"{native}.sizeof().toInt()"
            if platform == "jvmMain"
            else "result.sizeof()"
            if platform == "androidMain"
            else f"sizeOf<{native}>().toUInt()"
        )
        if any(f.role == "size" for f in value.fields):
            body.append("    " + values.assign(value, "size", base, size, platform))
        callbacks = [f for f in value.fields if f.name in value.registration.callbacks]
        for member, _, children, group in values.members(value):
            if group:
                raise Unsupported(
                    "callback descriptor presence group needs recursive preparation"
                )
            field = children[0]
            if field in callbacks:
                continue
            expression = "value." + member
            encoded = values.cast_native(
                field.value,
                expression + ("!!" if field.presence and field.presence.mask else ""),
                platform,
            )
            if platform == "nativeMain" and field.value.kind == "record":
                assignment = (
                    f"{encoded}.pointed.readValue().place({base}.{field.name}.ptr)"
                )
            else:
                assignment = values.assign(value, field.name, base, encoded, platform)
            if field.presence and field.presence.mask:
                assignment = f"if ({expression} != null) {{ {values.mark(value, field.presence, base, platform)}; {assignment} }}"
            body.append("    " + assignment)
        if all(f.value.nullable for f in callbacks):
            body.append(
                "    if ("
                + " && ".join(f"value.{identifier(f.name)} == null" for f in callbacks)
                + ") return result"
            )
        body.append("    val token = registrations.register(value)")
        user_data = (
            "MemorySegment.ofAddress(token)"
            if platform == "jvmMain"
            else "org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(token)"
            if platform == "androidMain"
            else "token.toCPointer<ByteVar>()"
        )
        body.append(
            "    "
            + values.assign(
                value, value.registration.user_data, base, user_data, platform
            )
        )
        for field in callbacks:
            function, thunk = callback_thunk(value, field, values, platform)
            thunks.append(thunk)
            property_name = function + "Stub"
            stubs.append(
                f"  private val {property_name} = {stub(field.value.native, function, values, platform)}"
            )
            pointer = property_name
            if field.value.nullable:
                null = "MemorySegment.NULL" if platform == "jvmMain" else "null"
                pointer = (
                    f"if (value.{identifier(field.name)} == null) {null} else {pointer}"
                )
            body.append(
                "    " + values.assign(value, field.name, base, pointer, platform)
            )
        release_field = next(
            f for f in value.fields if f.name == value.registration.release
        )
        release_callback = values.bound.callbacks[release_field.value.native]
        release_function = "generatedRelease" + public
        parameter = identifier(release_callback.context)
        parameter_type = native_type(
            release_callback.parameters[0].value, values, platform
        )
        thunks.append(
            f"private fun {release_function}({parameter}: {parameter_type}) {{ try {{ {ROOTS}.release({token_expression(release_callback.context, platform)}) }} catch (_: Throwable) {{}} }}"
        )
        stubs.append(
            f"  private val {release_function}Stub = {stub(release_field.value.native, release_function, values, platform)}"
        )
        body.append(
            "    "
            + values.assign(
                value,
                value.registration.release,
                base,
                release_function + "Stub",
                platform,
            )
        )
        body += ["    return result", "  }"]
        methods.append("\n".join(body))
        read = [f"  fun read{public}(source: {source_typ}): {public} {{"]
        if platform == "androidMain":
            read.append(
                f'    check(!org.maplibre.nativeffi.internal.javacpp.GeneratedCallbackBridge.has{public}Callbacks(source)) {{ "cannot copy an installed callback descriptor" }}'
            )
        else:
            for field in callbacks:
                pointer = values.field(value, field.name, "source", platform)
                present = (
                    f"{pointer}.address() != 0L"
                    if platform == "jvmMain"
                    else f"{pointer} != null"
                )
                read.append(
                    f'    check(!({present})) {{ "cannot copy an installed callback descriptor" }}'
                )
        if not all(f.value.nullable for f in callbacks):
            methods.append(
                f'  fun read{public}(source: {source_typ}): {public} = error("cannot copy an installed callback descriptor")'
            )
            continue
        args = []
        for member, _, children, group in values.members(value):
            field = children[0]
            expression = (
                "null"
                if field in callbacks
                else values.cast_public(
                    field.value,
                    values.field(value, field.name, "source", platform),
                    platform,
                )
            )
            if field.presence and field.presence.mask:
                expression = f"if ({values.condition(value, field.presence, 'source', platform)}) {expression} else null"
            args.append(f"{member} = {expression}")
        read += [f"    return {public}({', '.join(args)})", "  }"]
        methods.append("\n".join(read))
    annotation = (
        "@OptIn(ExperimentalForeignApi::class)\n" if platform == "nativeMain" else ""
    )
    return (
        annotation
        + "internal object GeneratedCallbacks {\n"
        + "\n".join(stubs + methods)
        + "\n}\n"
        + "\n".join(annotation + thunk for thunk in thunks)
        + "\n"
        + direct_conversions(values, platform)
    )


def operation(plan, values, platform):
    from .kotlin_ir import admission, call_arguments, parameter_name

    if plan.direct_registrations:
        return direct_operation(plan, values, platform)
    decision = next(
        (
            c.decision
            for c in values.bound.callbacks.values()
            if c.decision and plan.name in (c.decision.complete, c.decision.cancelled)
        ),
        None,
    )
    if plan.scoped_receiver or decision:
        lengths = {p.value.length for p in plan.inputs if p.value.kind == "buffer"}
        inputs = [
            p for p in plan.inputs if p.name != plan.receiver and p.name not in lengths
        ]
        params = ", ".join(
            f"{parameter_name(p.name)}: {values.public(p.value)}" for p in inputs
        )
        method = identifier(plan.name.removeprefix("mln_"))
        returns = "Boolean" if decision and plan.name == decision.cancelled else "Unit"
        if platform == "commonMain":
            return f"  public fun {method}({params}): {returns}\n"
        prefix = (
            "MapLibreNativeC."
            if platform == "jvmMain"
            else "MaplibreNativeC."
            if platform == "androidMain"
            else ""
        )
        arena = (
            "Arena.ofConfined().use { arena ->"
            if platform == "jvmMain"
            else "PointerScope().use { arena ->"
            if platform == "androidMain"
            else "memScoped { val arena = this;"
        )
        arguments = call_arguments(plan, values, platform)
        if decision:
            arguments[0] = "raw"
            hook = (
                "bindingCompleteResourceRequestHandle"
                if plan.name == decision.complete
                else "bindingReadResourceRequestHandle"
            )
            start = f"{hook} {{ raw -> {arena}"
            if returns == "Boolean":
                output = (
                    "arena.allocate(ValueLayout.JAVA_BOOLEAN)"
                    if platform == "jvmMain"
                    else "BoolPointer(1L)"
                    if platform == "androidMain"
                    else "alloc<BooleanVar>()"
                )
                arguments.append("out" if platform != "nativeMain" else "out.ptr")
                read = (
                    "out.get(ValueLayout.JAVA_BOOLEAN, 0)"
                    if platform == "jvmMain"
                    else "out.get(0)"
                    if platform == "androidMain"
                    else "out.value"
                )
                body = f"val out = {output}; BindingStatus.check({prefix}{plan.name}({', '.join(arguments)})); {read}"
            else:
                body = f"{prefix}{plan.name}({', '.join(arguments)})"
        else:
            scoped = parameter_name(plan.scoped_receiver)
            start = arena
            body = f'{scoped}.bindingScope.ensureActive(); org.maplibre.nativeffi.internal.callback.CallbackAdmission.check({scoped}.bindingAddress, "{plan.name}"); BindingStatus.check({prefix}{plan.name}({", ".join(arguments)}))'
        checks = admission(plan) + "; " if decision else ""
        loaded = (
            "NativeAccess.ensureLoaded(); "
            if platform in {"jvmMain", "androidMain"}
            else ""
        )
        ending = "} }" if decision else "}"
        return f"  public actual fun {method}({params}): {returns} {{ {loaded}{checks}return {start} {body} {ending} }}\n"
    if not plan.registrations or plan.owned_outputs:
        return None
    if not plan.completion or plan.result or plan.outputs:
        raise Unsupported(
            "callback registration needs its native admission transaction"
        )
    from .kotlin_ir import call_arguments, parameter_name

    inputs = [p for p in plan.inputs if p.name != plan.receiver]
    parameters = [(parameter_name(p.name), values.public(p.value)) for p in inputs]
    receiver = next(p for p in plan.inputs if p.name == plan.receiver)
    method = identifier(plan.name.removeprefix(receiver.value.native + "_"))
    result = "CommandCompletion" if plan.execution == "command" else "Unit"
    params = ", ".join(f"{n}: {t}" for n, t in parameters)
    if platform == "commonMain":
        return f"  public fun {method}({params}): Deferred<{result}>\n"
    arguments = call_arguments(plan, values, platform) + ["completion"]
    prefix = (
        "MapLibreNativeC."
        if platform == "jvmMain"
        else "MaplibreNativeC."
        if platform == "androidMain"
        else ""
    )
    arena = (
        "Arena.ofConfined().use { arena ->"
        if platform == "jvmMain"
        else "PointerScope().use { arena ->"
        if platform == "androidMain"
        else "memScoped { val arena = this;"
    )
    bridge = "command" if result == "CommandCompletion" else "unit"
    call = prefix + plan.name + "(" + ", ".join(arguments) + ")"
    return f'  public actual fun {method}({params}): Deferred<{result}> = {SCOPE}().use {{ registrations ->\n    {ADMISSION}.check(binding{name(receiver.value.native)}Handle().toLong(), "{plan.name}")\n    CompletionBridge.{bridge} {{ completion -> {arena}\n      val status = {call}\n      if (status == 0) registrations.accept(bindingCallbacks)\n      status\n    }} }}\n  }}\n'


def direct_conversions(values, platform):
    stubs, thunks = [], []
    for callback_value, release in getattr(values, "direct_callbacks", {}).values():
        callback = values.bound.callbacks[callback_value.native]
        public = name(callback_value.native)
        if release:
            wrapper = SimpleNamespace(
                native="mln_generated_"
                + callback_value.native.removeprefix("mln_")
                + "_registration"
            )
            field = SimpleNamespace(name="callback", value=callback_value)
            function, thunk = callback_thunk(wrapper, field, values, platform)
            thunks.append(thunk)
            stubs.append(
                f"  val {public}Stub = {stub(callback_value.native, function, values, platform)}"
            )
            release_plan = values.bound.callbacks[release.native]
            parameter = release_plan.parameters[0]
            release_function = "generatedDirectRelease" + public
            thunks.append(
                f"private fun {release_function}({identifier(parameter.name)}: {native_type(parameter.value, values, platform)}) {{ try {{ {ROOTS}.release({token_expression(parameter.name, platform)}) }} catch (_: Throwable) {{}} }}"
            )
            stubs.append(
                f"  val {public}ReleaseStub = {stub(release.native, release_function, values, platform)}"
            )
        else:
            parameter = callback.parameters[0]
            function = "generatedDirect" + public
            thunks.append(
                f"private fun {function}({identifier(parameter.name)}: {native_type(parameter.value, values, platform)}) {{ try {{ org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistry.dispatch({token_expression(parameter.name, platform)}) }} catch (_: Throwable) {{}} }}"
            )
            stubs.append(
                f"  val {public}Stub = {stub(callback_value.native, function, values, platform)}"
            )
    if not stubs:
        return ""
    annotation = (
        "@OptIn(ExperimentalForeignApi::class)\n" if platform == "nativeMain" else ""
    )
    return (
        annotation
        + "internal object GeneratedDirectCallbacks {\n"
        + "\n".join(stubs)
        + "\n}\n"
        + "\n".join(annotation + thunk for thunk in thunks)
        + "\n"
    )


def direct_operation(plan, values, platform):
    from .kotlin_ir import admission

    if len(plan.direct_registrations) != 1:
        raise Unsupported(
            "multiple direct callback registrations require separate transactions"
        )
    registration = plan.direct_registrations[0]
    callback_value = next(
        p.value for p in plan.inputs if p.name == registration.callback
    )
    callback = values.bound.callbacks[callback_value.native]
    release = next(
        (p.value for p in plan.inputs if p.name == registration.release_callback), None
    )
    values.check(callback_value)
    if not hasattr(values, "direct_callbacks"):
        values.direct_callbacks = {}
    values.direct_callbacks[callback_value.native] = (callback_value, release)
    public = name(callback_value.native)
    method = identifier(plan.name.removeprefix("mln_"))
    returns = "Boolean" if registration.accepted_unless else "Unit"
    params = f"callback: {values.public(callback_value)}"
    if platform == "commonMain":
        return f"  public fun {method}({params}): {returns}\n"
    prefix = (
        "MapLibreNativeC."
        if platform == "jvmMain"
        else "MaplibreNativeC."
        if platform == "androidMain"
        else ""
    )
    null = "MemorySegment.NULL" if platform == "jvmMain" else "null"
    token = (
        "MemorySegment.ofAddress(token)"
        if platform == "jvmMain"
        else "org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(token)"
        if platform == "androidMain"
        else "token.toCPointer<ByteVar>()"
    )
    loaded = (
        "NativeAccess.ensureLoaded(); "
        if platform in {"jvmMain", "androidMain"}
        else ""
    )
    if release:
        pointer = f"GeneratedDirectCallbacks.{public}Stub"
        releaser = f"GeneratedDirectCallbacks.{public}ReleaseStub"
        disabled = (
            f"if (callback == null) {{ BindingStatus.check({prefix}{plan.name}({null}, {null}, {null})); return }}; "
            if callback_value.nullable
            else ""
        )
        return f"  public actual fun {method}({params}) {{ {loaded}{admission(plan)}; {disabled}{SCOPE}().use {{ registrations -> val token = registrations.register(Generated{public}Registration(callback)); BindingStatus.check({prefix}{plan.name}({pointer}, {token}, {releaser})); registrations.accept(org.maplibre.nativeffi.internal.callback.CallbackOwner.global) }} }}\n"
    if (
        not registration.owner_release
        or not registration.accepted_unless
        or not callback.reentry_policy
        or not callback.reentry_policy.registration_owner
    ):
        raise Unsupported(
            "direct callback requires native release or a verified owner retirement"
        )
    arena = (
        "Arena.ofConfined().use { arena ->"
        if platform == "jvmMain"
        else "PointerScope().use { arena ->"
        if platform == "androidMain"
        else "memScoped { val arena = this;"
    )
    output = (
        "arena.allocate(ValueLayout.JAVA_BOOLEAN)"
        if platform == "jvmMain"
        else "BoolPointer(1L)"
        if platform == "androidMain"
        else "alloc<BooleanVar>()"
    )
    output_pointer = "out" if platform != "nativeMain" else "out.ptr"
    read = (
        "out.get(ValueLayout.JAVA_BOOLEAN, 0)"
        if platform == "jvmMain"
        else "out.get(0)"
        if platform == "androidMain"
        else "out.value"
    )
    allowed = (
        "setOf("
        + ", ".join('"' + op + '"' for op in callback.reentry_policy.operations)
        + ")"
    )
    invoke = f"{{ val scope = {ADMISSION}.scope(owner, {allowed}); try {{ callback() }} finally {{ scope.close() }} }}"
    call = f"{prefix}{plan.name}(raw, GeneratedDirectCallbacks.{public}Stub, {token}, {output_pointer})"
    return f"  public actual fun {method}({params}): Boolean {{ {loaded}val owner = bindingResourceRequestHandleHandle().toLong(); {admission(plan)}; return bindingRegisterResourceRequestHandleCancel({invoke}) {{ raw, token -> {arena} val out = {output}; val status = {call}; org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult(status, {read}) }} }} }}\n"


def android_bridge(values):
    """Inspect C callback fields without allocating JavaCPP FunctionPointer wrappers."""
    header = [
        "// Generated by tools/bindgen. Do not edit.",
        "#pragma once",
        "#include <maplibre_native_c.h>",
    ]
    java = [
        "// Generated by tools/bindgen. Do not edit.",
        "package org.maplibre.nativeffi.internal.javacpp;",
        "import org.bytedeco.javacpp.Pointer;",
        "import org.bytedeco.javacpp.annotation.Cast;",
        "import org.bytedeco.javacpp.annotation.Name;",
        "import org.bytedeco.javacpp.annotation.Platform;",
        "import org.bytedeco.javacpp.annotation.Properties;",
        '@Properties(inherit = MaplibreNativeCConfig.class, value = @Platform(include = "callback_bridge_generated.h"))',
        "public final class GeneratedCallbackBridge {",
        "private GeneratedCallbackBridge() {}",
    ]
    for value in values.used.values():
        if not value.registration:
            continue
        callbacks = [f for f in value.fields if f.name in value.registration.callbacks]
        if not all(f.value.nullable for f in callbacks):
            continue
        function = f"mln_android_{value.native.removeprefix('mln_')}_has_callbacks"
        present = (
            " || ".join(f"source->{f.name} != nullptr" for f in callbacks) or "false"
        )
        header.append(
            f"inline bool {function}(const {value.native}* source) {{ return {present}; }}"
        )
        java.append(
            f'@Name("{function}") public static native boolean has{name(value.native)}Callbacks(@Cast("const {value.native}*") Pointer source);'
        )
    java.append("}")
    return {
        "src/androidMain/javacpp/callback_bridge_generated.h": "\n".join(header) + "\n",
        "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/GeneratedCallbackBridge.java": "\n".join(
            java
        )
        + "\n",
    }
