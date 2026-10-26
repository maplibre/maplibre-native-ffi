package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the MapLibre Native FFI binding. */
public actual object Maplibre {
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  /** Checks the C ABI version of the library that the runtime KLIB links into the binary. */
  public actual fun loadNativeLibrary() {
    ensureNativeLibrary()
  }

  public actual fun getPluginRegisterFunctionV1(): NativePointer {
    ensureNativeLibrary()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }
}
