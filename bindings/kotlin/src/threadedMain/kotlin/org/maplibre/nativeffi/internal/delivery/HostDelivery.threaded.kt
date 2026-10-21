package org.maplibre.nativeffi.internal.delivery

// Native threads call Kotlin here, so every registration keeps its callbacks.
internal actual object HostDelivery {
  actual val queue: Long = 0L
}
