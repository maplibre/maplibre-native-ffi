"""Generate Kotlin APIs and native conversions from shared semantic plans."""

from tools.bindgen.compiler import compile_api
from tools.bindgen.managed_contracts import conflicting_functions
from tools.bindgen.model import Api
from tools.bindgen.semantic import BoundApi

from . import kotlin_callbacks, kotlin_ir
from .kotlin_values import Unsupported, Values, generated_owners
from .kotlin_values import name as value_name


def lower(api: Api | BoundApi):
    functions, unsupported = [], {}
    bound = compile_api(api)
    api = bound.source
    unsupported.update(
        {name: "\n".join(reasons) for name, reasons in bound.unsupported.items()}
    )
    conflicts = conflicting_functions(api, "kotlin")
    for plan in bound.operations:
        function = plan.function
        try:
            if function.name in conflicts:
                raise Unsupported("public method name collides after conversion")
            values = Values(bound)
            for platform in ("commonMain", "jvmMain", "androidMain", "nativeMain"):
                kotlin_ir.operation(plan, values, platform)
            for platform in ("jvmMain", "androidMain", "nativeMain"):
                values.conversions(platform)
            functions.append(function)
        except Unsupported as error:
            unsupported[function.name] = f"{function.location}: {error}"
    return functions, unsupported


