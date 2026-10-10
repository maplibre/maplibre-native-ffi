"""Derive every Kotlin handle owner from its handle, release, and decision plans."""

from dataclasses import dataclass

from .kotlin_operations import method_name, receiver_value
from .kotlin_values import generated_owners, name, owner_name

ASYNC_RELEASABLE = "org.maplibre.nativeffi.runtime.AsyncReleasable"


@dataclass(frozen=True)
class Release:
    """How an owner exposes its handle's release operation."""

    interface: str
    member: str
    method: str

    @property
    def inherited(self):
        return self.member == self.method


def receives(plan, native):
    return bool(plan.receiver) and receiver_value(plan).native == native


def decision(bound, native):
    return bound.decisions.get(native)


def release(bound, native, plans):
    """Map the release plan onto AutoCloseable or AsyncReleasable, when a host can call it bare."""
    handle = bound.handles[native]
    plan = next(
        (p for p in plans if p.name == handle.release and receives(p, native)), None
    )
    if plan is None:
        return None
    extra = [p for p in plan.inputs if p.name != plan.receiver]
    method = method_name(plan)
    if plan.completion:
        result = Release(ASYNC_RELEASABLE, "release", method)
    elif all(defaulted(p) for p in extra):
        result = Release("AutoCloseable", "close", method)
    else:
        return None
    if extra and result.inherited:
        raise ValueError(
            f"{plan.name}: a release with parameters cannot implement {result.member}()"
        )
    return result


def defaulted(parameter):
    value = (
        parameter.value.element
        if parameter.value.kind == "reference"
        else parameter.value
    )
    return bool(parameter.value.nullable or parameter.value.optional or value.default)


def enum_number(bound, symbol):
    for value in bound.values.values():
        for key, number in value.enum_values:
            if key == symbol:
                return number
    raise ValueError(f"decision constant {symbol} has no enum value")


def state_type(bound, native):
    return "DecisionOwnerState" if decision(bound, native) else "HandleStateCore"


def generate_owners(bound, plans):
    return {
        f"src/commonMain/kotlin/org/maplibre/nativeffi/generated/{owner_name(bound.handles[native])}.kt": owner(
            bound, native, plans
        )
        for native in generated_owners(bound)
    }


def owner(bound, native, plans):
    handle = bound.handles[native]
    family = name(native)
    owner_class = owner_name(handle)
    closes = release(bound, native, plans)
    supertypes = [f"Generated{family}Operations()"]
    if closes:
        supertypes.append(closes.interface)
    parameters = ["handle: Long"]
    parents = ""
    if handle.parent:
        if handle.parent_retention != "strong":
            raise ValueError(
                f"{native}: parent retention {handle.parent_retention} needs a Kotlin owner"
            )
        parameters.append(f"parent: {owner_name(bound.handles[handle.parent])}")
        parents = ", parent"
    dispose = family[0].lower() + family[1:]
    parameters.append(f"dispose: (Long) -> Unit = GeneratedOwnerDisposal::{dispose}")
    body = []
    choice = decision(bound, native)
    if choice:
        if handle.parent or handle.dispose != handle.release:
            raise ValueError(
                f"{native}: a decision owner releases through its disposal path"
            )
        accept = enum_number(bound, choice.accept)
        pass_through = enum_number(bound, choice.pass_through)
        body.append(
            f'  internal override val binding = DecisionOwnerState("{owner_class}", handle, accept = {accept}u, passThrough = {pass_through}u, dispose = dispose)'
        )
        body.append(
            '  @Suppress("unused") private val cleanup = closeWhenUnreachable(this, binding.core)'
        )
        body.append("  public val isClosed: Boolean get() = binding.isClosed")
    else:
        body.append(
            f'  internal override val binding = HandleStateCore("{owner_class}", handle{parents}, dispose = dispose)'
        )
        body.append(
            '  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)'
        )
        body.append("  public val isClosed: Boolean get() = binding.isReleased()")
    if closes and not closes.inherited:
        body.append(
            f"  public override fun close() {{ {closes.method}() }}"
            if closes.member == "close"
            else f"  public override fun release(): kotlinx.coroutines.Deferred<Unit> = {closes.method}()"
        )
    return (
        "// Generated from handle ownership plans by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.generated\n\n"
        "import org.maplibre.nativeffi.internal.lifecycle.*\n\n"
        f"public class {owner_class} internal constructor(\n  "
        + ",\n  ".join(parameters)
        + f",\n) : {', '.join(supertypes)} {{\n"
        + "\n".join(body)
        + "\n}\n"
    )
