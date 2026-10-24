@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.callback

internal actual fun platformReportUncaught(failure: Throwable) {
  // The page reports the failure as an uncaught error, to its error event and the console, and
  // keeps running. The error's mlnFailure property holds the throwable.
  reportError(failure.stackTraceToString(), failure.toJsReference())
}

private fun reportError(message: String, failure: JsReference<Throwable>): Unit =
  js(
    """{
      const error = new Error(message);
      error.mlnFailure = failure;
      globalThis.reportError(error);
    }""",
  )
