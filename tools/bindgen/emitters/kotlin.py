"""Generate the Kotlin binding: one common API over a primitive native-call shim per platform.

The common source set holds every generated value, codec, owner, and operation
that behaves the same on every target. A split callback registration has a form
per target instead (see kotlin_callbacks): the threadedMain source set holds the
forms that answer native on a MapLibre thread, with their scoped responses, and
wasmJsMain holds the forms that the page serves. The threadedMain source set
also holds the factories of native registrations. Each declares the operations
that take its forms as extensions. The JVM, Kotlin/Native, Android, and wasm
source sets also hold the declarations of the C functions and upcall stubs that
the shared code uses (see kotlin_native).
"""

import dataclasses

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Api
from tools.bindgen.semantic import BoundApi, DeliverySupport

from . import kotlin_callbacks, kotlin_owners
from .kotlin_native import NativeShims
from .kotlin_operations import (
    Native,
    method_name,
    operation,
    owner_disposal,
    receiver_value,
)
from .kotlin_values import Unsupported, Values, generated_owners, owner_name
from .kotlin_values import name as value_name

COMMON = "src/commonMain/kotlin/org/maplibre/nativeffi"
# The source set and file names of each split lowering's forms.
SPLIT = {
    kotlin_callbacks.THREADED: (
        "src/threadedMain/kotlin/org/maplibre/nativeffi",
        "GeneratedThreadedCallbacks",
    ),
    kotlin_callbacks.ASYNC: (
        "src/wasmJsMain/kotlin/org/maplibre/nativeffi",
        "GeneratedWasmCallbacks",
    ),
}
PLATFORMS = {
    "jvmMain": "jvm",
    "nativeMain": "native",
    "androidMain": "android",
    "wasmJsMain": "wasm",
}
# The source sets of the default hierarchy that hold others, and the platforms
# of the targets that each remaining source set compiles for. An operation that
# serves only some platforms goes in the fewest source sets that compile for
# exactly the targets that serve it.
SOURCE_SET_CHILDREN = {
    "commonMain": ("threadedMain", "wasmJsMain"),
    "threadedMain": ("jvmAndroidMain", "nativeMain"),
    "jvmAndroidMain": ("jvmMain", "androidMain"),
    "nativeMain": ("androidNativeMain", "appleMain", "linuxMain"),
    "appleMain": ("iosMain", "macosMain", "tvosMain"),
}
TARGET_PLATFORMS = {
    "jvmMain": frozenset({"linux", "macos", "windows"}),
    "androidMain": frozenset({"android"}),
    "androidNativeMain": frozenset({"android"}),
    "iosMain": frozenset({"ios"}),
    "macosMain": frozenset({"macos"}),
    "tvosMain": frozenset({"tvos"}),
    "linuxMain": frozenset({"linux"}),
    "wasmJsMain": frozenset({"web"}),
}
TARGETS = frozenset().union(*TARGET_PLATFORMS.values())


def source_sets(platforms: frozenset[str], source_set: str = "commonMain") -> list[str]:
    """The fewest source sets whose targets are those that serve `platforms`."""

    def targets(name: str) -> list[str]:
        children = SOURCE_SET_CHILDREN.get(name)
        return [t for child in children for t in targets(child)] if children else [name]

    if all(TARGET_PLATFORMS[target] & platforms for target in targets(source_set)):
        return [source_set]
    return [
        name
        for child in SOURCE_SET_CHILDREN.get(source_set, ())
        for name in source_sets(platforms, child)
    ]


def placement(plan) -> list[str] | None:
    """The source sets of an operation that serves only some platforms."""
    platforms = served(plan.function, TARGETS)
    return None if platforms is None else source_sets(platforms)


# The C functions that the handwritten `Maplibre` objects own, on the targets
# that can call them, so the generated API leaves them out. Their native
# declarations stay, because those objects call them.
HANDWRITTEN = {"mln_android_init", "mln_plugin_get_register_function_v1"}
# The factories of the native registrations, which only the threaded targets
# can pass (see kotlin_callbacks).
NATIVE_REGISTRATIONS = (
    "src/threadedMain/kotlin/org/maplibre/nativeffi/generated/"
    "GeneratedNativeRegistrations.kt"
)
JNI = "src/androidMain/jni/mln_jni_generated.c"
WASM_GLUE = "wasm/mln_kw_generated.c"
WASM_EXPORTS = "wasm/mln_kw_exports.txt"

