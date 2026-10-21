"""Generate the Kotlin binding: one common API over a primitive native-call shim per platform.

The common source set holds every generated value, codec, owner, and operation.
The JVM, Kotlin/Native, and Android source sets hold only the declarations of
the C functions and upcall stubs that common code uses (see kotlin_native).
"""

from tools.bindgen.compiler import compile_api
from tools.bindgen.model import Api
from tools.bindgen.semantic import BoundApi

from . import kotlin_callbacks, kotlin_owners
from .kotlin_native import NativeShims
from .kotlin_operations import (
    Native,
    operation,
    owner_disposal,
    receiver_value,
)
from .kotlin_values import Unsupported, Values, doc, generated_owners
from .kotlin_values import name as value_name

COMMON = "src/commonMain/kotlin/org/maplibre/nativeffi"
PLATFORMS = {"jvmMain": "jvm", "nativeMain": "native", "androidMain": "android"}
JNI = "src/androidMain/jni/mln_jni_generated.c"

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


def lower(api: Api | BoundApi):
    functions, unsupported, *_ = lowered(api)
    return functions, unsupported


def lowered(api: Api | BoundApi):
    functions, unsupported = [], {}
    bound = compile_api(api)
    values = Values(bound)
    native = Native(bound)
    bodies = {}
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
        try:
            bodies[function.name] = operation(plan, values, native)
        except Unsupported as error:
            unsupported[function.name] = f"{function.location}: {error}"
    functions = [plan.function for plan in bound.operations if plan.name in bodies]
    return functions, unsupported, bodies, values, native


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


def generate(api: Api | BoundApi) -> dict[str, str]:
    bound = compile_api(api)
    functions, _, rendered, values, native = lowered(bound)
    groups = {}
    for function in functions:
        plan = bound.operations_by_name[function.name]
        receiver = receiver_value(plan).native if plan.receiver else None
        groups.setdefault(receiver, []).append(plan)
    for owner in generated_owners(bound):
        groups.setdefault(owner, [])
    outputs = {}
    for receiver, plans in groups.items():
        bodies = [doc(bound, plan.name, "  ") + rendered[plan.name] for plan in plans]
        header = (
            "// Generated from the C headers by tools/bindgen. Do not edit.\n"
            "package org.maplibre.nativeffi.generated\n\n" + OPERATION_IMPORTS + "\n"
        )
        if receiver:
            family = value_name(receiver)
            class_name = f"Generated{family}Operations"
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
            commands = [p for p in plans if p.completion and p.completion.discardable]
            if commands:
                view = f"{family}Discarding"
                source = source.removesuffix("}\n") + (
                    f"  public val discarding: {view} get() = {view}(this)\n}}\n\n"
                    f"@JvmInline\npublic value class {view} internal constructor(private val owner: {class_name}) {{\n"
                    f"  private val binding get() = owner.binding\n"
                    + (
                        "  private val bindingCallbacks get() = owner.bindingCallbacks\n"
                        if any(p.registrations for p in commands)
                        else ""
                    )
                    + "\n".join(
                        operation(p, values, native, discard=True) for p in commands
                    )
                    + "}\n"
                )
        else:
            class_name = "GeneratedApi"
            source = (
                header + "public object GeneratedApi {\n" + "\n".join(bodies) + "}\n"
            )
        outputs[f"{COMMON}/generated/{class_name}.kt"] = source
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
    outputs[f"{COMMON}/internal/c/Upcalls.kt"] = kotlin_callbacks.upcalls(values)
    shims = NativeShims(
        bound, native.functions, kotlin_callbacks.sites(values).values()
    )
    outputs[f"{COMMON}/internal/c/C.kt"] = shims.common()
    outputs[f"{COMMON}/internal/c/RuntimeLayouts.kt"] = shims.runtime_layouts()
    for directory, platform in PLATFORMS.items():
        base = f"src/{directory}/kotlin/org/maplibre/nativeffi/internal/c"
        if platform == "android":
            files, jni = shims.android()
            outputs[JNI] = jni
        else:
            files = getattr(shims, platform)()
        for file, source in files.items():
            outputs[f"{base}/{file}"] = source
    return outputs


def coverage(api: Api | BoundApi) -> dict:
    bound = compile_api(api)
    functions, unsupported = lower(bound)
    generated = [function.name for function in functions]
    return {
        "generated": generated,
        "unsupported": unsupported,
    }
