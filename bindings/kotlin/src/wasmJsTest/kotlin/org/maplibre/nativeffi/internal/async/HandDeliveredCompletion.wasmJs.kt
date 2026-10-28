package org.maplibre.nativeffi.internal.async

import org.maplibre.nativeffi.TestSupport

internal actual fun callCompletion(callback: Long, userData: Long, result: Long) {
  TestSupport.callCompletion(callback, userData, result)
}

internal actual fun callCompletionRelease(release: Long, userData: Long) {
  TestSupport.callRelease(release, userData)
}
