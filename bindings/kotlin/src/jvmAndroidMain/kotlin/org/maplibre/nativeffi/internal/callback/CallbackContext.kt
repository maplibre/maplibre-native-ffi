package org.maplibre.nativeffi.internal.callback

internal actual object CallbackContext {
  private val local = ThreadLocal<CallbackScope?>()
  actual var current: CallbackScope?
    get() = local.get()
    set(value) {
      if (value == null) local.remove() else local.set(value)
    }
}
