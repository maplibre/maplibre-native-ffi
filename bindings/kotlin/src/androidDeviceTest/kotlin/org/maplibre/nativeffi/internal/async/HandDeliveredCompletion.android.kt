package org.maplibre.nativeffi.internal.async

import org.maplibre.nativeffi.render.TestGraphicsJni

internal actual fun callCompletion(callback: Long, userData: Long, result: Long) {
  TestGraphicsJni.callCompletion(callback, userData, result)
}

internal actual fun callCompletionRelease(release: Long, userData: Long) {
  TestGraphicsJni.callCompletionRelease(release, userData)
}
