package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status

// Emscripten publishes the module on the global scope of the thread that runs it, under a name any
// other module also takes, so an entry point decides which one this is. Reported rather than
// thrown, because a `@JsFun` body compiles to an arrow function that sees only its arguments.
@JsFun(
  """
  () => {
    const module = globalThis.Module
    if (module === undefined || typeof module._mln_c_version !== "function") return false
    globalThis.__maplibreNativeC = module
    return true
  }
"""
)
private external fun aliasModule(): Boolean

/**
 * Starts the binding on the thread that the Emscripten module imported it into.
 *
 * The module's `main()` runs on the pthread that `-sPROXY_TO_PTHREAD` gave it, imports this
 * distribution from `maplibre-native-kotlin.mjs` beside the module, and calls this. That thread may
 * block, so every call this binding makes into the C API is a same-thread call, as on every other
 * platform.
 *
 * A distribution built as an executable runs its own `main()` while it is being imported, which is
 * before this, so [org.maplibre.nativeffi.Maplibre.loadNativeLibrary] names the module as well.
 *
 * `@JsExport` rather than `@WasmExport`: the module calls this as a named export of the generated
 * JavaScript, and a raw WebAssembly export is reachable only through `wasmExports`.
 */
@OptIn(ExperimentalJsExport::class)
@JsExport
public fun mlnKotlinMain() {
  BrowserModule.attach()
}

/** The Emscripten module that this binding calls, which is the one Kotlin was imported into. */
internal object BrowserModule {
  private var attached = false

  /** Names the module for the generated entry points, each of which reads it on every call. */
  fun attach() {
    if (attached) return
    if (!aliasModule()) {
      throw Status.invalidState(
        "The MapLibre Native browser module is not on this thread's global scope. This binding runs " +
          "inside the module, on the thread that its main() imported Kotlin into. A Kotlin module " +
          "that a page or a worker of its own loaded has no native code to call."
      )
    }
    attached = true
  }
}
