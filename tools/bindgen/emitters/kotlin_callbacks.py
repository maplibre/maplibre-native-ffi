"""Lower retained callbacks and scoped responses to common Kotlin.

Each callback a registration hands to native gets one upcall site: a common
`Upcalls` method that finds the registration's root and runs the host
callback, and a platform stub that native calls. The platform stub converts
only primitives, so the C side of every site has the same shape.
"""

from __future__ import annotations

from dataclasses import dataclass

from ..model import CType
from ..protocol import COMPLETION
from ..semantic import public_stem
from .kotlin_values import Unsupported, doc, identifier, name, owner_class


@dataclass(frozen=True)
class Site:
    """A C function pointer that calls one `Upcalls` method."""

    name: str
    parameters: tuple[tuple[str, CType, str], ...]
    result: CType
    result_carrier: str
    body: str
    failure: str = "0"
    callback: str | None = None


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
        for parameter in callback.arguments:
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
                for p in callback.arguments
            )
            returns = (
                "Unit"
                if callback.result.native == "void" or status_callback(callback)
                else values.public(callback.result)
            )
            result.append(
                f"{doc(values.bound, value.native)}public typealias {public_name} = ({parameters}) -> {returns}"
            )
        elif value.response:
            result.append(
                f"{doc(values.bound, value.native)}public class {public_name} internal constructor(internal val bindingAddress: Long, internal val bindingScope: org.maplibre.nativeffi.internal.callback.CallbackScope)"
            )
        elif value.registration:
            parameters = []
            for member, typ, field in values.members(value):
                default = "null" if typ.endswith("?") else values.field_default(field)
                parameters.append(
                    doc(values.bound, f"{value.native}.{field.name}", "  ")
                    + f"  public val {member}: {typ}"
                    + (f" = {default}" if default else "")
                )
            result.append(
                f"{doc(values.bound, value.native)}public data class {public_name}(\n"
                + ",\n".join(parameters)
                + "\n)"
            )
    return "\n".join(result) + "\n"


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


VOID_POINTER = CType(
    "pointer", "void *", "void *", pointee=CType("void", "void", "void")
)
VOID = CType("void", "void", "void")


def completion_result_type(bound):
    """The result pointer that the completion registration's callback receives.

    Operations take a completion descriptor without exposing it as a value,
    so this reads the callback typedef that the descriptor's callback names.
    """
    descriptor = bound.source.records_by_name.get(COMPLETION)
    if descriptor is None:
        return None
    for field in descriptor.fields:
        callback = bound.callbacks.get(field.type.declaration or "")
        if callback is None:
            continue
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
                'contain("mln_user_data_release", Unit) { CompletionBridge.release(userData) }',
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
    number = values.bound.enum_numbers.get(symbol)
    if number is None:
        try:
            number = int(symbol, 0) if symbol else 0
        except ValueError as error:
            raise Unsupported(
                f"callback failure {symbol} needs a numeric constant"
            ) from error
    return number_literal(number, carrier(callback.result, values))


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
    decision = callback.decision
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
    owned = []
    for parameter in callback.arguments:
        if decision and parameter.name == decision.parameter:
            lines.append(
                f"val requestOwner = {owner_class(decision.handle)}({identifier(parameter.name)})"
            )
            arguments.append("requestOwner")
        elif parameter.value.kind == "handle" and parameter.value.ownership == "owned":
            # The callback owns the handle; a callback that throws releases it.
            local = identifier(parameter.name) + "Owner"
            lines.append(
                f"val {local} = {owner_class(parameter.value.handle)}({identifier(parameter.name)})"
            )
            arguments.append(local)
            owned.append(local)
        else:
            arguments.append(callback_argument(parameter, values))
    invoke = f"value.{member}"
    if callback_value.nullable:
        lines.insert(0, f"val invoke = {invoke} ?: return@upcall {failure}")
        invoke = "invoke"
    invocation = f"{invoke}({', '.join(arguments)})"
    if any(callback_scoped(p.value) for p in callback.arguments):
        invocation = f"withCallbackScope {{ scope -> {invocation} }}"
    if owned:
        closes = "; ".join(f"{local}.close()" for local in owned)
        invocation = f"try {{ {invocation} }} catch (failure: Throwable) {{ {closes}; throw failure }}"
    if decision:
        # Native takes back a request that the provider does not keep, including
        # when the provider throws.
        accept = number_literal(values.bound.enum_number(decision.accept), result)
        lines.append(
            f"val decision = try {{ {callback_result(callback.result, invocation, values)} }} "
            "catch (failure: Throwable) { requestOwner.binding.handOff(); throw failure }"
        )
        lines.append(f"if (decision != {accept}) requestOwner.binding.handOff()")
        lines.append("decision")
    elif callback.result.native == "void":
        lines.append(invocation)
    elif status_callback(callback):
        lines.append(invocation)
        lines.append("0")
    else:
        lines.append(callback_result(callback.result, invocation, values))
    context = identifier(callback.context)
    body = (
        f'upcall<{root_type}, {result}>("{callback.native}", {context}, {failure}) '
        f"{{ value -> {'; '.join(lines)} }}"
    )
    sites(values)[site_name] = Site(
        site_name,
        tuple(parameters),
        callback.result.ctype,
        result,
        body,
        failure,
        callback.native,
    )
    return site_name


def callback_scoped(value):
    """Whether a callback argument is a response that its callback's scope keeps valid."""
    return value.kind == "reference" and value.element.response


