"""Lower retained callbacks and scoped responses to Kotlin.

Each callback a registration hands to native gets one upcall site: an `Upcalls`
method that finds the registration's root and runs the host callback, and a
platform stub that native calls. The platform stub converts only primitives, so
the C side of every site has the same shape.

Common code takes each registration that behaves the same on every target, in
the form that `async_hosts` gives a host that cannot run code on a native
thread:

- A `queue` registration's callbacks receive what a queue delivery carries and
  return nothing. A threaded target's site runs the callback and returns what
  queue mode answers. The site owns each handle that the call passes with
  ownership, and releases it when the callback throws or the registration is
  gone. Kotlin/Wasm sets the registration's queue and runs the same site when
  it drains the delivery.
- A `wake`, `disabled`, or callback-less registration keeps its callbacks.

A registration whose answer a callback computes, or a rule replaces, is split:
its forms differ by target, so common code declares none of them (see
`Values.split`). The threaded targets, which run Kotlin on MapLibre's threads,
take it under its own name in the `threadedMain` source set, with callbacks
that compute native's answer as the C callback does. Kotlin/Wasm takes the form
that `async_hosts` gives it under a name that says so: `Queued` before a queue
form, whose callbacks return nothing, and `Rules` after a rules form, which
holds only the native rules. The operations that take a split registration,
and the scoped responses of its callbacks, are extensions in the source set of
each form.
"""

from __future__ import annotations

from dataclasses import dataclass

from .. import async_hosts, docs
from ..model import CType
from ..protocol import COMPLETION
from ..semantic import public_stem
from .kotlin_values import Unsupported, doc, identifier, name, owner_class

# The split lowerings: the threaded targets' forms, and Kotlin/Wasm's.
THREADED = "threaded"
ASYNC = "async"


@dataclass(frozen=True)
class Site:
    """A C function pointer, or a drained delivery, that calls one upcall method."""

    name: str
    parameters: tuple[tuple[str, CType, str], ...]
    result: CType
    result_carrier: str
    body: str
    failure: str = "0"
    callback: str | None = None
    # The split lowering whose form the site serves, THREADED or ASYNC, or None
    # for a common site.
    split: str | None = None
    # The `async_hosts` form of the registration that the site serves, which
    # tells a target without native-thread upcalls how to reach it.
    form: str | None = None
    # The Kotlin type of the registration whose root the site looks up.
    root: str | None = None

    @property
    def owner(self) -> str:
        return SPLIT_UPCALLS[self.split][0] if self.split else "Upcalls"


def status_callback(callback):
    return callback.status


def answered(callback):
    """Whether a callback's queue mode answers in its place: it has a queue
    delivery and returns a value that its host callback would compute."""
    return (
        callback.delivery is not None
        and callback.result.native != "void"
        and not status_callback(callback)
    )


def form(value, values):
    """The `async_hosts` form of a registration, or None."""
    return async_hosts.form(values.bound, value)


def splits(value, values):
    """Whether a registration's forms differ by target, so common code declares none."""
    if not value.registration:
        return False
    registration_form = form(value, values)
    if registration_form == "rules":
        return True
    return registration_form == "queue" and any(
        answered(values.bound.callbacks[field.value.native])
        for field in async_hosts.callback_fields(value)
    )


def split(value, values):
    """Whether [value] takes a target's own form in the current split lowering."""
    if not values.split:
        return False
    if value.registration:
        return splits(value, values)
    if value.kind == "callback":
        return answered(values.bound.callbacks[value.native])
    return False


def public_name(value, values):
    """The Kotlin name of a callback, registration, or response in the current lowering."""
    base = name(value.native)
    if values.split != ASYNC or not split(value, values):
        return base
    if value.registration and form(value, values) == "rules":
        return base + "Rules"
    return "Queued" + base


def queued(value, values):
    """Whether a callback takes its queue shape: it returns nothing, and native answers."""
    return answered(values.bound.callbacks[value.native]) and values.split != THREADED


def split_registration(plan, values):
    """Whether [plan] takes a split registration, so each target's source set extends it."""
    for parameter in plan.inputs:
        value = parameter.value
        if value.kind == "reference" and value.element:
            value = value.element
        if value.registration and splits(value, values):
            return True
    return False


def omitted_fields(value, values):
    """The callback fields that a registration's rules form leaves out."""
    if values.split == THREADED or form(value, values) != "rules":
        return set()
    return {
        field.name
        for field in async_hosts.callback_fields(value)
        if async_hosts.omitted(values.bound, field)
    }


