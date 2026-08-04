package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.internal.wasm.BrowserModule

/**
 * Copies the C thread-local diagnostic into a Kotlin string.
 *
 * `mln_thread_last_error_message` returns the message belonging to *the calling native thread*.
 * That makes it correct only for a call this binding ran on the thread it is reading from, which on
 * the browser is the small set of entry points that touch no runtime state and are therefore not
 * proxied anywhere -- extent scaling, version and capability queries.
 *
 * Every other call runs on the pthread that owns its runtime, so its diagnostic belongs to that
 * thread and is gone, or replaced, by the time the page resumes. Reading it here would report an
 * empty or unrelated message for the failure the caller is actually holding. The dispatch path
 * therefore copies the diagnostic on the executing pthread, beside the status, and publishes it
 * through [proxied].
 */
@JsFun(
  """
  () => {
    const module = globalThis.__maplibreNativeC
    const address = module._mln_thread_last_error_message()
    return address === 0 ? '' : module.UTF8ToString(address)
  }
"""
)
private external fun lastErrorMessage(): String

internal actual object NativeDiagnostics {
  /**
   * The diagnostic the last dispatched call brought back, or null when the last call this binding
   * made ran on the page.
   *
   * This is the page's stand-in for the owner thread's own slot, and it follows the same rule that
   * slot does: every dispatched call replaces it, with the empty string when it did not fail. So a
   * diagnostic is never older than the call whose status is being converted, which is the whole
   * point -- a message that outlived its call would name an unrelated failure.
   *
   * Null rather than empty for a page-thread call, because the page has a real slot of its own for
   * those and it is the authority on them.
   */
  private var proxied: String? = null

  /** Reports [diagnostic] as the current one until the next call this binding makes. */
  fun setProxiedDiagnostic(diagnostic: String) {
    proxied = diagnostic
  }

  /** Hands the page's own slot back the authority, for a call this binding makes on the page. */
  fun clearProxiedDiagnostic() {
    proxied = null
  }

  actual fun currentDiagnostic(): String {
    proxied?.let {
      return it
    }
    // A diagnostic is read on failure paths, including ones a host reaches before it loads the
    // module. Reporting no diagnostic is better than replacing the original failure with a
    // not-loaded error that hides it.
    if (!BrowserModule.isLoaded()) return ""
    return lastErrorMessage()
  }
}
