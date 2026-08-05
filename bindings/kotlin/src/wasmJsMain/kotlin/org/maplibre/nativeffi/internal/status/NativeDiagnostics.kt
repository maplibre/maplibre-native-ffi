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
 * therefore copies the diagnostic on the executing pthread, beside the status, and hands it to
 * [NativeDiagnostics.forDispatchedCall] for as long as that call's result is being read.
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
   * The diagnostic belonging to the dispatched call whose result is being read right now, or null
   * when no such call is being read.
   *
   * Scoped to that read rather than held until something happens to replace it. The two are not the
   * same rule, and the difference is the whole reason this is a scope: a value that stands until
   * replaced is authoritative for every call that follows it, so *every* other path to native has
   * to retire it, and one that forgets reports a message belonging to somebody else's failure --
   * or, worse, the empty string a call that succeeded left behind. Only the dispatch path knows it
   * has a proxied diagnostic, so only the dispatch path says so, and it says so for exactly as long
   * as the statement is true.
   *
   * Null the rest of the time, and null is the answer for every page-thread call: the page has a
   * real slot of its own, written by the call being converted, and it is the authority on it. So a
   * page-thread entry point needs to do nothing at all to be reported correctly, which is what
   * makes this safe for the ones that reach native without going through [NativeCall] --
   * `WakeSource.signal` and `RenderTargetExtent.physicalSize`.
   *
   * A plain field, saved and restored, because a page is one thread and the scope below spans no
   * suspension: nothing else on the page can run inside it, so the only nesting possible is a call
   * this binding makes from inside a result read.
   */
  private var proxied: String? = null

  /**
   * Reports [diagnostic] as the current one while [read] converts a dispatched call's result.
   *
   * The message travelled back in the call's completion because the C API's own slot is
   * thread-local to the owner thread, where the next call replaces it. This is the window in which
   * that copy is the answer, and it closes as the read returns however it returns.
   */
  fun <T> forDispatchedCall(diagnostic: String, read: () -> T): T = reporting(diagnostic, read)

  /**
   * Gives the page's own slot the authority while [call] runs on the page.
   *
   * Redundant at the top level, where nothing has claimed the authority anyway, and not redundant
   * inside a dispatched call's result read -- a read that copies a native list or snapshot makes
   * its own page-thread calls, and a failure in one of those is the page's to report rather than
   * the dispatched call's.
   */
  fun <T> forPageCall(call: () -> T): T = reporting(null, call)

  private fun <T> reporting(diagnostic: String?, body: () -> T): T {
    val enclosing = proxied
    proxied = diagnostic
    try {
      return body()
    } finally {
      proxied = enclosing
    }
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