OPERATION_IMPORTS = """\
import kotlin.jvm.JvmInline
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.runtime.CommandCompletion
"""

# What a call that waits for a thread to start does on each target, which the C
# reference cannot say.
THREAD_START_NOTE = (
    "Suspends until the call's thread starts. On the JVM, Android, and "
    "Kotlin/Native the call waits on the calling thread, which it blocks until "
    "then. On Kotlin/Wasm it runs on a thread of its own, so the page can call "
    "it. Cancellation does not interrupt the wait: a caller cancelled meanwhile "
    "receives the cancellation, and the binding disposes the result."
)


def operation_doc(bound, plan, indent: str = "") -> str:
    """The KDoc of an operation, with what a thread-start call does in Kotlin."""
    native = bound.doc(plan.name)
    if native is not None and plan.blocks == "thread_start":
        native = dataclasses.replace(native, notes=(THREAD_START_NOTE,))
    return docs.block_comment(native, indent)


def lower(api: Api | BoundApi):
    functions, unsupported, *_ = lowered(api)
    return functions, unsupported


def lowered(api: Api | BoundApi):
    """Lowers every operation in its common form, or in each split lowering's form
    when it takes a split registration. A scoped response exists only in the
    threaded lowering.

    The split operations of each lowering are in `split[lowering]`.
    """
    unsupported = {}
    bound = compile_api(api, TARGETS)
    values = Values(bound)
    native = Native(bound)
    bodies = {}
    split = {lowering: {} for lowering in SPLIT}
    # The discarding forms of split commands, which extend each split source
    # set's discarding view.
    values.split_discards = {lowering: {} for lowering in SPLIT}
    for value in bound.public_values.values():
        if value.kind == "enum":
            values.check(value)
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    groups = {}
    for plan in bound.operations:
        receiver = receiver_value(plan).native if plan.receiver else None
        groups.setdefault(receiver, []).append(plan)
    for plan in (plan for plans in groups.values() for plan in plans):
        function = plan.function
        if placement(plan) and (
            plan.scoped_receiver or kotlin_callbacks.split_registration(plan, values)
        ):
            unsupported[function.name] = (
                f"{function.location}: an operation that serves only some "
                "platforms takes no split registration"
            )
            continue
        if plan.scoped_receiver:
            lowerings = [kotlin_callbacks.THREADED]
        elif kotlin_callbacks.split_registration(plan, values):
            lowerings = list(SPLIT)
        else:
            lowerings = [None]
        try:
            for lowering in lowerings:
                values.split = lowering
                source = operation(plan, values, native)
                (split[lowering] if lowering else bodies)[function.name] = source
                if (
                    lowering
                    and plan.receiver
                    and plan.completion
                    and plan.completion.discardable
                ):
                    values.split_discards[lowering][function.name] = operation(
                        plan, values, native, discard=True
                    )
        except Unsupported as error:
            bodies.pop(function.name, None)
            for lowered_split in (*split.values(), *values.split_discards.values()):
                lowered_split.pop(function.name, None)
            unsupported[function.name] = f"{function.location}: {error}"
        finally:
            values.split = None
    for handwritten in HANDWRITTEN:
        bodies.pop(handwritten, None)
    functions = [
        plan.function
        for plan in bound.operations
        if plan.name in bodies or any(plan.name in s for s in split.values())
    ]
    return functions, unsupported, bodies, split, values, native


def callback_owner(bound, receiver):
    """Whether a family roots callback registrations, which common tests observe too."""
    return any(
        operation.registrations
        and (
            (operation.receiver and receiver_value(operation).native == receiver)
            or any(
                output.handle.native == receiver for output in operation.owned_outputs
            )
        )
        for operation in bound.operations
    )


def extension(source: str, receiver: str) -> str:
    """A member method's source as an extension of [receiver], which a split or
    platform source set declares."""
    for head in ("public suspend fun ", "public fun "):
        if source.startswith("  " + head):
            rest = source[len(head) + 2 :]
            generics = ""
            if rest.startswith("<"):
                generics, rest = rest.split("> ", 1)
                generics += "> "
            return head + generics + receiver + "." + rest
    raise ValueError(f"not a member method: {source[:40]}")


