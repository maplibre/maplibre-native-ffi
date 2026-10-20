package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.error.CallbackException

/**
 * Hands [failure] to the platform's handler for an exception that no caller can receive: the
 * current thread's uncaught exception handler on the JVM, the error log on Android, and the
 * unhandled exception hook on Kotlin/Native. None of them ends the process.
 */
internal expect fun platformReportCallbackFailure(failure: CallbackException)

/** Reports [error], which the upcall of the C [callback] type contained. Never throws. */
internal fun reportCallbackFailure(callback: String, error: Throwable) {
  try {
    platformReportCallbackFailure(CallbackException(callback, error))
  } catch (_: Throwable) {
    // A failing handler must not reach native either.
  }
}
