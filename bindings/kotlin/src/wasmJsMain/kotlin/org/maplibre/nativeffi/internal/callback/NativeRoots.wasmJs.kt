package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.lifecycle.WeakBox

// Native never reads a root on wasm: this thread looks each one up as it delivers. Token 0 names
// the delivery queue's own wake, and a token is a 32-bit user data pointer, so tokens run from 1
// below 2^31 and are never reused.
internal actual object NativeRoots {
  private const val LIMIT = 1L shl 31
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
}

/**
 * Whether [token] roots a callback registration rather than a completion, which share the module's
 * `mln_user_data_release` trampoline.
 */
internal fun isCallbackRoot(token: Long): Boolean = NativeRoots.get(token) is WeakBox<*>
