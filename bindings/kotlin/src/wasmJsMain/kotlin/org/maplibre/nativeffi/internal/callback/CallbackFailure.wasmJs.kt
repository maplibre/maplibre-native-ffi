@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.error.CallbackException

internal actual fun platformReportCallbackFailure(failure: CallbackException) {
  // The page reports the failure as an uncaught error, to its error event and the console, and
  // keeps running. The error's mlnCallbackFailure property holds the exception.
  reportError(failure.stackTraceToString(), failure.toJsReference())
}

private fun reportError(message: String, failure: JsReference<CallbackException>): Unit =
  js(
    """{
      const error = new Error(message);
      error.mlnCallbackFailure = failure;
      globalThis.reportError(error);
    }""",
  )
