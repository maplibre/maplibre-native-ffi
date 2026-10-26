package org.maplibre.nativeffi

import java.nio.file.Path
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the MapLibre Native FFI binding. */
public actual object Maplibre {
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  /**
   * Loads the native library from a runtime JAR on the classpath, or else with
   * `System.loadLibrary("maplibre-native-c")`, which searches `java.library.path`.
   */
  public actual fun loadNativeLibrary() {
    NativeAccess.ensureLoaded()
  }

  /**
   * Loads the native library from an exact file path, after the graphics libraries that sit beside
   * it.
   */
  public fun loadNativeLibrary(libraryPath: Path) {
    NativeAccess.load(libraryPath)
  }

  public actual fun getPluginRegisterFunctionV1(): NativePointer {
    NativeAccess.ensureLoaded()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }
}