def number_literal(number, carrier_type):
    """A literal of the callback result's [carrier_type] for the integer [number]."""
    if carrier_type == "Long":
        return f"{number}L"
    return str(number if number <= 0x7FFFFFFF else number - (1 << 32))


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
        "import org.maplibre.nativeffi.internal.callback.withCallbackScope\n"
        "import org.maplibre.nativeffi.internal.memory.*\n"
        "import org.maplibre.nativeffi.render.NativePointer\n\n"
        "/** The Kotlin side of each C function pointer the binding hands to native. */\n"
        "internal object Upcalls {\n" + "\n".join(entries) + "\n}\n\n"
        "/** The C function pointer that calls each [Upcalls] method. */\n"
        "internal expect object UpcallStubs {\n" + "\n".join(stubs) + "\n}\n"
    )


def put_function(value, values):
    """Write a callback registration descriptor, rooting its value until native releases it."""
    public_name = name(value.native)
    callbacks = [f for f in value.fields if f.name in value.registration.callbacks]
    lines = [
        f"internal fun NativeCall.{values.put(value)}(target: Long, value: {public_name}) {{"
    ]
    for field in value.fields:
        if field.role == "presence_mask":
            from .kotlin_values import literal

            lines.append(
                f"  var {identifier(field.name)} = {literal(0, values.scalar(field.value)[0])}"
            )
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
    for member, _typ, field in values.members(value):
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
    for field in value.fields:
        if field.role == "presence_mask":
            lines.append(
                "  "
                + values.write_scalar(
                    field.value,
                    values.at("target", value, field.name),
                    identifier(field.name),
                )
            )
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
    """Copy a registration's native default, whose callbacks are unset."""
    public_name = name(value.native)
    callbacks = [f for f in value.fields if f.name in value.registration.callbacks]
    locals = []
    for field in value.fields:
        if field.role == "presence_mask":
            suffix, _ = values.accessor(field.value)
            locals.append(
                f"val {identifier(field.name)} = read{suffix}({values.at('source', value, field.name)})"
            )
    arguments = []
    for member, _typ, field in values.members(value):
        if field in callbacks:
            arguments.append(f"{member} = null")
            continue
        decoded = values.field_read(value, field, None)
        if field.presence and field.presence.mask:
            decoded = f"if ({values.present('source', value, field.presence.mask, field.presence.bit)}) {decoded} else null"
        arguments.append(f"{member} = {decoded}")
    body = f"{public_name}({', '.join(arguments)})"
    if locals:
        return (
            f"internal fun {values.read(value)}(source: Long): {public_name} {{\n  "
            + "\n  ".join(locals)
            + f"\n  return {body}\n}}"
        )
    return f"internal fun {values.read(value)}(source: Long): {public_name} = {body}"


def operation(plan, values, native):
    from .kotlin_operations import (
        call_arguments,
        declaration,
        encodings,
        parameter_name,
    )

    if plan.scoped_receiver:
        lengths = {p.value.length for p in plan.inputs if p.value.kind == "buffer"}
        inputs = [
            p for p in plan.inputs if p.name != plan.receiver and p.name not in lengths
        ]
        params = [(parameter_name(p.name), values.public(p.value)) for p in inputs]
        # A response operation takes its response as an argument, so it keeps
        # the whole name that a free function has.
        method = identifier(plan.member if plan.receiver else public_stem(plan.name))
        arguments = call_arguments(plan, values)
        scoped = parameter_name(plan.scoped_receiver)
        index = [p.name for p in plan.inputs].index(plan.scoped_receiver)
        arguments[index] = "handle"
        return (
            f"  public fun {method}({declaration(params)}): Unit = "
            f"nativeRespond({scoped}.bindingScope, {scoped}.bindingAddress) "
            f"{{ {encodings(plan)}{native.checked(plan.function, arguments)} }}\n"
        )
    if not plan.registrations or plan.owned_outputs:
        return None
    if not plan.completion:
        return immediate_registration(plan, values, native)
    if plan.result or plan.outputs:
        raise Unsupported(
            "callback registration needs its native admission transaction"
        )
    return None


def immediate_registration(plan, values, native):
    """Lower a registration that native accepts before the call returns.

    A receiver's registration roots in that receiver's callback owner, so a
    callback that captures its receiver cannot keep it reachable, while native
    release still frees the root first when the receiver stays live. A
    registration that native reports it kept nothing from frees its root
    before returning.
    """
    from .kotlin_operations import (
        call_arguments,
        declaration,
        encodings,
        parameter_name,
        receiver_arguments,
    )

    declined = next(
        (r.accepted_unless for r in plan.registrations if r.accepted_unless), None
    )
    if (
        plan.result
        or [p.name for p in plan.outputs] != ([declined] if declined else [])
        or (declined and plan.function.parameters[-1].name != declined)
    ):
        raise Unsupported(
            "callback registration needs its native admission transaction"
        )
    inputs = [p for p in plan.inputs if p.name != plan.receiver]
    params = [(parameter_name(p.name), values.public(p.value)) for p in inputs]
    arguments = call_arguments(plan, values)
    owner = "bindingCallbacks" if plan.receiver else "CallbackOwner.global"
    if declined:
        flag = identifier(declined)
        body = (
            f"val out = allocate(1); {native.checked(plan.function, [*arguments, 'out'])}; "
            f"val {flag} = readBool(out); if (!{flag}) accept({owner}); {flag}"
        )
    else:
        body = f"{native.checked(plan.function, arguments)}; accept({owner})"
    return (
        f"  public fun {identifier(plan.member)}({declaration(params)}): "
        f"{'Boolean' if declined else 'Unit'} = "
        f"nativeCall({receiver_arguments(plan)}) "
        f"{{ {encodings(plan)}{body} }}\n"
    )