def check(value, values):
    if not (value.kind == "callback" or value.registration or value.response):
        return False
    used = values.split_used[values.split] if values.split else values.used
    if value.native in used:
        return True
    used[value.native] = value
    if value.kind == "callback":
        callback = values.bound.callbacks[value.native]
        if not callback.context:
            raise Unsupported("callback needs its context parameter")
        for parameter in callback.arguments:
            values.check(parameter.value)
        if (
            callback.result.native != "void"
            and not status_callback(callback)
            and not queued(value, values)
        ):
            values.check(callback.result)
    elif value.registration:
        for field in values.fields(value):
            values.check(field.value)
    return True


def public(value, values):
    if not check(value, values):
        return None
    return public_name(value, values) + (
        "?" if value.nullable or value.optional else ""
    )


def split_note(value, values):
    """What a split form says about the target that takes it and how it answers native."""
    if values.split == THREADED:
        values.split = ASYNC
        try:
            wasm = public_name(value, values)
        finally:
            values.split = THREADED
        return f"Kotlin/Wasm takes `{wasm}` in its place."
    if value.registration and form(value, values) == "rules":
        return (
            "This is the Kotlin/Wasm form, which holds only native header rules, "
            "because the page cannot run Kotlin while native waits for an answer. "
            f"The JVM, Android, and Kotlin/Native take `{name(value.native)}`."
        )
    if value.registration:
        return (
            "This is the Kotlin/Wasm queue form: the page receives each call after "
            "native has answered it. The JVM, Android, and Kotlin/Native take "
            f"`{name(value.native)}`, whose callback computes the answer."
        )
    answer = values.bound.callbacks[value.native].queue_result
    return (
        "The page receives the call after native has answered it, so the callback "
        f"returns nothing, and native answers `{answer}` as it does for a queue."
    )


def declaration_doc(value, values):
    """The KDoc of a declaration, which says how a split form differs by target."""
    found = values.bound.doc(value.native)
    if split(value, values):
        note = split_note(value, values)
    else:
        return doc(values.bound, value.native)
    summary = f"{found.summary} {note}" if found else note
    return docs.block_comment(
        docs.Doc(value.native, summary, found.url if found else "")
    )


def declaration(value, values):
    """The Kotlin declaration of one callback, registration, or response."""
    public = public_name(value, values)
    if value.kind == "callback":
        callback = values.bound.callbacks[value.native]
        parameters = ", ".join(
            f"{identifier(p.name)}: {values.public(p.value)}"
            for p in callback.arguments
        )
        returns = (
            "Unit"
            if callback.result.native == "void"
            or status_callback(callback)
            or queued(value, values)
            else values.public(callback.result)
        )
        return f"{declaration_doc(value, values)}public typealias {public} = ({parameters}) -> {returns}"
    if value.response:
        return (
            f"{doc(values.bound, value.native)}public class {public} internal constructor("
            "internal val bindingAddress: Long, "
            "internal val bindingScope: org.maplibre.nativeffi.internal.callback.CallbackScope)"
        )
    parameters = []
    for member, typ, field in values.members(value):
        default = "null" if typ.endswith("?") else values.field_default(field)
        parameters.append(
            doc(values.bound, f"{value.native}.{field.name}", "  ")
            + f"  public val {member}: {typ}"
            + (f" = {default}" if default else "")
        )
    return (
        f"{declaration_doc(value, values)}public data class {public}(\n"
        + ",\n".join(parameters)
        + "\n)"
    )


def common(values):
    """The callback, registration, and response types that common code declares."""
    result = [
        declaration(value, values)
        for value in list(values.used.values())
        if is_callback_value(value)
    ]
    return "\n".join(result) + "\n"


def is_callback_value(value):
    return value.kind == "callback" or bool(value.registration or value.response)


def split_declarations(values, lowering):
    """The forms of a split lowering, and the types that only they use, which its
    source set declares."""
    previous = values.split
    values.split = lowering
    try:
        return [
            declaration(value, values)
            for value in list(values.split_used[lowering].values())
            if is_callback_value(value)
            and (value.native not in values.used or split(value, values))
        ]
    finally:
        values.split = previous


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
                form="wake",
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


def constant_number(symbol, values, what):
    """The number of a callback result constant: an enum constant or an integer literal."""
    number = values.bound.enum_numbers.get(symbol)
    if number is None:
        try:
            number = int(symbol, 0) if symbol else 0
        except ValueError as error:
            raise Unsupported(
                f"callback {what} {symbol} needs a numeric constant"
            ) from error
    return number


def failure_literal(callback, values):
    if callback.result.native == "void":
        return "Unit"
    return number_literal(
        constant_number(callback.failure, values, "failure"),
        carrier(callback.result, values),
    )


