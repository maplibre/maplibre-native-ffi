package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.loader.ModuleLoader
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary

/** Verifies that [loadWasmModule] loaded the browser module, which no call can do synchronously. */
public actual fun loadNativeLibrary() {
  ensureNativeLibrary()
}

/**
 * Loads the browser module bundled by the selected `maplibre-native-ffi-runtime-opengl` or
 * `maplibre-native-ffi-runtime-webgpu` dependency. The runtime and binding must have the same
 * version. This checks the module's C ABI version. Every other call throws an
 * `IllegalStateException` until this returns. A later call returns once the first load finishes.
 * Cancelling a caller ends only that caller's wait, and the load goes on for the others.
 *
 * The module runs on the thread that loads it, and never blocks it: completions, events, and
 * callbacks arrive in tasks of their own after the call that started them returns, so await them.
 * Load it from Kotlin that runs on a dedicated Web Worker, so that the map's work doesn't delay the
 * page; on the page's thread it works too, but every frame and style parse runs there. The page
 * needs no cross-origin isolation.
 */
public suspend fun loadWasmModule() {
  ModuleLoader.load()
}
