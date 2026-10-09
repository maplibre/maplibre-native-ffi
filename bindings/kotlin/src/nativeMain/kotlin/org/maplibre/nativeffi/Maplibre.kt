package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the Kotlin/Native binding. */
public actual object Maplibre {
  /** C ABI contract version expected by this Kotlin/Native binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  /** Native libraries are linked by the host binary for Kotlin/Native. */
  public actual fun loadNativeLibrary() {
    ensureNativeLibrary()
  }

  public actual fun pluginRegisterFunctionV1(): NativePointer {
    ensureNativeLibrary()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }
}
