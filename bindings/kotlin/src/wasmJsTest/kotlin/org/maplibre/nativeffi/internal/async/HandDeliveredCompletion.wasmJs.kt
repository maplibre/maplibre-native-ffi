package org.maplibre.nativeffi.internal.async

import org.maplibre.nativeffi.internal.c.Upcalls

// A wasm completion names the page's delivery queue rather than a callback, and the page's drain
// hands each result to the bridge's upcalls, which the test calls in its place.

internal actual fun callCompletion(callback: Long, userData: Long, result: Long) {
  Upcalls.completion(userData, result)
}

internal actual fun callCompletionRelease(release: Long, userData: Long) {
  Upcalls.completionRelease(userData)
}
