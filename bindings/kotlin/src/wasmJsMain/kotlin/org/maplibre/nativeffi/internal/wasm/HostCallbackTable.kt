package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status

@JsFun("(index) => { globalThis.__maplibreNativeC.removeFunction(index) }")
private external fun removeTrampoline(index: Int)

/**
 * The module-global half of one family of host callbacks native invokes from its own threads.
 *
 * The callback native is registered with is one of the module's own thunks, and the thunk reaches
 * one host function pointer this installs. That pointer is module-global while a registration is a
 * runtime's or a source's, so this owns the trampoline and hands each registration a token to be
 * reached by. Native carries the token back as the `user_data` it was registered with, which is
 * what lets one trampoline serve a registration that is being replaced and the one replacing it at
 * the same time.
 *
 * The trampoline is added when the first registration arrives and removed when the last one goes.
 * The module requires a host to install before it registers and to clear after it has cleared the
 * registration, and both halves of that ordering are the caller's: it registers with native only
 * after [add] has returned, and it calls [remove] only after the registration is gone.
 *
 * Both callback families in this binding use this. `src/browser/sync_callback.c` proxies its two
 * synchronously, because the C API demands a decision back, and `src/browser/custom_geometry.c`
 * posts its two asynchronously, because those return void; the difference is in the proxy rather
 * than in how a registration is found, so it is not visible here.
 *
 * A plain map and a plain counter are the whole implementation, because a Kotlin/Wasm module runs
 * on one thread and every call here is made from the page.
 */
internal class HostCallbackTable<T : Any>(
  private val subject: String,
  private val addTrampoline: () -> Int,
  private val installHost: (Int) -> Boolean,
  private val retainTrampolines: () -> Unit,
) {
  private val registrations = mutableMapOf<Int, T>()
  private var trampoline = 0
  private var nextToken = 1

  /** Returns the registration [token] names, or null once that registration has gone. */
  fun find(token: Int): T? = registrations[token]

  /**
   * How many registrations this family still holds.
   *
   * Read by the tests that say a teardown released one, which is otherwise unobservable: a
   * registration that outlived its source and one that did not both do nothing until native calls,
   * and native calling is exactly what the teardown was supposed to have made impossible.
   */
  val registrationCount: Int
    get() = registrations.size

  /**
   * Installs the host trampoline if it is absent, and registers what [create] builds under a fresh
   * token.
   *
   * [create] runs before the installation, so a failure there leaves nothing registered and the
   * trampoline as it was.
   */
  fun add(create: (Int) -> T): T {
    val token = nextToken++
    val registration = create(token)
    install()
    registrations[token] = registration
    return registration
  }

  /**
   * Drops [token]'s registration, and clears the host pointer once the last one has gone.
   *
   * The host pointer is cleared before the table entry is released, because native reaches the
   * entry through that pointer.
   */
  fun remove(token: Int) {
    if (registrations.remove(token) == null) return
    if (registrations.isNotEmpty()) return
    installHost(0)
    removeTrampoline(trampoline)
    trampoline = 0
  }

  private fun install() {
    if (trampoline != 0) return
    BrowserModule.require()
    // Keeps this family's trampolines in the linked binary. Their only other reference is the
    // JavaScript string that reads them out of `wasmExports`, and dead-code elimination cannot see
    // inside a string, so a binary that links this module as a klib -- every host consuming the
    // published artifact -- drops them. `addFunction` is then handed `undefined` and reports that a
    // function import requires a callable, which names neither the export nor the reason.
    retainTrampolines()
    val added = addTrampoline()
    if (added == 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not place the $subject trampoline in its " +
          "function table."
      )
    }
    if (!installHost(added)) {
      removeTrampoline(added)
      throw Status.invalidState(
        "The MapLibre Native browser module could not install the $subject host callback."
      )
    }
    trampoline = added
  }
}
