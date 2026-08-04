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
 * through [pending] for the duration of the failure it belongs to.
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
   * The diagnostic a proxied call brought back, valid only while that call's failure is being
   * turned into an exception.
   *
   * Set by the dispatch path immediately around the status check, so nothing reads a diagnostic
   * that outlived the call that produced it.
   */
  private var pending: String? = null

  /** Runs [body] with [diagnostic] reported as the current one. */
  fun <T> withProxiedDiagnostic(diagnostic: String, body: () -> T): T {
    val previous = pending
    pending = diagnostic
    try {
      return body()
    } finally {
      pending = previous
    }
  }

  actual fun currentDiagnostic(): String {
    pending?.let {
      return it
    }
    // A diagnostic is read on failure paths, including ones a host reaches before it loads the
    // module. Reporting no diagnostic is better than replacing the original failure with a
    // not-loaded error that hides it.
    if (!BrowserModule.isLoaded()) return ""
    return lastErrorMessage()
  }
}
