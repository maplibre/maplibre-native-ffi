package org.maplibre.nativeffi.internal.callback

import kotlin.experimental.ExperimentalNativeApi
import org.maplibre.nativeffi.error.CallbackException

@OptIn(ExperimentalNativeApi::class)
internal actual fun platformReportCallbackFailure(failure: CallbackException) {
  // The hook receives the failure without the termination that follows a truly unhandled
  // exception. With no hook, the failure prints to standard error.
  getUnhandledExceptionHook()?.invoke(failure) ?: failure.printStackTrace()
}
