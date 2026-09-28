"""Owned batch wrappers use the same resolved disposal path on every Kotlin target."""

from .kotlin_ir import needs_read, receiver_value
from .kotlin_values import generated_owners, name


def generate_owners(bound):
    outputs = {}
    for native in generated_owners(bound):
        family = name(native)
        dispose = family[0].lower() + family[1:]
        for platform in ("commonMain", "jvmMain", "androidMain", "nativeMain"):
            common = platform == "commonMain"
            source = "// Generated from handle ownership plans. Do not edit.\npackage org.maplibre.nativeffi.generated\n\n"
            if common:
                source += f"public expect class {family}Handle : Generated{family}Operations, AutoCloseable {{\n  public val isClosed: Boolean\n  override fun close()\n}}\n"
            else:
                source += (
                    "import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore\n"
                )
                kn = platform == "nativeMain"
                source += (
                    "import kotlin.native.ref.createCleaner\nimport kotlin.experimental.ExperimentalNativeApi\n\n@OptIn(ExperimentalNativeApi::class)\n"
                    if kn
                    else "import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner\n"
                )
                typ = "ULong" if kn else "Long"
                raw = "handle.toLong()" if kn else "handle"
                source += f'''public actual class {family}Handle internal constructor(private val handle: {typ}): Generated{family}Operations(), AutoCloseable {{
  private val core = HandleStateCore("{family}Handle", {raw}, dispose = GeneratedOwnerDisposal::{dispose})
'''
                source += (
                    '  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }\n'
                    if kn
                    else "  init { HandleLeakCleaner.register(this, core.leakReport) }\n"
                )
                source += f"""  internal override fun binding{family}Handle(): {typ} {{ core.requireLive(); return handle }}
  internal override fun <T> bindingRead{family}(block: ({typ}) -> T): T = core.withLive {{ block(handle) }}
  internal override fun bindingClose{family}(call: ({typ}) -> Int) {{ core.closeOnce({{ call(handle) }}) }}
  public actual val isClosed: Boolean get() = core.isReleased()
  public actual override fun close() {{ core.closeOnce({{ GeneratedOwnerDisposal.{dispose}({raw}); 0 }}) }}
}}
"""
            if platform != "commonMain" and not any(
                needs_read(plan) and receiver_value(plan).native == native
                for plan in bound.operations
            ):
                source = source.replace(
                    f"  internal override fun <T> bindingRead{family}(block: ({typ}) -> T): T = core.withLive {{ block(handle) }}\n",
                    "",
                )
            outputs[
                f"src/{platform}/kotlin/org/maplibre/nativeffi/generated/{family}Handle.kt"
            ] = source
    return outputs
