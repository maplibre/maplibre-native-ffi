package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.CallbackException

/**
 * Hands [failure] to the platform's handler for an exception that no caller can receive: the
 * current thread's uncaught exception handler on the JVM, the log on Android, the unhandled
 * exception hook on Kotlin/Native, and the page's `reportError` on Kotlin/Wasm. None of them ends
 * the process.
 */
internal expect fun platformReportUncaught(failure: Throwable)

/** Reports [error], which the upcall of the C [callback] type contained. Never throws. */
internal fun reportCallbackFailure(callback: String, error: Throwable) {
  reportUncaught(CallbackException(callback, error))
}

/** Reports [failure] through [platformReportUncaught]. Never throws. */
internal fun reportUncaught(failure: Throwable) {
  try {
    platformReportUncaught(failure)
  } catch (_: Throwable) {
    // A failing handler must not reach native or a cleaner either.
  }
}
