package org.maplibre.nativeffi.internal.callback

// A Completion has no blocking wait here, so nothing reads the mark.
internal actual object DeliveryThread {
  actual fun enter(): Boolean = false

  actual fun exit(outer: Boolean) {}
}
