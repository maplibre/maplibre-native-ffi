"""Derive every Kotlin handle owner from its handle and release plans."""

from dataclasses import dataclass

from .kotlin_operations import disposal_method, method_name, needs_read, receiver_value
from .kotlin_values import doc, generated_owners, name, owner_name

ASYNC_RELEASABLE = "AsyncReleasable"


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
    """Map the scoped release and dispose onto AsyncReleasable and AutoCloseable.

    AsyncReleasable's `release`, which `useReleasing` awaits, is the release
    that waits. AutoCloseable's `close`, which the standard library's `use`
    calls and which can't wait, calls the scoped dispose: the dispose, or the
    release of a handle that its release frees at once.
    """
    handle = bound.handles[native]
    plan = next(
        (p for p in plans if p.name == handle.scoped_release and receives(p, native)),
        None,
    )
    if plan is None:
        return []
    extra = [p for p in plan.inputs if p.name != plan.receiver]
    if extra:
        return []
    result = []
    if plan.completion:
        result.append(Release(ASYNC_RELEASABLE, "release", method_name(plan)))
    disposal = next(
        (p for p in plans if p.name == handle.scoped_dispose and receives(p, native)),
        None,
    )
    if handle.scoped_dispose:
        result.append(
            Release(
                "AutoCloseable",
                "close",
                method_name(disposal) if disposal else disposal_method(handle),
            )
        )
    for scoped in result:
        if extra and scoped.inherited:
            raise ValueError(
                f"{plan.name}: a release with parameters cannot implement {scoped.member}()"
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
    scoped = releases(bound, native, plans)
    supertypes = [release.interface for release in scoped]
    operations, trailer = members
    for release in scoped:
        if release.inherited:
            # The release operation itself implements the interface's member.
            head = f"  public fun {release.member}("
            index = next(i for i, body in enumerate(operations) if head in body)
            operations[index] = operations[index].replace(
                head, f"  public override fun {release.member}(", 1
            )
    # A callback's borrowed owner has no parent to keep reachable, and
    # nothing disposes its handle.
    borrowable = native in bound.borrowed_handles
    parameters = ["handle: Long"]
    if handle.parent:
        # The owner, not its state, keeps the parent reachable, so leak cleanup
        # and exit abandonment never hold a parent.
        parameters.append(
            f'@Suppress("unused") private val parent: {owner_name(bound.handles[handle.parent])}'
            + ("?" if borrowable else "")
        )
    readied = bound.readied_handles.get(native)
    if readied:
        # A handle that no attach created, such as a borrowed one, is ready.
        parameters.append(
            f"/** Completes once the {readied} is ready, and fails with a [MaplibreException] "
            f"when it fails to become ready. The {readied} takes calls before then. */\n"
            "  public val ready: Deferred<Unit> = kotlinx.coroutines.CompletableDeferred(Unit)"
        )
    if borrowable:
        parameters.append("borrowed: Boolean = false")
    dispose = family[0].lower() + family[1:]
    body = [
        f'  internal val binding = {state_type(native, plans)}("{owner_class}", handle, GeneratedOwnerDisposal::{dispose}'
        + (", borrowed)" if borrowable else ")"),
        (
            '  @Suppress("unused") private val cleanup = if (borrowed) null else disposeWhenUnreachable(this, binding)'
            if borrowable
            else '  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)'
        ),
        "  /**\n"
        + (
            "   * The native handle value, which runtime events report as their source. It stays readable\n"
            "   * after release, so events drained later still match this handle.\n"
            if native in bound.event_sources
            else "   * The native handle value. It stays readable after release.\n"
        )
        + "   */\n"
        "  public val id: ULong get() = binding.handle.toULong()",
        "  /** Whether a release, dispose, or discard has ended the handle. */\n"
        + (
            "  public override val isClosed: Boolean"
            if any(release.interface == ASYNC_RELEASABLE for release in scoped)
            else "  public val isClosed: Boolean"
        )
        + " get() = binding.isReleased",
    ]
    for release in scoped:
        if release.inherited:
            continue
        waits = any(other.interface == ASYNC_RELEASABLE for other in scoped)
        body.append(
            f"  /**\n   * Calls [{release.method}], for the standard library's `use`"
            + (
                ", which can't wait. [useReleasing] waits for [release] instead.\n"
                if waits
                else ".\n"
            )
            + f"   */\n  public override fun close() {{ {release.method}() }}"
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