def generate(api: Api | BoundApi) -> dict[str, str]:
    bound = compile_api(api)
    functions, _ = lower(bound)
    values = Values(bound)
    for value in bound.public_values.values():
        if value.kind == "enum":
            values.check(value)
    for function in functions:
        kotlin_ir.operation(
            bound.operations_by_name[function.name], values, "commonMain"
        )
    outputs = {}
    for platform in ("commonMain", "jvmMain", "androidMain", "nativeMain"):
        common = platform == "commonMain"
        source = "// Generated from handle disposal relationships. Do not edit.\npackage org.maplibre.nativeffi.generated\n\n"
        if not common:
            source += "import org.maplibre.nativeffi.internal.status.Status\n"
            if platform == "nativeMain":
                source += "import kotlinx.cinterop.*\nimport org.maplibre.nativeffi.internal.c.*\n\n@OptIn(ExperimentalForeignApi::class)\n"
            elif platform == "jvmMain":
                source += "import org.maplibre.nativeffi.internal.c.MapLibreNativeC\n"
            else:
                source += (
                    "import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC\n"
                )
        source += (
            "internal "
            + ("expect" if common else "actual")
            + " object GeneratedOwnerDisposal {\n"
        )
        for handle in bound.handles.values():
            if not handle.dispose or handle.dispose in bound.source.runtime_exports:
                continue
            method = value_name(handle.native)
            method = method[0].lower() + method[1:]
            source += "  fun " if common else "  actual fun "
            source += method + "(handle: Long)"
            if not common:
                prefix = (
                    "MapLibreNativeC."
                    if platform == "jvmMain"
                    else "MaplibreNativeC."
                    if platform == "androidMain"
                    else ""
                )
                argument = "handle.toULong()" if platform == "nativeMain" else "handle"
                call = prefix + handle.dispose + "(" + argument + ")"
                function = next(
                    f for f in bound.source.functions if f.name == handle.dispose
                )
                if function.return_type.spelling != "void":
                    call = "Status.check(" + call + ")"
                source += (
                    f' {{ org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "{handle.dispose}"); '
                    + call
                    + " }"
                )
            source += "\n"
        source += "}\n"
        outputs[
            f"src/{platform}/kotlin/org/maplibre/nativeffi/generated/GeneratedOwnerDisposal.kt"
        ] = source
    from .kotlin_owners import generate_owners

    outputs.update(generate_owners(bound))
    for platform in ("commonMain", "jvmMain", "androidMain", "nativeMain"):
        imports = [
            "kotlinx.coroutines.Deferred",
            "org.maplibre.nativeffi.runtime.CommandCompletion",
            "org.maplibre.nativeffi.generated.*",
            "org.maplibre.nativeffi.internal.status.Status as BindingStatus",
            "org.maplibre.nativeffi.internal.async.adoptOwned",
            "org.maplibre.nativeffi.internal.callback.*",
        ]
        imports += [
            "org.maplibre.nativeffi.generated." + value_name(v.native)
            for v in values.used.values()
        ]
        if platform != "commonMain":
            imports += ["org.maplibre.nativeffi.internal.lifecycle.OwnerAdoption"]
        if platform == "jvmMain":
            imports += [
                "java.lang.foreign.Arena",
                "java.lang.foreign.MemorySegment",
                "java.lang.foreign.ValueLayout",
                "org.maplibre.nativeffi.internal.c.*",
                "org.maplibre.nativeffi.internal.c.MapLibreNativeC",
                "org.maplibre.nativeffi.internal.loader.CompletionBridge",
                "org.maplibre.nativeffi.internal.loader.NativeAccess",
            ]
        elif platform == "androidMain":
            imports += [
                "org.bytedeco.javacpp.*",
                "org.bytedeco.javacpp.FloatPointer",
                "org.bytedeco.javacpp.BoolPointer",
                "org.maplibre.nativeffi.NativeAccess",
                "org.maplibre.nativeffi.internal.async.CompletionBridge",
                "org.maplibre.nativeffi.internal.javacpp.ByteArrayViewScope",
                "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC",
            ]
        elif platform == "nativeMain":
            imports += [
                "kotlinx.cinterop.*",
                "platform.posix.size_t",
                "platform.posix.size_tVar",
                "org.maplibre.nativeffi.internal.c.*",
                "org.maplibre.nativeffi.internal.async.CompletionBridge",
            ]
        groups = {}
        for function in functions:
            plan = bound.operations_by_name[function.name]
            receiver = kotlin_ir.receiver_value(plan).native if plan.receiver else None
            groups.setdefault(receiver, []).append(function)
        # Every generated owner extends its operations class, even when no
        # operation names it as a receiver.
        for owner in ("mln_map", *generated_owners(bound)):
            if owner in bound.handles:
                groups.setdefault(owner, [])
        for receiver, members in groups.items():
            family = value_name(receiver) if receiver else "Api"
            class_name = "Generated" + family + ("Operations" if receiver else "")
            package = "map" if receiver == "mln_map" else "generated"
            source = f"// Generated from the C headers by tools/bindgen. Do not edit.\npackage org.maplibre.nativeffi.{package}\n\n"
            source += "\n".join(f"import {n}" for n in sorted(imports)) + "\n\n"
            if platform == "nativeMain":
                source += "@OptIn(ExperimentalForeignApi::class)\n"
            if receiver:
                source += (
                    f"public expect abstract class {class_name} internal constructor() {{\n"
                    if platform == "commonMain"
                    else f"public actual abstract class {class_name} internal actual constructor() {{\n"
                )
                if platform != "commonMain":
                    callback_owner = any(
                        (operation.registrations or operation.direct_registrations)
                        and (
                            (
                                operation.receiver
                                and kotlin_ir.receiver_value(operation).native
                                == receiver
                            )
                            or any(
                                output.handle.native == receiver
                                for output in operation.owned_outputs
                            )
                        )
                        for operation in bound.operations
                    )
                    if callback_owner:
                        source += "  internal val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()\n"
                    source += f"  internal abstract fun binding{family}Handle(): {'ULong' if platform == 'nativeMain' else 'Long'}\n"
                    if any(
                        kotlin_ir.needs_read(bound.operations_by_name[f.name])
                        for f in members
                    ):
                        raw = "ULong" if platform == "nativeMain" else "Long"
                        source += f"  internal abstract fun <T> bindingRead{family}(block: ({raw}) -> T): T\n"
                    if any(
                        callback.decision
                        and callback.decision.handle.native == receiver
                        for callback in bound.callbacks.values()
                    ):
                        raw = "ULong" if platform == "nativeMain" else "Long"
                        source += f"  internal abstract fun bindingComplete{family}(call: ({raw}) -> Int)\n"
                        source += f"  internal abstract fun <T> bindingRead{family}(block: ({raw}) -> T): T\n"
                        source += f"  internal abstract fun bindingRegister{family}Cancel(callback: () -> Unit, call: ({raw}, Long) -> org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult): Boolean\n"
                    if any(
                        bound.operations_by_name[f.name].receiver_access == "issued"
                        for f in members
                    ):
                        source += f"  internal abstract fun bindingIssued{family}Handle(): {'ULong' if platform == 'nativeMain' else 'Long'}\n"
                    if any(
                        bound.operations_by_name[f.name].consumes
                        and not bound.operations_by_name[f.name].completion
                        for f in members
                    ):
                        source += f"  internal abstract fun bindingClose{family}(call: ({'ULong' if platform == 'nativeMain' else 'Long'}) -> Int)\n"
                    if any(
                        bound.operations_by_name[f.name].consumes
                        and bound.operations_by_name[f.name].completion
                        for f in members
                    ):
                        source += f"  internal abstract fun bindingRetire{family}(call: ({'ULong' if platform == 'nativeMain' else 'Long'}) -> Deferred<Unit>): Deferred<Unit>\n"

                if platform != "commonMain" and any(
                    bound.handles.get(receiver)
                    and bound.handles[receiver].abandon == f.name
                    for f in members
                ):
                    source += "  internal abstract fun invalidateBindingViews()\n"
            else:
                source += (
                    "public expect object GeneratedApi {\n"
                    if platform == "commonMain"
                    else "public actual object GeneratedApi {\n"
                )
            bodies = []
            for function in members:
                bodies.append(
                    kotlin_ir.operation(
                        bound.operations_by_name[function.name], values, platform
                    )
                )
            source += "\n".join(bodies) + "}\n"
            outputs[
                f"src/{platform}/kotlin/org/maplibre/nativeffi/{package}/{class_name}.kt"
            ] = source
    outputs[
        "src/commonMain/kotlin/org/maplibre/nativeffi/generated/GeneratedValues.kt"
    ] = values.common()
    for platform in ("jvmMain", "androidMain", "nativeMain"):
        outputs[
            f"src/{platform}/kotlin/org/maplibre/nativeffi/generated/GeneratedValues.kt"
        ] = values.conversions(platform)
    outputs.update(kotlin_callbacks.android_bridge(values))
    return outputs


def coverage(api: Api | BoundApi) -> dict:
    functions, unsupported = lower(api)
    return {
        "generated": [function.name for function in functions],
        "unsupported": unsupported,
    }
