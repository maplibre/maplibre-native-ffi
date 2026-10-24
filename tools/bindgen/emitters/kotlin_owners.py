"""Derive every Kotlin handle owner from its handle and release plans."""

from dataclasses import dataclass

from .kotlin_operations import method_name, needs_read, receiver_value
from .kotlin_values import doc, generated_owners, name, owner_name

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


def state_type(native, plans):
    return (
        "ReadHandleState"
        if any(receives(plan, native) and needs_read(plan) for plan in plans)
        else "HandleState"
    )


def release(bound, native, plans):
    """Map the scoped release onto AutoCloseable or AsyncReleasable, when a host can call it bare."""
    handle = bound.handles[native]
    plan = next(
        (p for p in plans if p.name == handle.scoped_release and receives(p, native)),
        None,
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
    value = parameter.value.unwrapped
    return bool(parameter.value.nullable or parameter.value.optional or value.default)


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
    if handle.parent:
        # The owner, not its state, keeps the parent reachable, so leak cleanup
        # and exit abandonment never hold a parent.
        parameters.append(
            f'@Suppress("unused") private val parent: {owner_name(bound.handles[handle.parent])}'
        )
    dispose = family[0].lower() + family[1:]
    body = [
        f'  internal override val binding = {state_type(native, plans)}("{owner_class}", handle, GeneratedOwnerDisposal::{dispose})',
        '  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)',
        (
            "  /**\n"
            "   * The native handle value, which runtime events report as their source. It stays readable\n"
            "   * after close, so events drained later still match this handle.\n"
            "   */\n"
            "  public val id: ULong get() = binding.handle.toULong()"
        ),
    ]
    if closes and closes.interface == ASYNC_RELEASABLE:
        body.append(
            "  public override val isClosed: Boolean get() = binding.isReleased"
        )
    elif native == "mln_render_session":
        body.append(
            "  /** Whether a release or dispose has ended the handle. */\n"
            "  public val isClosed: Boolean get() = binding.isReleased"
        )
    if closes and not closes.inherited:
        body.append(
            f"  public override fun close() {{ {closes.method}() }}"
            if closes.member == "close"
            else f"  public override fun release(): kotlinx.coroutines.Deferred<Unit> = {closes.method}()"
        )
    body.append(
        f'  override fun toString(): String = "{owner_class}(id=0x${{binding.handle.toString(16)}}, closed=${{binding.isReleased}})"'
    )
    return (
        "// Generated from handle ownership plans by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.generated\n\n"
        "import org.maplibre.nativeffi.internal.lifecycle.*\n\n"
        + doc(bound, native)
        + f"public class {owner_class} internal constructor(\n  "
        + ",\n  ".join(parameters)
        + f",\n) : {', '.join(supertypes)} {{\n"
        + "\n".join(body)
        + "\n}\n"
    )
