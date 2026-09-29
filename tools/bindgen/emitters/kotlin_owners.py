"""Derive every Kotlin handle owner from its handle, release, and decision plans."""

from dataclasses import dataclass

from .kotlin_ir import method_name, needs_read, receiver_value
from .kotlin_values import generated_owners, name, owner_name

PLATFORMS = ("commonMain", "jvmMain", "androidMain", "nativeMain")
ASYNC_RELEASABLE = "org.maplibre.nativeffi.runtime.AsyncReleasable"


@dataclass(frozen=True)
class Hooks:
    """The owner hooks a handle's generated operations call."""

    read: bool = False
    close: bool = False
    retire: bool = False
    issued: bool = False
    decision: object = None


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


def hooks(bound, native, plans):
    """Collect the hooks that the generated operations in [plans] need from their owner."""
    members = [plan for plan in plans if receives(plan, native)]
    decision = next(
        (
            callback.decision
            for callback in bound.callbacks.values()
            if callback.decision and callback.decision.handle.native == native
        ),
        None,
    )
    return Hooks(
        read=bool(decision)
        or any(needs_read(plan) or plan.direct_registrations for plan in members),
        close=any(plan.consumes and not plan.completion for plan in members),
        retire=any(plan.consumes and plan.completion for plan in members),
        issued=any(plan.receiver_access == "issued" for plan in members),
        decision=decision,
    )


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


def generate_owners(bound, plans):
    outputs = {}
    for native in generated_owners(bound):
        for platform in PLATFORMS:
            outputs[
                f"src/{platform}/kotlin/org/maplibre/nativeffi/generated/{owner_name(native)}.kt"
            ] = owner(bound, native, plans, platform)
    return outputs


def owner(bound, native, plans, platform):
    handle = bound.handles[native]
    family = name(native)
    owner_class = owner_name(native)
    needs = hooks(bound, native, plans)
    closes = release(bound, native, plans)
    supertypes = [f"Generated{family}Operations"]
    if closes:
        supertypes.append(closes.interface)
    source = "// Generated from handle ownership plans. Do not edit.\npackage org.maplibre.nativeffi.generated\n\n"
    if platform == "commonMain":
        members = ["  public val isClosed: Boolean"]
        if closes and not closes.inherited:
            members.append(
                "  override fun close()"
                if closes.member == "close"
                else "  override fun release(): kotlinx.coroutines.Deferred<Unit>"
            )
        return (
            source
            + f"public expect class {owner_class} : {', '.join(supertypes)} {{\n"
            + "\n".join(members)
            + "\n}\n"
        )
    kn = platform == "nativeMain"
    typ = "ULong" if kn else "Long"
    raw = "handle.toLong()" if kn else "handle"
    dispose = family[0].lower() + family[1:]
    if kn:
        source += "import kotlin.experimental.ExperimentalNativeApi\nimport kotlin.native.ref.createCleaner\n\n@OptIn(ExperimentalNativeApi::class)\n"
    parameters = [f"private val handle: {typ}"]
    if handle.parent:
        if handle.parent_retention != "strong":
            raise ValueError(
                f"{native}: parent retention {handle.parent_retention} needs a Kotlin owner"
            )
        parameters.append(f"parent: {owner_name(handle.parent)}")
    parameters.append(f"dispose: (Long) -> Unit = GeneratedOwnerDisposal::{dispose}")
    source += (
        f"public actual class {owner_class} internal constructor(\n  "
        + ",\n  ".join(parameters)
        + f",\n) : {supertypes[0]}(){''.join(', ' + s for s in supertypes[1:])} {{\n"
    )
    body = []
    if needs.decision:
        decision = needs.decision
        if handle.parent or handle.dispose != handle.release:
            raise ValueError(
                f"{native}: a decision owner releases through its disposal path"
            )
        accept = enum_number(bound, decision.accept)
        pass_through = enum_number(bound, decision.pass_through)
        body.append(
            f'  private val state = org.maplibre.nativeffi.internal.lifecycle.DecisionOwnerState("{owner_class}", {raw}, accept = {accept}u, passThrough = {pass_through}u, dispose = dispose)'
        )
        body.append(
            '  @Suppress("unused") private val cleaner = createCleaner(state.core) { it.close() }'
            if kn
            else "  init { val core = state.core; org.maplibre.nativeffi.internal.lifecycle.UnreachableActions.register(this, Runnable { core.close() }) }"
        )
        body.append(
            f"  internal override fun binding{family}Handle(): {typ} = state.withLive {{ handle }}"
        )
        body.append(
            f"  internal override fun <T> bindingRead{family}(block: ({typ}) -> T): T = state.withLive {{ block(handle) }}"
        )
        body.append(
            f"  internal override fun bindingComplete{family}(call: ({typ}) -> Unit) {{ state.complete {{ call(handle) }} }}"
        )
        if needs.close:
            # The release waits for in-flight calls and may run after this call
            # returns, so it takes the disposal path that unreachable cleanup uses.
            body.append(
                f'  internal override fun bindingClose{family}(call: ({typ}) -> Unit) {{ org.maplibre.nativeffi.internal.callback.CallbackAdmission.check({raw}, "{handle.release}"); state.close() }}'
            )
        if needs.retire:
            raise ValueError(f"{native}: a decision owner cannot retire asynchronously")
        body.append(
            "  internal fun finishBindingDecision(raw: UInt): UInt = state.finishDecision(raw)"
        )
        body.append(
            "  internal fun finishBindingException(): UInt = state.finishException()"
        )
        body.append("  public actual val isClosed: Boolean get() = state.isClosed")
    else:
        parents = ", parent" if handle.parent else ""
        body.append(
            f'  private val core = org.maplibre.nativeffi.internal.lifecycle.HandleStateCore("{owner_class}", {raw}{parents}, dispose = dispose)'
        )
        body.append(
            '  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }'
            if kn
            else "  init { org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport) }"
        )
        body.append(
            f"  internal override fun binding{family}Handle(): {typ} {{ core.requireLive(); return handle }}"
        )
        if needs.read:
            body.append(
                f"  internal override fun <T> bindingRead{family}(block: ({typ}) -> T): T = core.withLive {{ block(handle) }}"
            )
        if needs.close:
            body.append(
                f"  internal override fun bindingClose{family}(call: ({typ}) -> Unit) {{ core.closeOnce({{ call(handle) }}) }}"
            )
        if needs.retire:
            body.append(
                f"  internal override fun bindingRetire{family}(call: ({typ}) -> kotlinx.coroutines.Deferred<Unit>): kotlinx.coroutines.Deferred<Unit> = core.retire({{ call(handle) }})"
            )
        body.append("  public actual val isClosed: Boolean get() = core.isReleased()")
    if needs.issued:
        body.append(
            f"  internal override fun bindingIssued{family}Handle(): {typ} = handle"
        )
    if closes and not closes.inherited:
        body.append(
            f"  public actual override fun close() {{ {closes.method}() }}"
            if closes.member == "close"
            else f"  public actual override fun release(): kotlinx.coroutines.Deferred<Unit> = {closes.method}()"
        )
    return source + "\n".join(body) + "\n}\n"
