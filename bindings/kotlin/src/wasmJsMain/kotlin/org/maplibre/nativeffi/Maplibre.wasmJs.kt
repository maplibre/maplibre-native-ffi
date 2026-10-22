package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ModuleLoader
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.status.invalidArgument
import org.maplibre.nativeffi.render.NativePointer

/** Process-global entry points for the Kotlin/Wasm binding. */
public actual object Maplibre {
  /** C ABI contract version expected by this Kotlin/Wasm binding. */
  public actual const val EXPECTED_C_ABI_VERSION: Long =
    org.maplibre.nativeffi.internal.loader.EXPECTED_C_ABI_VERSION

  /** Verifies that [loadModule] loaded the browser module, which no call can do synchronously. */
  public actual fun loadNativeLibrary() {
    ensureNativeLibrary()
  }

  public actual fun pluginRegisterFunctionV1(): NativePointer {
    ensureNativeLibrary()
    return NativePointer.ofAddress(C.mln_plugin_get_register_function_v1())
  }

  /**
   * Loads the browser module from [url], the `maplibre-native-kotlin.mjs` that the binding's module
   * build installs beside its `.wasm`, and checks its C ABI version. Every other call throws a
   * `MaplibreException` with `Status.INVALID_STATE` until this returns. A later call returns once
   * the first load finishes, and ignores its own arguments. Cancelling a caller ends only that
   * caller's wait, and the load goes on for the others.
   *
   * The module starts [pthreadPoolSize] Workers for its threads before it loads, or by default
   * enough for one runtime with one map and one render session. A thread that finds the pool empty
   * starts once the page yields, so a larger pool only saves that wait.
   *
   * The page must be cross-origin isolated, because the module's threads share its memory.
   */
  public suspend fun loadModule(url: String, pthreadPoolSize: Int? = null) {
    if (pthreadPoolSize != null && pthreadPoolSize <= 0) {
      throw invalidArgument("pthreadPoolSize must be positive, got $pthreadPoolSize")
    }
    ModuleLoader.load(url, pthreadPoolSize)
  }
}
