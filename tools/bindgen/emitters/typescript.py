"""Generate the TypeScript binding: its public API and its Node-API addon.

Each operation lowers once into both halves. The addon half converts the
JavaScript arguments into C storage, calls the C function directly, and
returns a JavaScript value. The TypeScript half names the operation on its
owner class or as a top-level function and calls the runtime helper for its
execution shape.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from .. import async_hosts, docs, model
from ..compiler import compile_api
from ..model import Api
from ..protocol import COMPLETION_RESULT
from ..semantic import (
    BoundApi,
    OperationPlan,
    OwnedOutputPlan,
)
from .typescript_native_values import NativeValues
from .typescript_values import (
    Unsupported,
    Values,
    enum_members,
    identifier,
    native_fields,
    owner_class,
    public_name,
    registration,
    union_kinds,
)

TS_DIRECTORY = "bindings/typescript/src"
NATIVE_DIRECTORY = "bindings/typescript/native"


@dataclass
class Lowered:
    """One operation, as both halves of the binding call it."""

    plan: OperationPlan
    owner: str | None
    method: str
    # (name, type, default) of each public parameter.
    parameters: list[tuple[str, str, str | None]] = field(default_factory=list)
    # The TypeScript expressions that the addon function receives, before
    # the completion token.
    arguments: list[str] = field(default_factory=list)
    # (name, type) of each addon parameter, for the addon interface.
    native_parameters: list[tuple[str, str]] = field(default_factory=list)
    native_result: str = "void"
    shape: str = "call"
    returns: str = "void"
    owned: OwnedOutputPlan | None = None
    absent: bool = False
    registrations: bool = False
    # The output whose true value reports that native kept no registration.
    declined: bool = False
    # The record a borrowed view scopes, for a scoped view result.
    view: str | None = None
    # The decoder that converts the completion's value, if any.
    decoder: str | None = None
    # Whether a discarding view of the receiver also calls the command.
    discardable: bool = False
    # The C statement that releases a drained batch after its slice is copied.
    drained: str | None = None
    c_locals: list[str] = field(default_factory=list)
    c_body: list[str] = field(default_factory=list)
    # File-scope C that the export calls, such as a thread start's function.
    c_helpers: str = ""


def owner_native(entry: Lowered) -> str | None:
    """The handle whose class declares a method: its receiver, or the handle
    that a factory creates as a static method."""
    return entry.owner or entry.plan.factory


def attachment_name(owned: OwnedOutputPlan) -> str:
    return "New" + owner_class(owned.handle).removesuffix("Handle")


def scoped_name(native: str) -> str:
    return "Scoped" + public_name(native)


# The platforms that the Node-API addon builds for, and the browser module's.
# Every entry point shares one operations module, so it leaves out only what
# none of them can use.
TARGETS = frozenset({"linux", "macos", "ohos", "web"})


def lower_operations(api: Api | BoundApi):
    from .typescript_operations import lower_function

    bound = compile_api(api, TARGETS)
    values = Values(bound)
    natives = NativeValues(values)
    lowered: list[Lowered] = []
    unsupported = {
        name: "\n".join(reasons) for name, reasons in bound.unsupported.items()
    }
    for plan in bound.operations:
        try:
            # A failed attempt leaves no converter or declaration behind.
            local_values = Values(bound)
            local_natives = NativeValues(local_values)
            result = lower_function(plan, local_values, local_natives)
            values.merge(local_values)
            natives.merge(local_natives)
            lowered.append(result)
        except Unsupported as error:
            unsupported[plan.name] = f"{plan.function.location}: {error}"
    # Native rules answer in place of a callback that TypeScript cannot run, so
    # the operations that only such a callback calls have no caller.
    registrations = [v for v in values.used.values() if v.registration]
    for name in async_hosts.rule_answered_operations(bound, registrations):
        unsupported.pop(name, None)
    return bound, values, natives, lowered, unsupported


def symbols(bound: BoundApi) -> docs.Symbol:
    """How TypeScript spells the C declarations that its summaries name.

    `lower` records them before it renders a summary.
    """
    return bound.symbols("typescript", _unlowered)


def _unlowered(bound: BoundApi) -> docs.Symbol:
    raise AssertionError("TypeScript names its symbols only while it lowers")


def _symbols(bound: BoundApi, values: Values, lowered: list[Lowered]) -> docs.Symbol:
    # The names come from the same lowering that declares them, so a summary
    # names only what this binding generates.
    methods = {entry.plan.name: entry for entry in lowered}
    # A record that holds only a union presents the union under its own name.
    holders = {
        member.value.native: value
        for value in values.used.values()
        if (member := value.union_member)
    }

    def type_(native: str) -> str | None:
        if native in bound.public_handles:
            return owner_class(bound.public_handles[native])
        value = holders.get(native) or values.used.get(native)
        return public_name(value.native) if value else None

    def operation(plan: OperationPlan) -> str | None:
        entry = methods.get(plan.name)
        if entry is None:
            return None
        owner = owner_native(entry)
        if owner is None:
            return entry.method
        return f"{owner_class(bound.handles[owner])}.{entry.method}"

    def constant(value, member: str) -> str | None:
        typ = type_(value.native)
        name = next((m for m, c, _ in enum_members(value) if c == member), None)
        return f"{typ}.{name}" if typ and name else None

    def field(value, member) -> str | None:
        typ = type_(value.native)
        if typ is None or value.union_member:
            return None
        if value.kind == "union":
            # A union's members are its variants, which `kind` tells apart.
            kind = next(k for _, k, arm in union_kinds(value) if arm == member.name)
            return f'{typ} & {{ kind: "{kind}" }}'
        if value.registration:
            entry = registration(bound, value)
            if entry.native and member in entry.native:
                return f"{native_fields(typ)}.{identifier(member.name)}"
        if member not in values.fields(value):
            return None
        return f"{typ}.{identifier(member.name)}"

    def flag(value, mask_flag) -> str | None:
        typ = type_(value.native)
        if typ is None or value.registration:
            return None
        return f"{typ}.{identifier(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=constant,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{name}": f"CommandCompletion.{identifier(name)}"
            for name in ("generation", "render_update_generation")
        },
        # Native queues each call, and the realm dispatches the queue on its
        # event loop after the native callback has returned.
        threads=(
            "The callback runs asynchronously on the JavaScript thread that "
            "registered it, after native queues the call."
        ),
    )


def lower(api: Api | BoundApi) -> model.Lowered:
    from .typescript_native import render_native
    from .typescript_operations import render_operations, view_scopes

    bound, values, natives, lowered, unsupported = lower_operations(api)
    bound.symbols("typescript", lambda bound: _symbols(bound, values, lowered))
    # The operations name every value, so the value table follows them.
    outputs = render_operations(bound, values, lowered, sorted(natives.callbacks))
    outputs[f"{TS_DIRECTORY}/generated_values.ts"] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n\n"
        + values.render()
    )
    outputs.update(render_native(bound, natives, lowered, view_scopes(lowered)))
    return model.Lowered(outputs, unsupported, [entry.plan.name for entry in lowered])
