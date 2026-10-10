package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.error.CallbackException

internal actual fun platformReportCallbackFailure(failure: CallbackException) {
  // Without a handler of its own, a thread reports through its group, which prints the exception
  // or passes it to the default handler, and the thread keeps running.
  val thread = Thread.currentThread()
  thread.uncaughtExceptionHandler?.uncaughtException(thread, failure)
}
