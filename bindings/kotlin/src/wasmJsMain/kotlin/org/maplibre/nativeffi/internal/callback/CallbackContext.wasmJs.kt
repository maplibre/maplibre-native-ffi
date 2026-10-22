package org.maplibre.nativeffi.internal.callback

// Kotlin runs on one thread in the browser.
internal actual object CallbackContext {
  actual var current: CallbackScope? = null
}