def callback_argument(parameter, values):
    """The public value of one callback argument, from its carrier local."""
    from .kotlin_operations import PUBLIC_FROM_CARRIER

    value = parameter.value
    local = identifier(parameter.name)
    if value.kind == "reference":
        if value.element.response:
            return f"{public_name(value.element, values)}({local}, scope)"
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


def owns(callback, parameter):
    """Whether the callback receives [parameter] as a handle that it owns."""
    if callback.decision and parameter.name == callback.decision.parameter:
        return True
    return parameter.value.kind == "handle" and parameter.value.ownership == "owned"


def callback_site(site_name, callback_value, member, root_type, values, site_form):
    """Register the upcall site that runs [member] of the root's [root_type] value."""
    callback = values.bound.callbacks[callback_value.native]
    result = carrier(callback.result, values)
    parameters = tuple(
        (identifier(p.name), p.value.ctype, carrier(p.value, values))
        for p in callback.parameters
    )
    if site_form == "queue":
        return queued_site(
            site_name, callback, callback_value, member, root_type, values, parameters
        )
    failure = failure_literal(callback, values)
    decision = callback.decision
    arguments = []
    lines = []
    owned = []
    for parameter in callback.arguments:
        if decision and parameter.name == decision.parameter:
            lines.append(
                f"val requestOwner = {owner_class(decision.handle)}({identifier(parameter.name)})"
            )
            arguments.append("requestOwner")
        elif owns(callback, parameter):
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
        parameters,
        callback.result.ctype,
        result,
        body,
        failure,
        callback.native,
        values.split,
        site_form,
        root_type,
    )
    return site_name


