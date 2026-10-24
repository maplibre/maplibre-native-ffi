package org.maplibre.nativeffi.internal.callback

import kotlin.experimental.ExperimentalNativeApi

@OptIn(ExperimentalNativeApi::class)
internal actual fun platformReportUncaught(failure: Throwable) {
  // The hook receives the failure without the termination that follows a truly unhandled
  // exception. With no hook, the failure prints to standard error.
  getUnhandledExceptionHook()?.invoke(failure) ?: failure.printStackTrace()
}