def platform_extension(source: str, receiver: str, method: str) -> str:
    """A member method's source as an extension of [receiver] that a platform
    source set declares, where the receiver's label is the method's name."""
    return extension(source, receiver).replace(f"this@{receiver}", f"this@{method}")


def delivery_functions(bound):
    """The delivery queue functions that a host thread's runtime drains with."""
    return {
        plan.name: plan.function
        for plan in bound.runtime_operations
        if isinstance(plan.support, DeliverySupport)
    }


def generate(api: Api | BoundApi) -> dict[str, str]:
    bound = compile_api(api, TARGETS)
    functions, _, rendered, split, values, native = lowered(bound)
    # The extensions that each platform source set declares.
    platform_extensions: dict[str, list[str]] = {}
    groups = {}
    for function in functions:
        if function.name not in rendered:
            continue
        plan = bound.operations_by_name[function.name]
        receiver = receiver_value(plan).native if plan.receiver else None
        groups.setdefault(receiver, []).append(plan)
    for owner in generated_owners(bound):
        groups.setdefault(owner, [])
    outputs = {}
    for receiver, plans in groups.items():
        family = value_name(receiver) if receiver else None
        class_name = f"Generated{family}Operations" if receiver else "GeneratedApi"
        for plan in plans:
            if (sets := placement(plan)) is None:
                continue
            method = method_name(plan)
            extensions = [
                operation_doc(bound, plan)
                + platform_extension(rendered[plan.name], class_name, method)
            ]
            if plan.receiver and plan.completion and plan.completion.discardable:
                extensions.append(
                    f"/** [{owner_name(bound.handles[receiver])}.{method}] without its completion. */\n"
                    + extension(
                        operation(plan, values, native, discard=True),
                        f"{family}Discarding",
                    )
                )
            for name in sets:
                platform_extensions.setdefault(name, []).extend(extensions)
        gated = [plan for plan in plans if placement(plan)]
        common = [plan for plan in plans if not placement(plan)]
        bodies = [
            operation_doc(bound, plan, "  ") + rendered[plan.name] for plan in common
        ]
        header = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "package org.maplibre.nativeffi.generated\n\n" + OPERATION_IMPORTS + "\n"
        )
        if receiver:
            members = [
                f"  internal abstract val binding: {kotlin_owners.state_type(receiver, plans)}\n"
            ]
            if callback_owner(bound, receiver):
                members.append(
                    "  internal val bindingCallbacks: CallbackOwner = CallbackOwner()\n"
                )
            source = (
                header
                + f"public abstract class {class_name} internal constructor() {{\n"
                + "".join(members)
                + "\n".join(bodies)
                + "}\n"
            )
            commands = [p for p in common if p.completion and p.completion.discardable]
            split_commands = any(
                p.name in discards
                for p in bound.operations
                if p.receiver and receiver_value(p).native == receiver
                for discards in values.split_discards.values()
            )
            gated_commands = any(
                p.completion and p.completion.discardable for p in gated
            )
            if commands or split_commands or gated_commands:
                view = f"{family}Discarding"
                owner = owner_name(bound.handles[receiver])
                source = source.removesuffix("}\n") + (
                    "  /**\n"
                    "   * This handle's commands without their completions. A discarding call returns once native\n"
                    "   * accepts the command, and native logs a command that fails as a warning.\n"
                    "   */\n"
                    f"  public val discarding: {view} get() = {view}(this)\n}}\n\n"
                    f"/** The commands of a [{owner}] that drop their completions. */\n"
                    f"@JvmInline\npublic value class {view} internal constructor(internal val owner: {class_name}) {{\n"
                    f"  internal val binding get() = owner.binding\n"
                    + (
                        "  internal val bindingCallbacks get() = owner.bindingCallbacks\n"
                        if any(p.registrations for p in commands) or split_commands
                        else ""
                    )
                    + "\n".join(
                        f"  /** [{owner}.{method_name(p)}] without its completion. */\n"
                        + operation(p, values, native, discard=True)
                        for p in commands
                    )
                    + "}\n"
                )
        else:
            source = (
                header + "public object GeneratedApi {\n" + "\n".join(bodies) + "}\n"
            )
        outputs[f"{COMMON}/generated/{class_name}.kt"] = source
    for name, extensions in sorted(platform_extensions.items()):
        stem = name.removesuffix("Main")
        outputs[
            f"src/{name}/kotlin/org/maplibre/nativeffi/generated/"
            f"Generated{stem[0].upper()}{stem[1:]}Operations.kt"
        ] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "package org.maplibre.nativeffi.generated\n\n"
            + OPERATION_IMPORTS
            + "\n"
            + "\n".join(extensions)
        )
    # The split forms come first: their writers can name records whose common
    # codecs and values are emitted below.
    for lowering, (directory, file) in SPLIT.items():
        extensions = []
        for function in functions:
            if function.name not in split[lowering]:
                continue
            plan = bound.operations_by_name[function.name]
            if plan.receiver:
                receiver = (
                    f"Generated{value_name(receiver_value(plan).native)}Operations"
                )
            elif plan.scoped_receiver:
                receiver = kotlin_callbacks.scoped_response(plan)
            else:
                receiver = "GeneratedApi"
            extensions.append(
                operation_doc(bound, plan)
                + extension(split[lowering][function.name], receiver)
            )
            if function.name in values.split_discards[lowering]:
                family = value_name(receiver_value(plan).native)
                owner = owner_name(bound.handles[receiver_value(plan).native])
                extensions.append(
                    f"/** [{owner}.{method_name(plan)}] without its completion. */\n"
                    + extension(
                        values.split_discards[lowering][function.name],
                        f"{family}Discarding",
                    )
                )
        declarations = kotlin_callbacks.split_declarations(values, lowering)
        codecs = values.split_codecs(lowering)
        outputs[f"{directory}/generated/{file}.kt"] = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "package org.maplibre.nativeffi.generated\n\n"
            + OPERATION_IMPORTS
            + "import org.maplibre.nativeffi.internal.delivery.HostDelivery\n\n"
            + "\n\n".join([*declarations, *codecs])
            + "\n\n"
            + "\n".join(extensions)
        )
        upcalls = kotlin_callbacks.SPLIT_UPCALLS[lowering][0]
        outputs[f"{directory}/internal/c/{upcalls}.kt"] = (
            kotlin_callbacks.split_upcalls(values, lowering)
        )
    outputs.update(
        kotlin_owners.generate_owners(
            bound, [bound.operations_by_name[f.name] for f in functions]
        )
    )
    outputs[f"{COMMON}/generated/GeneratedOwnerDisposal.kt"] = (
        "// Generated from handle disposal relationships by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.generated\n\n"
        "import org.maplibre.nativeffi.internal.c.C\n"
        "import org.maplibre.nativeffi.internal.status.NativeDiagnostics\n\n"
        + owner_disposal(bound, native)
    )
    outputs[f"{COMMON}/generated/GeneratedCodecs.kt"] = values.codecs()
    outputs[f"{COMMON}/generated/GeneratedValues.kt"] = values.common()
    outputs[NATIVE_REGISTRATIONS] = (
        "// Generated from the C headers by tools/bindgen. Do not edit.\n"
        "package org.maplibre.nativeffi.generated\n\n"
        "import org.maplibre.nativeffi.internal.callback.NativeRegistration\n"
        "import org.maplibre.nativeffi.render.NativePointer\n\n"
        + "\n\n".join(kotlin_callbacks.native_factories(values))
        + "\n"
    )
    outputs[f"{COMMON}/internal/c/Upcalls.kt"] = kotlin_callbacks.upcalls(values)
    shims = NativeShims(
        bound,
        {**native.functions, **delivery_functions(bound)},
        kotlin_callbacks.sites(values).values(),
    )
    outputs[f"{COMMON}/internal/c/C.kt"] = shims.common()
    outputs[f"{COMMON}/internal/c/RuntimeLayouts.kt"] = shims.runtime_layouts()
    for directory, platform in PLATFORMS.items():
        base = f"src/{directory}/kotlin/org/maplibre/nativeffi/internal/c"
        if platform == "android":
            files, jni = shims.android()
            outputs[JNI] = jni
        elif platform == "wasm":
            files, outputs[WASM_GLUE], outputs[WASM_EXPORTS] = shims.wasm()
        else:
            files = getattr(shims, platform)()
        for file, source in files.items():
            outputs[f"{base}/{file}"] = source
    return outputs


def coverage(api: Api | BoundApi) -> dict:
    bound = compile_api(api, TARGETS)
    functions, unsupported = lower(bound)
    generated = [function.name for function in functions]
    return {
        "generated": generated,
        "unsupported": unsupported,
    }