def queued_site(
    site_name, callback, callback_value, member, root_type, values, parameters
):
    """Register the site of a queue-shaped callback.

    The site answers native as queue mode does, whatever the callback does. It
    owns each handle that the call passes with ownership before it looks up the
    registration, and releases the handle when the callback throws or the
    registration is gone, so a delivery that a queue carries reaches the same
    site.
    """
    result = carrier(callback.result, values)
    answer = (
        "Unit"
        if callback.result.native == "void"
        else number_literal(
            constant_number(callback.queue_result, values, "queue result"), result
        )
    )
    adopted, owners, arguments = [], [], []
    for parameter in callback.arguments:
        if owns(callback, parameter):
            handle = (
                callback.decision.handle
                if callback.decision and parameter.name == callback.decision.parameter
                else parameter.value.handle
            )
            local = identifier(parameter.name) + "Owner"
            adopted.append(
                f"val {local} = {owner_class(handle)}({identifier(parameter.name)})"
            )
            owners.append(local)
            arguments.append(local)
        else:
            arguments.append(callback_argument(parameter, values))
    released = "; ".join(f"{local}.close()" for local in owners)
    invoke = f"value.{member}"
    lines = []
    if callback_value.nullable:
        lines.append(f"val invoke = {invoke} ?: return@upcall false")
        invoke = "invoke"
    lines.append(f"{invoke}({', '.join(arguments)})")
    lines.append("true")
    context = identifier(callback.context)
    call = (
        f'upcall<{root_type}, Boolean>("{callback.native}", {context}, false) '
        f"{{ value -> {'; '.join(lines)} }}"
    )
    if adopted:
        statements = [*adopted, f"if (!{call}) {{ {released} }}"]
    else:
        statements = [call]
    statements.append(answer)
    body = "run { " + "; ".join(statements) + " }"
    sites(values)[site_name] = Site(
        site_name,
        parameters,
        callback.result.ctype,
        result,
        body,
        answer,
        callback.native,
        values.split,
        "queue",
        root_type,
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


# JvmStatic is an optional expectation, which only shared source sets can name.
JVM_STATIC = "import kotlin.jvm.JvmStatic\n"
UPCALL_IMPORTS = (
    "import org.maplibre.nativeffi.generated.*\n"
    "import org.maplibre.nativeffi.internal.async.CompletionBridge\n"
    "import org.maplibre.nativeffi.internal.callback.CallbackRoots\n"
    "import org.maplibre.nativeffi.internal.callback.contain\n"
    "import org.maplibre.nativeffi.internal.callback.upcall\n"
    "import org.maplibre.nativeffi.internal.callback.withCallbackScope\n"
    "import org.maplibre.nativeffi.internal.memory.*\n"
    "import org.maplibre.nativeffi.render.NativePointer\n\n"
)


def upcall_object(entries, owner, doc_line, static="@JvmStatic "):
    methods = []
    for site in entries:
        parameters = ", ".join(f"{local}: {typ}" for local, _c, typ in site.parameters)
        methods.append(
            f"  {static}fun {site.name}({parameters}): {site.result_carrier} = {site.body}"
        )
    return (
        f"/** {doc_line} */\ninternal object {owner} {{\n"
        + "\n".join(methods)
        + "\n}\n"
    )


def upcalls(values):
    """The common `Upcalls` methods, and the `UpcallStubs` behind every site that
    native calls as a function pointer."""
    every = [site for site in sites(values).values() if site.split != ASYNC]
    common_sites = [site for site in every if not site.split]
    return (
        "// Generated by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.internal.c\n\n"
        + JVM_STATIC
        + UPCALL_IMPORTS
        + upcall_object(
            common_sites,
            "Upcalls",
            "The Kotlin side of each C function pointer the binding hands to native.",
        )
        + "\n/**\n"
        " * The C function pointer that calls each [Upcalls] method, and on a threaded target each\n"
        " * `ThreadedUpcalls` method.\n"
        " */\n"
        "internal expect object UpcallStubs {\n"
        + "\n".join(f"  val {site.name}: Long" for site in every)
        + "\n}\n"
    )


# The upcall object of each split lowering's sites, and its KDoc.
SPLIT_UPCALLS = {
    THREADED: (
        "ThreadedUpcalls",
        (
            "The Kotlin side of each threaded form's C function pointer, which "
            "computes native's answer on the calling thread."
        ),
    ),
    ASYNC: (
        "WasmUpcalls",
        (
            "The Kotlin side of each Kotlin/Wasm form's delivery, which the page "
            "runs as it drains the delivery."
        ),
    ),
}


def split_upcalls(values, lowering):
    """The upcall object of a split lowering's sites, which its source set declares."""
    owner, line = SPLIT_UPCALLS[lowering]
    # Only JNI and FFM call a threaded site by its static method; the page calls
    # a wasm site from Kotlin.
    static = lowering == THREADED
    return (
        "// Generated by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.internal.c\n\n"
        + (JVM_STATIC if static else "")
        + UPCALL_IMPORTS
        + upcall_object(
            [site for site in sites(values).values() if site.split == lowering],
            owner,
            line,
            "@JvmStatic " if static else "",
        )
    )


def put_function(value, values):
    """Write a callback registration descriptor, rooting its value until native releases it."""
    public = public_name(value, values)
    threaded = values.split == THREADED and split(value, values)
    registration_form = "direct" if threaded else form(value, values)
    omitted = omitted_fields(value, values)
    callbacks = [
        f
        for f in value.fields
        if f.name in value.registration.callbacks and f.name not in omitted
    ]
    lines = [
        f"internal fun NativeCall.{values.put(value)}(target: Long, value: {public}) {{"
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
    if not callbacks:
        # Native rules answer in place of every callback, so nothing is rooted.
        lines.append("}")
        lines.append(writer(value, values, public))
        return "\n".join(lines)
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
    writes = []
    for field in callbacks:
        site = callback_site(
            public[0].lower() + public[1:] + name(field.name),
            field.value,
            identifier(field.name),
            public,
            values,
            registration_form,
        )
        stub = f"UpcallStubs.{site}"
        if field.value.nullable and not (all_nullable and len(callbacks) == 1):
            stub = f"if (value.{identifier(field.name)} == null) 0L else {stub}"
        writes.append(f"writeAddress({values.at('target', value, field.name)}, {stub})")
    writes.append(
        f"writeAddress({values.at('target', value, value.registration.release)}, UpcallStubs.releaseRoot)"
    )
    if registration_form == "queue" and values.split == ASYNC:
        # Only Kotlin/Wasm takes this form, and it always names its queue.
        queue = values.at("target", value, value.registration.queue)
        lines.append(f"  writeI64({queue}, HostDelivery.queue)")
    elif registration_form == "queue":
        # A target that delivers callbacks to its host thread names its queue
        # in place of the callbacks, and drains each call to the same site.
        queue = values.at("target", value, value.registration.queue)
        lines.append("  val queue = HostDelivery.queue")
        lines.append(f"  if (queue != 0L) writeI64({queue}, queue)")
        lines.append("  else {")
        lines += ["    " + line for line in writes]
        lines.append("  }")
    else:
        lines += ["  " + line for line in writes]
    lines.append("}")
    lines.append(writer(value, values, public))
    return "\n".join(lines)


def writer(value, values, public):
    return (
        f"internal fun NativeCall.{values.write(value)}(value: {public}): Long = "
        f"allocate({values.size(value.native)}, {values.align(value.native)}).also {{ {values.put(value)}(it, value) }}"
    )


def read_function(value, values):
    """Copy a registration's native default, whose callbacks are unset."""
    public_name = name(value.native)
    omitted = omitted_fields(value, values)
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
        if field.name in omitted:
            continue
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
        if values.split != THREADED:
            raise Unsupported("a callback response exists only inside a threaded form")
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
