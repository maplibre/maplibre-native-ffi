"""Lower retained callbacks, decisions, and scoped responses to common Kotlin.

Each callback a registration hands to native gets one upcall site: a common
`Upcalls` method that finds the registration's root and runs the host
callback, and a platform stub that native calls. The platform stub converts
only primitives, so the C side of every site has the same shape.
"""

from __future__ import annotations

from dataclasses import dataclass, replace

from ..model import CType
from ..protocol import COMPLETION
from ..semantic import public_stem
from .kotlin_values import Unsupported, identifier, name, owner_class


@dataclass(frozen=True)
class Site:
    """A C function pointer that calls one `Upcalls` method."""

    name: str
    # (Kotlin name, C type, carrier) per parameter, then the C result type.
    parameters: tuple[tuple[str, CType, str], ...]
    result: CType
    result_carrier: str
    body: str
    # The carrier literal native receives when the upcall cannot run.
    failure: str = "0"


def status_callback(callback):
    return callback.status


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
        public_name = name(value.native)
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
            result.append(
                f"public typealias {public_name} = ({parameters}) -> {returns}"
            )
        elif value.response:
            result.append(
                f"public class {public_name} internal constructor(internal val bindingAddress: Long, internal val bindingScope: org.maplibre.nativeffi.internal.callback.CallbackScope)"
            )
        elif value.registration:
            parameters = []
            for member, typ, children, _group in values.members(value):
                default = (
                    "null" if typ.endswith("?") else values.default(children[0].value)
                )
                parameters.append(
                    f"  public val {member}: {typ}"
                    + (f" = {default}" if default else "")
                )
            result.append(
                f"public data class {public_name}(\n" + ",\n".join(parameters) + "\n)"
            )
    for callback in values.direct_callbacks.values():
        result.append(
            f"internal class {registration_class(callback)}(val callback: {values.public(callback)})"
        )
    return "\n".join(result) + "\n"


def registration_class(callback):
    return f"Generated{name(callback.native)}Registration"


def argument(value, expression):
    if value.response:
        return f"{expression}.bindingAddress.also {{ {expression}.bindingScope.ensureActive() }}"
    return None


def check_decode(value):
    """Rejects callback values that native cannot hand back to Kotlin."""
    if value.registration:
        if any(
            not field.value.nullable
            for field in value.fields
            if field.name in value.registration.callbacks
        ):
            raise Unsupported(
                "callback descriptor output has no host callback identity"
            )
    elif value.response:
        raise Unsupported("a callback response is valid only during its callback")


# Upcall sites.


VOID_POINTER = CType(
    "pointer", "void *", "void *", pointee=CType("void", "void", "void")
)
VOID = CType("void", "void", "void")


def completion_result_type(bound):
    """The result pointer that the completion registration's callback receives."""
    completion = bound.values.get(COMPLETION)
    if completion is None or completion.registration is None:
        return None
    for member in completion.registration.callbacks:
        field = next(f for f in completion.fields if f.name == member)
        callback = bound.callbacks[field.value.native]
        for parameter in callback.parameters:
            if parameter.name != callback.context:
                return parameter.value.ctype
    return None


def sites(values):
    """The registry of upcall sites, starting with the runtime's own."""
    if values.sites is None:
        values.sites = {}
        result = completion_result_type(values.bound) or VOID_POINTER
        context = ("userData", VOID_POINTER, "Long")
        for site in (
            Site(
                "completion",
                (context, ("result", result, "Long")),
                VOID,
                "Unit",
                'contain("mln_completion_callback", Unit) { CompletionBridge.complete(userData, result) }',
            ),
            Site(
                "completionRelease",
                (context,),
                VOID,
                "Unit",
                'contain("mln_completion_release", Unit) { CompletionBridge.release(userData) }',
            ),
            Site(
                "releaseRoot",
                (context,),
                VOID,
                "Unit",
                'contain("release_user_data", Unit) { CallbackRoots.release(userData) }',
            ),
        ):
            values.sites[site.name] = site
    return values.sites


