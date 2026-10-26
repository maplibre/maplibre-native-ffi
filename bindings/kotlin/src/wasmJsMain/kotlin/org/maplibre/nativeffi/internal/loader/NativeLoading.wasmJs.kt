@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.loader

import kotlin.js.Promise
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.await
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.delivery.HostDelivery

/**
 * One load that every caller shares. The load runs in a scope of its own, so a cancelled caller
 * stops waiting and leaves the load to the other callers. A failed load leaves nothing behind, so a
 * later call starts another.
 */
internal class SharedLoad {
  private val scope = CoroutineScope(SupervisorJob())
  private var loading: Deferred<Unit>? = null

  /** Waits for the load in progress or finished, or starts [load] when there is none. */
  suspend fun await(load: suspend () -> Unit) {
    val current =
      loading
        ?: scope
          .async(start = CoroutineStart.LAZY) { load() }
          .also { started ->
            loading = started
            started.invokeOnCompletion { failure ->
              if (failure != null && loading === started) loading = null
            }
            started.start()
          }
    current.await()
  }
}

/**
 * The browser module that every generated call reaches through a `Symbol.for` global.
 *
 * Instantiating a module is asynchronous, so no call can load it on demand: `loadWasmModule` loads
 * it, and every call before then throws.
 */
internal object ModuleLoader {
  private val shared = SharedLoad()
  var loaded: Boolean = false
    private set

  suspend fun load(url: String, poolSize: Int?) = shared.await { loadOnce(url, poolSize) }

  private suspend fun loadOnce(url: String, poolSize: Int?) {
    try {
      val module = instantiate(url, poolSize ?: 0).await<JsAny>()
      install(module) { kind, token, status -> HostDelivery.deliver(kind, token, status) }
      checkAbiVersion(C.mln_c_version().toUInt().toLong())
      HostDelivery.install()
      loaded = true
    } catch (failure: Throwable) {
      // A failed load drops the module's global, so a later call can load again. A module that
      // instantiated keeps its heap and workers until the page unloads.
      uninstall()
      throw failure
    }
  }

  /**
   * Runs [block] as a page that has not loaded the module would, with the module's global gone, and
   * then restores both. [block] cannot suspend, so no delivery runs while the module is gone.
   */
  fun <T> withoutModuleForTesting(block: () -> T): T {
    check(loaded) { "the browser module is not loaded" }
    val module = installed()
    loaded = false
    uninstall()
    try {
      return block()
    } finally {
      reinstall(module)
      loaded = true
    }
  }
}

internal actual fun ensureNativeLibrary() {
  check(ModuleLoader.loaded) { "the browser module is not loaded; call loadWasmModule first" }
}

/** A loaded module has passed its C ABI version check already. */
internal actual fun loadNativeLibraryUnchecked() = ensureNativeLibrary()

// The module is an Emscripten ES module factory that the bundler must leave to the browser. Its
// link reads the pool size as Module.mlnPoolSize, and a size of 0 selects the link's default.
private fun instantiate(url: String, poolSize: Int): Promise<JsAny> =
  js("import(/* webpackIgnore: true */ url).then((m) => m.default({ mlnPoolSize: poolSize }))")

// Scalars go through one DataView over the module's fixed heap, which never grows.
private fun install(module: JsAny, deliver: (Int, Int, Int) -> Unit): Unit =
  js(
    """{
      module.mlnKwView = new DataView(module.HEAPU8.buffer);
      module.mlnKwDeliver = deliver;
      globalThis[Symbol.for('org.maplibre.nativeffi.c')] = module;
    }""",
  )

private fun uninstall(): Unit = js("delete globalThis[Symbol.for('org.maplibre.nativeffi.c')]")

private fun installed(): JsAny = js("globalThis[Symbol.for('org.maplibre.nativeffi.c')]")

private fun reinstall(module: JsAny): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')] = module")
