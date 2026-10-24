package org.maplibre.nativeffi.internal.callback

internal actual fun platformReportUncaught(failure: Throwable) {
  // Without a handler of its own, a thread reports through its group, which prints the exception
  // or passes it to the default handler, and the thread keeps running.
  val thread = Thread.currentThread()
  thread.uncaughtExceptionHandler?.uncaughtException(thread, failure)
}