def carrier(value, values):
    """The native-call carrier of a callback parameter or result."""
    from .kotlin_abi import Record

    if value.native == "void":
        return "Unit"
    if value.kind in {"reference", "buffer", "native_pointer", "callback"}:
        return "Long"
    if value.kind == "handle":
        return "Long"
    kind = values.abi.classify(value.ctype)
    if isinstance(kind, Record):
        return "Long"
    return kind.carrier


def failure_literal(callback, values):
    if callback.result.native == "void":
        return "Unit"
    symbol = callback.failure
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
            number = int(symbol, 0) if symbol else 0
        except ValueError as error:
            raise Unsupported(
                f"callback failure {symbol} needs a numeric constant"
            ) from error
    typ = carrier(callback.result, values)
    if typ == "Long":
        return f"{number}L"
    return str(number if number <= 0x7FFFFFFF else number - (1 << 32))


def callback_argument(parameter, values):
    """The public value of one callback argument, from its carrier local."""
    from .kotlin_operations import PUBLIC_FROM_CARRIER

    value = parameter.value
    local = identifier(parameter.name)
    if value.kind == "reference":
        if value.element.response:
            return f"{name(value.element.native)}({local}, scope)"
        decoded = values.decode(value.element, local)
        return f"if ({local} == 0L) null else {decoded}" if value.nullable else decoded
    if value.kind == "buffer" and value.length == "nul":
        return (
            f"readCStringOrNull({local})" if value.nullable else f"readCString({local})"
        )
    if value.kind == "record":
        return values.decode(value, local)
    if value.kind == "enum":
        _suffix, typ = values.accessor(value)
        return f"{name(value.native)}({local}{PUBLIC_FROM_CARRIER[typ]})"
    if value.kind == "scalar":
        return local + PUBLIC_FROM_CARRIER[values.scalar(value)[0]]
    if value.kind == "native_pointer":
        return f"NativePointer.ofAddress({local})"
    raise Unsupported("callback argument requires another Kotlin rule")


def callback_site(site_name, callback_value, member, root_type, values):
    """Register the upcall site that runs [member] of the root's [root_type] value."""
    callback = values.bound.callbacks[callback_value.native]
    owner = None
    if callback.reentry_policy and callback.reentry_policy.registration_owner:
        owner = "{ it.owner }"
    elif callback.reentry_policy and callback.reentry_policy.owner_parameter:
        owner = "{ " + identifier(callback.reentry_policy.owner_parameter) + " }"
    allowed = (
        "null"
        if callback.reentry == "allow"
        else "setOf("
        + ", ".join(
            f'"{operation}"'
            for operation in (
                callback.reentry_policy.operations if callback.reentry_policy else ()
            )
        )
        + ")"
    )
    failure = failure_literal(callback, values)
    result = carrier(callback.result, values)
    parameters = []
    for parameter in callback.parameters:
        parameters.append(
            (
                identifier(parameter.name),
                parameter.value.ctype,
                carrier(parameter.value, values),
            )
        )
    arguments = []
    lines = []
    decision = callback.decision
    for parameter in callback.parameters:
        if parameter.name == callback.context:
            continue
        if decision and parameter.name == decision.parameter:
            lines.append(
                f"val decisionOwner = {owner_class(decision.handle)}({identifier(parameter.name)})"
            )
            arguments.append("decisionOwner")
        else:
            arguments.append(callback_argument(parameter, values))
    invoke = f"value.{member}"
    if callback_value.nullable:
        lines.insert(0, f"val invoke = {invoke} ?: return@upcall {failure}")
        invoke = "invoke"
    invocation = f"{invoke}({', '.join(arguments)})"
    if decision:
        lines.append(
            f'decisionOwner.binding.decide(decisionOwner, "{callback.native}") {{ {invocation}.rawValue }}'
            + (".toInt()" if result == "Int" else "")
        )
    elif callback.result.native == "void":
        lines.append(invocation)
    elif status_callback(callback):
        lines.append(invocation)
        lines.append("0")
    else:
        lines.append(callback_result(callback.result, invocation, values))
    context = identifier(callback.context)
    owner_argument = f", {owner}" if owner else ""
    body = (
        f'upcall<{root_type}, {result}>("{callback.native}", {context}, {failure}, {allowed}{owner_argument}) '
        f"{{ value, scope -> {'; '.join(lines)} }}"
    )
    sites(values)[site_name] = Site(
        site_name,
        tuple(parameters),
        callback.result.ctype,
        result,
        body,
        failure,
    )
    return site_name


