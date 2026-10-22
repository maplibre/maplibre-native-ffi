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
   * Loads the native library from the first of these that is set or present:
   * 1. the file that the `org.maplibre.nativeffi.library.path` system property or the
   *    `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH` environment variable names;
   * 2. a runtime JAR on the classpath;
   * 3. `System.loadLibrary("maplibre-native-c")`, after the library's dependencies from the
   *    directories that `org.maplibre.nativeffi.library.dirs` or `MAPLIBRE_NATIVE_FFI_LIBRARY_DIRS`
   *    lists.
   */
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
