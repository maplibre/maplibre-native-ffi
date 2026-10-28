package org.maplibre.nativeffi.internal.delivery

// Native threads call Kotlin here, so no registration names a queue.
internal actual object HostDelivery {
  actual val queue: Long = 0L
}
