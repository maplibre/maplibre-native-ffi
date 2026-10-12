package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the Android JNI bridge. */
public actual object Maplibre {
  /** C ABI contract version expected by this Android binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  /** Loads the Android JNI bridge library. */
  public actual fun loadNativeLibrary() {
    ensureNativeLibrary()
  }

  public actual fun pluginRegisterFunctionV1(): NativePointer {
    ensureNativeLibrary()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }
}