def callback_result(value, expression, values):
    from .kotlin_values import CARRIER

    if value.kind == "enum":
        _suffix, typ = values.accessor(value)
        return f"{expression}.rawValue{CARRIER[typ]}"
    if value.kind == "scalar":
        return expression + CARRIER[values.scalar(value)[0]]
    raise Unsupported("callback result requires a scalar")


def upcalls(values):
    """The common `Upcalls` methods and the `UpcallStubs` they back."""
    entries = []
    for site in sites(values).values():
        parameters = ", ".join(f"{local}: {typ}" for local, _c, typ in site.parameters)
        entries.append(
            f"  @JvmStatic fun {site.name}({parameters}): {site.result_carrier} = {site.body}"
        )
    stubs = [f"  val {site.name}: Long" for site in sites(values).values()]
    return (
        "// Generated by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.internal.c\n\n"
        "import kotlin.jvm.JvmStatic\n"
        "import org.maplibre.nativeffi.generated.*\n"
        "import org.maplibre.nativeffi.internal.async.CompletionBridge\n"
        "import org.maplibre.nativeffi.internal.callback.CallbackRoots\n"
        "import org.maplibre.nativeffi.internal.callback.contain\n"
        "import org.maplibre.nativeffi.internal.callback.upcall\n"
        "import org.maplibre.nativeffi.internal.memory.*\n"
        "import org.maplibre.nativeffi.render.NativePointer\n\n"
        "/** The Kotlin side of each C function pointer the binding hands to native. */\n"
        "internal object Upcalls {\n" + "\n".join(entries) + "\n}\n\n"
        "/** The C function pointer that calls each [Upcalls] method. */\n"
        "internal expect object UpcallStubs {\n" + "\n".join(stubs) + "\n}\n"
    )


# Registration descriptors.


def put_function(value, values):
    """Write a callback registration descriptor, rooting its value until native releases it."""
    public_name = name(value.native)
    callbacks = [f for f in value.fields if f.name in value.registration.callbacks]
    lines = [
        f"internal fun NativeCall.{values.put(value)}(target: Long, value: {public_name}) {{"
    ]
    if value.default:
        lines.append("  " + values.default_call(value) + "")
    size = next((f for f in value.fields if f.role == "size"), None)
    if size:
        lines.append(
            "  "
            + values.write_scalar(
                size.value,
                values.at("target", value, size.name),
                f"{values.size(value.native)}.toUInt()",
            )
        )
    for member, _typ, children, group in values.members(value):
        if group:
            raise Unsupported(
                "callback descriptor presence group needs recursive preparation"
            )
        field = children[0]
        if field in callbacks:
            continue
        expression = "value." + member
        present = bool(field.presence and field.presence.mask)
        writes = values.field_write(value, field, "it" if present else expression)
        if present:
            mark = values.mark(value, field.presence.mask, field.presence.bit)
            lines.append(f"  {expression}?.let {{ {mark}; {'; '.join(writes)} }}")
        else:
            lines += ["  " + line for line in writes]
    # With every callback nullable, the descriptor stays unregistered when all
    # are absent. A lone callback is then known present past that return.
    all_nullable = all(f.value.nullable for f in callbacks)
    if all_nullable:
        lines.append(
            "  if ("
            + " && ".join(f"value.{identifier(f.name)} == null" for f in callbacks)
            + ") return"
        )
    lines.append(
        f"  writeAddress({values.at('target', value, value.registration.user_data)}, registrations.register(value))"
    )
    for field in callbacks:
        site = callback_site(
            name(value.native)[0].lower() + name(value.native)[1:] + name(field.name),
            field.value,
            identifier(field.name),
            public_name,
            values,
        )
        stub = f"UpcallStubs.{site}"
        if field.value.nullable and not (all_nullable and len(callbacks) == 1):
            stub = f"if (value.{identifier(field.name)} == null) 0L else {stub}"
        lines.append(
            f"  writeAddress({values.at('target', value, field.name)}, {stub})"
        )
    lines.append(
        f"  writeAddress({values.at('target', value, value.registration.release)}, UpcallStubs.releaseRoot)"
    )
    lines.append("}")
    lines.append(
        f"internal fun NativeCall.{values.write(value)}(value: {public_name}): Long = "
        f"allocate({values.size(value.native)}, {values.align(value.native)}).also {{ {values.put(value)}(it, value) }}"
    )
    return "\n".join(lines)


