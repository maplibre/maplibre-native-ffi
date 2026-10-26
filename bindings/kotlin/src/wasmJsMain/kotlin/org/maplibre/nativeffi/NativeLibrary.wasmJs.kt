package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.loader.ModuleLoader
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary

/** Verifies that [loadWasmModule] loaded the browser module, which no call can do synchronously. */
public actual fun loadNativeLibrary() {
  ensureNativeLibrary()
}

/**
 * Loads the browser module from [url], the `maplibre-native-kotlin.mjs` that the binding's module
 * build installs beside its `.wasm`, and checks its C ABI version. Every other call throws an
 * `IllegalStateException` until this returns. A later call returns once the first load finishes,
 * and ignores its own arguments. Cancelling a caller ends only that caller's wait, and the load
 * goes on for the others.
 *
 * The module starts [pthreadPoolSize] Workers for its threads before it loads, or by default enough
 * for one runtime with one map and one render session. A thread that finds the pool empty starts
 * once the page yields, so a larger pool only saves that wait.
 *
 * The page must be cross-origin isolated, because the module's threads share its memory.
 */
public suspend fun loadWasmModule(url: String, pthreadPoolSize: Int? = null) {
  require(pthreadPoolSize == null || pthreadPoolSize > 0) {
    "pthreadPoolSize must be positive, got $pthreadPoolSize"
  }
  ModuleLoader.load(url, pthreadPoolSize)
}
