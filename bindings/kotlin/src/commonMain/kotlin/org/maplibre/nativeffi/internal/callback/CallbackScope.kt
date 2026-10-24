package org.maplibre.nativeffi.internal.callback

internal expect object CallbackContext {
  var current: CallbackScope?
}

/**
 * The run of one callback on its thread. A response object that the callback receives points into
 * native memory that is valid only during that run, so it works only inside its scope.
 */
internal class CallbackScope(val parent: CallbackScope?) : AutoCloseable {
  private var active = true

  fun ensureActive() {
    var current = CallbackContext.current
    while (current != null && current !== this) current = current.parent
    check(active && current != null) { "callback value is outside its invoking thread or scope" }
  }

  override fun close() {
    check(CallbackContext.current === this)
    active = false
    CallbackContext.current = parent
  }
}

internal inline fun <T> withCallbackScope(block: (CallbackScope) -> T): T =
  CallbackScope(CallbackContext.current).also { CallbackContext.current = it }.use(block)