def read_function(value, values):
    """Copy a descriptor that holds no installed callback."""
    public_name = name(value.native)
    callbacks = [f for f in value.fields if f.name in value.registration.callbacks]
    checks = " || ".join(
        f"readAddress({values.at('source', value, f.name)}) != 0L" for f in callbacks
    )
    arguments = []
    for member, _typ, children, _group in values.members(value):
        field = children[0]
        if field in callbacks:
            arguments.append(f"{member} = null")
            continue
        decoded = values.field_read(value, field, None)
        if field.presence and field.presence.mask:
            decoded = f"if ({values.present('source', value, field.presence.mask, field.presence.bit)}) {decoded} else null"
        arguments.append(f"{member} = {decoded}")
    return (
        f"internal fun {values.read(value)}(source: Long): {public_name} {{ "
        f'check(!({checks or "false"})) {{ "cannot copy an installed callback descriptor" }}; '
        f"return {public_name}({', '.join(arguments)}) }}"
    )


# Operations that a callback transaction shapes.


def operation(plan, values, native):
    from .kotlin_operations import (
        call_arguments,
        declaration,
        encodings,
        parameter_name,
        receiver_arguments,
    )

    if plan.direct_registrations:
        return direct_operation(plan, values, native)
    decision = next(
        (
            decision
            for decision in values.bound.decisions.values()
            if plan.name in (decision.complete, decision.cancelled)
        ),
        None,
    )
    if plan.scoped_receiver or decision:
        lengths = {p.value.length for p in plan.inputs if p.value.kind == "buffer"}
        inputs = [
            p for p in plan.inputs if p.name != plan.receiver and p.name not in lengths
        ]
        params = [(parameter_name(p.name), values.public(p.value)) for p in inputs]
        # A response operation takes its response as an argument, so it keeps
        # the whole name that a free function has.
        method = identifier(plan.member if plan.receiver else public_stem(plan.name))
        arguments = call_arguments(plan, values)
        if decision and plan.name == decision.cancelled:
            arguments.append("out")
            body = (
                encodings(plan)
                + "val out = allocate(1); "
                + native.checked(plan.function, arguments)
                + "; readBool(out)"
            )
            return (
                f"  public fun {method}({declaration(params)}): Boolean = "
                f'nativeCall(this, binding, "{plan.name}", Access.READ) {{ {body} }}\n'
            )
        if decision:
            return (
                f"  public fun {method}({declaration(params)}): Unit = "
                f'nativeComplete(this, binding, "{plan.name}") '
                f"{{ {encodings(plan)}{native.checked(plan.function, arguments)} }}\n"
            )
        scoped = parameter_name(plan.scoped_receiver)
        # nativeRespond has checked the scope and passes the response as the call's handle.
        index = [p.name for p in plan.inputs].index(plan.scoped_receiver)
        arguments[index] = "handle"
        return (
            f"  public fun {method}({declaration(params)}): Unit = "
            f'nativeRespond({scoped}.bindingScope, {scoped}.bindingAddress, "{plan.name}") '
            f"{{ {encodings(plan)}{native.checked(plan.function, arguments)} }}\n"
        )
    if not plan.registrations or plan.owned_outputs:
        return None
    if not plan.completion or plan.result or plan.outputs:
        raise Unsupported(
            "callback registration needs its native admission transaction"
        )
    inputs = [p for p in plan.inputs if p.name != plan.receiver]
    params = [(parameter_name(p.name), values.public(p.value)) for p in inputs]
    receiver = next(p for p in plan.inputs if p.name == plan.receiver)
    method = identifier(plan.name.removeprefix(receiver.value.native + "_"))
    arguments = call_arguments(plan, values) + ["completion"]
    helper = "nativeCommand" if plan.execution == "command" else "nativeUnit"
    result = "CommandCompletion" if plan.execution == "command" else "Unit"
    return (
        f"  public fun {method}({', '.join(f'{n}: {t}' for n, t in params)}): Deferred<{result}> = "
        f'{helper}({receiver_arguments(plan)}, "{plan.name}", bindingCallbacks) '
        f"{{ {encodings(plan)}{native.checked(plan.function, arguments)} }}\n"
    )


