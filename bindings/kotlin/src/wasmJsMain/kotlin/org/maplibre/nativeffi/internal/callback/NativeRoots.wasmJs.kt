package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.Status

// Native threads never read a root on wasm: the page looks each one up as it delivers. The wake
// shim posts a token in the upper 30 bits of a 32-bit word, and token 0 names the delivery queue's
// own wake, so tokens run from 1 below 2^30 and are never reused.
internal actual object NativeRoots {
  private const val LIMIT = 1L shl 30
  private var next = 1L
  private val roots = HashMap<Long, Any>()

  actual fun retain(value: Any): Long {
    if (next >= LIMIT) {
      throw MaplibreException(Status.NATIVE_ERROR, "the page has used every callback token")
    }
    return next++.also { roots[it] = value }
  }

  actual fun get(token: Long): Any? = roots[token]

  actual fun release(token: Long): Any? = roots.remove(token)

  /** Counts the roots that native holds tokens for and has not released. */
  fun countForTesting(): Int = roots.size
}
