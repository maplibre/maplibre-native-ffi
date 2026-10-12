package org.maplibre.nativeffi

import java.nio.file.Path
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the Kotlin/JVM FFM bridge. */
public actual object Maplibre {
  /** C ABI contract version expected by this Kotlin/JVM binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  /** Loads the native library using the binding's standard lookup order. */
  public actual fun loadNativeLibrary() {
    NativeAccess.ensureLoaded()
  }

  /** Loads the native library from an exact file path. */
  public fun loadNativeLibrary(libraryPath: Path) {
    NativeAccess.load(libraryPath)
  }

  public actual fun pluginRegisterFunctionV1(): NativePointer {
    NativeAccess.ensureLoaded()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }
}
