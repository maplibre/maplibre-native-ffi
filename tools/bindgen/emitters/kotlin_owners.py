"""Derive every Kotlin handle owner from its handle and release plans."""

from dataclasses import dataclass

from .kotlin_operations import disposal_method, method_name, needs_read, receiver_value
from .kotlin_values import doc, generated_owners, name, owner_name

ASYNC_RELEASABLE = "AsyncReleasable"
# The handles that `mln_runtime_event_source_type` names as an event's source.
EVENT_SOURCES = frozenset({"mln_runtime", "mln_map"})


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


def releases(bound, native, plans):
    """Map the scoped release onto AutoCloseable or AsyncReleasable, when a host can call it bare.

    An asynchronous release of a handle with a dispose maps onto both: `use`
    disposes, and the suspending `use` awaits the release.
    """
    handle = bound.handles[native]
    plan = next(
        (p for p in plans if p.name == handle.scoped_release and receives(p, native)),
        None,
    )
    if plan is None:
        return []
    extra = [p for p in plan.inputs if p.name != plan.receiver]
    method = method_name(plan)
    if plan.completion:
        result = [Release(ASYNC_RELEASABLE, "release", method)]
        if handle.dispose:
            result.append(Release("AutoCloseable", "close", disposal_method(handle)))
    elif not extra:
        result = [Release("AutoCloseable", "close", method)]
    else:
        return []
    for closes in result:
        if extra and closes.inherited:
            raise ValueError(
                f"{plan.name}: a release with parameters cannot implement {closes.member}()"
            )
    return result


def generate_owners(bound, plans, factories, members, imports):
    """Every handle's owner, which declares its handle's operations. The owner
    of a handle with factories has a companion object, which declares the
    common factories and which the factories of only some source sets extend."""
    return {
        f"src/commonMain/kotlin/org/maplibre/nativeffi/{owner_name(bound.handles[native])}.kt": owner(
            bound, native, plans, factories.get(native, []), members[native], imports
        )
        for native in generated_owners(bound)
    }


def owner(bound, native, plans, factories, members, imports):
    handle = bound.handles[native]
    family = name(native)
    owner_class = owner_name(handle)
    closes = releases(bound, native, plans)
    supertypes = [release.interface for release in closes]
    operations, trailer = members
    for release in closes:
        if release.inherited:
            # The release operation itself implements the interface's member.
            head = f"  public fun {release.member}("
            index = next(i for i, body in enumerate(operations) if head in body)
            operations[index] = operations[index].replace(
                head, f"  public override fun {release.member}(", 1
            )
    parameters = ["handle: Long"]
    if handle.parent:
        # The owner, not its state, keeps the parent reachable, so leak cleanup
        # and exit abandonment never hold a parent.
        parameters.append(
            f'@Suppress("unused") private val parent: {owner_name(bound.handles[handle.parent])}'
        )
    dispose = family[0].lower() + family[1:]
    body = [
        f'  internal val binding = {state_type(native, plans)}("{owner_class}", handle, GeneratedOwnerDisposal::{dispose})',
        '  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)',
        "  /**\n"
        + (
            "   * The native handle value, which runtime events report as their source. It stays readable\n"
            "   * after close, so events drained later still match this handle.\n"
            if native in EVENT_SOURCES
            else "   * The native handle value. It stays readable after close.\n"
        )
        + "   */\n"
        "  public val id: ULong get() = binding.handle.toULong()",
        "  /** Whether a release, dispose, close, or discard has ended the handle. */\n"
        + (
            "  public override val isClosed: Boolean"
            if any(release.interface == ASYNC_RELEASABLE for release in closes)
            else "  public val isClosed: Boolean"
        )
        + " get() = binding.isReleased",
    ]
    for release in closes:
        if release.inherited:
            continue
        body.append(
            f"  public override fun close() {{ {release.method}() }}"
            if release.member == "close"
            else f"  public override fun release(): Deferred<Unit> = {release.method}()"
        )
    body.extend(operations)
    body.append(
        f'  override fun toString(): String = "{owner_class}(id=0x${{binding.handle.toString(16)}}, closed=$isClosed)"'
    )
    if any(plan.factory == native for plan in plans):
        body.append(
            "  public companion object {\n"
            + "\n".join(
                "".join("  " + line if line else line for line in f.splitlines(True))
                for f in factories
            )
            + "  }"
        )
    return (
        "// Generated from handle ownership plans by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi\n\n"
        + imports
        + "\n"
        + doc(bound, native)
        + f"public class {owner_class} internal constructor(\n  "
        + ",\n  ".join(parameters)
        + ",\n)"
        + (f" : {', '.join(supertypes)}" if supertypes else "")
        + " {\n"
        + "\n".join(body)
        + "\n}\n"
        + trailer
    )