def direct_operation(plan, values, native):
    """Lower a registration whose root native releases after its last callback.

    A receiver's registration roots in that receiver's callback owner, so a
    callback that captures its receiver cannot keep it reachable, while native
    release still frees the root first when the receiver stays live. A
    registration that native reports it did not store frees its root before
    returning.
    """
    from .kotlin_operations import receiver_arguments

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
    if release is None:
        raise Unsupported("direct callback registration requires a native release")
    policy = callback.reentry_policy
    owned = bool(policy and policy.registration_owner)
    if owned and not plan.receiver:
        raise Unsupported("registration-owned reentry requires a receiver handle")
    condition = registration.accepted_unless
    if condition and callback_value.nullable:
        raise Unsupported("a conditional registration cannot clear its callback")
    roles = {
        plan.receiver,
        registration.callback,
        registration.user_data,
        registration.release_callback,
        condition,
    }
    if any(p.name not in roles for p in plan.function.parameters):
        raise Unsupported(
            "direct callback registration takes only its registration parameters"
        )
    values.check(callback_value)
    values.direct_callbacks[callback_value.native] = callback_value
    wrapper = registration_class(callback_value)
    site = callback_site(
        name(callback_value.native)[0].lower() + name(callback_value.native)[1:],
        replace(callback_value, nullable=False),
        "callback",
        wrapper,
        values,
    )
    method = identifier(plan.member)

    def call(disabled):
        arguments = []
        for parameter in plan.function.parameters:
            if parameter.name == plan.receiver:
                arguments.append("handle")
            elif parameter.name == registration.callback:
                arguments.append("0L" if disabled else f"UpcallStubs.{site}")
            elif parameter.name == registration.user_data:
                arguments.append("0L" if disabled else "token")
            elif parameter.name == registration.release_callback:
                arguments.append("0L" if disabled else "UpcallStubs.releaseRoot")
            else:
                arguments.append("out")
        return native.checked(plan.function, arguments)

    owner = "bindingCallbacks" if plan.receiver else "CallbackOwner.global"
    register = (
        f"val token = registrations.register({wrapper}(callback)"
        + (", handle" if owned else "")
        + ")"
    )
    if condition:
        flag = identifier(condition)
        body = f"{register}; val out = allocate(1); {call(False)}; val {flag} = readBool(out); if (!{flag}) accept({owner}); {flag}"
    else:
        body = f"{register}; {call(False)}; accept({owner})"
    if callback_value.nullable:
        body = f"if (callback == null) {call(True)} else {{ {body} }}"
    access = ", Access.READ" if plan.receiver else ""
    returns = "Boolean" if condition else "Unit"
    return (
        f"  public fun {method}(callback: {values.public(callback_value)}): {returns} = "
        f'nativeCall({receiver_arguments(plan)}, "{plan.name}"{access}) {{ {body} }}\n'
    )
